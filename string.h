#ifndef STRING_H
#define STRING_H

#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* dynamic string */
struct string {
        /* number of bytes allocated (not including nul byte) */
        size_t  s_cap;
        /* one past last valid byte in string */
        size_t  s_len;
        /* nul terminated array of bytes */
        char    *s_arr;
};

/**
 * Create a new string:
 *
 * args:
 *      @cap:   initial capacity (or zero for default)
 * ret:
 *      @success:       pointer to new string
 *      @failure:       NULL and errno set
 */
extern struct string *string_new(size_t cap);

/**
 * Append character onto end of string:
 *
 * args:
 *      @sp:    pointer to string
 *      @c:     character to add
 * ret:
 *      @success:       0
 *      @failure:       -1 and errno set
 */
extern int string_append(struct string *sp, char c);

/**
 * Free a string:
 *
 * args:
 *      @spp:   pointer to pointer to string
 * ret:
 *      @success:       0 and *spp set to NULL
 *      @failure:       -1 and errno set
 */
extern int string_free(struct string **spp);

#endif
