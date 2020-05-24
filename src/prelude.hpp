#pragma once

#include "value.hpp"

Value t();
Value quote(Value v);

Value from_bool(bool b);
bool to_bool(Value v);

Value initial_env();

// pair of (evaled value, new env)
std::tuple<Value, Value> eval(Value v, Value env);

inline Value list() {
  return nil();
}
template<class...Args>
inline Value list(Value v, Args...args) {
  return make_cons(v, list(args...));
}
template<class...Args>
inline Value list(char const* s, Args...args) {
  return make_cons(make_symbol(s), list(args...));
}
