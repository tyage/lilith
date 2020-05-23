#include "value.hpp"

// ポインタが4byteアライメントされてるということを以下仮定。
// 下2bitが
//   00 ポインタ(この時cons cellである)
//   01 数字(上位bitにsigned int(最上位が1なら負))
//   10 Symbol(上位bitをアドレスだと思って指した先にnull terminated stringで名前が入っている)
//   11 undef(最後まで余ってたら、↑をshort string optimizationする？)

// nil以外はtruety

// envとは？
//   (assoc . parent)

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

Value t() {
  // 毎回異なる `#t` が産まれて愉快。
  return make_symbol("#t");
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
  switch(v & 3) { // 下2bit
  case 0:
    return ValueType::Cons;
  case 1:
    return ValueType::Integer;
  default:
    return ValueType::Symbol; //
  }
}

Value from_bool(bool b) {
  return b ? t() : nil();
}
bool to_bool(Value v) {
  return v != nil();
}

char const* c_str(Value v) {
  assert(type(v) == ValueType::Symbol);
  return reinterpret_cast<char const*>(v - 0b10);
}


Value atom(Value v) {
  return from_bool(type(v) == ValueType::Cons);
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
    return lhs == rhs;
  default:
    return symbol_eq_bool(lhs, rhs);
  }
}

Value eq(Value lhs, Value rhs) {
  return from_bool(eq_bool(lhs, rhs));
}

bool self_eval(Value v) {
  return type(v) == ValueType::Integer;
}

Value define_variable(Value name, Value def, Value env) {
  Value assoc = car(env);
  Value parent = cdr(env);
  Value pair = make_cons(name, def); // ここnameをunquoteしないといけないかも？
  Value new_assoc = make_cons(pair, assoc); // assocの先頭につっこんでおけば更新もできるし、追加もできる。
  return make_cons(new_assoc, parent);
}

Value initial_env() {
  Value env = make_cons(nil(), nil());
  env = define_variable(t(), t(), env); // 1つ目のtはquoteしないといけない？ // ↑ のunquoteと打ち消されるのでどっちもなしで良い気もする？
  env = define_variable(make_symbol("nil"), nil(), env);

  return env;
}

// assoc listの中から該当の名前をもつものがあるかどうかを探す。
// みつかった時は `(name . val)` を返し、なかった時は `nil` を返す。
Value lookup(Value name, Value list) {
  while(list != nil()) {
    Value const candidate = car(list);
    if(eq_bool(name, car(candidate))) {
      return candidate;
    }
    list = cdr(list);
  }
  return nil();
}

Value find(Value name, Value env) {
  Value assoc = car(env);
  Value parent = cdr(env);

  Value res = lookup(name, assoc);
  if(res != nil()) {
    return cdr(res);
  }
  if(parent != nil()) {
    return find(name, parent);
  }

  throw "unbound variable";
}

std::tuple<Value, Value> eval(Value v, Value env) {
  // 多分evalの実装にvalueの詳細が必要なことはないので、別のファイルに切り出す。
  if(self_eval(v)) {
    return std::make_tuple(v, env);
  }
  if(type(v) == ValueType::Symbol) { // variable
    return std::make_tuple(find(v, env), env);
  }
  throw "pie";
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
    return c_str(v);
  }
}
