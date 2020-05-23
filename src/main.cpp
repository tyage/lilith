#include <iostream>
#include "value.hpp"

Value make_symbol(char const*); // internal func

int main(int, char**) {
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
}
