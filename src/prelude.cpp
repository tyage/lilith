#include "prelude.hpp"

#include <array>
#include <cassert>

std::tuple<Value, Value> eval_define(Value v, Value env);

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

Value prelude_lisp_defines(Value env) {
  std::array defines = {
    list("define", "ans", 42_i),
    list("define", "id", list("lambda", list("x"), "x")),
  };
  for(auto v: defines) {
    std::tie(std::ignore, env) = eval_define(v, env);
  }
  return env;
}

Value initial_env() {
  Value env = make_cons(nil(), nil());
  env = define_variable(t(), t(), env); // 1つ目のtはquoteしないといけない？ // ↑ のunquoteと打ち消されるのでどっちもなしで良い気もする？
  env = define_variable(make_symbol("nil"), nil(), env);
  env = define_primitives(env);
  env = prelude_lisp_defines(env);
  env = expand_env(env);

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
};

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

bool is_quoted_bool(Value v) {
  return is_tagged_list_bool(v, make_symbol("quote"));
}
bool is_if_bool(Value v) {
  return is_tagged_list_bool(v, make_symbol("if"));
}
bool is_define_bool(Value v) {
  return is_tagged_list_bool(v, make_symbol("define"));
}
bool is_lambda_bool(Value v) {
  return is_tagged_list_bool(v, make_symbol("lambda"));
}
bool is_procedure_bool(Value v) {
  return is_tagged_list_bool(v, make_symbol("procedure"));
}

Value procedure_args(Value f) {
  assert(len(f) == 4_i);
  return car(cdr(f));
}
Value procedure_body(Value f) {
  assert(len(f) == 4_i);
  return car(cdr(cdr(f)));
}
Value procedure_env(Value f) {
  assert(len(f) == 4_i);
  return car(cdr(cdr(cdr(f))));
}
#include <iostream>

Value bind_args(Value params, Value args, Value env) {
  std::cout << "xxxxxxxxxx hewe" << std::endl;
  std::cout << "xxxxxxxxxx params: " << show(params) << std::endl;
  std::cout << "xxxxxxxxxx args: " << show(args) << std::endl;
  if(is_atom_bool(params)) { // `(lambda xs xs)`
  std::cout << "xxxxxxxxxx hewe1!!1" << std::endl;
    return define_variable(params, args, env);
  }
  // `(lambda (x) x)`
    std::cout << "xxx# env-before bind!: assoc: " << show(car(env)) << std::endl;
    std::cout << "xxx#                  parent: " << show(cdr(env)) << std::endl;
  assert(len(params) == len(args));
  while(params != nil()) {
    env = define_variable(car(params), car(args), env);
    params = cdr(params);
    args = cdr(args);
  }
    std::cout << "xxx# env-after bind!: assoc: " << show(car(env)) << std::endl;
    std::cout << "xxx#                 parent: " << show(cdr(env)) << std::endl;
  return env;
}

bool last_exp_bool(Value exps) {
  return cdr(exps) == nil();
}

std::tuple<Value, Value> eval_sequence(Value exps, Value env) {
  std::cout << "seq!: " << show(exps) << std::endl;
  if(last_exp_bool(exps)) {
    Value exp = car(exps);
    std::cout << "evaluating!: " << show(exp) << std::endl;
    return eval(exp, env);
  }
  Value exp = car(exps);
  std::cout << "evaluating!: " << show(exp) << std::endl;
  std::tie(std::ignore, env) = eval(exp, env);
  return eval_sequence(cdr(exps), env);
}

Value apply(Value f, Value args) {
  if(is_primitive_bool(f)) {
    Value name = car(cdr(f));
    return apply_primitive(name, args);
  }
  std::cout << "#?#?# " << show(f) << std::endl;
  if(is_procedure_bool(f)) {
    Value env = expand_env(procedure_env(f));
    env = bind_args(procedure_args(f), args, env);
    std::cout << "env-after bind!: assoc: " << show(car(env)) << std::endl;
    std::cout << "                parent: " << show(cdr(env)) << std::endl;
    return std::get<0>(eval_sequence(procedure_body(f), env));
  }
  throw "ha?(apply)";
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
  if(is_atom_bool(names)) { // `(define x 42)`
    assert(is_symbol(names));
    std::tie(bodies, env) = eval(bodies, env);
    env = define_variable(names, bodies, env);
    return std::make_tuple(names, env);
  }
  // `(define (id x) x)`
  throw "unimpled yet...";
}

Value make_procedure(Value lambda, Value env) {
  std::cout << "???? " << show(lambda) << std::endl;
  Value args = car(cdr(lambda));
  Value body = cdr(cdr(lambda));
  // (lambda (x) x)
  // (lambda list list)
  return list("procedure", args, body, env);
}

std::tuple<Value, Value> eval(Value v, Value env) {
  if(is_self_eval(v)) return std::make_tuple(v, env);
  if(is_variable(v)) return std::make_tuple(find(v, env), env);
  if(is_quoted_bool(v)) return std::make_tuple(unquote(v), env);
  if(is_if_bool(v)) return eval_if(v, env);
  if(is_define_bool(v)) return eval_define(v, env);
  if(is_lambda_bool(v)) return std::make_tuple(make_procedure(v, env), env);
  if(is_application(v)) {
    Value op = car(v);
    std::tie(op, env) = eval(op, env);
    Value operands = cdr(v);
    operands = list_of_values(operands, env);
    return std::make_tuple(apply(op, operands), env);
  }
  std::cout << "!!!! " << show(v) << std::endl;
  throw "pie";
};
