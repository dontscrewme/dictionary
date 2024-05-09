#ifndef _DICTIONARY_H_
#define _DICTIONARY_H_

#include <stdio.h>

struct bucket {
  char *key;
  char *value;
  struct bucket *next;
};

struct dictionary {
  unsigned int n;
  unsigned int size;
  struct bucket **table;
};

struct dictionary *dictionary_new(size_t size);
void dictionary_del(struct dictionary *vd);
const char *dictionary_get(const struct dictionary *d, const char *key,
                           const char *def);
int dictionary_set(struct dictionary *vd, const char *key, const char *val);
void dictionary_unset(struct dictionary *d, const char *key);
void dictionary_dump(const struct dictionary *d, FILE *out);

#endif
