#pragma once

struct MemPool {

    char* data = nullptr;
    unsigned int size = 0;
    unsigned int used = 0;

};

void release_pool(MemPool* pool);

void init_pool(MemPool* pool);

void reset_pool(MemPool* pool);

void* pool_alloc(void* user, unsigned int size);

void pool_free(void* user, void* ptr);
