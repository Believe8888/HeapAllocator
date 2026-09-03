#include "HeapAllocator.hpp"

#include <stdio.h>

HeapAllocator::HeapAllocator(Heap& heap) noexcept
    : heap(heap)
{}

void* HeapAllocator::reallocate(void* ptr, size_t size) noexcept
{
    if (!ptr)
        return allocate(size);

    if (size == 0) {
        free(ptr);
        return nullptr;
    }

    const auto block = reinterpret_cast<Heap::BlockHeader *>(uintptr_t(ptr) - sizeof(Heap::BlockHeader));
    const size_t payload_size = block->block_size - sizeof(Heap::BlockHeader);
    if (payload_size >= size)
        return ptr;

    void* new_ptr = allocate(size);
    if (new_ptr) {
        // copy values
        const size_t end = payload_size >> 3;
        for (size_t i = 0; i < end; ++i) {
            reinterpret_cast<uint64_t *>(new_ptr)[i] = reinterpret_cast<uint64_t *>(ptr)[i];
        }
        free(ptr);
        return new_ptr;
    }

    return nullptr;
}


void* HeapAllocator::allocate(size_t size) noexcept
{
    const size_t block_size = ALIGN_UP(sizeof(Heap::BlockHeader) + size, Heap::alignment);

    if (heap.heap_header->used_size + block_size > heap.heap_header->capacity)
        return nullptr;

    Heap::BlockHeader* block = find_free_block(block_size);
    if (!block) return nullptr;

    try_split_block(block, block_size);
    block->is_free = 0;

    heap.heap_header->used_size += block->block_size;

    return reinterpret_cast<void *>(uintptr_t(block) + sizeof(Heap::BlockHeader));
}

void HeapAllocator::free(void* ptr) noexcept {
    if (ptr) {
        auto* block = reinterpret_cast<Heap::BlockHeader *>(uintptr_t(ptr) - sizeof(Heap::BlockHeader));
        if (!block->is_free) {
            block->is_free = 1;
            heap.heap_header->used_size -= block->block_size;
            coalesce_adjacent_free_blocks(block);
        }
    }
}

Heap::BlockHeader* HeapAllocator::find_free_block(size_t required_size) const noexcept
{
    for (Heap::BlockHeader* b = first_block(); b; b = next_block(b)) {
        if (b->is_free && b->block_size >= required_size) {
            return b;
        }
    }
    return nullptr;
}

void HeapAllocator::coalesce_adjacent_free_blocks(Heap::BlockHeader* const block) noexcept
{
    if (!block) return;

    auto* const prev = block->prev;
    auto* const next = next_block(block);

    const bool prev_is_free = prev && prev->is_free;
    const bool next_is_free = next && next->is_free;

    if (prev_is_free && next_is_free) {
        prev->block_size += block->block_size + next->block_size;
        if (auto* const next_next = next_block(next)) {
            next_next->prev = prev;
        }
    }
    else if (prev_is_free) {
        prev->block_size += block->block_size;
        if (next) {
            next->prev = prev;
        }
    }
    else if (next_is_free) {
        block->block_size += next->block_size;
        if (auto* const next_next = next_block(next)) {
            next_next->prev = block;
        }
    }
}

bool HeapAllocator::try_split_block(Heap::BlockHeader* const block, size_t allocated_size) noexcept
{
    if (allocated_size >= block->block_size)
        return false;

    const size_t remaining_block_size = block->block_size - allocated_size;
    if (remaining_block_size >= Heap::minimum_block_size /* && IS_ALIGNED(split_block_size, alignment) */) {
        auto* const remaining_block = reinterpret_cast<Heap::BlockHeader *>(uintptr_t(block) + allocated_size);
        remaining_block->block_size = remaining_block_size;
        remaining_block->is_free    = 1;
        remaining_block->prev       = block;

        if (Heap::BlockHeader* const next = next_block(remaining_block))
            next->prev = remaining_block;

        block->block_size = allocated_size;

        return true;
    }
    return false;
}

Heap::BlockHeader* HeapAllocator::first_block() const noexcept {
    return reinterpret_cast<Heap::BlockHeader *>(uintptr_t(heap.heap_header) + sizeof(Heap::HeapHeader));
}

Heap::BlockHeader* HeapAllocator::next_block(const Heap::BlockHeader* const block) const noexcept
{
    const uintptr_t heap_end_address   = uintptr_t(heap.heap_header) + heap.heap_header->capacity;
    const uintptr_t next_block_address = uintptr_t(block) + block->block_size;

    if (next_block_address >= heap_end_address ||
        heap_end_address - next_block_address < Heap::minimum_block_size
    ) {
        return nullptr;
    }

    return reinterpret_cast<Heap::BlockHeader *>(next_block_address);
}

void HeapAllocator::print_data() const noexcept
{
    puts("----------------------------------------\n");
    printf("Heap usage: 0x%zX/0x%zX\n", heap.used_space(), heap.heap_header->capacity);

    size_t block_index = 0;
    for (Heap::BlockHeader* b = first_block(); b; b = next_block(b)) {
        printf("Block[%zu] | address: %p | size: 0x%X\n", block_index, b, b->block_size);

        printf("Prev block address: 0x%p\n", b->prev);
        if (b->is_free)
            printf("freed\n");
        else {
            for (size_t i = 0, end = b->block_size >> 3; i < end; ++i) {
                printf("%016llX\n", reinterpret_cast<u64 *>(b)[i]);

                // Little-endian format
                // この形式にする場合、ループ条件を i < b->block_size に変更する必要がある
                // printf(
                //     "%02X%s"
                //    ,reinterpret_cast<u8 *>(b)[i]
                //    ,((i + 1) & 7) == 0 ? "\t\n" : " "
                // );
            }
        }
        putchar('\n');
        ++block_index;
    }
}
