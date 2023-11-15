#include "string.h"

struct string *
string_new(size_t cap)
{
        struct string *s;

        if (cap == 0)
                cap = 1;

        s = malloc(sizeof(*s));
        if (!s)
                err(EX_SOFTWARE, "string_new");

        s->s_arr = malloc(cap + 1);
        if (!s->s_arr)
                err(EX_SOFTWARE, "string_new");

        s->s_cap = cap;
        s->s_len = 0;
        return s;
}

void
string_append(struct string *sp, char c)
{
        if (sp->s_len == sp->s_cap) {
                size_t newcap;
                char *p;

                newcap = sp->s_cap * 2;
                p = sp->s_arr;
                p = realloc(p, newcap + 1);
                if (!p)
                        err(EX_SOFTWARE, "string_append");

                sp->s_arr = p;
                sp->s_cap = newcap;
        }
        sp->s_arr[sp->s_len++] = c;
        sp->s_arr[sp->s_len] = 0;
}

void
string_free(struct string **spp)
{
        free((*spp)->s_arr);
        free(*spp);
        *spp = NULL;
}
