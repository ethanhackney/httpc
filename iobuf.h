#ifndef IOBUF_H
#define IOBUF_H

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

/* I/O buffer */
struct iobuf {
        /* I/O buffer size */
        size_t  ib_size;
        /* input buffer */
        char    *ib_inbuf;
        /* next place to read from in ib_inbuf */
        char    *ib_inbufp;
        /* one past last valid byte in ib_inbuf */
        char    *ib_inendp;
        /* output buffer */
        char    *ib_outbuf;
        /* next place to write to in ib_outbuf */
        char    *ib_outbufp;
        /* file descriptor */
        int     ib_fd;
};

/**
 * Create a new iobuf:
 *
 * args:
 *      @fd:    file descriptor to read/write to
 *      @size:  size (or zero for default)
 * ret:
 *      @success:       pointer to iobuf
 *      @failure:       NULL and errno set
 */
extern struct iobuf *iobuf_new(int fd, size_t size);

/**
 * Read a character from iobuf:
 *
 * args:
 *      @ip:    pointer to iobuf
 * ret:
 *      @success:       character read
 *      @failure:       -1 and errno set
 */
extern int iobuf_getc(struct iobuf *ip);

/**
 * Write a character into iobuf:
 *
 * args:
 *      @ip:    pointer to iobuf
 *      @c:     character to write
 * ret:
 *      @success:       0
 *      @failure:       -1 and errno set
 */
extern int iobuf_putc(struct iobuf *ip, char c);

/**
 * Free an iobuf:
 *
 * args:
 *      @ipp:   pointer to pointer to iobuf
 * ret:
 *      @success:       0
 *      @failure:       -1 and errno set
 */
extern int iobuf_free(struct iobuf **ipp);

/**
 * Flush output buffer:
 *
 * args:
 *      @ip:    pointer to iobuf
 * ret:
 *      @success:       0
 *      @failure:       -1 and errno set
 */
extern int iobuf_flush_out(struct iobuf *ip);

#endif
