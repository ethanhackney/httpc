#include "iobuf.h"

struct iobuf *
iobuf_new(int fd, size_t size)
{
        struct iobuf    *ip = NULL;

        if (!size)
                size = sysconf(_SC_PAGESIZE);

        ip = malloc(sizeof(*ip));
        if (ip == NULL)
                goto ret;

        ip->ib_inbuf = malloc(size);
        if (ip->ib_inbuf == NULL)
                goto free_ip;

        ip->ib_outbuf = malloc(size);
        if (ip->ib_outbuf == NULL)
                goto free_inbuf;

        ip->ib_inbufp = ip->ib_inendp = ip->ib_inbuf;
        ip->ib_outbufp = ip->ib_outbuf;
        ip->ib_size = size;
        ip->ib_fd = fd;
        goto ret;
free_inbuf:
        free(ip->ib_inbuf);
free_ip:
        free(ip);
        ip = NULL;
ret:
        return ip;
}

static int iobuf_sanity(const struct iobuf *ip);

int
iobuf_getc(struct iobuf *ip)
{
        ssize_t nread;

        if (iobuf_sanity(ip) < 0)
                return -1;

        if (ip->ib_inbufp < ip->ib_inendp)
                return *ip->ib_inbufp++;

        nread = read(ip->ib_fd, ip->ib_inbuf, ip->ib_size);
        if (nread <= 0)
                return nread;

        ip->ib_inbufp = ip->ib_inbuf;
        ip->ib_inendp = ip->ib_inbuf + nread;
        return *ip->ib_inbufp++;
}

static int
iobuf_sanity(const struct iobuf *ip)
{
        errno = EINVAL;
        if (ip == NULL)
                return -1;
        if (ip->ib_inbuf == NULL)
                return -1;
        if (ip->ib_outbuf == NULL)
                return -1;
        if (ip->ib_fd < 0)
                return -1;
        if (ip->ib_size == 0)
                return -1;
        if (ip->ib_inbufp == NULL)
                return -1;
        if (ip->ib_inbufp > ip->ib_inbuf + ip->ib_size)
                return -1;
        if (ip->ib_inbufp < ip->ib_inbuf)
                return -1;
        if (ip->ib_inendp == NULL)
                return -1;
        if (ip->ib_inendp > ip->ib_inbuf + ip->ib_size)
                return -1;
        if (ip->ib_inendp < ip->ib_inbuf)
                return -1;
        if (ip->ib_inendp < ip->ib_inbufp)
                return -1;
        if (ip->ib_outbufp > ip->ib_outbuf + ip->ib_size)
                return -1;
        if (ip->ib_outbufp < ip->ib_outbuf)
                return -1;
        errno = 0;
        return 0;
}

int
iobuf_putc(struct iobuf *ip, char c)
{
        if (iobuf_sanity(ip) < 0)
                return -1;
        if (iobuf_flush_out(ip) < 0)
                return -1;
        *ip->ib_outbufp++ = c;
        return 0;
}

int
iobuf_free(struct iobuf **ipp)
{
        struct iobuf    *ip = NULL;

        if (ipp == NULL) {
                errno = EINVAL;
                return -1;
        }

        ip = *ipp;
        if (iobuf_sanity(ip) < 0)
                return -1;

        if (iobuf_flush_out(ip) < 0)
                return -1;

        free(ip->ib_inbuf);
        free(ip->ib_outbuf);
        free(ip);
        *ipp = NULL;
        return 0;
}

int
iobuf_flush_out(struct iobuf *ip)
{
        ssize_t nwritten;
        size_t  ntowrite;

        if (iobuf_sanity(ip) < 0)
                return -1;

        ntowrite = ip->ib_outbufp - ip->ib_outbuf;
        if (ntowrite == 0)
                return 0;

        nwritten = write(ip->ib_fd, ip->ib_outbuf, ntowrite);
        if (nwritten != (ssize_t)ntowrite)
                return -1;

        ip->ib_outbufp = ip->ib_outbuf;
        return 0;
}
