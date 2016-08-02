#include <glib.h>
#include <stdio.h>

int main(void) {
    GHashTable *hash = g_hash_table_new(g_str_hash, g_str_equal);
    g_hash_table_insert(hash, g_strdup("key1"), g_strdup("123"));
    g_hash_table_insert(hash, g_strdup("key2"), g_strdup("456"));
    g_hash_table_insert(hash, g_strdup("key3"), g_strdup("7"));
    printf("key1 value=%s\n", (gchar*) g_hash_table_lookup(hash, "key1"));
    printf("key2 value=%s\n", (gchar*) g_hash_table_lookup(hash, "key2"));
    printf("key3 value=%s\n", (gchar*) g_hash_table_lookup(hash, "key3"));
    printf("key4 value=%s\n", (gchar*) g_hash_table_lookup(hash, "key4"));
}
