#include "value.hpp"
#include "prelude.hpp"

// ポインタが4byteアライメントされてるということを以下仮定。
// 下2bitが
//   00 ポインタ(この時cons cellである)
//   01 数字(上位bitにsigned int(最上位が1なら負))
//   10 Symbol(上位bitをアドレスだと思って指した先にnull terminated stringで名前が入っている)
//   11 lambda(上位アドレスに以下の形で入っている)

// nil以外はtruety

// envとは？
//   (assoc . parent)

// lambda
//   (params body . env)

#include <sstream>
#include <cassert>
#include <cstdlib>
#include <cstring>

void* alloc(size_t size) {
  // あとで置き換える。 GCを書きたくてlispを書きはじめたので……。
  // 今は全部おもらし。
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

Value make_symbol(char const* name) {
  size_t len = std::strlen(name);
  char* p = static_cast<char*>(alloc(len + 1));
  std::strcpy(p, name);
  return to_Value(p) | 0b10;
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
  Lambda,
};

ValueType type(Value v) {
  switch(v & 3) { // 下2bit
  case 0:
    return ValueType::Cons;
  case 1:
    return ValueType::Integer;
  case 2:
    return ValueType::Symbol;
  case 3:
    return ValueType::Lambda;
  }
}

char const* c_str(Value v) {
  assert(type(v) == ValueType::Symbol);
  return reinterpret_cast<char const*>(v - 0b10);
}

bool is_atom_bool(Value v) {
  return type(v) != ValueType::Cons || v == nil();
}

Value atom(Value v) {
  return from_bool(is_atom_bool(v));
}

bool symbol_eq_bool(Value lhs, Value rhs) {
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
  case ValueType::Lambda:
    return lhs == rhs;
  case ValueType::Symbol:
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

bool is_self_eval(Value v) {
  return type(v) == ValueType::Integer || is_lambda_bool(v);
}
bool is_symbol(Value v) {
  return type(v) == ValueType::Symbol;
}
bool is_variable(Value v) {
  return is_symbol(v);
}
bool is_lambda_bool(Value v) {
  return type(v) == ValueType::Lambda;
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

Value params(Value v) {
  assert(type(v) == ValueType::Lambda);
  v -= 3;
  return car(v);
}
Value body(Value v) {
  assert(type(v) == ValueType::Lambda);
  v -= 3;
  return car(cdr(v));
}
Value lambda_env(Value v) {
  assert(type(v) == ValueType::Lambda);
  v -= 3;
  return cdr(cdr(v));
}

Value succ(Value v) {
  return to_Value(to_int(v) + 1);
}
Value prev(Value v) {
  return to_Value(to_int(v) - 1);
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
  case ValueType::Symbol:
    return c_str(v);
  case ValueType::Lambda:
    ss << "#<lambda " << show(params(v)) << ": " << show(body(v)) << ">";
    return ss.str();
  }
}
