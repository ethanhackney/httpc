#include "iobuf.h"

struct iobuf *
iobuf_new(int fd, size_t size)
{
        struct iobuf    *ip;

        if (!size)
                size = sysconf(_SC_PAGESIZE);

        ip = malloc(sizeof(*ip));
        if (!ip)
                err(EX_SOFTWARE, "iobuf_new");

        ip->ib_inbuf = malloc(size);
        if (!ip->ib_inbuf)
                err(EX_SOFTWARE, "iobuf_new");

        ip->ib_outbuf = malloc(size);
        if (!ip->ib_outbuf)
                err(EX_SOFTWARE, "iobuf_new");

        ip->ib_inbufp = ip->ib_inendp = ip->ib_inbuf;
        ip->ib_outbufp = ip->ib_outbuf;
        ip->ib_size = size;
        ip->ib_fd = fd;
        return ip;
}

int
iobuf_getc(struct iobuf *ip)
{
        ssize_t nread;

        if (ip->ib_inbufp < ip->ib_inendp)
                return *ip->ib_inbufp++;

        nread = read(ip->ib_fd, ip->ib_inbuf, ip->ib_size);
        if (nread < 0)
                err(EX_SOFTWARE, "iobuf_getc()");
        if (nread == 0)
                return 0;

        ip->ib_inbufp = ip->ib_inbuf;
        ip->ib_inendp = ip->ib_inbuf + nread;
        return *ip->ib_inbufp++;
}

void
iobuf_putc(struct iobuf *ip, char c)
{
        if (ip->ib_outbufp == ip->ib_outbuf + ip->ib_size)
                iobuf_flush_out(ip);
        *ip->ib_outbufp++ = c;
}

void
iobuf_free(struct iobuf **ipp)
{
        iobuf_flush_out(*ipp);
        free((*ipp)->ib_inbuf);
        free((*ipp)->ib_outbuf);
        free(*ipp);
        *ipp = NULL;
}

void
iobuf_flush_out(struct iobuf *ip)
{
        ssize_t nwritten;
        size_t  ntowrite;

        ntowrite = ip->ib_outbufp - ip->ib_outbuf;
        if (!ntowrite)
                return;

        nwritten = write(ip->ib_fd, ip->ib_outbuf, ntowrite);
        if (nwritten != (ssize_t)ntowrite)
                err(EX_SOFTWARE, "iobuf_flush_out");

        ip->ib_outbufp = ip->ib_outbuf;
}
