#include "allocator.hpp"
#include "prelude.hpp"

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cassert>

#ifdef DEBUG
#define DEBUG_SHOW 1
#else
#define DEBUG_SHOW 0
#endif
#define DEBUGMSG if(DEBUG_SHOW)

// value.cppの中で、Valueの下2bitに情報をつめこんでいるので、allocの返り値は最低でも4byte alignmentはないと壊れる。

enum class AllocatorStrategy {
  NOP,
  PreAllocateNOP,
  MarkSweep,
  MoveCompact,
};

AllocatorStrategy const strategy = AllocatorStrategy::MoveCompact;

void* NOP_alloc(size_t size) {
  // 全部おもらし。
  return std::malloc(size);
}

bool is_cons(Value v) {
  return !is_atom_bool(v);
}

size_t roundup(size_t size, size_t round) {
  if(size % round == 0) return size;
  return (size / round + 1) * round;
}

// すべての型を破壊する。それらは再生できない。
void set(unsigned char val, void* base, size_t offset = 0) {
  *(static_cast<unsigned char*>(base) + offset) = val;
}
unsigned char get(void* base, size_t offset = 0) {
  return *(static_cast<unsigned char*>(base) + offset);
}

void* PreAlloc_pointer_slice_alloc(size_t size) {
  // おもらしするポインタずらし。
  size_t const block_size = 1024 * 32 * 1024;
  size_t const alignment_size = 4;
  static void* current_block = nullptr;
  static size_t current_offset = block_size;

  size = roundup(size, alignment_size);

  if(size + current_offset > block_size) {
    current_block = std::malloc(block_size);
    current_offset = 0;
  }
  void* result = current_block;
  current_block = static_cast<char*>(current_block) + size;
  current_offset += size;
  return result;
}

/*
class MarkSweepAllocator {
  size_t static const header_size = 4; // 4も管理用領域に必要無いけど、4-align(mallocは16-alignという仮定)。
  struct Object {
    unsigned char header[header_size];
    Value cell[2];

    unsigned char static const alive = 0x1;
    unsigned char static const dead = 0;
    unsigned char static const marked = 0xff;
  };
  size_t static const base = sizeof(Object);
 // size_t static const in_block = 1024 * 1024;
  size_t static const in_block = 1024;
  size_t static const block_size = base * in_block;
  struct Block {
    Object* area;
    size_t offset;
    Block() {
      void* p = std::malloc(block_size);
      std::memset(p, 0, block_size);
      area = static_cast<Object*>(p);
      offset = 0;
    }
    Object* alloc() {
      for(; offset < in_block; ++offset){
        Object* obj = area + offset;
        if(get(obj) != Object::alive) {
          set(Object::alive, obj);
          return obj;
        }
      }
      return nullptr;
    }
    void show_map() {
      std::cout << "----------" << std::endl;
      for(size_t i{}; i < in_block; ++i) {
        Object* obj = &area[i];
        std::cout << int(get(obj)) << ",";
      }
      std::cout << std::endl;
      std::cout << "----------" << std::endl;
    }
    void sweep() {
      show_map();
      int cnt{};
      for(size_t i{}; i < in_block; ++i) {
        Object* obj = area + i;
        if(get(obj) == Object::marked) {
          set(Object::alive, obj);
        } else {
          ++cnt;
          std::cout << (int)get(obj) << std::endl;
          set(Object::dead, obj);
          std::cout << "sweeping " << show(*obj->cell) << std::endl;
          *(obj->cell) = make_symbol("dead!");
          *(obj->cell+1) = make_symbol("beef!");
        }
      }
      offset = 0;
      std::cout << cnt << " objects destroied" << std::endl;
      show_map();
    }
    size_t alive_cnt() {
      int cnt{};
      for(size_t i{}; i < in_block; ++i) {
        Object* obj = &area[i];
        if(get(obj) == Object::alive) {
          ++cnt;
        }
      }
      return cnt;
    }
  };
  std::vector<Block> blocks;
public:
  MarkSweepAllocator() {}
  Value* alloc_cons() {
    for(size_t i{}; i < blocks.size(); ++i) {
      auto& b = blocks[i];
      auto p = b.alloc();
      if(p) return (Value*)((char*)(p) + 4);
      std::cout << "!block" << i << "is full!" << std::endl;
      std::cout << b.alive_cnt() << std::endl;
    }
    blocks.emplace_back();
    return blocks.back().alloc()->cell;
  }
  void mark_cons(Value v) {
    std::cout << "marking: " << show(v) << std::endl;
    if(!is_cons(v)) {
      std::cout << "not cons skip!" << std::endl;
      return;
    }
    if(get(to_ptr(v), -4) == Object::marked) {
      std::cout << "already marked!" << std::endl;
      return; // 他のpathで既にmarkされてる。
    }
    set(Object::marked, to_ptr(v), -4);
    std::cout << "mark!" << std::endl;

    mark_cons(car(v));
    mark_cons(cdr(v));
  }
  void sweep() {
    for(auto& b: blocks) {
      b.sweep();
      std::cout << "####### " << b.alive_cnt() << " objects alive" << std::endl;
    }
    std::cout << "####### " << blocks.size() << "blocks" << std::endl;
  }
  void collect(Value rootset) {
    mark_cons(rootset);
    sweep();
    std::cout << "!!!!!!";
    show_env(rootset);
    std::cout << std::endl;
    show(rootset);
    std::cout << (rootset) << "!!!!!!";
    std::cout << std::endl;
  }
} markSweepAllocator;
*/


class MoveCompactAllocator {
  std::vector<bool> bitmap;
  size_t const par_page = 256; // 1024; // page にいくつのobjectがあるか
  size_t const page_size = par_page * sizeof(ConsCell);
  size_t offset; // page先頭からのオフセット
  std::vector<ConsCell*> pages; // array of page
public:
  MoveCompactAllocator() : bitmap{}, offset{par_page}, pages{} {}
  ConsCell* alloc_cons() {
    if (offset > par_page - 1) { // このpageにはもう入らない。
      ConsCell* p = static_cast<ConsCell*>(std::malloc(page_size));
      if (p == nullptr) {
        throw std::bad_alloc();
      }
      pages.push_back(p);
      offset = 0;
    }
    auto addr = pages.back() + offset;
    ++offset;
    return addr;
  }
  size_t addr2page(ConsCell* v) {
    for(size_t i{}; i < pages.size(); ++i) {
      auto base = pages[i];
      std::cout << "v: " << v << " base: " << base << " emax: " << (base + par_page) << " emax-v: " << (base + par_page) - v << std::endl;
      if (base <= v && v < base + par_page) { // ここの比較本当はアドレスでやるとダメなので整数型にしないといけない気はするけど実際動かないことはなさそう。
        // O(n)かかってヤバそうなら考えましょう。
        // pageの増減時にしか変わらないのでなんとかできそう。
        return i;
      }
    }
  }
  void mark_cons(Value v) {
    DEBUGMSG std::cout << "marking: " << show(v) << " addr: " << to_ptr(v) << std::endl;
    if(!is_cons(v)) {
      DEBUGMSG std::cout << "not cons skip! " << std::endl;
      return;
    }
    // consであれば、いずれかのiについて `pages[i]` 以上 `pages[i] + par_page * sizeof(Value)` 未満に入ってることがあるので、どのpageに属しているかがわかる。
    ConsCell* vp = to_ptr(v);
    auto vpage = addr2page(vp);
    size_t voffset = vp - pages[vpage];
    assert(voffset <= par_page);
    auto base = vpage * par_page + voffset;
    if (bitmap[base]) {
      DEBUGMSG std::cout << "already marked!" << std::endl;
      return;
    }
    bitmap[base] = true;
    std::cout << "marked at: " << base << " vpage: " << vpage << " voffset: " << voffset << " vp: " << vp << std::endl;

    mark_cons(car(v));
    mark_cons(cdr(v));
  }
  Value compact(Value root) {
    size_t free = 0;
    size_t scan = pages.size() * par_page - 1;
    while(free != scan) {
      if (free > scan) break; // このへんも境界怪しい。
      while(bitmap[free]) {
        ++free;
      }
      while(!bitmap[scan]) --scan;
      // https://gyazo.com/d778d19b52397d0a7930a01ebba11695
      auto to = pages[free / par_page] + free % par_page;
      auto from = pages[scan / par_page] + scan % par_page;
      std::cout << "to: " << to << " from: " << from << " free: " << std::dec << free << " scan: " << scan << std::endl;
   //   DEBUGMSG std::cout << "to content is " << show(to_Value(to, nullptr)) << std::endl;
//      DEBUGMSG std::cout << "from content is " << show(to_Value(static_cast<void*>(to), nullptr)) << "(is nil?: " << (nil() == from->cell[0]) << ")" << std::endl;
      to->cell[0] = from->cell[0];
      to->cell[1] = from->cell[1];
      *reinterpret_cast<ConsCell**>(from) = to;
      // 引っ越し先のアドレスを元の住所に書いておく。1cellで2Value分の領域があり、Valueはstd::uintptr_tなので必ず収まる。
      // scanより後ろで、bitが立っているところは引越しした。
      ++free;
      --scan;
    }
    std::cout << "scan is " << std::dec << scan << std::endl;

    for(size_t i{}; i < scan; ++i) {
      auto e = pages[i / par_page] + i % par_page;
      for(size_t j{}; j < 2; ++j) {
        auto v = e->cell[j];
        if (!is_cons(v)) continue;
        ConsCell* vp = to_ptr(v);
        auto vpage = addr2page(vp);
        assert(vpage < pages.size());
        size_t voffset = vp - pages[vpage];
        assert(voffset <= par_page);
        std::cout << "vp: " << vp << " vpage: " << vpage << " v: " << std::hex << v << std::dec << std::endl;
        auto base = vpage * par_page + voffset;
        if (base > scan) { // 境界？
          // これread/writeバリアでやったほうがいいかもしれない。そもそも動いてないけど……。
          std::cout << "moved object found!: " << base << " vpage: " << vpage << " voffset: " << voffset << std::endl;
          e->cell[j] = to_Value(*(reinterpret_cast<ConsCell**>(pages[vpage] + voffset)), nullptr);
        }
      }
    }

    // rootも引越ししてるかもしれないので、新たなrootを返す。
    ConsCell* vp = to_ptr(root);
    auto vpage = addr2page(vp);
    assert(vpage < pages.size());
    size_t voffset = vp - pages[vpage];
    assert(voffset <= par_page);
    std::cout << "rvp: " << vp << " rvpage: " << vpage << " rv: " << std::hex << root << std::dec << std::endl;
    auto base = vpage * par_page + voffset;
    if (base > scan) { // 境界？
      // これread/writeバリアでやったほうがいいかもしれない。そもそも動いてないけど……。
      std::cout << "moved root found!: " << base << " vpage: " << vpage << " voffset: " << voffset << std::endl;
      return to_Value(*(reinterpret_cast<ConsCell**>(pages[vpage] + voffset)), nullptr);
    }
    return root;
  }
  void show_bitmap() {
    for(auto e: bitmap) {
      std::cout << (e ? '.' : ' ');
    }
    std::cout << "| kokomade" << std::endl;
    for(size_t i{}; i < bitmap.size(); ++i) {
      auto base = pages[i / par_page] + i % par_page;
      auto head = base->cell[0];
      auto tail = base->cell[1];
      /*
      std::cout << "addr index: " << std::dec << i << "(raw: " << base << ") mark: " << bitmap[i] << std::hex << " content: " << show(to_Value(base, nullptr))
      << " (raw: " << head << ", " << tail << ")" << std::endl; */
    }
  }
  Value collect(Value root) {
    bitmap = std::vector<bool>(pages.size() * par_page);
    mark_cons(root);
    DEBUGMSG std::cout << "marked bit cnt is " << std::count(begin(bitmap), end(bitmap), true) << std::endl;
    show_bitmap();
    Value new_root = compact(root);
   // show_bitmap();
    DEBUGMSG std::cout << "!!!!!!" << show_env(new_root) << std::endl;
    DEBUGMSG std::cout << "!!!!!!" << show(new_root) << std::endl;
    DEBUGMSG std::cout << "old root: " << std::hex << root << " new root: " <<  new_root << std::endl;
    return new_root;
  }
} moveCompactAllocator;

static int alloc_cnt = 0;

void* alloc(size_t size) {
  switch(strategy) {
  case AllocatorStrategy::NOP:
  default:
    return NOP_alloc(size);
  case AllocatorStrategy::PreAllocateNOP:
    return PreAlloc_pointer_slice_alloc(size);
  }
}

ConsCell* alloc_cons() {
  ++alloc_cnt;
  switch(strategy) { /*
  case AllocatorStrategy::MarkSweep:
    return markSweepAllocator.alloc_cons(); */
  case AllocatorStrategy::MoveCompact:
    return moveCompactAllocator.alloc_cons();
  default:
    size_t const cell_size = sizeof(Value) * 2;
    return static_cast<ConsCell*>(alloc(cell_size));
  }
}

Value collect(Value root) {
  std::cout << alloc_cnt << " cons total allocations!" << std::endl;
  switch(strategy) { /*
  case AllocatorStrategy::MarkSweep:
    markSweepAllocator.collect(root);
    return; */
  case AllocatorStrategy::MoveCompact:
    return moveCompactAllocator.collect(root);
  default:
    return nil(); // nop
  }
}
