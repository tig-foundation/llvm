#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <stdint.h>

#define ARENA_SIZE (1024UL * 1024 * 1024 * 32) // 32gb
#define ALLOC_MAGIC 0xC001C0DE
#define ARENA_FIXED_ADDRESS ((void*)0x40000000000)

typedef struct AllocHeader {
    size_t size;
    unsigned int magic;
    struct AllocHeader* next;
} AllocHeader;

static void* s_arena_start = NULL;
static size_t s_arena_used = 0;
static AllocHeader* s_free_list_head = NULL;

static pthread_once_t s_init_once = PTHREAD_ONCE_INIT;
static pthread_mutex_t s_alloc_mutex = PTHREAD_MUTEX_INITIALIZER;

static void write_stderr(const char *msg) {
    write(STDERR_FILENO, msg, strlen(msg));
}

void init_allocator() {
    s_arena_start = mmap(
        ARENA_FIXED_ADDRESS,
        ARENA_SIZE,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE,
        -1,
        0
    );
    if (s_arena_start == MAP_FAILED) {
        perror("FATAL: Failed to mmap custom memory arena at fixed address");
        if (errno == EEXIST) {
            write_stderr("Reason: The address range is already in use.\n");
        }
        exit(errno);
    }
    if (s_arena_start != ARENA_FIXED_ADDRESS) {
        write_stderr("FATAL: mmap did not return the requested fixed address.\n");
        exit(1);
    }
    printf("--- Custom Malloc Initialized ---\nArena start address successfully mapped at: %p, size: %p\n", s_arena_start, ARENA_SIZE);
}

static AllocHeader *find_free_block(size_t size) {
    AllocHeader *current = s_free_list_head;
    AllocHeader *prev = NULL;
    while (current) {
        if (current->size >= size) {
            if (prev) { prev->next = current->next; }
            else { s_free_list_head = current->next; }
            return current;
        }
        prev = current;
        current = current->next;
    }
    return NULL;
}

void *internal_malloc(size_t size) {
    if (size == 0) return NULL;
    
    AllocHeader *header = NULL;
    pthread_mutex_lock(&s_alloc_mutex);
    header = find_free_block(size);
    if (!header) {
        size_t total_size = sizeof(AllocHeader) + size;
        if (s_arena_used + total_size > ARENA_SIZE) {
            pthread_mutex_unlock(&s_alloc_mutex);
            write_stderr("Custom malloc: Out of memory in arena.\n");
            return NULL;
        }
        header = (AllocHeader *)((char *)(s_arena_start) + s_arena_used);
        s_arena_used += total_size;
    }
    header->size = size;
    header->magic = ALLOC_MAGIC;
    header->next = NULL;
    pthread_mutex_unlock(&s_alloc_mutex);
    return (void*)(header + 1);
}

void internal_free(void *ptr) {
    if (ptr == NULL) return;

    pthread_mutex_lock(&s_alloc_mutex);
    AllocHeader *header = ((AllocHeader *)(ptr)) - 1;
    if (header->magic != ALLOC_MAGIC) {
        write_stderr("FATAL: Invalid pointer or double free detected in custom_free().\n");
        pthread_mutex_unlock(&s_alloc_mutex);
        return;
    }
    
    header->next = s_free_list_head;
    s_free_list_head = header;
    header->magic = 0;
    pthread_mutex_unlock(&s_alloc_mutex);
}

void* aligned_malloc(size_t size, size_t align) {
    pthread_once(&s_init_once, init_allocator);
    if (size == 0) return NULL;

    if (align < sizeof(void *)) {
        align = sizeof(void *);
    }
    
    size_t total_alloc_size = size + align + sizeof(AllocHeader) + sizeof(void *);
    void *raw_ptr = internal_malloc(total_alloc_size);
    if (!raw_ptr) return NULL;

    uintptr_t aligned_addr = ((uintptr_t)(raw_ptr) + sizeof(void *) + align - 1) & ~(align - 1);
    void *aligned_ptr = (void *)(aligned_addr);
    
    ((void **)(aligned_ptr))[-1] = raw_ptr;

    return aligned_ptr;
}

void aligned_free(void *ptr) {
    if (!ptr) return;
    void* raw_ptr = ((void **)(ptr))[-1];
    internal_free(raw_ptr);
}

void *__rust_alloc(size_t size, size_t align) {
    return aligned_malloc(size, align);
}

void __rust_dealloc(void *ptr, size_t size, size_t align) {
    (void)size;
    (void)align;
    aligned_free(ptr);
}

void *__rust_realloc(void *ptr, size_t old_size, size_t align, size_t new_size) {
    (void)old_size;
    if (!ptr) {
        return aligned_malloc(new_size, align);
    }
    if (new_size == 0) {
        aligned_free(ptr);
        return NULL;
    }
    
    void *new_ptr = aligned_malloc(new_size, align);
    if (!new_ptr) return NULL;
    
    memcpy(new_ptr, ptr, old_size < new_size ? old_size : new_size);
    aligned_free(ptr);
    
    return new_ptr;
}

void *__rust_alloc_zeroed(size_t size, size_t align) {
    void *ptr = aligned_malloc(size, align);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

void *calloc(size_t nmemb, size_t size) {
    void *ptr =  aligned_malloc(nmemb * size, sizeof(void*));
    if (ptr) {
        memset(ptr, 0, nmemb * size);
    }
    return ptr;
}

void *realloc(void *ptr, size_t size) {
    if (!ptr) {
        return aligned_malloc(size, sizeof(void*));
    }
    if (size == 0) {
        aligned_free(ptr);
        return NULL;
    }
    void *new_ptr = aligned_malloc(size, sizeof(void*));
    if (!new_ptr) return NULL;
    memcpy(new_ptr, ptr, size);
    aligned_free(ptr);
    return new_ptr;
}

void free(void *ptr) {
    aligned_free(ptr);
}

void *malloc(size_t size) {
    return aligned_malloc(size, sizeof(void*));
}
