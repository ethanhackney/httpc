#include "string.h"

struct string *
string_new(size_t cap)
{
        struct string   *s = NULL;

        if (cap == 0)
                cap = 1;

        s = malloc(sizeof(*s));
        if (s == NULL)
                return NULL;

        s->s_arr = malloc(cap + 1);
        if (s->s_arr == NULL) {
                free(s);
                return NULL;
        }

        s->s_cap = cap;
        s->s_len = 0;
        return s;
}

static int string_sanity(const struct string *sp);
static int string_grow(struct string *sp);

int
string_append(struct string *sp, char c)
{
        if (string_sanity(sp) < 0)
                return -1;

        if (sp->s_len == sp->s_cap) {
                if (string_grow(sp) < 0)
                        return -1;
        }

        sp->s_arr[sp->s_len++] = c;
        sp->s_arr[sp->s_len] = 0;
        return 0;
}

static int
string_sanity(const struct string *sp)
{
        errno = EINVAL;
        if (sp == NULL)
                return -1;
        if (sp->s_cap == 0)
                return -1;
        if (sp->s_len > sp->s_cap)
                return -1;
        if (sp->s_arr == NULL)
                return -1;
        errno = 0;
        return 0;
}

static int
string_grow(struct string *sp)
{
        size_t  newcap;
        char    *p = NULL;

        newcap = sp->s_cap * 2;
        p = sp->s_arr;
        p = realloc(p, newcap + 1);
        if (!p)
                return -1;

        sp->s_arr = p;
        sp->s_cap = newcap;
        return 0;
}

int
string_free(struct string **spp)
{
        struct string   *sp = NULL;

        if (spp == NULL) {
                errno = EINVAL;
                return -1;
        }

        sp = *spp;
        if (string_sanity(sp) < 0)
                return -1;

        free(sp->s_arr);
        free(sp);
        *spp = NULL;
        return 0;
}
