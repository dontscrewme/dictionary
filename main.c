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

    assert(dictionary_set(dict, "nullkey", NULL) == 0);
    assert(dictionary_get(dict, "nullkey", "default") == NULL);

    assert(dictionary_set(dict, "nullkey", "after") == 0);
    assert(strcmp(dictionary_get(dict, "nullkey", "default"), "after") == 0);

    assert(dictionary_set(dict, "nullkey", NULL) == 0);
    assert(dictionary_get(dict, "nullkey", "default") == NULL);

    assert(dictionary_set(dict, "dup", "first") == 0);
    assert(strcmp(dictionary_get(dict, "dup", "def"), "first") == 0);

    /* 覆寫為第二個值：應成功且 get() 取回最新字串 */
    assert(dictionary_set(dict, "dup", "second") == 0);
    assert(strcmp(dictionary_get(dict, "dup", "def"), "second") == 0);

    /* 再覆寫第三次，確認仍可更新 */
    assert(dictionary_set(dict, "dup", "third") == 0);
    assert(strcmp(dictionary_get(dict, "dup", "def"), "third") == 0);

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

void test_dictionary_unset(void) {

    struct dictionary *dict = dictionary_new(100);

    dictionary_set(dict, "key1", "value1");
    dictionary_unset(dict, "key1");

    const char *val = dictionary_get(dict, "key1", "default");
    assert(strcmp(val, "default") == 0);

    dictionary_del(dict);
}

void test_error_input(void)
{
    struct dictionary *dict = dictionary_new(16);
    const char *def = "defval";

    /* 1. dictionary_set() ── NULL 字典指標 */
    assert(dictionary_set(NULL, "k", "v")    == -1);
    assert(dictionary_set(NULL, "k", NULL)   == -1);

    /* 2. dictionary_set() ── NULL key */
    assert(dictionary_set(dict, NULL, "v")   == -1);
    assert(dictionary_set(dict, NULL, NULL)  == -1);

    /* 3. dictionary_get() ── NULL 字典指標 → 應回傳預設值 */
    assert(strcmp(dictionary_get(NULL, "k", def), def) == 0);

    /* 4. dictionary_unset() ── NULL 字典或 NULL key
          此 API 無回傳值，僅確認不會崩潰 (程式繼續執行即通過) */
    dictionary_unset(NULL, "k");   /* 應安全忽略 */
    dictionary_unset(dict, NULL);  /* 應安全忽略 */

    /* 5. 綜合：正常 unset 後 get() 應回傳 NULL */
    assert(dictionary_set(dict, "ok", "1") == 0);
    dictionary_unset(dict, "ok");
    assert(dictionary_get(dict, "ok", NULL) == NULL);

    dictionary_del(dict);
}

void test_collision(void)
{
    /* 用很小的初始容量 4，方便產生碰撞 */
    struct dictionary *dict = dictionary_new(4);
    assert(dict != NULL);

    /* -------------------------------------------------------- */
    /*  1. 以程式動態尋找兩個雜湊到同一 bucket 的 key           */
    /* -------------------------------------------------------- */
    char key1[16] = {0};
    char key2[16] = {0};
    unsigned bucket_index = 0;
    int found = 0;

    for (int i = 0; !found && i < 10000; i++) {
        snprintf(key1, sizeof key1, "K%d", i);
        bucket_index = dictionary_hash(key1) % dict->size;

        for (int j = i + 1; j < 10000; j++) {
            snprintf(key2, sizeof key2, "K%d", j);
            if (dictionary_hash(key2) % dict->size == bucket_index) {
                found = 1;
                break;
            }
        }
    }
    assert(found && "Failed to find colliding keys within 10k iterations");

    /* -------------------------------------------------------- */
    /*  2. 插入碰撞 key，確認皆可正確取回                        */
    /* -------------------------------------------------------- */
    assert(dictionary_set(dict, key1, "v1") == 0);
    assert(dictionary_set(dict, key2, "v2") == 0);

    assert(strcmp(dictionary_get(dict, key1, NULL), "v1") == 0);
    assert(strcmp(dictionary_get(dict, key2, NULL), "v2") == 0);

    /* -------------------------------------------------------- */
    /*  3. 刪除其中一把 key，確保另一把仍存在                    */
    /* -------------------------------------------------------- */
    dictionary_unset(dict, key1);
    assert(strcmp(dictionary_get(dict, key1, "notfound"),"notfound") == 0);
    assert(strcmp(dictionary_get(dict, key2, NULL), "v2") == 0);
    
    dictionary_unset(dict, key2);
    assert(strcmp(dictionary_get(dict, key2, "notfound"),"notfound") == 0);

    dictionary_del(dict);
}

void test_dictionary_dump(char* file_name)
{
    /* 建立字典並插入兩筆：一筆正常值、一筆 NULL 值 */
    struct dictionary *dict = dictionary_new(8);
    assert(dict);

    assert(dictionary_set(dict, "alpha", "one") == 0);
    assert(dictionary_set(dict, "beta",  NULL ) == 0);

    /* ---------- 用 tmpnam() 產生唯一檔名，再 fopen() ---------- */
    char fname[L_tmpnam];
    assert(tmpnam(fname) != NULL);          /* 產出路徑 */

    FILE *fp = fopen(file_name, "w+");          /* r/w 模式，之後可讀回 */
    assert(fp != NULL && "fopen() failed for temp file");

    /* 將輸出寫入檔案 */
    dictionary_dump(dict, fp);

    /* 讀回檢查內容 */
    fflush(fp);
    rewind(fp);

    char line[256];
    int have_alpha = 0, have_beta = 0;

    while (fgets(line, sizeof line, fp)) {
        if (strstr(line, "alpha") && strstr(line, "[one]"))
            have_alpha = 1;
        if (strstr(line, "beta")  && strstr(line, "[UNDEF]"))
            have_beta  = 1;
    }
    fclose(fp);
    remove(fname);                          /* 清理暫存檔 */

    /* 兩行都必須存在，否則測試失敗 */
    assert(have_alpha && have_beta);

    dictionary_del(dict);
}

int main(void) {
  test_dictionary_new();
  test_dictionary_set_and_get();
  test_dictionary_resize();
  test_dictionary_unset();
  test_error_input();
  test_collision();
  test_dictionary_dump("test_dump.ini");
  
  
  printf("All test passed!");
  return 0;
}
