#include "dictionary.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

void test_dictionary_new() {

    struct dictionary *dict = dictionary_new(100);
    assert(dict != NULL);
    assert(dict->size >= 100);

    dictionary_del(dict);
}

void test_dictionary_set_and_get() {

    struct dictionary *dict = dictionary_new(100);

    assert(dictionary_set(dict, "key1", "value1") == 0);
    const char *val = dictionary_get(dict, "key1", "default");
    assert(strcmp(val, "value1") == 0);

    dictionary_del(dict);
}

void test_dictionary_resize() {
  
  struct dictionary *dict = dictionary_new(10);

  for (int i = 0; i < 20; i++) {
      char key[10];
      snprintf(key, sizeof(key), "key%d", i);
      dictionary_set(dict, key, "value");
  }

  for (int i = 0; i < 20; i++) {
      char key[10];
      snprintf(key, sizeof(key), "key%d", i);
      const char *val = dictionary_get(dict, key, NULL);
      assert(val != NULL);
  }

  dictionary_del(dict);
  
}

void test_dictionary_unset() {

    struct dictionary *dict = dictionary_new(100);

    dictionary_set(dict, "key1", "value1");
    dictionary_unset(dict, "key1");

    const char *val = dictionary_get(dict, "key1", "default");
    assert(strcmp(val, "default") == 0);

    dictionary_del(dict);
}

int main(void) {
  test_dictionary_new();
  test_dictionary_set_and_get();
  test_dictionary_resize();
  test_dictionary_unset();
  
  
  printf("All test passed!");
  return 0;
}
