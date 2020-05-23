#pragma once

#include <string>
#include <cstdint>

class Value {
public:
  using base_t = std::uintptr_t;
  Value(base_t v_) : v(v_) {}
  operator base_t() const { return v; }
  base_t operator |=(base_t other) { return v | other; }
private:
  base_t v;
};

Value nil();
Value t();

Value make_cons(Value car, Value cdr);
Value car(Value cons);
Value cdr(Value cons);
Value atom(Value v);
Value eq(Value lhs, Value rhs);

Value to_Value(std::int64_t);
std::string show(Value v);
