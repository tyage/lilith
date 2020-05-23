#pragma once

#include <string>
#include <cstdint>

using Value = std::uintptr_t;

Value nil();
Value t();

Value make_cons(Value car, Value cdr);
Value car(Value cons);
Value cdr(Value cons);
Value atom(Value v);
Value eq(Value lhs, Value rhs);

Value to_Value(std::int64_t);
std::string show(Value v);
