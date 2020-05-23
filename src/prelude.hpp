#pragma once

#include "value.hpp"

Value t();

Value initial_env();

// pair of (evaled value, new env)
std::tuple<Value, Value> eval(Value v, Value env);
