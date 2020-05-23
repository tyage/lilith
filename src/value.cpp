#include "value.hpp"

// ポインタが4byteアライメントされてるということを以下仮定。
// 下2bitが
//   00 ポインタ(この時cons cellである)
//   01 数字(上位bitにsigned int(最上位が1なら負))
//   10 シンボル(上位bitをアドレスだと思って指した先にnull terminated stringで名前が入っている)
//   11 undef(最後まで余ってたら、↑をshort string optimizationする？)

#include <sstream>
#include <cassert>
#include <cstdlib>

void* alloc(size_t size) {
  // あとで置き換える。
  return std::malloc(size);
}

Value to_Value(void* v) {
  return reinterpret_cast<Value>(v);
}
Value* to_ptr(Value v) {
  return reinterpret_cast<Value*>(v);
}

size_t const cell_size = sizeof(Value);

Value nil() {
  return 0;
}
Value t() {
  char* p = static_cast<char*>(alloc(2));
  p[0] = 't';
  p[1] = '\0';
  return to_Value(p);
}

Value make_cons(Value car, Value cdr) {
  auto const region = alloc(2 * cell_size);
  *static_cast<Value*>(region) = car;
  *(static_cast<Value*>(region) + 1) = cdr;
  return to_Value(region);
}

Value car(Value cons) {
  return *to_ptr(cons);
}
Value cdr(Value cons) {
  return *(to_ptr(cons) + 1);
}

enum class ValueType {
  Cons,
  Integer,
  Symbol,
};

ValueType type(Value v) {
  switch(v & 3) {
  case 0:
    return ValueType::Cons;
  case 1:
    return ValueType::Integer;
  default:
    return ValueType::Symbol; //
  }
}

Value atom(Value v) {
  if(type(v) == ValueType::Cons) {
    return nil();
  }
  return t();
}

Value from_bool(bool b) {
  return b ? t() : nil();
}

Value eq(Value lhs, Value rhs) {
  auto const t = type(lhs);
  if(t != type(rhs)) {
    return nil();
  }
  switch(t) {
  case ValueType::Cons:
  case ValueType::Integer:
    return from_bool(lhs == rhs);
  default:
    return from_bool(lhs == rhs); // 嘘: ポインタを辿って比較する必要がある。
  }
}

std::int64_t to_int(Value v) {
  assert(type(v) == ValueType::Integer);
  std::int64_t res = (v << 1) >> 3; // 最上位bitを落とした値
  if(v & (1LL << 63)) { // 最上位が1
    res *= -1;
  }
  return res;
}

Value to_Value(std::int64_t v) {
  Value res = v << 2;
  if(v < 0) {
    res = (-v) << 2;
    res |= 1LL << 63; // set sign
  }
  res |= 1; // set type
  return res;
}

std::string show(Value v) {
  auto const t = type(v);
  std::stringstream ss;
  switch(t) {
  case ValueType::Cons:
    if(v == nil()) {
      return "()";
    }
    ss << '(' << show(car(v));
    while(type(cdr(v)) == ValueType::Cons) {
      if(cdr(v) == nil()) { break; }
      v = cdr(v);
      ss << ' ' << show(car(v));
    }
    if(cdr(v) == nil()) {
      // proper list
      ss << ')';
    } else {
      // improper
      ss << ". " << show(cdr(v)) << ')';
    }
    return ss.str();
  case ValueType::Integer:
    ss << to_int(v);
    return ss.str();
  default:
    1;
  }
}
