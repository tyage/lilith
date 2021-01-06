#include "prelude.hpp"
#include "allocator.hpp"
#include "lisp_prelude.hpp"

#include <array>
#include <set>
#include <sstream>
#include <cassert>

std::tuple<Value, Value> eval_define(Value v, Value env);
Value apply(Value f, Value args);

Value t() {
  static Value t = make_symbol("#t");
  return t;
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
  return make_cons(make_symbol("env"), make_cons(nil(), env));
}

Value get_assoc(Value env) {
  assert(to_bool(eq(car(env), make_symbol("env"))));
  return car(cdr(env));
}
Value get_parent(Value env) {
  assert(to_bool(eq(car(env), make_symbol("env"))));
  return cdr(cdr(env));
}

Value define_variable(Value name, Value def, Value env) {
  assert(to_bool(eq(car(env), make_symbol("env"))));
  Value assoc = get_assoc(env);
  Value pair = make_cons(name, def);
  Value new_assoc = make_cons(pair, assoc); // assocの先頭につっこんでおけば更新もできるし、追加もできる。
  set_car(cdr(env), new_assoc);
  return env;
}

Value define_primitives(Value env) {
  std::array prims = {"cons", "car", "cdr", "atom", "eq", "succ", "pred", "apply"};
  for(auto e: prims) {
    env = define_variable(make_symbol(e), list("prim", e), env);
  }
  return env;
}

Value initial_env() {
  Value env = make_cons(make_symbol("env"), make_cons(nil(), nil()));
  env = define_variable(t(), t(), env);
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
}

Value find(Value name, Value env) {
  assert(to_bool(eq(car(env), make_symbol("env"))));
  if(env != nil()) {
    Value assoc = get_assoc(env);
    Value parent = get_parent(env);

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
  return is_tagged_list_bool(v, make_symbol("prim"));
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
Value primitive_succ(Value arg, bool plus) {
  assert(is_integer(arg));
  if(plus) {
    return succ(arg);
  }
  return pred(arg);
}

Value apply_primitive(Value name, Value args) {
  if(eq_bool(name, make_symbol("cons"))) return primitive_cons(args);
  if(eq_bool(name, make_symbol("car"))) return car(car(args)); // argに来るのはlistです。
  if(eq_bool(name, make_symbol("cdr"))) return cdr(car(args));
  if(eq_bool(name, make_symbol("eq"))) return primitive_eq(args);
  if(eq_bool(name, make_symbol("atom"))) return atom(car(args));
  if(eq_bool(name, make_symbol("succ"))) return primitive_succ(car(args), true);
  if(eq_bool(name, make_symbol("pred"))) return primitive_succ(car(args), false);
  if(eq_bool(name, make_symbol("apply"))) return apply(car(args), cdr(args));
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
  return is_tagged_list_bool(v, make_symbol("proced"));
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

Value bind_args(Value params, Value args, Value env) {
  if(is_atom_bool(params)) { // `(lambda xs xs)`
    return define_variable(params, args, env);
  }
  // `(lambda (x) x)`
  assert(len(params) == len(args));
  while(params != nil()) {
    env = define_variable(car(params), car(args), env);
    params = cdr(params);
    args = cdr(args);
  }
  return env;
}

bool last_exp_bool(Value exps) {
  return cdr(exps) == nil();
}

std::tuple<Value, Value> eval_sequence(Value exps, Value env) {
  if(last_exp_bool(exps)) {
    Value exp = car(exps);
    return eval(exp, env);
  }
  Value exp = car(exps);
  std::tie(std::ignore, env) = eval(exp, env);
  return eval_sequence(cdr(exps), env);
}

Value apply(Value f, Value args) {
  if(is_primitive_bool(f)) {
    Value name = car(cdr(f));
    return apply_primitive(name, args);
  }
  if(is_procedure_bool(f)) {
    Value env = expand_env(procedure_env(f));
    env = bind_args(procedure_args(f), args, env);
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
  Value args = car(cdr(lambda));
  Value body = cdr(cdr(lambda));
  // (lambda (x) x)
  // (lambda list list)
  return list("proced", args, body, env);
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
  throw "pie";
}

bool is_digit(char c) {
  return '0' <= c && c <= '9';
}

bool is_spaces(char c) {
  return c == ' ' || c == '\n';
}

void skip_spaces(std::istream& is) {
  int peek = is.peek();
  if(peek == EOF) throw EOF;
  while(is_spaces(peek)) {
    is.get();
    peek = is.peek();
  }
}

Value read_number(std::istream& is) {
  int peek;
  std::int64_t res{};
  while(peek = is.peek(), is_digit(peek)) {
    res = res * 10 + peek - '0';
    is.get();
  }
  return to_Value(res);
}

Value reverse_impl(Value list, Value res) {
  if(list == nil()) return res;
  return reverse_impl(cdr(list), make_cons(car(list), res));
}
Value reverse(Value list) {
  return reverse_impl(list, nil());
}

Value read_list(std::istream& is) {
  is.get(); // '('
  skip_spaces(is);
  Value l = nil();
  while(is.peek() != ')') {
    Value v = read(is);
    l = make_cons(v, l);
    skip_spaces(is);
  }
  char c = is.get(); // ')'
  assert(c == ')');
  return reverse(l);
}

bool is_alpha(char c) {
  return ('a' <= c && c <= 'z')
      || ('A' <= c && c <= 'Z');
}

template<class T>
bool contains(std::set<T> const& s, T const& v) {
  // std::set::contains since C++20
  return s.find(v) != end(s);
}

bool is_identifier_start(char c) {
  std::set const s = {'*', '_', '-', '+', '/', '#', '?', '<', '>'};
  return is_alpha(c) || contains(s, c);
}

bool is_identifier_char(char c) {
  return is_identifier_start(c) || is_digit(c);
}

size_t const MAX_ID_LEN = 128;
Value read_identifier(std::istream& is) {
  char buf[MAX_ID_LEN] = {}; // あとで動的に確保したい。
  // どうせこの後確保するから良いか？

  size_t len{};
  char peek;
  while(len < MAX_ID_LEN - 1) {
    peek = is.peek();
    if(!is_identifier_char(peek)) break;
    buf[len++] = is.get();
  }
  return make_symbol(buf);
}

Value read(std::istream& is) {
  skip_spaces(is);
  int peek = is.peek();
  if(peek == EOF) throw EOF;
  if(is_digit(peek)) {
    return read_number(is);
  }
  if(peek == '(') {
    return read_list(is);
  }
  if(is_identifier_start(peek)) {
    return read_identifier(is);
  }

  std::cout << "???? " << peek << std::endl;
  throw "read fail";
  return nil();
}

bool const rethrow(false); // for debug, set true
bool const showenv(true);

[[noreturn]] void repl(std::istream& is) {
  Value env = initial_env();
  Value res;
  while(true) {
    try{
      std::cout << "> ";
      Value input = read(is);
      std::cout << "# => " << show(input) << std::endl;
      std::tie(res, env) = eval(input, env);
      std::cout << show(res) << std::endl;
    } catch(char const* msg) {
      std::cout << "*** catch ***" << std::endl;
      std::cout << msg << std::endl;
      if(rethrow) throw msg;
    }
    if(showenv) std::cout << "*** env:" << show_env(env) << std::endl;
    collect(env);
  }
}

std::string show_env(Value env) {
  if (!to_bool(eq(car(env), make_symbol("env")))) {
    std::cout << "!!!!" << std::dec << car(env) << ' ' << std::hex << car(env) << std::endl;
    std::cout << "env itself is : " << env << std::endl;
    std::cout << show(car(env)) << std::endl;
  }
  assert(to_bool(eq(car(env), make_symbol("env"))));
  Value assoc = get_assoc(env);
  Value parent = get_parent(env);
  std::stringstream ss;
  ss << "defined: { " << show(assoc, env) << " }";
  if(parent != nil()) {
    ss << ", parent: { " << show_env(parent) << " }";
  }

  return ss.str();
}
