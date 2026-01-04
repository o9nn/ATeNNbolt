
#ifndef MEMORY_POOL_HPP
#define MEMORY_POOL_HPP

#include <cstddef>
#include <vector>
#include <mutex>
#include <memory>

namespace bolt {

class MemoryPool {
private:
    struct Block {
        void* data;
        bool used;
        size_t size;
        Block* next;
    };

    std::mutex mtx;
    Block* freeList;
    size_t blockSize;
    size_t totalSize;
    std::vector<void*> memoryChunks;

    Block* findFreeBlock(size_t size) {
        Block* current = freeList;
        while (current) {
            if (!current->used && current->size >= size) {
                return current;
            }
            current = current->next;
        }
        return nullptr;
    }

public:
    explicit MemoryPool(size_t initialSize = 1024 * 1024) // 1MB default
        : freeList(nullptr), blockSize(initialSize), totalSize(0) {
        expandPool();
    }

    ~MemoryPool() {
        for (void* chunk : memoryChunks) {
            ::operator delete(chunk);
        }
    }

    void* allocate(size_t size) {
        std::lock_guard<std::mutex> lock(mtx);
        
        Block* block = findFreeBlock(size);
        if (!block) {
            expandPool();
            block = findFreeBlock(size);
        }

        if (block) {
            block->used = true;
            return block->data;
        }
        
        return nullptr;
    }

    void deallocate(void* ptr) {
        std::lock_guard<std::mutex> lock(mtx);
        
        Block* current = freeList;
        while (current) {
            if (current->data == ptr) {
                current->used = false;
                // Coalesce adjacent free blocks
                coalesceFreeBlocks();
                return;
            }
            current = current->next;
        }
    }

private:
    void expandPool() {
        void* newChunk = ::operator new(blockSize);
        memoryChunks.push_back(newChunk);

        Block* newBlock = new Block{newChunk, false, blockSize, freeList};
        freeList = newBlock;
        totalSize += blockSize;
    }

    void coalesceFreeBlocks() {
        Block* current = freeList;
        while (current && current->next) {
            if (!current->used && !current->next->used) {
                current->size += current->next->size;
                Block* temp = current->next;
                current->next = temp->next;
                delete temp;
            } else {
                current = current->next;
            }
        }
    }
};

template<typename T>
class PoolAllocator {
    static MemoryPool pool;

public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    template<typename U>
    struct rebind {
        using other = PoolAllocator<U>;
    };

    pointer allocate(size_type n) {
        size_type size = n * sizeof(T);
        void* p = pool.allocate(size);
        if (!p) throw std::bad_alloc();
        return static_cast<pointer>(p);
    }

    void deallocate(pointer p, size_type) {
        pool.deallocate(p);
    }

    template<typename U, typename... Args>
    void construct(U* p, Args&&... args) {
        new(p) U(std::forward<Args>(args)...);
    }

    template<typename U>
    void destroy(U* p) {
        p->~U();
    }
};

template<typename T>
MemoryPool PoolAllocator<T>::pool;

} // namespace bolt

#endif
