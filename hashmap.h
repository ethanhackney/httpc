#ifndef HASHMAP_H
#define HASHMAP_H

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* user facing portion on hashmap entry */
struct hash_entry {
        /* key (nul terminated string) */
        char    *he_key;
        /* value (anything) */
        void    *he_value;
};

/* entry in hashmap */
struct hash_link {
        /* user facing portion of hashmap entry */
        struct hash_entry       hl_entry;
        /* next on hash chain */
        struct hash_link        *hl_next;
        /* saved hash value */
        size_t                  hl_hash;
};

/* singly linked hash map */
struct hashmap {
        /* array of singly linked lists */
        struct hash_link        **hm_tab;
        /* number of buckets */
        size_t                  hm_size;
        /* number of entries */
        size_t                  hm_count;
};

/**
 * Create a new hashmap:
 *
 * args:
 *      @size:  initial number of buckets (or zero for default)
 * ret:
 *      @success:       0
 *      @failure:       NULL and errno set
 */
extern struct hashmap *hashmap_new(size_t size);

/**
 * Free a hashmap:
 *
 * args:
 *      @hpp:   pointer to pointer to hashmap
 * ret:
 *      @success:       0 and *hpp set to NULL
 *      @failure:       -1 and errno set
 */
extern int hashmap_free(struct hashmap **hpp);

/**
 * Retrieve an entry from hashmap:
 *
 * args:
 *      @hp:    pointer to hashmap
 *      @key:   key to search for
 * ret:
 *      @success:       pointer to hash_entry
 *      @failure:       NULL and errno possibly set
 */
extern struct hash_entry *hashmap_get(struct hashmap *hp, char *key);

/**
 * Set key to value in hashmap:
 *
 * args:
 *      @hp:    pointer to hashmap
 *      @key:   key
 *      @value: value
 * ret:
 *      @success:       pointer to hash_entry
 *      @failure:       NULL and errno possibly set
 */
extern struct hash_entry *hashmap_set(struct hashmap *hp, char *key, void *value);

/**
 * Iterate through a hashmap:
 *
 * args:
 *      @hp:    pointer to hashmap
 *      @fn:    function to apply to each entry
 * ret:
 *      @success:       0
 *      @failure:       -1 and errno set
 */
extern int hashmap_for(struct hashmap *hp, void (*fn)(struct hash_entry *));

#endif
