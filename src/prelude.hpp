#pragma once

#include "value.hpp"

#include <iostream>

Value t();
Value quote(Value v);

Value from_bool(bool b);
bool to_bool(Value v);

Value initial_env();

// pair of (evaled value, new env)
std::tuple<Value, Value> eval(Value v, Value env);
std::tuple<Value, Value> eval_define(Value v, Value env); // internai?

inline Value to_Value_(Value v) { return v; }
inline Value to_Value_(char const* s) { return make_symbol(s); }

inline Value list() {
  return nil();
}
template<typename T, class...Args>
inline Value list(T v, Args&&...args) {
  return make_cons(to_Value_(v), list(std::forward<Args>(args)...));
}

Value read(std::istream&);
[[noreturn]] void repl(std::istream&);

// for impl show
std::string show_env(Value env);
