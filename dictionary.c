#include "dictionary.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static int default_error_callback(const char *format, ...)
{
  int ret;
  va_list argptr;
  va_start(argptr, format);
  ret = vfprintf(stderr, format, argptr);
  va_end(argptr);
  return ret;
}

int (*error_callback)(const char *, ...) = default_error_callback;

void dictionary_set_error_callback(int (*errback)(const char *, ...))
{
  if (errback)
  {
    error_callback = errback;
  }
}

static unsigned dictionary_hash(const char *key)
{

  unsigned int hash = 0;
  unsigned int len = strlen(key);
  for (unsigned int i = 0; i < len; i++)
  {
    hash += (unsigned)key[i];
    hash += (hash << 10);
    hash ^= (hash >> 6);
  }

  hash += (hash << 3);
  hash ^= (hash >> 11);
  hash += (hash << 15);

  return hash;
}

static int dictionary_grow(struct dictionary *d)
{
  struct bucket **new_table = calloc(d->size * 2, sizeof(struct bucket *));
  if (!new_table)
  {
    error_callback("%s: calloc() failed\n", __func__);
    return -1;
  }

  for (unsigned int i = 0; i < d->size; i++)
  {
    struct bucket *current = d->table[i];
    while (current)
    {
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

/** Minimal allocated number of entries in a dictionary */
#define DICTMINSZ 128
struct dictionary *dictionary_new(size_t size)
{
  struct dictionary *d = malloc(sizeof(struct dictionary));
  if (!d)
  {
    error_callback("%s: malloc() failed\n", __func__);
    return NULL;
  }

  /* If no size was specified, allocate space for DICTMINSZ */
  if (size < DICTMINSZ)
  {
    size = DICTMINSZ;
  }

  d->table = calloc(size, sizeof(struct bucket *));
  if (!d->table)
  {
    error_callback("%s: calloc() failed\n", __func__);
    free(d);
    return NULL;
  }

  d->size = size;
  d->numOfElements = 0;

  return d;
}

void dictionary_del(struct dictionary *d)
{
  if (!d)
  {
    error_callback("%s: invalid input\n", __func__);
    return;
  }

  for (unsigned i = 0; i < d->size; i++)
  {
    struct bucket *curr = d->table[i];
    while (curr)
    {
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
                           const char *def)
{
  if (!d || !key)
  {
    error_callback("%s: invalid input\n", __func__);
    return def;
  }

  unsigned int index = dictionary_hash(key) % d->size;
  struct bucket *curr = d->table[index];
  while (curr)
  {
    if (strcmp(curr->key, key) == 0)
    {
      return curr->value;
    }
    curr = curr->next;
  }

  return def;
}

int dictionary_set(struct dictionary *d, const char *key, const char *val)
{
  if (!d || !key)
  {
    error_callback("%s: invalid input\n", __func__);
    return -1;
  }

  unsigned int index = dictionary_hash(key) % d->size;
  struct bucket *curr = d->table[index];
  while (curr)
  {
    if (strcmp(curr->key, key) == 0)
    {
      free(curr->value);
      if (val)
      {
        curr->value = strdup(val);
        if (!curr->value)
        {
          error_callback("%s: strdup() failed\n", __func__);
          return -1;
        }
      }
      else
      {
        curr->value = NULL; // Set to NULL if val is NULL
      }
      return 0;
    }
    curr = curr->next;
  }

  if (d->numOfElements >= d->size * 0.7)
  {
    if (dictionary_grow(d) != 0)
    {
      error_callback("%s: dictionary_grow() failed\n", __func__);
      return -1;
    }
  }

  struct bucket *new_bucket = malloc(sizeof(struct bucket));
  if (!new_bucket)
  {
    error_callback("%s: malloc() failed\n", __func__);
    return -1;
  }

  new_bucket->key = strdup(key);
  if (!new_bucket->key)
  {
    error_callback("%s: strdup() failed\n", __func__);
    free(new_bucket);
    return -1;
  }

  if (val)
  {
    new_bucket->value = strdup(val);
    if (!new_bucket->value)
    {
      error_callback("%s: strdup() failed\n", __func__);
      free(new_bucket->key);
      free(new_bucket);
      return -1;
    }
  }
  else
  {
    new_bucket->value = NULL;
  }

  new_bucket->next = d->table[index];
  d->table[index] = new_bucket;
  d->numOfElements++;

  return 0;
}

void dictionary_unset(struct dictionary *d, const char *key)
{
  if (!key || !d)
  {
    error_callback("%s: invalid input\n", __func__);
    return;
  }

  unsigned int index = dictionary_hash(key) % d->size;

  struct bucket *curr = d->table[index];
  struct bucket *prev = NULL;

  while (curr)
  {
    if (strcmp(curr->key, key) == 0)
    {
      if (!prev)
      {
        d->table[index] = curr->next;
      }
      else
      {
        prev->next = curr->next;
      }
      free(curr->key);
      free(curr->value);
      free(curr);
      d->numOfElements--;
      return;
    }
    prev = curr;
    curr = curr->next;
  }
}

void dictionary_dump(const struct dictionary *d, FILE *out)
{
  if (!d || !out)
  {
    error_callback("%s: invalid input\n", __func__);
    return;
  }

  if (d->numOfElements < 1)
  {
    error_callback("%s: empty dictionary\n", __func__);
    return;
  }

  for (unsigned int i = 0; i < d->size; i++)
  {
    struct bucket *curr = d->table[i];
    while (curr)
    {
      fprintf(out, "%20s\t[%s]\n", curr->key,
              curr->value ? curr->value : "UNDEF");
      curr = curr->next;
    }
  }
  return;
}
