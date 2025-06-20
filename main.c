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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "iniparser.h"

#define EPS 1e-6 /*⎯ small tolerance when comparing doubles ⎯*/

/* ------------------------------------------------------------
 * Helpers
 * ----------------------------------------------------------*/
static void write_sample_ini(FILE *fp)
{
    /* minimal demo INI keeping focus on API coverage */
    fprintf(fp,
            "[general]\n"
            "name   = ChatGPT\n"
            "answer = 42\n"
            "pi     = 3.1415926535\n"
            "active = TRUE\n"
            "oct    = 052    ; octal for 42\n"
            "hex    = 0x2A  ; hex   for 42\n"
            "\n[paths]\n"
            "home   = /home/user\n"
            "temp   = /tmp\n");
    fflush(fp);
    rewind(fp);
}

/* convenience: create sample file on disk and return its path */
static const char *create_sample_file(const char *path)
{
    FILE *fp = fopen(path, "w+");
    assert(fp && "fopen() failed");
    write_sample_ini(fp);
    fclose(fp);
    return path;
}

/* ------------------------------------------------------------
 * Tests
 * ----------------------------------------------------------*/
static void test_basic_load_and_query(void)
{
    const char *filename = create_sample_file("sample_basic.ini");
    struct dictionary *d = iniparser_load(filename);
    assert(d && "iniparser_load returned NULL");

    /* string */
    assert(strcmp(iniparser_getstring(d, "general:name", NULL), "ChatGPT") == 0);

    /* integers in different bases */
    assert(iniparser_getint(d, "general:answer", -1) == 42);
    assert(iniparser_getint(d, "general:oct",    -1) == 42);
    assert(iniparser_getint(d, "general:hex",    -1) == 42);

    /* double – compare with EPS tolerance */
    assert(fabs(iniparser_getdouble(d, "general:pi", -1.0) - 3.1415926535) < EPS);

    /* boolean */
    assert(iniparser_getboolean(d, "general:active", 0) == 1);

    /* section / key helpers */
    assert(iniparser_getnsec(d) == 2);
    assert(iniparser_find_entry(d, "paths:home") == 1);

    iniparser_freedict(d);
    remove(filename);
}

static void test_getseckeys(void)
{
    const char *filename = create_sample_file("sample_paths.ini");
    struct dictionary *d = iniparser_load(filename);

    int n = iniparser_getsecnkeys(d, "paths");
    assert(n == 2);

    const char *buf[2];
    const char **ret = iniparser_getseckeys(d, "paths", buf);
    assert(ret != NULL);

    const char *expected[] = { "paths:home", "paths:temp" };
    int found[2] = {0};

    for (int i = 0; i < n; ++i) {
        assert(ret[i] && "returned key is NULL");
        for (int j = 0; j < 2; ++j) {
            if (strcmp(ret[i], expected[j]) == 0) {
                found[j] = 1;
            }
        }
    }
    /* make sure we saw both expected keys (order independent) */
    assert(found[0] && found[1] && "getseckeys missing expected keys");

    iniparser_freedict(d);
    remove(filename);
}

static void test_set_and_unset(void)
{
    struct dictionary *d = dictionary_new(0);
    assert(d);

    iniparser_set(d, "foo:bar", "baz");
    assert(strcmp(iniparser_getstring(d, "foo:bar", NULL), "baz") == 0);

    iniparser_unset(d, "foo:bar");
    assert(iniparser_find_entry(d, "foo:bar") == 0);

    iniparser_freedict(d);
}

int main(void) {
    test_dictionary_new();
    test_dictionary_set_and_get();
    test_dictionary_resize();
    test_dictionary_unset();
    test_error_input();
    test_collision();
    test_dictionary_dump("test_dump.ini");
    printf("All dictionary test passed!\n");

    test_basic_load_and_query();
    test_getseckeys();
    test_set_and_unset();
    printf("All iniparser test passed!\n");
  return 0;
}
