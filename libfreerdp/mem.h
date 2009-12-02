
#ifndef __MEM_H
#define __MEM_H

void *
xmalloc(int size);
void *
xrealloc(void * oldmem, size_t size);
void
xfree(void * mem);
char *
xstrdup(const char * s);

#endif
