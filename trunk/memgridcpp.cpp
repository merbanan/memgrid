#define MEMGRIDCPP_IMP

#include <windows.h>
#include <stdio.h>
#include "memgrid.h"

void *operator new (size_t size, const char *file, int line) {
	return malloc_ld(size, file, line);
}

void *operator new[] (size_t size, const char *file, int line) {
	return malloc_ld(size, file, line);
}

/* Dummy to avoid warnings */
void operator delete (void *p, char const *file, int line) {
	free_ld(p, file, line);
}

void operator delete[] (void *p, char const *file, int line) {
	free_ld(p, file, line);
}


void operator delete (void *p) {
	free_ld(p, NULL, NULL);
}

void operator delete[] (void *p) {
	free_ld(p, NULL, NULL);
}
