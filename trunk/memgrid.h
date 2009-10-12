/*

Copyright (c) 2009 Benjamin Larsson

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

*/

#define MG_MEMORY_LEAK_DETECTOR


#if defined __cplusplus
extern "C" {
#endif


#ifdef MG_MEMORY_LEAK_DETECTOR
#define malloc(x)     malloc_ld((x),__FILE__,__LINE__)
#define calloc(x,y)   calloc_ld((x),(y),__FILE__,__LINE__)
#define realloc(x,y)  realloc_ld((x),(y),__FILE__,__LINE__)
#define free(x)       free_ld((x),__FILE__,__LINE__)
#define strdup(s)     strdup_ld((s),__FILE__,__LINE__)

void* malloc_ld(size_t size, const char* file, int line);
void* calloc_ld(size_t size, unsigned int num,const char* file, int line);
void* realloc_ld(void *ptr, unsigned int size,const char* file, int line);
void  free_ld(void *ptr, const char* file, int line);
char* strdup_ld(const char *str, const char *file, int line);

void ld_dump();
#endif

#if defined __cplusplus
}


#ifndef MEMGRIDCPP_IMP
void * operator new (size_t size, const char * file, int line);
void * operator new[] (size_t size, const char * file, int line);
void operator delete (void * p, char const * file, int line);
#define new new(__FILE__, __LINE__)

#endif

#endif



