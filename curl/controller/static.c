/*
 * 	static.c
 *
 *  Created on: 2014-9-7
 *  Author: Administrator
 */

#include "static.h"


static void static_dump_request_cb(struct evhttp_request *req, void *arg)
{

	const char *cmdtype;
	struct evkeyvalq *headers;
	struct evkeyval *header;
	struct evbuffer *buf;


	switch (evhttp_request_get_command(req)) {
		case EVHTTP_REQ_GET:
			cmdtype = "GET";
		break;
		case EVHTTP_REQ_POST:
			cmdtype = "POST";
		break;
		default:
			cmdtype = "unknown";
		break;
	}
	printf("Received a %s request for %s\nHeaders:\n",
	    cmdtype, evhttp_request_get_uri(req));

	headers = evhttp_request_get_input_headers(req);
	for (header = headers->tqh_first; header;
	    header = header->next.tqe_next) {
		printf("  %s: %s\n", header->key, header->value);
	}

	buf = evhttp_request_get_input_buffer(req);
	puts("Input data: <<<");
	printf("len:%d", evbuffer_get_length(buf));

	/*
	while (evbuffer_get_length(buf)) {
		int n;
		char cbuf[128];
		n = evbuffer_remove(buf, cbuf, sizeof(buf)-1);
		if (n > 0)
			(void) fwrite(cbuf, 1, n, stdout);
	}
	puts(">>>");
	*/

	//get params
	struct evkeyvalq *query_params_ptr = calloc(sizeof(struct evkeyvalq), 1);
	getParams(req, query_params_ptr);
	char *test = NULL;
	test = getParam(query_params_ptr, "test");
	if (test != NULL) {
		fprintf(stderr, "param test: %s", test);
	}
	evhttp_clear_headers(query_params_ptr);
	free(test);


	//post params
	struct evkeyvalq *post_params_ptr = calloc(sizeof(struct evkeyvalq), 1);
	postParams(req, post_params_ptr);
	char *name = NULL;
	name = postParam(post_params_ptr, "name");
	if (name != NULL) {
		fprintf(stderr, "param name: %s", name);
	}

	evhttp_clear_headers(post_params_ptr);
	free(name);

	evhttp_send_reply(req, 200, "OK", NULL);

	//evhttp_request_free(req);
}

static void static_list_request_cb(struct evhttp_request *req, void *arg)
{
	struct evbuffer *evb = NULL;
	struct evhttp_uri *decoded = NULL;
	const char *path;
	char *decoded_path;
	int fd = -1;
	struct stat st;
	char uri_root[512] = {0};
	struct evhttp_bound_socket *handle = (struct evhttp_bound_socket *)arg;

	size_t len;
	char *whole_path = NULL;

	const char *uri = evhttp_request_get_uri(req);
	get_uri_root(handle, uri_root, sizeof(uri_root));


	decoded = evhttp_uri_parse(uri);
	if (decoded == NULL) {
		LOG(LOG_ERROR, "%s", "evhttp_decode_uri error");
		evhttp_send_error(req, HTTP_BADREQUEST, 0);
		exit(1);
	}

	path = evhttp_uri_get_path(decoded);
	if (!path) path = "/";

	decoded_path = evhttp_uridecode(path, 0, NULL);
	if (decoded_path == NULL)
		goto err;

	if (strstr(decoded_path, ".."))
		goto err;

	len = strlen(decoded_path)+strlen(docroot)+2;
	if (!(whole_path = malloc(len))) {
		perror("malloc");
		goto err;
	}
	evutil_snprintf(whole_path, len, "%s/%s", docroot, decoded_path);

	if (stat(whole_path, &st)<0) {
		goto err;
	}

	evb = evbuffer_new();

	if (S_ISDIR(st.st_mode)) {
		DIR *d;
		struct dirent *ent;

		const char *trailing_slash = "";

		if (!strlen(path) || path[strlen(path)-1] != '/')
			trailing_slash = "/";

		if (!(d = opendir(whole_path)))
			goto err;

		evbuffer_add_printf(evb, "<html>\n <head>\n"
		    "  <title>%s</title>\n"
		    "  <base href='%s%s%s'>\n"
		    " </head>\n"
		    " <body>\n"
		    "  <h1>%s</h1>\n"
		    "  <ul>\n",
		    decoded_path, /* XXX html-escape this. */
		    uri_root, path, /* XXX html-escape this? */
		    trailing_slash,
		    decoded_path /* XXX html-escape this */);

		while((ent = readdir(d))) {
			const char *name = ent->d_name;
			evbuffer_add_printf(evb, "    <li><a href=\"%s\">%s</a>\n", name, name);
			evbuffer_add_printf(evb, "</ul></body></html>\n");
		}
		closedir(d);
		evhttp_add_header(evhttp_request_get_output_headers(req), "Content-type", "text/html");
	} else {
		const char *type = guess_content_type(decoded_path);
		if ((fd = open(whole_path, O_RDONLY)) < 0) {
			perror("open");
			goto err;
		}

		if (fstat(fd, &st)<0) {
					/* Make sure the length still matches, now that we
					 * opened the file :/ */
					perror("fstat");
					goto err;
		}

		evhttp_add_header(evhttp_request_get_output_headers(req),
			    "Content-Type", type);

		evbuffer_add_file(evb, fd, 0, st.st_size);
	}

	evhttp_send_reply(req, HTTP_OK, "OK", evb);
	goto done;

err:
	evhttp_send_error(req, HTTP_NOTFOUND, "Document was not found");
	if (fd>=0)
		close(fd);

done:
	if (decoded)
		evhttp_uri_free(decoded);
	if (decoded_path)
		free(decoded_path);
	if (whole_path)
		free(whole_path);
	if (evb)
		evbuffer_free(evb);
}



