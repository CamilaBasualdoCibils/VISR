#include <tracy/Tracy.hpp>

#include <cstdlib>
#include <new>

namespace {
void *Allocate(std::size_t size) {
  void *memory = std::malloc(size == 0 ? 1 : size);
  if (memory == nullptr)
    throw std::bad_alloc{};
  TracyAlloc(memory, size);
  return memory;
}

void Deallocate(void *memory) noexcept {
  if (memory == nullptr)
    return;
  TracyFree(memory);
  std::free(memory);
}
} // namespace

void *operator new(std::size_t size) {
  return Allocate(size);
}

void *operator new[](std::size_t size) {
  return Allocate(size);
}

void operator delete(void *memory) noexcept {
  Deallocate(memory);
}

void operator delete[](void *memory) noexcept {
  Deallocate(memory);
}

void operator delete(void *memory, std::size_t) noexcept {
  Deallocate(memory);
}

void operator delete[](void *memory, std::size_t) noexcept {
  Deallocate(memory);
}
