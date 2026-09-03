#pragma once

#include "Heap.hpp"

class HeapAllocator {
public:
    explicit HeapAllocator(Heap& heap) noexcept;

    template <class T>
    auto allocate() noexcept {
        return reinterpret_cast<T *>(allocate(sizeof(T)));
    }

    void* reallocate(void* ptr, size_t size) noexcept;
    void* allocate(size_t size)              noexcept;
    void  free(void* ptr)                    noexcept;

    void print_data() const noexcept;
private:
    Heap::BlockHeader* first_block()                                    const noexcept;
    Heap::BlockHeader* next_block(const Heap::BlockHeader* const block) const noexcept;
    Heap::BlockHeader* find_free_block(size_t required_size)            const noexcept;
    void coalesce_adjacent_free_blocks(Heap::BlockHeader* const block)        noexcept;

    bool try_split_block(Heap::BlockHeader* const block, size_t allocated_size) noexcept;
private:
    Heap& heap;
};
