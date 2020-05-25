#include "lisp_prelude.hpp"

#include <sstream>

Value to_Lisp(char const* code) {
  std::stringstream ss{code};
  return read(ss);
}

Value operator""_lisp(char const* code, size_t) {
  return to_Lisp(code);
}

Value prelude_lisp_defines(Value env) {
  std::array defines = {
    "(define id (lambda (x) x))",
    "(define + (lambda (x y) (if (eq y 0) x (+ (succ x) (pred y)))))",
    "(define x 42)",
  };
  for(auto v: defines) {
    std::tie(std::ignore, env) = eval_define(to_Lisp(v), env);
  }
  return env;
}
