#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include <list>
#include <algorithm>

class TLSFAllocator {
public:
    explicit TLSFAllocator(std::size_t memoryPoolSize);
    ~TLSFAllocator();
    void* allocate(std::size_t size);
    void deallocate(void* ptr);
    void* getMemoryPoolStart() const;
    std::size_t getMemoryPoolSize() const;
    std::size_t getMaxAvailableBlockSize() const;
    TLSFAllocator(const TLSFAllocator&) = delete;
    TLSFAllocator& operator=(const TLSFAllocator&) = delete;
    TLSFAllocator(TLSFAllocator&&) = delete;
    TLSFAllocator& operator=(TLSFAllocator&&) = delete;
private:
    struct Block {
        std::size_t offset; // offset from pool start
        std::size_t size;   // size of block
        bool free;
    };
    void* memoryPool;
    std::size_t poolSize;
    std::list<Block> blocks; // maintain order by offset
    std::size_t findFreeBlock(std::size_t size);
    void splitBlock(std::list<Block>::iterator it, std::size_t size);
    void coalesce();
};
