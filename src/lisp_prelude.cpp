#include "lisp_prelude.hpp"

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

