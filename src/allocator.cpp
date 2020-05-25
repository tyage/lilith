#include "allocator.hpp"

#include <iostream>
#include <cstdlib>

enum class AllocatorStrategy {
  NOP,
  PreAllocateNOP,
};

AllocatorStrategy const strategy = AllocatorStrategy::PreAllocateNOP;

void* NOP_alloc(size_t size) {
  // 全部おもらし。
  return std::malloc(size);
}

size_t roundup(size_t size, size_t round) {
  if(size % round == 0) return size;
  return (size / round + 1) * round;
}

void* PreAlloc_pointer_slice_alloc(size_t size) {
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

static int alloc_cnt = 0;

void* alloc(size_t size) {
  ++alloc_cnt;
  switch(strategy) {
  case AllocatorStrategy::NOP:
    return NOP_alloc(size);
  case AllocatorStrategy::PreAllocateNOP:
    return PreAlloc_pointer_slice_alloc(size);
  }
}

void collect(Value rootset) {
  std::cout << alloc_cnt << "allocations!" << std::endl;
}
