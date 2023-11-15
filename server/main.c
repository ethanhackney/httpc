#include "../http.h"
#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <sysexits.h>
#include <unistd.h>
#include <string.h>

static void rootfn(struct http_request *req, struct http_response *res);
static void loginfn(struct http_request *req, struct http_response *res);

int
main(void)
{
        struct http_server *server;
        struct addrinfo info;
        struct addrinfo *infolist;
        struct addrinfo *p;
        char service[] = "8080";
        char host[] = "localhost";
        void (*funcs[])(struct http_request *, struct http_response *) = {
                rootfn,
                loginfn,
        };
        char *funcnames[] = {
                "/",
                "/login",
                NULL
        };
        size_t i;
        int ret;

        memset(&info, 0, sizeof(info));
        info.ai_family = AF_INET;
        info.ai_socktype = SOCK_STREAM;
        info.ai_flags = AI_PASSIVE;
        ret = getaddrinfo(host, service, &info, &infolist);
        if (ret)
                errx(EX_SOFTWARE, "getaddrinfo: %s", gai_strerror(ret));

        for (p = infolist; p; p = p->ai_next) {
                server = http_server_new(p);
                if (server)
                        break;
        }
        freeaddrinfo(infolist);
        if (!p) {
                printf("could not bind to %s:%s\n", host, service);
                exit(EXIT_FAILURE);
        }

        for (i = 0; funcnames[i]; ++i) {
                struct http_handler *hdlr;

                hdlr = malloc(sizeof(*hdlr));
                if (!hdlr)
                        err(EX_SOFTWARE, "malloc()");

                hdlr->hh_fn = funcs[i];
                http_server_add_handler(server, funcnames[i], hdlr);
        }

        if (http_server_listen(server, 10) < 0)
                err(EX_SOFTWARE, "http_server_listen()");

        if (http_server_free(&server) < 0)
                err(EX_SOFTWARE, "http_server_free()");
}

static void
rootfn(struct http_request *req, struct http_response *res)
{
        char *p;

        char buf[] = "HTTP/1.1 200 OK\r\n"
                     "Content-Length: 5\r\n"
                     "Content-Type: text/plain\r\n"
                     "\r\nhi:)\n";

        for (p = buf; *p; ++p)
                iobuf_putc(req->rq_buf, *p);

        iobuf_flush_out(req->rq_buf);
}

static void
loginfn(struct http_request *req, struct http_response *res)
{
        char *p;
        char buf[] = "HTTP/1.1 200 OK\r\n"
                     "Content-Length: 6\r\n"
                     "Content-Type: text/plain\r\n"
                     "\r\nlogin\n";

        for (p = buf; *p; ++p)
                iobuf_putc(req->rq_buf, *p);

        iobuf_flush_out(req->rq_buf);
}
