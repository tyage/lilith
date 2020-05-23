#pragma once

#include <string>
#include <tuple>
#include <cstdint>

using Value = std::uintptr_t;

Value nil();

Value make_cons(Value car, Value cdr);
Value car(Value cons);
Value cdr(Value cons);
Value atom(Value v);
Value eq(Value lhs, Value rhs);

Value If(Value cond, Value t, Value f);
Value quote(Value v);
Value lambda(Value names, Value body);
Value define(Value names, Value body);

Value to_Value(std::int64_t);
std::string show(Value v);

// for impl prelude(あとで隠す)
Value make_symbol(char const* name);
bool eq_bool(Value lhs, Value rhs);
bool is_self_eval(Value v);
bool is_variable(Value v);
