#include "dictionary.h"
#include <stdlib.h>
#include <string.h>

/** Minimal allocated number of entries in a dictionary */
#define DICTMINSZ 128

static unsigned dictionary_hash(const char *key) {
  size_t len;
  unsigned hash;
  size_t i;

  if (!key)
    return 0;

  len = strlen(key);
  for (hash = 0, i = 0; i < len; i++) {
    hash += (unsigned)key[i];
    hash += (hash << 10);
    hash ^= (hash >> 6);
  }
  hash += (hash << 3);
  hash ^= (hash >> 11);
  hash += (hash << 15);
  return hash;
}

static int dictionary_grow(struct dictionary *d) {
  struct bucket **new_table = calloc(d->size * 2, sizeof(struct bucket *));
  if (new_table == NULL) {
    return -1;
  }

  for (unsigned int i = 0; i < d->size; i++) {
    struct bucket *current = d->table[i];
    while (current != NULL) {
      unsigned int new_index = dictionary_hash(current->key) % (d->size * 2);
      struct bucket *tmp = current->next;
      current->next = new_table[new_index];
      new_table[new_index] = current;
      current = tmp;
    }
  }

  free(d->table);
  d->size *= 2;
  d->table = new_table;
  return 0;
}

struct dictionary *dictionary_new(size_t size) {
  struct dictionary *d = malloc(sizeof(struct dictionary));
  if (d == NULL) {
    return NULL;
  }

  /* If no size was specified, allocate space for DICTMINSZ */
  if (size < DICTMINSZ) {
    size = DICTMINSZ;
  }

  d->table = calloc(size, sizeof(struct bucket *));
  if (d->table == NULL) {
    free(d);
    return NULL;
  }

  d->size = size;
  d->n = 0;

  return d;
}

void dictionary_del(struct dictionary *d) {
  if (d == NULL) {
    return;
  }

  for (unsigned i = 0; i < d->size; i++) {
    struct bucket *curr = d->table[i];
    while (curr != NULL) {
      struct bucket *prev = curr;
      curr = curr->next;
      free(prev->key);
      free(prev->value);
      free(prev);
    }
  }

  free(d->table);
  free(d);
}

const char *dictionary_get(const struct dictionary *d, const char *key,
                           const char *def) {
  if (d == NULL || key == NULL) {
    return def;
  }

  unsigned int index = dictionary_hash(key) % d->size;
  struct bucket *curr = d->table[index];
  while (curr != NULL) {
    if (strcmp(curr->key, key) == 0) {
      return curr->value;
    }
    curr = curr->next;
  }

  return def;
}

int dictionary_set(struct dictionary *d, const char *key, const char *val) {
  if (d == NULL || key == NULL || val == NULL) {
    return -1;
  }

  unsigned int index = dictionary_hash(key) % d->size;
  struct bucket *curr = d->table[index];
  while (curr != NULL) {
    if (strcmp(curr->key, key) == 0) {
      char *new_val = strdup(val);
      if (new_val == NULL) {
        return -1;
      }
      free(curr->value);
      curr->value = new_val;
      return 0;
    }
    curr = curr->next;
  }

  if (d->n >= d->size * 0.7) {
    if (dictionary_grow(d) != 0) {
      return -1;
    }
  }

  struct bucket *new_bucket = malloc(sizeof(struct bucket));
  if (new_bucket == NULL) {
    return -1;
  }

  new_bucket->key = strdup(key);
  if (new_bucket->key == NULL) {
    free(new_bucket);
    return -1;
  }
  new_bucket->value = strdup(val);
  if (new_bucket->value == NULL) {
    free(new_bucket->key);
    free(new_bucket);
    return -1;
  }
  new_bucket->next = d->table[index];
  d->table[index] = new_bucket;
  d->n++;

  return 0;
}

void dictionary_unset(struct dictionary *d, const char *key) {
  if (key == NULL || d == NULL) {
    return;
  }

  unsigned int index = dictionary_hash(key) % d->size;

  struct bucket *curr = d->table[index];
  struct bucket *prev = NULL;

  while (curr != NULL) {
    if (strcmp(curr->key, key) == 0) {
      if (prev == NULL) {
        d->table[index] = curr->next;
      } else {
        prev->next = curr->next;
      }
      free(curr->key);
      free(curr->value);
      free(curr);
      d->n--;
      return;
    }
    prev = curr;
    curr = curr->next;
  }
}

void dictionary_dump(const struct dictionary *d, FILE *out) {

  if (d == NULL || out == NULL)
    return;
  if (d->n < 1) {
    fprintf(out, "empty dictionary\n");
    return;
  }

  for (unsigned int i = 0; i < d->size; i++) {
    struct bucket *curr = d->table[i];
    if (curr != NULL) {
      fprintf(out, "%20s\t[%s]\n", d->table[i]->key,
              d->table[i]->value ? d->table[i]->value : "UNDEF");
      curr = curr->next;
    }
  }
  return;
}
