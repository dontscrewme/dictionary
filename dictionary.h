#ifndef _DICTIONARY_H_
#define _DICTIONARY_H_

extern int (*error_callback)(const char *, ...);
void dictionary_set_error_callback(int (*errback)(const char *, ...));

#include <stdio.h>

struct bucket {
	char *key;
	char *value;
	struct bucket *next;
};

struct dictionary {
	unsigned int numOfElements;
	unsigned int size;
	struct bucket **table;
};

unsigned dictionary_hash(const char *key);
struct dictionary *dictionary_new(size_t size);
void dictionary_del(struct dictionary *d);
const char *dictionary_get(const struct dictionary *d, const char *key,
													 const char *def);
int dictionary_set(struct dictionary *vd, const char *key, const char *val);
void dictionary_unset(struct dictionary *d, const char *key);
void dictionary_dump(const struct dictionary *d, FILE *out);

#endif
