#ifndef JMALLOC_H_06262020
#define JMALLOC_H_06262020

void* jmalloc(size_t sz);
void  jfree(void* ptr);
void* jrealloc(void* ptr, size_t new_size); 

#endif
