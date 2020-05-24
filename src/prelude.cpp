#include "prelude.hpp"

#include <array>
#include <cassert>

Value t() {
  // 毎回異なる `#t` が産まれて愉快。
  return make_symbol("#t");
}

Value from_bool(bool b) {
  return b ? t() : nil();
}
bool to_bool(Value v) {
  return v != nil();
}

Value quote(Value v) {
  return list("quote", v);
}

bool is_pair_bool(Value v) {
  return !is_atom_bool(v);
}

bool is_tagged_list_bool(Value v, Value tag) {
  if(!is_pair_bool(v)) return false;
  return eq_bool(car(v), tag);
}

Value unquote(Value v) {
  return car(cdr(v));
}

Value expand_env(Value env) {
  return make_cons(nil(), env);
}

Value define_variable(Value name, Value def, Value env) {
  Value assoc = car(env);
  Value parent = cdr(env);
  Value pair = make_cons(name, def); // ここnameをunquoteしないといけないかも？
  Value new_assoc = make_cons(pair, assoc); // assocの先頭につっこんでおけば更新もできるし、追加もできる。
  return make_cons(new_assoc, parent);
}

Value define_primitives(Value env) {
  std::array prims = {"cons", "car", "cdr", "atom", "eq"};
  for(auto e: prims) {
    env = define_variable(make_symbol(e), list("primitive", e), env);
  }
  return env;
}

Value initial_env() {
  Value env = make_cons(nil(), nil());
  env = define_variable(t(), t(), env); // 1つ目のtはquoteしないといけない？ // ↑ のunquoteと打ち消されるのでどっちもなしで良い気もする？
  env = define_variable(make_symbol("nil"), nil(), env);
  env = define_primitives(env);
  env= expand_env(env);

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
  if(env != nil()) {
    Value assoc = car(env);
    Value parent = cdr(env);

    Value res = lookup(name, assoc);
    if(res != nil()) {
      return cdr(res);
    }
    if(parent != nil()) {
      return find(name, parent);
    }
  }

  throw c_str(name);
}

bool is_application(Value v) {
  return !is_atom_bool(v);
}

Value list_of_values(Value values, Value env) {
  if(values == nil()) return nil();

  Value head = car(values);
  std::tie(head, env) = eval(head, env);
  return make_cons(head, list_of_values(cdr(values), env));
}

Value len(Value list) {
  if(list == nil()) {
    return to_Value(0);
  }
  return succ(len(cdr(list)));
}

bool is_primitive_bool(Value v) {
  return is_tagged_list_bool(v, make_symbol("primitive"));
}

Value primitive_cons(Value args) {
  assert(len(args) == 2_i);
  Value car_ = car(args);
  Value cdr_ = car(cdr(args));
  return make_cons(car_, cdr_);
}
Value primitive_eq(Value args) {
  assert(len(args) == 2_i);
  Value car_ = car(args);
  Value cdr_ = car(cdr(args));
  return eq(car_, cdr_);
}

Value apply_primitive(Value name, Value args) {
  if(eq_bool(name, make_symbol("cons"))) return primitive_cons(args);
  if(eq_bool(name, make_symbol("car"))) return car(car(args)); // argに来るのはlistです。
  if(eq_bool(name, make_symbol("cdr"))) return cdr(car(args));
  if(eq_bool(name, make_symbol("eq"))) return primitive_eq(args);
  if(eq_bool(name, make_symbol("atom"))) return atom(car(args));
  throw "unknown primitive";
}

Value apply(Value f, Value args) {
  if(is_primitive_bool(f)) {
    Value name = car(cdr(f));
    return apply_primitive(name, args);
  }
  Value params_ = params(f);
  Value body_ = body(f);
  Value env = lambda_env(f);
  env = expand_env(env);
  assert(len(args) == len(params_));
  while(params_ != nil()) {
    Value name = car(params_);
    Value val = car(args);
    env = define_variable(name, val, env);

    params_ = cdr(params_);
    args = cdr(args);
  }

  Value res;
  std::tie(res, env) = eval(body_, env);
  return res;
}

bool is_quoted_bool(Value v) {
  return is_tagged_list_bool(v, make_symbol("quote"));
}
bool is_if_bool(Value v) {
  return is_tagged_list_bool(v, make_symbol("if"));
}
bool is_define_bool(Value v) {
  return is_tagged_list_bool(v, make_symbol("define"));
}

std::tuple<Value, Value> eval_if(Value v, Value env) {
  v = cdr(v); // headは`if`。
  Value cond = car(v);
  v = cdr(v);
  Value consequent = car(v);
  v = cdr(v);
  Value alter = car(v);
  std::tie(cond, env) = eval(cond, env);
  if(to_bool(cond)) return eval(consequent, env);
  return eval(alter, env);
}
std::tuple<Value, Value> eval_define(Value v, Value env) {
  Value names = car(cdr(v));
  Value bodies = car(cdr(cdr(v)));
  if(is_atom_bool(names)) {
    assert(is_symbol(names));
    std::tie(bodies, env) = eval(bodies, env);
    env = define_variable(names, bodies, env);
    return std::make_tuple(names, env);
  }
  throw "unimpled yet...";
}

std::tuple<Value, Value> eval(Value v, Value env) {
  if(is_self_eval(v)) return std::make_tuple(v, env);
  if(is_variable(v)) return std::make_tuple(find(v, env), env);
  if(is_quoted_bool(v)) return std::make_tuple(unquote(v), env);
  if(is_if_bool(v)) return eval_if(v, env);
  if(is_define_bool(v)) return eval_define(v, env);
  if(is_application(v)) {
    Value op = car(v);
    std::tie(op, env) = eval(op, env);
    Value operands = cdr(v);
    operands = list_of_values(operands, env);
    return std::make_tuple(apply(op, operands), env);
  }
  throw "pie";
}
