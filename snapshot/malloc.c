#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#define ARENA_SIZE (1024UL * 1024 * 1024 * 10) // 10 GB
#define ALLOC_MAGIC 0xDEADBEEF

#define ARENA_FIXED_ADDRESS ((void*)0x200000000000)

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

static void write_stderr(const char* msg) {
    write(STDERR_FILENO, msg, strlen(msg));
}

void init_allocator() {
    s_arena_start = mmap(
        ARENA_FIXED_ADDRESS,
        ARENA_SIZE,
        PROT_READ | PROT_WRITE,
        // Use MAP_FIXED_NOREPLACE to demand the address but fail if it's taken
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE,
        -1,
        0
    );

    if (s_arena_start == MAP_FAILED) {
        perror("FATAL: Failed to mmap custom memory arena at fixed address");
        if (errno == EEXIST) {
            write_stderr("Reason: The address range is already in use.\n");
            write_stderr("Check /proc/self/maps to see what is mapped there.\n");
        }
        exit(1);
    }
    
    if (s_arena_start != ARENA_FIXED_ADDRESS) {
        write_stderr("FATAL: mmap did not return the requested fixed address.\n");
        exit(1);
    }

    printf("--- Custom Malloc Initialized ---\nArena start address successfully mapped at: %p\n", s_arena_start);
}

static AllocHeader* find_free_block(size_t size) {
    AllocHeader* current = s_free_list_head;
    AllocHeader* prev = NULL;
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

void* malloc(size_t size) {
    pthread_once(&s_init_once, init_allocator);
    if (size == 0) return NULL;
    
    size = (size + 7) & ~7; // align to 8 bytes
    AllocHeader* header = NULL;
    void* user_ptr = NULL;
    pthread_mutex_lock(&s_alloc_mutex);
    header = find_free_block(size);
    if (!header) {
        size_t total_size = sizeof(AllocHeader) + size;
        if (s_arena_used + total_size > ARENA_SIZE) {
            pthread_mutex_unlock(&s_alloc_mutex);
            write_stderr("Custom malloc: Out of memory in arena.\n");
            return NULL;
        }

        header = (AllocHeader*)((char*)s_arena_start + s_arena_used);
        s_arena_used += total_size;
    }

    header->size = size;
    header->magic = ALLOC_MAGIC;
    header->next = NULL;
    user_ptr = (void*)(header + 1);
    pthread_mutex_unlock(&s_alloc_mutex);

    return user_ptr;
}

void free(void* ptr) {
    if (ptr == NULL) return;

    pthread_mutex_lock(&s_alloc_mutex);
    AllocHeader* header = (AllocHeader*)ptr - 1;
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

void* realloc(void* ptr, size_t new_size) {
    if (!ptr) return malloc(new_size);
    if (new_size == 0) { free(ptr); return NULL; }

    AllocHeader* header = (AllocHeader*)ptr - 1;
    size_t old_size = header->size;
    if (new_size <= old_size) return ptr;
    
    void* new_ptr = malloc(new_size);
    if (!new_ptr) return NULL;

    memcpy(new_ptr, ptr, old_size);
    free(ptr);
    return new_ptr;
}

void* calloc(size_t nmemb, size_t size) {
    size_t total_size = nmemb * size;
    if (size != 0 && total_size / size != nmemb) return NULL;
    void* ptr = malloc(total_size);
    if (ptr) { memset(ptr, 0, total_size); }
    return ptr;
}