#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "jmalloc.h"

int main(void)
{
    int pgsize = sysconf(_SC_PAGESIZE);
    srand(time(NULL));
    int *p0 = jmalloc(100 * sizeof(int));
    int *p1 = jmalloc(100 * sizeof(int));
    int *p2 = jmalloc(100 * sizeof(int));
    jfree(p1);
    int *p3 = jmalloc(50 * sizeof(int));
    int *p4 = jmalloc(50 * sizeof(int));

    p3 = jrealloc(p3, 60 * sizeof(int));
    p4 = jrealloc(p4, (pgsize / sizeof(int)) * sizeof(int));

    jfree(p0);
    jfree(p2);
    jfree(p3);
    jfree(p4);

    int *p5 = jmalloc(pgsize * sizeof(int));
    
    jfree(p5);
}
