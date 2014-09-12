/*
 * curl_http.h
 *
 *  Created on: 2014-9-1
 *      Author: Administrator
 */

#ifndef CURL_HTTP_H_
#define CURL_HTTP_H_

#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/keyvalq_struct.h>
#include <event2/http_struct.h>
#include <event2/util.h>


#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>
#include <fcntl.h>

#include "helper.h"
#include "controller/controller.h"

extern char *docroot;

//文件扩展
struct table_entry {
	const char *extension;
	const char *content_type;
};

static const struct table_entry content_type_table[] = {
	{ "txt", "text/plain" },
	{ "c", "text/plain" },
	{ "h", "text/plain" },
	{ "html", "text/html" },
	{ "htm", "text/htm" },
	{ "css", "text/css" },
	{ "gif", "image/gif" },
	{ "jpg", "image/jpeg" },
	{ "jpeg", "image/jpeg" },
	{ "png", "image/png" },
	{ "pdf", "application/pdf" },
	{ "ps", "application/postsript" },
	{ NULL, NULL },
};

struct request_cb_complex{
	int (*cb)(struct evhttp_request *, void *, struct evbuffer *);
	char *path;
	struct request_cb_complex *next;
};


struct event_base *base;
struct evhttp *http;
struct request_cb_complex *request_cb_complex_head;


int getParams(struct evhttp_request *req, struct evkeyvalq *query_params_ptr);

char *getParam(struct evkeyvalq *query_params_ptr, const char *name);

int postParams(struct evhttp_request *req, struct evkeyvalq *post_params_ptr);

char *postParam(struct evkeyvalq *post_params_ptr, const char *name);

//是否get请求
int is_get_request(struct evhttp_request *req);

//是否post请求
int is_post_request(struct evhttp_request *req);

//检测是否为application/x-www-form-urlencoded
int is_x_www_form_urlencoded(struct evhttp_request *req);

void setup_http_server(const char *ip, const unsigned short port);

void start_http_server();

void add_request_cb(int (*cb)(struct evhttp_request *, void *, struct evbuffer *), const char *path);

struct request_cb_complex  *find_request_cb(struct request_cb_complex *head, char *path);

void free_request_cb_list(struct request_cb_complex *head);


#endif /* CURL_HTTP_H_ */
