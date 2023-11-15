#include "hashmap.h"

static bool is_prime(size_t n);
static size_t next_prime(size_t n);

struct hashmap *
hashmap_new(size_t size)
{
        struct hashmap  *hp;

        if (!size)
                size = 1;

        hp = malloc(sizeof(*hp));
        if (!hp)
                err(EX_SOFTWARE, "hashmap_new");

        size = next_prime(size);
        hp->hm_tab = calloc(size, sizeof(*hp->hm_tab));
        if (!hp->hm_tab)
                err(EX_SOFTWARE, "hashmap_new");

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
        if (!(n % 2))
                return false;
        if (!(n % 3))
                return false;

        for (i = 5; i * i <= n; ++i) {
                if (!(n % i))
                        return false;
                if (!(n % (i + 2)))
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

void
hashmap_free(struct hashmap **hpp)
{
        size_t  freed;
        size_t  i;

        for (freed = i = 0; freed < (*hpp)->hm_count; ++i) {
                struct hash_link *next;
                struct hash_link *p;

                for (p = (*hpp)->hm_tab[i]; p; p = next) {
                        next = p->hl_next;
                        free(p);
                        ++freed;
                }
        }

        free((*hpp)->hm_tab);
        free(*hpp);
        *hpp = NULL;
}

static size_t hashfn(char *key);

struct hash_entry *
hashmap_get(struct hashmap *hp, char *key)
{
        struct hash_link        *p;
        size_t                  hash;
        size_t                  i;

        hash = hashfn(key);
        i = hash % hp->hm_size;
        for (p = hp->hm_tab[i]; p; p = p->hl_next) {
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
        while (*key)
                hash = hash * 31 + *key++;

        return hash;
}

static void hashmap_grow(struct hashmap *hp);

struct hash_entry *
hashmap_set(struct hashmap *hp, char *key, void *value)
{
        struct hash_link        *p;
        size_t                  hash;
        size_t                  i;

        hash = hashfn(key);
        i = hash % hp->hm_size;
        for (p = hp->hm_tab[i]; p; p = p->hl_next) {
                if (!strcmp(key, p->hl_entry.he_key))
                        break;
        }

        if (p) {
                p->hl_entry.he_value = value;
                return &p->hl_entry;
        }

        if (hp->hm_count == hp->hm_size) {
                hashmap_grow(hp);
                i = hash % hp->hm_size;
        }

        p = malloc(sizeof(*p));
        if (!p)
                err(EX_SOFTWARE, "hashmap_set");
        p->hl_entry.he_key = key;
        p->hl_entry.he_value = value;
        p->hl_hash = hash;
        p->hl_next = hp->hm_tab[i];
        hp->hm_tab[i] = p;

        ++hp->hm_count;
        return NULL;
}

static void
hashmap_grow(struct hashmap *hp)
{
        struct hash_link        **newtab;
        size_t                  newsize;
        size_t                  moved;
        size_t                  i;

        newsize = next_prime(hp->hm_size * 2);
        newtab = calloc(newsize, sizeof(*newtab));
        if (!newtab)
                err(EX_SOFTWARE, "hashmap_grow()");

        for (moved = i = 0; moved < hp->hm_count; ++i) {
                struct hash_link *next;
                struct hash_link *p;

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
}

void
hashmap_for(struct hashmap *hp, void (*fn)(struct hash_entry *))
{
        size_t  seen;
        size_t  i;

        for (seen = i = 0; seen < hp->hm_count; ++i) {
                struct hash_link *p;

                for (p = hp->hm_tab[i]; p; p = p->hl_next) {
                        fn(&p->hl_entry);
                        ++seen;
                }
        }
}
