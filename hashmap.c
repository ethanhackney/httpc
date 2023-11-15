#include "hashmap.h"
#include <stdio.h>

static bool is_prime(size_t n);
static size_t next_prime(size_t n);

struct hashmap *
hashmap_new(size_t size)
{
        struct hashmap  *hp = NULL;

        if (size == 0)
                size = 1;

        hp = malloc(sizeof(*hp));
        if (hp == NULL)
                return NULL;

        size = next_prime(size);
        hp->hm_tab = calloc(size, sizeof(*hp->hm_tab));
        if (hp->hm_tab == NULL) {
                free(hp);
                return NULL;
        }

        hp->hm_size = size;
        hp->hm_count = 0;
        return hp;
}

static bool
is_prime(size_t n)
{
        size_t  i;

        if (n <= 1)
                return false;
        if (n <= 3)
                return true;
        if ((n % 2) == 0)
                return false;
        if ((n % 3) == 0)
                return false;

        for (i = 5; i * i <= n; ++i) {
                if ((n % i) == 0)
                        return false;
                if ((n % (i + 2)) == 0)
                        return false;
        }

        return true;
}

static size_t
next_prime(size_t n)
{
        while (!is_prime(n))
                ++n;
        return n;
}

static int hashmap_sanity(const struct hashmap *hp);

int
hashmap_free(struct hashmap **hpp)
{
        struct hashmap  *hp = *hpp;
        size_t          freed;
        size_t          i;

        if (hpp == NULL) {
                errno = EINVAL;
                return -1;
        }

        hp = *hpp;
        if (hashmap_sanity(hp) < 0)
                return -1;

        for (freed = i = 0; freed < (*hpp)->hm_count; ++i) {
                struct hash_link *next = NULL;
                struct hash_link *p = NULL;

                for (p = hp->hm_tab[i]; p != NULL; p = next) {
                        next = p->hl_next;
                        free(p);
                        ++freed;
                }
        }

        free(hp->hm_tab);
        free(hp);
        *hpp = NULL;
        return 0;
}

static int
hashmap_sanity(const struct hashmap *hp)
{
        errno = EINVAL;
        if (hp == NULL)
                return -1;
        if (hp->hm_tab == NULL)
                return -1;
        if (hp->hm_size == 0)
                return -1;
        errno = 0;
        return 0;
}

static size_t hashfn(char *key);

struct hash_entry *
hashmap_get(struct hashmap *hp, char *key)
{
        struct hash_link        *p = NULL;
        size_t                  hash;
        size_t                  i;

        if (hashmap_sanity(hp) < 0) {
                puts("here");
                return NULL;
        }

        hash = hashfn(key);
        i = hash % hp->hm_size;
        for (p = hp->hm_tab[i]; p != NULL; p = p->hl_next) {
                if (!strcmp(key, p->hl_entry.he_key))
                        return &p->hl_entry;
        }

        return NULL;
}

static size_t
hashfn(char *key)
{
        size_t  hash;

        hash = 0;
        while (*key != '\0')
                hash = hash * 31 + *key++;

        return hash;
}

static int hashmap_grow(struct hashmap *hp);

struct hash_entry *
hashmap_set(struct hashmap *hp, char *key, void *value)
{
        struct hash_link        *p;
        size_t                  hash;
        size_t                  i;

        if (hashmap_sanity(hp) < 0)
                return NULL;

        hash = hashfn(key);
        i = hash % hp->hm_size;
        for (p = hp->hm_tab[i]; p != NULL; p = p->hl_next) {
                if (!strcmp(key, p->hl_entry.he_key))
                        break;
        }

        if (p) {
                p->hl_entry.he_value = value;
                return &p->hl_entry;
        }

        if (hp->hm_count == hp->hm_size) {
                if (hashmap_grow(hp) < 0)
                        return NULL;
                i = hash % hp->hm_size;
        }

        p = malloc(sizeof(*p));
        if (!p)
                return NULL;
        p->hl_entry.he_key = key;
        p->hl_entry.he_value = value;
        p->hl_hash = hash;
        p->hl_next = hp->hm_tab[i];
        hp->hm_tab[i] = p;

        ++hp->hm_count;
        return &p->hl_entry;
}

static int
hashmap_grow(struct hashmap *hp)
{
        struct hash_link        **newtab = NULL;
        size_t                  newsize;
        size_t                  moved;
        size_t                  i;

        newsize = next_prime(hp->hm_size * 2);
        newtab = calloc(newsize, sizeof(*newtab));
        if (!newtab)
                return -1;

        for (moved = i = 0; moved < hp->hm_count; ++i) {
                struct hash_link *next = NULL;
                struct hash_link *p = NULL;

                for (p = hp->hm_tab[i]; p; p = next) {
                        size_t j;

                        next = p->hl_next;
                        j = p->hl_hash % newsize;
                        p->hl_next = newtab[j];
                        newtab[j] = p;
                        ++moved;
                }
        }

        free(hp->hm_tab);
        hp->hm_tab = newtab;
        hp->hm_size = newsize;
        return 0;
}

int
hashmap_for(struct hashmap *hp, void (*fn)(struct hash_entry *))
{
        size_t  seen;
        size_t  i;

        if (fn == NULL) {
                errno = EINVAL;
                return -1;
        }

        if (hashmap_sanity(hp) < 0)
                return -1;

        for (seen = i = 0; seen < hp->hm_count; ++i) {
                struct hash_link *p = NULL;

                for (p = hp->hm_tab[i]; p != NULL; p = p->hl_next) {
                        fn(&p->hl_entry);
                        ++seen;
                }
        }

        return 0;
}
