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
#include <stdbool.h>
#include <limits.h>

// --- Arena Configuration ---
#define MAIN_ARENA_SIZE         (1024UL * 1024 * 512)     // 512MB for user allocations
#define METADATA_ARENA_SIZE     (1024UL * 1024 * 64)      // 64MB for metadata (grows down)
#define MAIN_ARENA_ADDRESS      ((void*)0x40000000000)
#define METADATA_ARENA_ADDRESS  ((void*)0x50000000000)

// --- Allocator Constants ---
#define ALLOC_MAGIC             0xC001C0DE
#define MIN_ALIGNMENT           sizeof(void*)
#define HUGEPAGE_SIZE_2MB       (2 * 1024 * 1024)

// --- Core Data Structures ---
typedef struct AllocHeader {
    size_t block_size;  // Total size of the block, including header/footer
    size_t user_size;   // Size requested by the user
    uint32_t magic;
    bool is_free;
    struct AllocHeader* next_free;
    struct AllocHeader* prev_free;
} AllocHeader;

typedef struct BlockFooter {
    size_t block_size;
} BlockFooter;

#define MIN_BLOCK_SIZE (sizeof(AllocHeader) + sizeof(BlockFooter))

// --- Global State ---
static void* s_main_arena_start = NULL;
static size_t s_main_arena_size = 0;
static AllocHeader* s_main_free_list = NULL;
static pthread_mutex_t s_main_mutex = PTHREAD_MUTEX_INITIALIZER;

static void* s_metadata_arena_start = NULL;
static size_t s_metadata_arena_size = 0;
static void* s_metadata_current = NULL; // Current allocation pointer for downward growth
static pthread_mutex_t s_metadata_mutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_once_t s_init_once = PTHREAD_ONCE_INIT;

// --- Forward Declarations ---
void init_allocator();
void* aligned_malloc(size_t size, size_t align);
void aligned_free(void* ptr);
void* metadata_alloc(size_t size);

// --- Stubbed Helper Functions (for portability) ---
// TODO: Replace these with a real implementation for your target OS (e.g., parse /proc/meminfo)
static size_t get_hugepage_size() {
    return HUGEPAGE_SIZE_2MB; // Assume 2MB for simplicity
}

static bool has_hugepages_available(size_t required_size) {
    (void)required_size; // Unused parameter
    // On a real system, check if enough huge pages are free.
    // Returning false makes the allocator fall back to standard pages safely.
    return false;
}

// --- Helper Functions ---
static size_t align_up(size_t size, size_t align) {
    return (size + align - 1) & ~(align - 1);
}

// --- Internal Core Allocator (Main Arena) ---
// NOTE: These functions assume the s_main_mutex is already held.

static void add_to_free_list(AllocHeader* header) {
    header->is_free = true;
    header->next_free = s_main_free_list;
    header->prev_free = NULL;
    if (s_main_free_list) {
        s_main_free_list->prev_free = header;
    }
    s_main_free_list = header;
}

static void remove_from_free_list(AllocHeader* header) {
    if (header->prev_free) {
        header->prev_free->next_free = header->next_free;
    } else {
        s_main_free_list = header->next_free;
    }
    if (header->next_free) {
        header->next_free->prev_free = header->prev_free;
    }
    header->is_free = false;
}

// Allocates a raw block from the main arena. Not alignment-aware.
// Assumes s_main_mutex is held.
static void* block_alloc(size_t size) {
    size_t total_block_size = align_up(size + sizeof(AllocHeader) + sizeof(BlockFooter), MIN_ALIGNMENT);
    if (total_block_size < MIN_BLOCK_SIZE) {
        total_block_size = MIN_BLOCK_SIZE;
    }

    AllocHeader* block = NULL;
    for (AllocHeader* current = s_main_free_list; current; current = current->next_free) {
        if (current->is_free && current->block_size >= total_block_size) {
            block = current;
            break;
        }
    }

    if (!block) return NULL; // Out of memory

    remove_from_free_list(block);

    size_t remaining_size = block->block_size - total_block_size;
    if (remaining_size >= MIN_BLOCK_SIZE) {
        block->block_size = total_block_size;
        BlockFooter* footer = (BlockFooter*)((char*)block + block->block_size - sizeof(BlockFooter));
        footer->block_size = block->block_size;

        AllocHeader* remainder = (AllocHeader*)((char*)block + block->block_size);
        remainder->block_size = remaining_size;
        remainder->magic = ALLOC_MAGIC;
        BlockFooter* remainder_footer = (BlockFooter*)((char*)remainder + remaining_size - sizeof(BlockFooter));
        remainder_footer->block_size = remaining_size;
        add_to_free_list(remainder); // Add the new smaller block to the free list
    }

    block->magic = ALLOC_MAGIC;
    return (void*)block;
}

// Frees a raw block of memory.
// Assumes s_main_mutex is held.
static void block_free(void* block_ptr) {
    if (!block_ptr) return;
    
    AllocHeader* header = (AllocHeader*)block_ptr;
    if (header->magic != ALLOC_MAGIC || header->is_free) {
        // Double free or invalid pointer
        return;
    }

    // Coalesce with next block
    AllocHeader* next_block = (AllocHeader*)((char*)header + header->block_size);
    if ((void*)next_block < s_main_arena_start + s_main_arena_size && next_block->magic == ALLOC_MAGIC && next_block->is_free) {
        remove_from_free_list(next_block);
        header->block_size += next_block->block_size;
    }

    // Coalesce with previous block
    BlockFooter* prev_footer = (BlockFooter*)((char*)header - sizeof(BlockFooter));
    if ((void*)prev_footer >= s_main_arena_start) { // Boundary check
        AllocHeader* prev_block = (AllocHeader*)((char*)header - prev_footer->block_size);
        if (prev_block->magic == ALLOC_MAGIC && prev_block->is_free) {
            remove_from_free_list(prev_block);
            prev_block->block_size += header->block_size;
            header = prev_block;
        }
    }

    BlockFooter* footer = (BlockFooter*)((char*)header + header->block_size - sizeof(BlockFooter));
    footer->block_size = header->block_size;
    add_to_free_list(header);
}

// --- Public-Facing Aligned Allocator ---
void* aligned_malloc(size_t size, size_t align) {
    if (size == 0) return NULL;
    pthread_once(&s_init_once, init_allocator);

    if (align == 0 || (align & (align - 1)) != 0) align = MIN_ALIGNMENT;
    if (align < MIN_ALIGNMENT) align = MIN_ALIGNMENT;

    // Request enough memory to guarantee alignment can be met, plus space for the header pointer
    size_t total_request_size = size + align - 1 + sizeof(void*);

    pthread_mutex_lock(&s_main_mutex);
    void* raw_block = block_alloc(total_request_size);
    pthread_mutex_unlock(&s_main_mutex);

    if (!raw_block) return NULL;

    // Align the pointer and store the original block address just before it
    void* user_ptr_start = (void*)((char*)raw_block + sizeof(AllocHeader) + sizeof(void*));
    void* aligned_ptr = (void*)align_up((uintptr_t)user_ptr_start, align);
    
    ((void**)aligned_ptr)[-1] = raw_block;

    // Store the original user size for realloc
    AllocHeader* header = (AllocHeader*)raw_block;
    header->user_size = size;

    return aligned_ptr;
}

void aligned_free(void* ptr) {
    if (!ptr) return;
    
    // Retrieve the original block address from the stored pointer
    void* raw_block = ((void**)ptr)[-1];

    pthread_mutex_lock(&s_main_mutex);
    block_free(raw_block);
    pthread_mutex_unlock(&s_main_mutex);
}

// --- Metadata Allocator (Simple Bump Allocator) ---
void* metadata_alloc(size_t size) {
    pthread_once(&s_init_once, init_allocator);
    size = align_up(size, MIN_ALIGNMENT);

    pthread_mutex_lock(&s_metadata_mutex);
    
    if ((char*)s_metadata_current - size < (char*)s_metadata_arena_start) {
        // Out of metadata memory
        pthread_mutex_unlock(&s_metadata_mutex);
        return NULL;
    }
    
    // Allocate downwards
    s_metadata_current = (char*)s_metadata_current - size;
    void* ptr = s_metadata_current;
    
    pthread_mutex_unlock(&s_metadata_mutex);
    return ptr;
}

// --- Standard C Library Overrides ---
void* malloc(size_t size) {
    return aligned_malloc(size, MIN_ALIGNMENT);
}

void free(void* ptr) {
    aligned_free(ptr);
}

void* calloc(size_t nmemb, size_t size) {
    if (nmemb > 0 && SIZE_MAX / nmemb < size) return NULL; // Check for overflow
    size_t total_size = nmemb * size;
    void* ptr = aligned_malloc(total_size, MIN_ALIGNMENT);
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

void* realloc(void* ptr, size_t new_size) {
    if (!ptr) return aligned_malloc(new_size, MIN_ALIGNMENT);
    if (new_size == 0) {
        aligned_free(ptr);
        return NULL;
    }
    
    void* raw_block = ((void**)ptr)[-1];
    AllocHeader* header = (AllocHeader*)raw_block;
    size_t old_user_size = header->user_size;

    // Optimization: If the new size fits in the current block, do nothing.
    // A more complex implementation could try to expand in place.
    if (new_size <= old_user_size) {
        header->user_size = new_size;
        return ptr;
    }

    void* new_ptr = aligned_malloc(new_size, MIN_ALIGNMENT);
    if (!new_ptr) return NULL;
    
    // memcpy knows the exact old user size now, which is safe.
    memcpy(new_ptr, ptr, old_user_size);
    aligned_free(ptr);
    
    return new_ptr;
}

// --- Initializer ---
void init_allocator() {
    // === MAIN ARENA (User allocations) ===
    int main_flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE;
    s_main_arena_size = MAIN_ARENA_SIZE;
    
    // Try hugepages first
    size_t hugepage_size = get_hugepage_size();
    if (hugepage_size > 0 && has_hugepages_available(MAIN_ARENA_SIZE)) {
        s_main_arena_size = align_up(MAIN_ARENA_SIZE, hugepage_size);
        main_flags |= MAP_HUGETLB;
        printf("Attempting to use hugepages for main arena\n");
    }
    
    s_main_arena_start = mmap(MAIN_ARENA_ADDRESS, s_main_arena_size, PROT_READ | PROT_WRITE, main_flags, -1, 0);
    
    if (s_main_arena_start == MAP_FAILED && (main_flags & MAP_HUGETLB)) {
        // Fallback without hugepages
        printf("Hugepage allocation failed, falling back to standard pages\n");
        s_main_arena_size = MAIN_ARENA_SIZE;
        main_flags &= ~MAP_HUGETLB;
        s_main_arena_start = mmap(MAIN_ARENA_ADDRESS, s_main_arena_size, PROT_READ | PROT_WRITE, main_flags, -1, 0);
    }
    
    if (s_main_arena_start == MAP_FAILED) {
        perror("FATAL: Failed to allocate main arena");
        exit(errno);
    }

    // Initialize the main arena with a single large free block
    AllocHeader* first_block = (AllocHeader*)s_main_arena_start;
    first_block->block_size = s_main_arena_size;
    first_block->user_size = 0;
    first_block->magic = ALLOC_MAGIC;
    BlockFooter* first_footer = (BlockFooter*)((char*)first_block + s_main_arena_size - sizeof(BlockFooter));
    first_footer->block_size = s_main_arena_size;
    add_to_free_list(first_block); // Initializes the free list
    
    // === METADATA ARENA (Snapshots, etc.) ===
    int metadata_flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE;
    s_metadata_arena_size = METADATA_ARENA_SIZE;
    s_metadata_arena_start = mmap(METADATA_ARENA_ADDRESS, s_metadata_arena_size, PROT_READ | PROT_WRITE, metadata_flags, -1, 0);
    
    if (s_metadata_arena_start == MAP_FAILED) {
        perror("FATAL: Failed to allocate metadata arena");
        // Consider cleanup of main arena before exiting
        munmap(s_main_arena_start, s_main_arena_size);
        exit(errno);
    }
    
    // Initialize metadata bump pointer to the top of the arena
    s_metadata_current = (char*)s_metadata_arena_start + s_metadata_arena_size;
    
    printf("--- Dual Arena Allocator Initialized ---\n");
    printf("Main arena:     %p, size: %zu MB\n", s_main_arena_start, s_main_arena_size / (1024 * 1024));
    printf("Metadata arena: %p, size: %zu MB (grows down)\n", s_metadata_arena_start, s_metadata_arena_size / (1024 * 1024));
}

// Add these at the end before init_allocator()
void* __rust_alloc(size_t size, size_t align) {
    return aligned_malloc(size, align);
}

void __rust_dealloc(void* ptr, size_t old_size, size_t align) {
    (void)old_size; (void)align;
    aligned_free(ptr);
}

void* __rust_realloc(void* ptr, size_t old_size, size_t align, size_t new_size) {
    if (!ptr) return aligned_malloc(new_size, align);
    if (new_size == 0) {
        aligned_free(ptr);
        return NULL;
    }

    void* new_ptr = aligned_malloc(new_size, align);
    if (!new_ptr) return NULL;

    void* raw_block = ((void**)ptr)[-1];
    AllocHeader* header = (AllocHeader*)raw_block;
    size_t copy_size = header->user_size < new_size ? header->user_size : new_size;
    memcpy(new_ptr, ptr, copy_size);
    aligned_free(ptr);

    return new_ptr;
}

void* __rust_alloc_zeroed(size_t size, size_t align) {
    void* ptr = aligned_malloc(size, align);
    if (ptr) memset(ptr, 0, size);
    return ptr;
}