#include <iostream>
#include "prelude.hpp"

int main(int argc, char** argv) {
  if(argc >= 2) {
    std::string cmd{argv[1]};
    if(cmd == "repl") {
      repl(std::cin);
      return 0;
    }
  }
  std::cout << "sizeof(Value) = " << sizeof(Value) << std::endl;
  auto cons = make_cons(to_Value(1), nil());
  std::cout << cons << std::endl;
  std::cout << car(cons) << ','<< cdr(cons) << std::endl;
  std::cout << show(cons) << std::endl;
  cons = make_cons(to_Value(2), cons);
  std::cout << show(cons) << std::endl;
  cons = make_cons(to_Value(-1), cons);
  std::cout << show(cons) << std::endl;
  cons = make_cons(to_Value(0), cons);
  std::cout << show(cons) << std::endl;
  cons = make_cons(to_Value(4294967296L + 128), cons);
  std::cout << show(cons) << std::endl;
  cons = make_cons(to_Value(-4294967296L), cons);
  std::cout << show(cons) << std::endl;
  cons = make_cons(to_Value(1), to_Value(2));
  std::cout << show(cons) << std::endl;
  cons = list(1_i, 2_i, 42_i);
  std::cout << show(cons) << std::endl;

  auto env = initial_env();
  Value res;

  std::tie(res, env) = eval(to_Value(0), env);
  std::cout << "##env: "<< show(env) << std::endl;
  std::cout << "val: "<< show(res) << std::endl;

  std::tie(res, env) = eval(t(), env);
  std::cout << "##env: "<< show(env) << std::endl;
  std::cout << "val: "<< show(res) << std::endl;

  std::tie(res, env) = eval(make_symbol("nil"), env);
  std::cout << "##env: "<< show(env) << std::endl;
  std::cout << "val: "<< show(res) << std::endl;

  cons = make_symbol("cons");
  auto ev = [&res, &env] (Value v) {
    std::cout << "---------------------------------" << std::endl;
    std::cout << "# => " << show(v) << std::endl;
    try {
      std::tie(res, env) = eval(v, env);
      std::cout << "##env: "<< show(env) << std::endl;
      std::cout << "val: "<< show(res) << std::endl;
    } catch (char const* msg) {
      std::cout << "### catch!!" << std::endl;
      std::cout << msg << std::endl;
    }
  };
  ev(cons);

  Value const cons12 = list("cons", 1_i, 2_i);
  ev(cons12);

  cons = list("car", cons12);
  ev(cons);
  cons = list("cdr", cons12);
  ev(cons);
  cons = list("eq", 1_i, 2_i);
  ev(cons);
  cons = list("eq", 0_i, 0_i);
  ev(cons);
  cons = list("atom", "nil");
  ev(cons);
  cons = list("atom", 0_i);
  ev(cons);
  cons = list("atom", cons12);
  ev(cons);
  cons = list("if", "nil", 0_i, 1_i);
  ev(cons);
  cons = list("if", "#t", 0_i, 1_i);
  ev(cons);
  cons = list("quote", 42_i);
  ev(cons);
  cons = list("quote", list("cons", "a", "b"));
  ev(cons);
  cons = list("define", "x", list("cons", 0_i, 1_i));
  ev(cons);
  ev(make_symbol("x"));
  ev(list("id", 42_i));
}
