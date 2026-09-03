#include "Heap.hpp"
#include "HeapAllocator.hpp"

int main() {
    Heap heap(0x1000);
    HeapAllocator allocator(heap);

    auto* p1 = allocator.allocate<uint64_t>();
    void* p2 = allocator.allocate(32);

    allocator.free(p1);
    allocator.free(p2);

    return 0;
}
