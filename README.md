# JMalloc

An implementation of `malloc`, `free`, and `realloc`.

Uses `mmap` to expand and `munmap` to contract the heap.
Uses a linked list to keep track of allocated buffers.

Running `test` results in the following output:

```
jmalloc(400)
Allocated 400 bytes at 0x108c36018
jmalloc(400)
Allocated 400 bytes at 0x108c361c0
jmalloc(400)
Allocated 400 bytes at 0x108c36368
jfree(0x108c361c0)
jmalloc(200)
Found empty slot: 108c361c0
Allocated 200 bytes at 0x108c361c0
jmalloc(200)
Allocated 200 bytes at 0x108c36510
jrealloc(0x108c361c0, 240)
jrealloc(0x108c36510, 4096)
jrealloc - expanding heap
jfree(0x108c36018)
jfree(0x108c36368)
jfree(0x108c361c0)
jfree(0x108c36510)
jfree - shrinking heap
jmalloc(16384)
jmalloc - expanding heap
Allocated 16384 bytes at 0x108c36510
jfree(0x108c36510)
jfree - shrinking heap
```
