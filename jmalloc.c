#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/mman.h>

#ifdef NDEBUG
#define dbgprintf(...) do{}while(0)
#else
#define dbgprintf(...) printf(__VA_ARGS__)
#endif

// linked list that stores pointers to jmalloced arrays
struct heap_ptr
{
    void* addr;
    size_t len;
    struct heap_ptr* next;
};

int pgsize; // size of a page - I'd like it to be const, but don't think it'll happen
void* h_start = NULL; // start of the heap
void* h_ptr = NULL;   // start of unallocated memory on the heap
void* h_end = NULL;   // end of the heap - grows when needed

struct heap_ptr *heap_lst = NULL;     // First element of the heap_ptr list
struct heap_ptr *heap_lst_end = NULL; // Last element of the heap_ptr list

size_t hp_sz = sizeof(struct heap_ptr); // size of a heap_ptr

/**
 *  The jmalloc function allocates memory on the heap. To do this,
 *  it uses mmap with anonymous pages to acquire memory for the program.
 *  Every malloced pointer is stored in a structure called a heap_ptr,
 *  which knows both the pointer to the memory and its length. To simplify
 *  heap structure, this heap_ptr is stored immediately before the allocated
 *  memory.
 **/
void* jmalloc(size_t sz)
{
    dbgprintf("jmalloc(%zu)\n", sz);
    static bool initialized = false;
    if(!initialized)
    {
        pgsize = sysconf(_SC_PAGESIZE);
        // Grab one page
        h_start = mmap(0, pgsize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        if(h_start == (void*) -1)
        {
            perror("0: mmap returned -1");
            return NULL;
        }

        h_ptr = h_start;
        h_end = h_start + pgsize;
        initialized = true;
    }

    assert(h_start <= h_ptr);
    assert(h_ptr <= h_end);

    // Potentially find an empty slot
    struct heap_ptr** tracer = &heap_lst;
    while(*tracer != NULL)
    {
        if((**tracer).next != NULL &&
                sz + hp_sz < ((void*)(**tracer).next - ((**tracer).addr + (**tracer).len)))
        {
            // empty slot found
            dbgprintf("Found empty slot: %" PRIxPTR "\n", (uintptr_t)((**tracer).addr + (**tracer).len) + hp_sz);
            struct heap_ptr* ins = (**tracer).addr + (**tracer).len;
            ins->addr = (void*)ins + hp_sz;
            ins->len = sz;
            ins->next = (**tracer).next;
            (**tracer).next = ins;
            dbgprintf("Allocated %zu bytes at 0x%" PRIxPTR "\n", sz, (uintptr_t)ins->addr);
            return ins->addr;
        } 
        

        tracer = &((**tracer).next);
    }

    // If no empty slot, write to the end
    if((h_end - h_ptr) < sz + hp_sz)
    {
        // need to allocate more memory
        dbgprintf("jmalloc - expanding heap\n");
        size_t more = sz + hp_sz - (h_end - h_ptr);
        size_t pgs = (more / pgsize) + (more % pgsize ? 1 : 0);
        void* added = mmap(h_end, pgs * pgsize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_FIXED | MAP_PRIVATE, -1, 0);
        if(added == (void*) -1)
        {
            perror("1: mmap returned -1");
            return NULL;
        }
        assert(added == h_end);
        h_end = added + pgs * pgsize;
    }
    *tracer = h_ptr;
    (**tracer).addr = h_ptr + hp_sz;
    (**tracer).len = sz;
    (**tracer).next = NULL;
    h_ptr += sz + hp_sz;
    heap_lst_end = *tracer; // might end up removing heap_lst_end
    dbgprintf("Allocated %zu bytes at 0x%" PRIxPTR "\n", sz, (uintptr_t)heap_lst_end->addr);
    return heap_lst_end->addr;
}

void jfree(void* ptr)
{
    dbgprintf("jfree(0x%" PRIxPTR ")\n", (uintptr_t)ptr);

    for(struct heap_ptr* prev = NULL, *next = heap_lst; next != NULL; prev = next, next = next->next)
    {
        if(next->addr == ptr){
            size_t nlen = next->len;
            next->addr = NULL;
            next->len = 0;
            if(prev != NULL)
                prev->next = next->next;
            if(next == heap_lst)
                heap_lst = next->next;
            next->next = NULL;

            if(next == heap_lst_end)
            {
                h_ptr -= nlen + hp_sz;
                heap_lst_end = prev;

                assert(h_start <= h_ptr);
                assert(h_ptr <= h_end);

                // shrink the heap if there are empty pages
                if(h_end - h_ptr >= pgsize)
                {
                    dbgprintf("jfree - shrinking heap\n");
                    size_t pgs = (h_end - h_ptr) / pgsize;
                    h_end -= pgs * pgsize;
                    munmap(h_end, pgs * pgsize);
                }
            }
            return;
        }
    }
    fprintf(stderr, "Potential Double Free of 0x%" PRIxPTR "!\n", (uintptr_t)ptr);
    abort();
}

void *jrealloc(void* ptr, size_t new_size)
{
    dbgprintf("jrealloc(0x%" PRIxPTR ", %zu)\n", (uintptr_t)ptr, new_size);
    for(struct heap_ptr* hp = heap_lst; hp != NULL; hp = hp->next)
    {
        if(hp->addr == ptr)
        {
            if(hp == heap_lst_end)
            {
                if((h_end - hp->addr) < new_size)
                {
                    // need to allocate more memory
                    dbgprintf("jrealloc - expanding heap\n");
                    size_t more = new_size - (h_end - hp->addr);;
                    size_t pgs = (more / pgsize) + (more % pgsize ? 1 : 0);
                    void* added = mmap(h_end, pgs * pgsize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_FIXED | MAP_PRIVATE, -1, 0);
                    if(added == (void*) -1)
                    {
                        perror("2: mmap returned -1");
                        return NULL;
                    }
                    assert(added == h_end);
                    h_end = added + pgs * pgsize;
                }

                h_ptr += new_size - hp->len;
                hp->len = new_size;
                return hp->addr;
            }

            if(new_size <= hp->len || (void*)hp->next - hp->addr >= new_size)
            {
                hp->len = new_size;
                return hp->addr;
            }

            void* ret = jmalloc(new_size);
            memcpy(ret, hp->addr, hp->len);
            jfree(hp->addr);
            return ret;
        }
    }
    fprintf(stderr, "Cannot jrealloc 0x%" PRIxPTR " because it has not be previously jmalloced\n", (uintptr_t)ptr);
    abort();
}
