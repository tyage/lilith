#include "allocator.hpp"

enum class AllocatorStrategy {
  NOP,
};

AllocatorStrategy const strategy = AllocatorStrategy::NOP;

#include <cstdlib>
void* NOP_alloc(size_t size) {
  // 全部おもらし。
  return std::malloc(size);
}

void* alloc(size_t size) {
  switch(strategy) {
  case AllocatorStrategy::NOP:
    return NOP_alloc(size);
  }
}

void collect(Value rootset) {
}
