#include "allocator.hpp"
#include <cstdlib>
#include <algorithm>

TLSFAllocator::TLSFAllocator(std::size_t memoryPoolSize) : poolSize(memoryPoolSize) {
    // Allocate raw memory pool
    memoryPool = std::malloc(poolSize);
    if (!memoryPool) {
        throw std::bad_alloc();
    }
    // Initially one large free block
    Block initial{0, poolSize, true};
    blocks.push_back(initial);
}

TLSFAllocator::~TLSFAllocator() {
    if (memoryPool) {
        std::free(memoryPool);
        memoryPool = nullptr;
    }
    blocks.clear();
}

void* TLSFAllocator::getMemoryPoolStart() const {
    return memoryPool;
}

std::size_t TLSFAllocator::getMemoryPoolSize() const {
    return poolSize;
}

std::size_t TLSFAllocator::getMaxAvailableBlockSize() const {
    std::size_t maxSize = 0;
    for (const auto& blk : blocks) {
        if (blk.free) {
            maxSize = std::max(maxSize, blk.size);
        }
    }
    return maxSize;
}

std::size_t TLSFAllocator::findFreeBlock(std::size_t size) {
    // Simple first-fit search, returns offset or throws
    for (auto it = blocks.begin(); it != blocks.end(); ++it) {
        if (it->free && it->size >= size) {
            // Split if larger than needed
            if (it->size > size) {
                splitBlock(it, size);
            }
            it->free = false;
            return it->offset;
        }
    }
    // No suitable block
    throw std::bad_alloc();
}

void TLSFAllocator::splitBlock(std::list<Block>::iterator it, std::size_t size) {
    // Create a new free block for the remaining part
    std::size_t remainingSize = it->size - size;
    std::size_t newOffset = it->offset + size;
    Block remaining{newOffset, remainingSize, true};
    // Adjust current block size
    it->size = size;
    // Insert the remaining block after current
    blocks.insert(std::next(it), remaining);
}

void TLSFAllocator::coalesce() {
    if (blocks.empty()) return;
    for (auto it = blocks.begin(); it != blocks.end();) {
        auto nextIt = std::next(it);
        if (nextIt != blocks.end() && it->free && nextIt->free) {
            // Merge next into it
            it->size += nextIt->size;
            blocks.erase(nextIt);
            // stay on same it to possibly merge further
        } else {
            ++it;
        }
    }
}

void* TLSFAllocator::allocate(std::size_t size) {
    // Align size to at least 8 bytes for simplicity
    const std::size_t alignment = 8;
    size = ((size + alignment - 1) / alignment) * alignment;
    std::size_t offset = findFreeBlock(size);
    return static_cast<char*>(memoryPool) + offset;
}

void TLSFAllocator::deallocate(void* ptr) {
    if (!ptr) return;
    std::size_t offset = static_cast<char*>(ptr) - static_cast<char*>(memoryPool);
    // Find the block with this offset
    for (auto it = blocks.begin(); it != blocks.end(); ++it) {
        if (it->offset == offset) {
            it->free = true;
            // Try to merge with neighbours
            coalesce();
            return;
        }
    }
    // If not found, ignore (could also throw)
}
