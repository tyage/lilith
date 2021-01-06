#include "value.hpp"
#include "prelude.hpp"
#include "allocator.hpp"

// ポインタが4byteアライメントされてるということを以下仮定。
// https://www.gnu.org/software/libc/manual/html_node/Aligned-Memory-Blocks.html
// glibcだと8byte保証があるらしい。
// 下2bitが
//   00 ポインタ(この時cons cellである)
//   01 数字(上位bitにsigned int(最上位が1なら負))
//   10 Symbol(上位bitをアドレスだと思って指した先にnull terminated stringで名前が入っている)
//   11 Symbol(short string opt) 上位7byteにnull terminated(7文字なら無し)で名前が入る。

// nil以外はtruety

// envとは？
//   ("env" . (assoc . parent))

// lambda
//   (params body . env)

#include <sstream>
#include <cassert>
#include <cstring>

Value to_Value(void* v) {
  return reinterpret_cast<Value>(v);
}
Value to_Value(void* v, std::nullptr_t) {
  return reinterpret_cast<Value>(v);
}
ConsCell* to_ptr(Value v) {
  return reinterpret_cast<ConsCell*>(v);
}


Value nil() {
  return 0;
}

Value make_symbol(char const* name) {
  size_t len = std::strlen(name);
  if(len <= 7) { // short string opt
    Value res{0b11};
    for(size_t i{}; i < len; ++i) {
      res |= static_cast<unsigned long long>(name[i]) << (7 * 8 - i * 8);
    }
    return res;
  }

  char* p = static_cast<char*>(alloc(len + 1));
  std::strcpy(p, name);
  return to_Value(p) | 0b10;
}

Value make_cons(Value car, Value cdr) {
  auto const region = alloc_cons();
  region->cell[0] = car;
  region->cell[1] = cdr;
  return to_Value(region);
}

enum class ValueType {
  Cons,
  Integer,
  Symbol,
};

ValueType type(Value v) {
  switch(v & 3) { // 下2bit
  case 0:
    return ValueType::Cons;
  case 1:
    return ValueType::Integer;
  case 2:
  default:
    return ValueType::Symbol;
  }
}

Value car(Value cons) {
  if (type(cons) != ValueType::Cons) {
    std::cout << "wrong cons!!!! " << cons << std::endl;
  }
  assert(type(cons) == ValueType::Cons);
  return to_ptr(cons)->cell[0];
}
Value cdr(Value cons) {
  assert(type(cons) == ValueType::Cons);
  return to_ptr(cons)->cell[1];
}
void set_car(Value cons, Value car) {
  assert(type(cons) == ValueType::Cons);
  to_ptr(cons)->cell[0] = car;
}
void set_cdr(Value cons, Value car);

bool is_long_str(Value v) {
  assert(type(v) == ValueType::Symbol);
  return (v & 3) == 0b10;
}

char const* c_str(Value v) {
  assert(type(v) == ValueType::Symbol);
  if(is_long_str(v)) return reinterpret_cast<char const*>(v - 0b10);
  char* p = static_cast<char*>(alloc(8));
  p[7] = '\0';
  for(int i{}; i < 7; ++i) {
    int offset = 7 * 8 - i * 8;
    p[i] = ((0xFFLL << offset) & v) >> offset;
  }

  return p;
}

bool is_atom_bool(Value v) {
  return type(v) != ValueType::Cons || v == nil();
}

Value atom(Value v) {
  return from_bool(is_atom_bool(v));
}

bool symbol_eq_bool(Value lhs, Value rhs) {
  if(!is_long_str(lhs)) {
    return lhs == rhs;
  }
  return std::strcmp(c_str(lhs), c_str(rhs)) == 0;
}

bool eq_bool(Value lhs, Value rhs) {
  auto const t = type(lhs);
  if(t != type(rhs)) {
    return nil();
  }
  switch(t) {
  case ValueType::Cons:
  case ValueType::Integer:
    return lhs == rhs;
  case ValueType::Symbol:
  default:
    return symbol_eq_bool(lhs, rhs);
  }
}

Value eq(Value lhs, Value rhs) {
  return from_bool(eq_bool(lhs, rhs));
}

Value lambda(Value names, Value body, Value env) {
  Value res = make_cons(names, make_cons(body, env));
  return res | 3;
}

bool is_integer(Value v) {
  return type(v) == ValueType::Integer;
}
bool is_self_eval(Value v) {
  return is_integer(v) || v == nil();
}
bool is_symbol(Value v) {
  return type(v) == ValueType::Symbol;
}
bool is_variable(Value v) {
  return is_symbol(v);
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
  Value res(static_cast<std::uint64_t>(v) << 2);
  if(v < 0) {
    res = Value(static_cast<std::uint64_t>(-v) << 2);
    res |= 1LL << 63; // set sign
  }
  res |= 1; // set type
  return res;
}

Value succ(Value v) {
  return to_Value(to_int(v) + 1);
}
Value pred(Value v) {
  return to_Value(to_int(v) - 1);
}

std::string show(Value v, Value ignore) {
  if(v == ignore && ignore != nil()) {
    return "(*** ignored ***)";
  }
  auto const t = type(v);
  std::stringstream ss;
  switch(t) {
  case ValueType::Cons:
    if(v == nil()) {
      return "()";
    }
    if(to_bool(eq(car(v), make_symbol("env")))) {
      return show_env(v);
    }
    if(to_bool(eq(car(v), make_symbol("proced")))) {
      return "#<lambda>";
    }
    ss << '(' << show(car(v), ignore);
    while(type(cdr(v)) == ValueType::Cons) {
      if(cdr(v) == nil()) { break; }
      v = cdr(v);
      ss << ' ' << show(car(v), ignore);
    }
    if(cdr(v) == nil()) {
      // proper list
      ss << ')';
    } else {
      // improper
      ss << ". " << show(cdr(v), ignore) << ')';
    }
    return ss.str();
  case ValueType::Integer:
    ss << to_int(v);
    return ss.str();
  case ValueType::Symbol:
  default:
    return c_str(v);
  }
}
