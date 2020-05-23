#include "prelude.hpp"

Value t() {
  // 毎回異なる `#t` が産まれて愉快。
  return make_symbol("#t");
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
  if(is_self_eval(v)) {
    return std::make_tuple(v, env);
  }
  if(is_variable(v)) {
    return std::make_tuple(find(v, env), env);
  }
  throw "pie";
}

