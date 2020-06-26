# JMalloc

An implementation of `malloc`, `free`, and `realloc`.

Uses `mmap` to expand and `munmap` to contract the heap.
Uses a linked list to keep track of allocated buffers.
