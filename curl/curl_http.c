/*
 * 通过http管理元素监控
 *
 *  Created on: 2014-9-1
 *  Author: lijianwei
 */

#include "curl_http.h"

const static  struct table_entry content_type_table[] = {
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
	{"js", "application/x-javascript"},
	{ NULL, NULL },
};

//可以处理的静态扩展
const static char *staticExts[] = {
		"js",
		"css",
		"html",
		"htm",
		"png",
		"jpg",
		"jpeg",
		"gif",
		"bmp",
		"map", //js符号映射
		NULL
};

static const char *guess_content_type(const char *path)
{
	const char *last_period, *extension;
	const struct table_entry *ent;
	last_period = strrchr(path, '.');
	if (!last_period || strchr(last_period, '/'))
		goto not_found; /* no exension */
	extension = last_period + 1;
	for (ent = &content_type_table[0]; ent->extension; ++ent) {
		if (!evutil_ascii_strcasecmp(ent->extension, extension))
			return ent->content_type;
	}

not_found:
	return "application/misc";
}

static int get_uri_root(struct evhttp_bound_socket *handle, char *uri_root, size_t uri_root_size)
{
		struct sockaddr_storage ss;
		evutil_socket_t fd;
		ev_socklen_t socklen = sizeof(ss);
		char addrbuf[128];
		void *inaddr;
		const char *addr;
		int got_port = -1;


		fd = evhttp_bound_socket_get_fd(handle);

		memset(&ss, 0, sizeof(ss));
		if (getsockname(fd, (struct sockaddr *)&ss, &socklen)) {
			perror("getsockname() failed");
			return 1;
		}
		if (ss.ss_family == AF_INET) {
			got_port = ntohs(((struct sockaddr_in*)&ss)->sin_port);
			inaddr = &((struct sockaddr_in*)&ss)->sin_addr;
		}
		addr = evutil_inet_ntop(ss.ss_family, inaddr, addrbuf,
			sizeof(addrbuf));
		if (addr) {
			evutil_snprintf(uri_root, uri_root_size,
				"http://%s:%d", addr, got_port);

			return 0;
		} else {
			fprintf(stderr, "evutil_inet_ntop failed\n");
			return 1;
		}
}

/*
 * 获取$_GET  参照 http://zyan.cc/httpsqs_1_7/
 *
 * 注意最后使用evhttp_clear_headers(query_params_ptr);
 */
int getParams(struct evhttp_request *req, struct evkeyvalq *query_params_ptr)
{
	const char *http_query_part;

	http_query_part = evhttp_uri_get_query(req->uri_elems);
	if (http_query_part == NULL) {
		LOG(LOG_ERROR, "%s", "evhttp_uri_get_query error");
		return -1;
	}

	return evhttp_parse_query_str(http_query_part, query_params_ptr);
}

/**
 * $_GET['name']
 *
 * 注意释放返回值
 */
char *getParam(struct evkeyvalq *query_params_ptr, const char *name)
{
	char *data = NULL;
	const char *value = NULL;
	value = evhttp_find_header(query_params_ptr, name);
	if (value != NULL)
		data = strdup(value);
	return data;
}

/**
 * $_POST
 * 注意最后使用evhttp_clear_headers(post_params_ptr);
 */
int postParams(struct evhttp_request *req, struct evkeyvalq *post_params_ptr)
{
	char *data = NULL;
	ev_ssize_t n = 0;

	struct  evbuffer *buf = evhttp_request_get_input_buffer(req);
	size_t bufLen = evbuffer_get_length(buf);
#ifdef DEBUG
	printf("bufLen:%d\n", bufLen);
#endif

	//data = evbuffer_pullup(buf, bufLen);  使用这个因为pullup 因为有可能整理成连续内存   造成多拷贝一些多余字符
	//evbuffer_readline evbuffer_remove  evbuffer_drain
	data = calloc(bufLen, 1);
	n = evbuffer_copyout(buf, data, bufLen);
#ifdef DEBUG
	printf("evbuffer_copyout len:%d\n", n);
#endif

#ifdef DEBUG
	printf("postParams data %s\n", data);
#endif

	if (evhttp_parse_query_str(data,  post_params_ptr)) {
		LOG(LOG_ERROR, "%s", "evhttp_parse_query_str error");
		free(data);
		return -1;
	}

	free(data);

	return 0;
}

/**
 * $_POST['name']
 *
 * 注意释放返回值
 */
char *postParam(struct evkeyvalq *post_params_ptr, const char *name)
{
	char *data = NULL;
	const char *value = NULL;
	value = evhttp_find_header(post_params_ptr, name);
	if (value != NULL)
		data = strdup(value);
	return data;
}

//是否含有get参数
int have_get_params(struct evhttp_request *req)
{
	const char *http_query_part = evhttp_uri_get_query(req->uri_elems);
	return http_query_part != NULL;
}


//是否get请求
int is_get_request(struct evhttp_request *req)
{
	return evhttp_request_get_command(req) == EVHTTP_REQ_GET;
}

//是否post请求
int is_post_request(struct evhttp_request *req)
{
	return evhttp_request_get_command(req) == EVHTTP_REQ_POST;
}

//检测是否为application/x-www-form-urlencoded
int is_x_www_form_urlencoded(struct evhttp_request *req)
{
	struct evkeyvalq *headers = evhttp_request_get_input_headers(req);
	const char *content_type = evhttp_find_header(headers, "Content-Type");
	return is_post_request(req) && !strncasecmp(content_type, "application/x-www-form-urlencoded", 33);
}

static void gen_request_cb(struct evhttp_request *req, void *arg)
{
	//呵呵，加个标记
	evhttp_add_header(evhttp_request_get_output_headers(req), "X-Powered-By", "monitor_collect_change");

	const char *uri = NULL, *path = NULL;
	char *decoded_path = NULL;
	struct evhttp_uri *decoded = NULL;
	struct request_cb_complex   *request_cb_complex_ptr = NULL;
	struct evbuffer *buf = NULL;

	char *extension = NULL;
	char *whole_path = NULL;
	struct stat st;
	int fd = -1;
	size_t len = 0;


	buf = evbuffer_new();

	uri = evhttp_request_get_uri(req);
	decoded = evhttp_uri_parse(uri);

	path = evhttp_uri_get_path(decoded);
	if (!path) path = "/index.html";

	decoded_path = evhttp_uridecode(path, 0, NULL);

	//获取扩展名
	extension =  getExtension(decoded_path);
	if (extension != NULL) {
		extension = strtolower(extension);
#ifdef DEBUG
	LOG(LOG_DEBUG, "decoded_path:%s, extension:%s", decoded_path, extension);
#endif
	}

	//静态文件处理  参考libevent/sample/http-server.c
	if (extension != NULL && !in_array(staticExts, extension)) {

		len = strlen(decoded_path) + strlen(docroot) + strlen("web") + 3;
		if (!(whole_path = malloc(len))) {
			LOG(LOG_ERROR, "malloc error:%s", strerror(errno));
			goto err;
		}

		evutil_snprintf(whole_path, len, "%s/%s/%s", docroot, "web", decoded_path);

		if (stat(whole_path, &st)<0) {
			goto err;
		}

		//如果是一般文件
		if (S_ISREG(st.st_mode)) {
			const char *type = guess_content_type(decoded_path);
			if ((fd = open(whole_path, O_RDONLY)) < 0) {
				LOG(LOG_ERROR, "open error:%s", strerror(errno));
				goto err;
			}

			if (fstat(fd, &st)<0) {
				LOG(LOG_ERROR, "fstat error:%s", strerror(errno));
				goto err;
			}
			evhttp_add_header(evhttp_request_get_output_headers(req),"Content-Type", type);
			evbuffer_add_file(buf, fd, 0, st.st_size);
			evhttp_send_reply(req, 200, "OK", buf);
		}
	} else {
		if ((request_cb_complex_ptr = find_request_cb(request_cb_complex_head, decoded_path))) {
			if (!(*request_cb_complex_ptr->cb)(req, arg, buf)) {
				evhttp_send_reply(req, 200, "OK", buf);
			} else {
				evhttp_send_error(req, HTTP_INTERNAL, "error");
			}
		} else {
			evhttp_send_error(req, HTTP_NOTFOUND, "can't find request callback");
		}
	}
	goto done;

err:
	evhttp_send_error(req, 404, "404 not found");
	if (fd>=0)
		close(fd);
done:
	if (decoded)
		evhttp_uri_free(decoded);
	if (decoded_path)
		free(decoded_path);
	if (whole_path)
		free(whole_path);
	if (buf != NULL)
		evbuffer_free(buf);


	evhttp_send_reply_end(req);
}

void setup_http_server(const char *ip, const unsigned short port)
{
	struct evhttp_bound_socket *handle;

	base = event_base_new();
	if (base == NULL) {
		LOG(LOG_ERROR, "%s", "event_base_new error");
		exit(1);
	}

	http = evhttp_new(base);
	if (http == NULL) {
		LOG(LOG_ERROR, "%s", "evhttp_new error");
		exit(1);
	}

	handle = evhttp_bind_socket_with_handle(http, ip, port);

	if (handle == NULL) {
		LOG(LOG_ERROR, "evhttp_bind port:%ud failure", port);
		exit(1);
	}

	evhttp_set_gencb(http, gen_request_cb, handle);
}

void add_request_cb(int (*cb)(struct evhttp_request *, void *, struct evbuffer *), const char *path)
{
	static struct request_cb_complex *request_cb_complex_prev = NULL;

	//添加
	struct request_cb_complex *request_cb_complex_pointer = NULL;
	request_cb_complex_pointer = calloc(1, sizeof(struct request_cb_complex));
	request_cb_complex_pointer->path = strdup(path);
	request_cb_complex_pointer->cb = cb;


	//串起来
	if (request_cb_complex_prev == NULL) {
		request_cb_complex_head = calloc(1, sizeof(struct request_cb_complex));
		request_cb_complex_head->next = request_cb_complex_pointer;
	} else {
		request_cb_complex_prev->next = request_cb_complex_pointer;
	}
	request_cb_complex_prev = request_cb_complex_pointer;
}

struct request_cb_complex  *find_request_cb(struct request_cb_complex *head, char *path)
{
	struct request_cb_complex  *pointer = NULL;
	pointer = head->next;

	while(pointer != NULL) {
		if (!strcasecmp(pointer->path, path))
			return pointer;

		pointer = pointer->next;
	}

	return NULL;
}


//释放内存
void free_request_cb_list(struct request_cb_complex *head)
{
	struct request_cb_complex *prev = NULL, *pointer = NULL;
	prev = head;

	while(prev->next != NULL) {
		pointer = prev->next;

		free(pointer->path);

		free(prev);
		prev = pointer;
	}
	free(prev);
}


void start_http_server()
{
	event_base_dispatch(base);
}
