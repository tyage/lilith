#pragma once

#include "value.hpp"

#include <cstddef>
void* alloc(size_t size);
void collect(Value rootset);
