#pragma once

#include "value.hpp"

#include <cstddef>
void* alloc(size_t size);
ConsCell* alloc_cons();

void collect(Value rootset);
