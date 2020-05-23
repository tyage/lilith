#pragma once

#include <string>
#include <tuple>
#include <cstdint>

using Value = std::uintptr_t;

Value nil();
Value t();

Value make_cons(Value car, Value cdr);
Value car(Value cons);
Value cdr(Value cons);
Value atom(Value v);
Value eq(Value lhs, Value rhs);

Value If(Value cond, Value t, Value f);
Value quote(Value v);
Value lambda(Value names, Value body);
Value define(Value names, Value body);

Value initial_env();
// pair of (evaled value, new env)
std::tuple<Value, Value> eval(Value v, Value env);

Value to_Value(std::int64_t);
std::string show(Value v);
