#include "mempool.h"
#include "platform.h"
#include <cstdlib>

#define POOL_SIZE 1048576 // 1 MB

void release_pool(MemPool* pool) {

    if (pool->data != nullptr) {
        free(pool->data);
        pool->data = nullptr;
        pool->size = 0;
        pool->used = 0;
    }

}

void init_pool(MemPool* pool) {

    release_pool(pool);

    pool->data = (char*)malloc(POOL_SIZE);
    pool->size = POOL_SIZE;
    pool->used = 0;

}

void reset_pool(MemPool* pool) {

    pool->used = 0;

}

void* pool_alloc(void* user, unsigned int size) {

    MemPool* pool = (MemPool*)user;

    if (size > (pool->size - pool->used)) {
        logMsg("Ran out of pool AAAAHHHH\n");
        return nullptr;
    }

    void* ptr = pool->data + pool->used;

    pool->used += size;

    return ptr;

}

void pool_free(void* user, void* ptr) {
    // don't bother
}