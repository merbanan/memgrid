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

#include <windows.h>
#include <stdio.h>


typedef struct ld_member {
	int           pre_marker;
	void         *member_ptr;
	unsigned int  size;
	void         *memptr;
	const char   *file;
	int           line;
	int           age;
	int           post_marker;
} ld_member;


/* Hash table size and definition */

// Other possible prime sizes:
// 1000003
// 5000011
#define LD_ENTRY_SIZE 100007
static ld_member ld_tab[LD_ENTRY_SIZE];

static unsigned int ld_members = 0;
static unsigned int ld_errors  = 0;
static unsigned int ld_np_free = 0;
static unsigned int ld_mallocs = 0;
static unsigned int ld_frees = 0;

/* ld_tab[] members points to last entry while chained members points to previous in chain */

static void insert_ld(const char *file, int line, unsigned int size, void *ptr) {
	unsigned int  hash_idx;
	ld_member    *member;
	ld_member    *parent;
	
	ld_members++;
	ld_mallocs++;
	hash_idx = (unsigned long) ptr % LD_ENTRY_SIZE;
	member = &ld_tab[hash_idx];
	parent = member;
	
	/* The hash table should be filled so the else cases barely happen */
	if (member->member_ptr == NULL) {
		member->size       = size;
		member->line       = line;
		member->memptr     = ptr;
		member->file       = file;
		member->member_ptr = parent;
		member->age        = 0;
		member->pre_marker = 0xECECECEC;
		member->post_marker= 0xFAFAFAFA;
	} else {
		member = malloc(sizeof(ld_member));
		member->size       = size;
		member->line       = line;
		member->memptr     = ptr;
		member->file       = file;
		member->age        = 0;
		member->pre_marker = 0xECECECEC;
		member->post_marker= 0xFAFAFAFA;
		member->member_ptr = parent->member_ptr;
		parent->member_ptr = member;
	}
}

static void remove_ld(const char *file, int line, void *ptr) {
	unsigned int  hash_idx;
	ld_member    *member;
	ld_member    *parent;
	ld_member    *previous;
	
	ld_frees++;
	
	hash_idx = (unsigned long) ptr % LD_ENTRY_SIZE;
	member = &ld_tab[hash_idx];
	parent = member;
	
	if (!ptr) {
		ld_np_free++;
		return;
	}
	
	if (member->member_ptr == NULL) {
		ld_errors++;
		return;
	}
	if (member->member_ptr == parent) {
		ld_members--;
		memset(member, 0, sizeof(ld_member));
		return;
	} else if (member->memptr == ptr) {
		ld_members--;
		previous = member->member_ptr;
		memcpy(member, member->member_ptr, sizeof(ld_member));
		free(previous);
		return;
	} else {
		previous = parent;
		member  = parent->member_ptr;
		while (member != parent) {
			if (ptr == member->memptr) {
				previous->member_ptr = member->member_ptr;
				free(member);
				ld_members--;
				return;
			}
			previous = member;
			member = member->member_ptr;
		}
	}
	ld_errors++;
}

void *malloc_ld(unsigned int size, const char *file, int line) {
	void *ret = malloc(size);
	if (ret) insert_ld(file, line, size, ret);
	return ret;
}

void *calloc_ld(unsigned int num, unsigned int size, const char *file, int line) {
	void *ret = calloc(num, size);
	if (ret) insert_ld(file, line, num * size, ret);
	return ret;
}

void free_ld(void *ptr, const char *file, int line) {
	remove_ld(file, line, ptr);
	free(ptr);
}

void *realloc_ld(void *ptr, unsigned int size, const char *file, int line) {
	void *ret;
	if (ptr) remove_ld(file, line, ptr);
	ret = realloc(ptr, size);
	if (ret) insert_ld(file, line, size, ret);
	return ret;
}

char *strdup_ld(const char *str, const char *file, int line) {
	char *ret = strdup(str);
	if (ret) insert_ld(file, line, strlen(ret) + 1, ret);
	return ret;
}

void ld_check() {
	int i, allocation_count = 0;
	ld_member *member;
	ld_member *parent;
	
	for (i = 0; i < LD_ENTRY_SIZE; i++) {
		member = &ld_tab[i];
		parent = member;
		if (member->member_ptr) {
			allocation_count++;
			while (member->member_ptr != parent){
				allocation_count++;
				member=member->member_ptr;
			}
		}
	}

	if (allocation_count != ld_members) {
		fprintf(stdout, "Allocation count vs ld_members does not match! %d:%d\n", allocation_count, ld_members);
		allocation_count = 0;
	}
	
	return;
}


void ld_dump() {
	static int dump_count = 0;
	unsigned int i, allocation_count = 0, currently_used = 0;
	char filename[32];
	ld_member *member;
	ld_member *parent;
	FILE *dfp;
	
	sprintf(filename, "memgrid-dump-%d.txt", dump_count++);
	dfp = fopen(filename, "w");
	fprintf(dfp, "ld_members = %d, ld_errors = %d, ld_np_free = %d\n\n", ld_members, ld_errors, ld_np_free);
	
	fprintf(dfp, "Current allocation table:\n");
	fprintf(dfp, "Source\tAge\tLocation\tSize\n");
	for (i = 0; i < LD_ENTRY_SIZE; i++) {
		member = &ld_tab[i];
		parent = member;
		if (member->member_ptr) {
			allocation_count++;
			if ((member->pre_marker != 0xECECECEC) && (member->post_marker != 0xFAFAFAFA)) {
				fprintf(dfp, "Hash table got corrupted! Aborting!\n");
				fclose(dfp);
				__asm int 3;
			}
			fprintf(dfp, "%s:%d\t%d\t0x%p\t%d\n", member->file, member->line, member->age, member->memptr, member->size);
			member->age++;
			currently_used += member->size;
			while (member->member_ptr != parent) {
				allocation_count++;
				if ((member->pre_marker != 0xECECECEC) && (member->post_marker != 0xFAFAFAFA)) {
					fprintf(dfp, "Hash table got corrupted! Aborting!\n");
					fclose(dfp);
					__asm int 3;
				}
				fprintf(dfp, "%s:%d\t%d\t0x%p\t%d\n", member->file, member->line, member->age, member->memptr, member->size);
				member->age++;
				currently_used += member->size;
				member=member->member_ptr;
			}
		}
	}
	fprintf(dfp, "Total allocation count = %d\n", allocation_count);
	fprintf(dfp, "Total count of mallocs = %d\n", ld_mallocs);
	fprintf(dfp, "Total count of frees = %d\n", ld_frees);
	fprintf(dfp, "Total currently used memory = %d\n", currently_used);
	fclose(dfp);
}
