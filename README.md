# HeapAllocator

A 16-byte aligned custom heap allocator for Windows.

## Features

- Guaranteed 16-byte alignment
- Automatic block splitting & coalescing
- Zero dependencies (built on the Windows API's `VirtualAlloc` function)

## Thread Safety
This allocator is not thread-safe.
Concurrent access to the same allocator instance is not supported.

## Usage

```cpp
#include "Heap.hpp"
#include "HeapAllocator.hpp"

int main() {
    Heap heap(0x1000);
    HeapAllocator allocator(heap);

    auto* p1 = allocator.allocate<int>();
    void* p2 = allocator.allocate(32);

    allocator.free(p1);
    allocator.free(p2);

    return 0;
}
```
