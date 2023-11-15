#ifndef HTTP_H
#define HTTP_H

#include "hashmap.h"
#include "iobuf.h"
#include "string.h"
#include <errno.h>
#include <netdb.h>

/* http request */
struct http_request {
        /* http request headers */
        struct hashmap  *rq_headers;
        /* connected socket buffer */
        struct iobuf    *rq_buf;
        /* http method */
        char            *rq_method;
        /* resource */
        char            *rq_resource;
        /* http version */
        char            *rq_version;
};

/* http response */
struct http_response {
        /* http response headers */
        struct hashmap          *rs_headers;
        /* http version */
        char                    *rs_version;
        /* http response code */
        char                    *rs_code;
        /* http response code message */
        char                    *rs_msg;
};

/* http handler */
struct http_handler {
        /* handler function */
        void (*hh_fn)(struct http_request *req, struct http_response *res);
};

/* http server */
struct http_server {
        /* mapping of resources to handlers */
        struct hashmap  *sv_handlers;
        /* listening socket */
        int             sv_fd;
};

/**
 * Create a new http_server:
 *
 * args:
 *      @ap:    address info
 * ret:
 *      @success:       pointer to new http_server
 *      @failure:       NULL and errno set
 */
extern struct http_server *http_server_new(struct addrinfo *ap);

/**
 * Add a new handler to http_server:
 *
 * args:
 *      @hp:            pointer to http_server
 *      @resource:      resource this handler is handling
 *      @fn:            function to call when resource is requested
 * ret:
 *      @success:       0
 *      @failure:       -1 and errno set
 */
extern int http_server_add_handler(struct http_server *hp,
                                   char *resource,
                                   struct http_handler *handler);

/**
 * Listen on http_server:
 *
 * args:
 *      @hp:    pointer to http_server
 *      @qsize: backlog size
 * ret:
 *      @success:       0
 *      @failure:       -1 and errno set
 */
extern int http_server_listen(struct http_server *hp, int qsize);

/**
 * Free an http_server:
 *
 * args:
 *      @hpp:   pointer to pointer to http_server
 * ret:
 *      @success:       0
 *      @failure:       -1 and errno set
 */
extern int http_server_free(struct http_server **hpp);

#endif
