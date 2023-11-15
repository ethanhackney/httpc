#include "http.h"
#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <err.h>
#include <sysexits.h>

struct http_server *
http_server_new(struct addrinfo *ap)
{
        struct http_server      *s = NULL;
        int                     saved_errno;
        int                     y;

        s = malloc(sizeof(*s));
        if (s == NULL)
                goto ret;

        s->sv_handlers = hashmap_new(0);
        if (s->sv_handlers == NULL)
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

int
http_server_add_handler(struct http_server *hp,
                        char *resource,
                        struct http_handler *handler)
{
        struct hash_entry       *ep = NULL;

        ep = hashmap_set(hp->sv_handlers, resource, handler);
        return ep != NULL ? 0 : -1;
}

static int http_server_sanity(const struct http_server *server);
static void sigchld_handler(int signo);
static void http_server_client(struct http_server *hp, int connfd);

int
http_server_listen(struct http_server *hp, int qsize)
{
        struct sigaction        act;

        if (http_server_sanity(hp) < 0)
                return -1;

        memset(&act, 0, sizeof(act));
        act.sa_handler = sigchld_handler;
        act.sa_flags = 0;
        sigemptyset(&act.sa_mask);
        if (sigaction(SIGCHLD, &act, NULL) < 0)
                return -1;

        if (listen(hp->sv_fd, qsize) < 0)
                return -1;

        for (;;) {
                pid_t   pid;
                int     connfd;

                connfd = accept(hp->sv_fd, NULL, NULL);
                if (connfd < 0 && errno == EINTR)
                        continue;
                if (connfd < 0)
                        break;

                pid = fork();
                if (pid < 0)
                        break;

                if (pid == 0) {
                        (void)close(hp->sv_fd);
                        http_server_client(hp, connfd);
                        close(connfd);
                        exit(0);
                }

                if (close(connfd) < 0)
                        break;
        }

        return -1;
}

static int
http_server_sanity(const struct http_server *server)
{
        errno = EINVAL;
        if (server == NULL)
                return -1;
        if (server->sv_handlers == NULL)
                return -1;
        if (server->sv_fd < 0)
                return -1;
        errno = 0;
        return 0;
}

static void
sigchld_handler(int signo)
{
        pid_t pid;

        while ((pid = waitpid(-1, NULL, WNOHANG)) > 0)
                ;
}

static struct http_request *http_request_new(int connfd);
static struct http_response *http_response_new(void);
static int http_request_free(struct http_request **reqp);
static void free_hash_entry(struct hash_entry *ep);
static int http_response_free(struct http_response **resp);
static int http_parse_first_line(struct http_request *req, char *linep);
static int http_parse_header(struct http_request *req, char *linep);

static void
http_server_client(struct http_server *hp, int connfd)
{
        struct http_response    *res = NULL;
        struct http_request     *req = NULL;
        struct http_handler     *hdlr = NULL;
        struct hash_entry       *ep = NULL;
        struct string           *line = NULL;
        int                     firstline;
        int                     c;
        int                     reqok;

        req = http_request_new(connfd);
        if (req == NULL)
                err(EX_SOFTWARE, "http_request_new()");

        res = http_response_new();
        if (res == NULL)
                err(EX_SOFTWARE, "http_response_new()");

        line = string_new(0);
        if (line == NULL)
                err(EX_SOFTWARE, "string_new()");

        firstline = 1;
        reqok = 1;
        while ((c = iobuf_getc(req->rq_buf)) > 0) {
                char    *linep = NULL;

                if (string_append(line, c) < 0) {
                        warn("failed to read line");
                        reqok = 0;
                        break;
                }

                if (c != '\n')
                        continue;

                linep = line->s_arr;
                if (linep[0] == '\r')
                        break;

                if (firstline) {
                        if (http_parse_first_line(req, linep) < 0) {
                                warn("malformed first line: %s", linep);
                                reqok = 0;
                                break;
                        }
                        firstline = 0;
                } else {
                        if (http_parse_header(req, linep) < 0) {
                                warn("malformed header: %s", linep);
                                reqok = 0;
                                break;
                        }
                }

                line->s_len = 0;
        }
        if (string_free(&line) < 0)
                warn("could not free string");

        if (reqok) {
                ep = hashmap_get(hp->sv_handlers, req->rq_resource);
                if (ep != NULL) {
                        hdlr = ep->he_value;
                        hdlr->hh_fn(req, res);
                } else {
                        warn("no handler for %s", req->rq_resource);
                }
        }

        (void)http_response_free(&res);
        (void)http_request_free(&req);
}

static struct http_request *
http_request_new(int connfd)
{
        struct http_request     *req = NULL;

        req = malloc(sizeof(*req));
        if (req == NULL)
                goto ret;

        req->rq_headers = hashmap_new(0);
        if (req->rq_headers == NULL)
                goto free_req;

        req->rq_buf = iobuf_new(connfd, 0);
        if (req->rq_buf == NULL)
                goto free_headers;

        req->rq_method = NULL;
        req->rq_version = NULL;
        req->rq_resource = NULL;
        goto ret;
free_headers:
        (void)hashmap_free(&req->rq_headers);
free_req:
        free(req);
        req = NULL;
ret:
        return req;

}

static struct http_response *
http_response_new(void)
{
        struct http_response    *res = NULL;

        res = malloc(sizeof(*res));
        if (res == NULL)
                return NULL;

        res->rs_headers = hashmap_new(0);
        if (res->rs_headers == NULL) {
                free(res);
                return NULL;
        }

        res->rs_version = NULL;
        res->rs_code = NULL;
        res->rs_msg = NULL;
        return res;
}

static int http_request_sanity(const struct http_request *req);

static int
http_request_free(struct http_request **reqp)
{
        struct http_request     *req = NULL;

        if (reqp == NULL) {
                errno = EINVAL;
                return -1;
        }
        req = *reqp;

        if (http_request_sanity(req) < 0)
                return -1;

        if (iobuf_free(&req->rq_buf) < 0)
                return -1;

        if (hashmap_for(req->rq_headers, free_hash_entry) < 0)
                return -1;

        if (hashmap_free(&req->rq_headers) < 0)
                return -1;

        free(req->rq_method);
        free(req->rq_version);
        free(req->rq_resource);
        free(req);
        *reqp = NULL;
        return 0;
}

static int
http_request_sanity(const struct http_request *req)
{
        errno = EINVAL;
        if (req == NULL)
                return -1;
        if (req->rq_buf == NULL)
                return -1;
        if (req->rq_headers == NULL)
                return -1;
        if (req->rq_method == NULL)
                return -1;
        if (req->rq_version == NULL)
                return -1;
        if (req->rq_resource == NULL)
                return -1;
        errno = 0;
        return 0;
}

static void
free_hash_entry(struct hash_entry *ep)
{
        free(ep->he_key);
        free(ep->he_value);
}

static int http_response_sanity(const struct http_response *res);

static int
http_response_free(struct http_response **resp)
{
        struct http_response    *res = NULL;

        if (resp == NULL) {
                errno = EINVAL;
                return -1;
        }

        res = *resp;
        if (http_response_sanity(res) < 0)
                return -1;

        if (hashmap_for(res->rs_headers, free_hash_entry) < 0)
                return -1;

        if (hashmap_free(&res->rs_headers) < 0)
                return -1;

        /* have no added code to initialize these yet
        free(res->rs_code);
        free(res->rs_msg);
        free(res->rs_version);
        */
        free(res);
        *resp = NULL;
        return 0;
}

static int
http_response_sanity(const struct http_response *res)
{
        errno = 0;
        if (res == NULL)
                return -1;
        if (res->rs_headers == NULL)
                return -1;
        /* have no added code to initialize these yet
        if (res->rs_code == NULL)
                return -1;
        if (res->rs_msg == NULL)
                return -1;
        if (res->rs_version == NULL)
                return -1;
                */
        errno = 0;
        return 0;
}

static int http_server_sanity(const struct http_server *server);

int
http_server_free(struct http_server **hpp)
{
        struct http_server      *hp = NULL;

        if (hpp == NULL) {
                errno = EINVAL;
                return -1;
        }

        hp = *hpp;
        if (http_server_sanity(hp) < 0)
                return -1;

        if (hashmap_for(hp->sv_handlers, free_hash_entry) < 0)
                return -1;

        if (hashmap_free(&hp->sv_handlers) < 0)
                return -1;

        return close(hp->sv_fd);
}

static int
http_parse_first_line(struct http_request *req, char *linep)
{
        char    *method = NULL;
        char    *resource = NULL;
        char    *version = NULL;

        method = strsep(&linep, " ");
        if (method == NULL)
                return -1;

        resource = strsep(&linep, " ");
        if (resource == NULL)
                return -1;

        version = strsep(&linep, "\r");
        if (version == NULL)
                return -1;

        req->rq_method = strdup(method);
        if (req->rq_method == NULL)
                return -1;

        req->rq_resource = strdup(resource);
        if (req->rq_resource == NULL) {
                free(req->rq_method);
                req->rq_method = NULL;
                return -1;
        }

        req->rq_version = strdup(version);
        if (req->rq_version == NULL) {
                free(req->rq_method);
                req->rq_method = NULL;
                free(req->rq_resource);
                req->rq_resource = NULL;
                return -1;
        }

        return 0;
}

static int
http_parse_header(struct http_request *req, char *linep)
{
        struct hash_entry       *p = NULL;
        char                    *header = NULL;
        char                    *value = NULL;

        header = strsep(&linep, ":");
        if (header == NULL)
                return -1;

        if (linep == NULL)
                return -1;

        ++linep;
        value = strsep(&linep, "\r");
        if (value == NULL)
                return -1;

        p = hashmap_get(req->rq_headers, header);
        if (p != NULL) {
                value = strdup(value);
                if (value == NULL)
                        return -1;
                free(p->he_value);
                p->he_value = value;
        } else {
                header = strdup(header);
                if (header == NULL)
                        return -1;
                value = strdup(value);
                if (value == NULL) {
                        free(header);
                        return -1;
                }
                p = hashmap_set(req->rq_headers, header, value);
                if (p == NULL) {
                        free(header);
                        free(value);
                        return -1;
                }
        }

        return 0;
}
