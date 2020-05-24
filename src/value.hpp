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

Value lambda(Value names, Value body, Value env);

Value to_Value(std::int64_t);
inline Value operator""_i(unsigned long long v) { return to_Value(v); }
Value succ(Value);
Value prev(Value);
std::string show(Value v);

// for impl prelude(あとで隠す)
Value make_symbol(char const* name);
bool eq_bool(Value lhs, Value rhs);
bool is_self_eval(Value v);
bool is_symbol(Value v);
bool is_variable(Value v);
bool is_atom_bool(Value v);
char const* c_str(Value v); // only for debug!!!
std::int64_t to_int(Value v);
