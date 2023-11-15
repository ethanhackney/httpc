#include "http.h"
#include <stdio.h>

struct http_server *
http_server_new(struct addrinfo *ap)
{
        struct http_server *s;
        int saved_errno;
        int y;

        s = malloc(sizeof(*s));
        if (!s)
                goto ret;

        s->sv_handlers = hashmap_new(0);
        if (!s->sv_handlers)
                goto free_server;

        s->sv_fd = socket(ap->ai_family, ap->ai_socktype, ap->ai_protocol);
        if (s->sv_fd < 0)
                goto free_handlers;

        y = 1;
        if (setsockopt(s->sv_fd, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(y)) < 0)
                goto close_fd;

        if (bind(s->sv_fd, ap->ai_addr, ap->ai_addrlen) < 0)
                goto close_fd;

        goto ret;
close_fd:
        saved_errno = errno;
        (void)close(s->sv_fd);
        errno = saved_errno;
free_handlers:
        (void)hashmap_free(&s->sv_handlers);
free_server:
        free(s);
        s = NULL;
ret:
        return s;
}

void
http_server_add_handler(struct http_server *hp,
                        char *resource,
                        struct http_handler *handler)
{
        hashmap_set(hp->sv_handlers, resource, handler);
}

static void http_server_client(struct http_server *hp, int connfd);

void
http_server_listen(struct http_server *hp, int qsize)
{
        if (listen(hp->sv_fd, qsize))
                err(EX_OSERR, "http_server_listen");

        for (;;) {
                pid_t   pid;
                int     connfd;

                connfd = accept(hp->sv_fd, NULL, NULL);
                if (connfd < 0 && errno == EINTR)
                        continue;
                if (connfd < 0)
                        err(EX_OSERR, "http_server_listen");

                if ((pid = fork()) < 0)
                        err(EX_OSERR, "http_server_listen");

                if (!pid) {
                        if (close(hp->sv_fd))
                                err(EX_OSERR, "http_server_listen");
                        http_server_client(hp, connfd);
                        exit(0);
                }

                if (close(connfd))
                        err(EX_OSERR, "http_server_listen");
        }
}

static struct http_request *http_request_new(int connfd);
static struct http_response *http_response_new(void);
static char *e_strdup(char *str);
static void http_request_free(struct http_request *req);
static void http_response_free(struct http_response *res);

static void free_kvp(struct hash_entry *ep)
{
        free(ep->he_key);
        free(ep->he_value);
}

static void
http_server_client(struct http_server *hp, int connfd)
{
        struct http_request *req;
        struct http_response *res;
        struct http_handler *hdlr;
        struct string *line;
        struct hash_entry *ep;
        int firstline;
        int c;

        req = http_request_new(connfd);
        res = http_response_new();
        line = string_new(0);
        firstline = 1;
        while ((c = iobuf_getc(req->rq_buf)) > 0) {
                char *linep;

                string_append(line, c);
                if (c != '\n')
                        continue;

                linep = line->s_arr;
                if (linep[0] == '\r')
                        break;

                if (firstline) {
                        char *method = strsep(&linep, " ");
                        char *resource = strsep(&linep, " ");
                        char *version = strsep(&linep, "\r");

                        req->rq_method = e_strdup(method);
                        req->rq_resource = e_strdup(resource);
                        req->rq_version = e_strdup(version);

                        firstline = 0;
                } else {
                        char *header = e_strdup(strsep(&linep, ":"));
                        char *value;

                        ++linep;
                        value = e_strdup(strsep(&linep, "\r"));

                        hashmap_set(req->rq_headers, header, value);
                }

                line->s_len = 0;
        }
        string_free(&line);

        ep = hashmap_get(hp->sv_handlers, req->rq_resource);
        if (ep) {
                hdlr = ep->he_value;
                hdlr->hh_fn(req, res);
        }

        http_response_free(res);
        http_request_free(req);
}

static struct http_request *
http_request_new(int connfd)
{
        struct http_request *req;

        req = malloc(sizeof(*req));
        if (!req)
                err(EX_SOFTWARE, "http_response_new");

        req->rq_headers = hashmap_new(0);
        if (!req->rq_headers)
                err(EX_SOFTWARE, "http_response_new");

        req->rq_buf = iobuf_new(connfd, 0);
        if (!req->rq_buf)
                err(EX_SOFTWARE, "http_response_new");

        req->rq_method = NULL;
        req->rq_version = NULL;
        req->rq_resource = NULL;
        return req;

}

static struct http_response *
http_response_new(void)
{
        struct http_response *res;

        res = malloc(sizeof(*res));
        if (!res)
                err(EX_SOFTWARE, "http_response_new");

        res->rs_headers = hashmap_new(0);
        if (!res->rs_headers)
                err(EX_SOFTWARE, "http_response_new");

        res->rs_version = NULL;
        res->rs_code = NULL;
        res->rs_msg = NULL;
        return res;
}

static char *
e_strdup(char *str)
{
        char *dup;

        dup = strdup(str);
        if (!dup)
                err(EX_SOFTWARE, "e_strdup");

        return dup;
}

static void
http_request_free(struct http_request *req)
{
        iobuf_free(&req->rq_buf);
        hashmap_for(req->rq_headers, free_kvp);
        hashmap_free(&req->rq_headers);
        free(req->rq_method);
        free(req->rq_version);
        free(req->rq_resource);
        free(req);
}

static void
http_response_free(struct http_response *res)
{
        hashmap_free(&res->rs_headers);
        free(res);
}

void
http_server_free(struct http_server *hp)
{
        hashmap_for(hp->sv_handlers, free_kvp);
        hashmap_free(&hp->sv_handlers);
        close(hp->sv_fd);
}
