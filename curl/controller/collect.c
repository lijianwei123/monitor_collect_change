#include "collect.h"

static int collect_update_match(MYSQL *conn_ptr, const char *id, int flag);
static int collect_preg_match(char *url, char *pattern);


//列表
int collect_list_request_cb(struct evhttp_request *req, void *arg, struct evbuffer *buf)
{
	MYSQL conn;
	memset(&conn, 0, sizeof(MYSQL));
	int retCode = -1;
	char *select_sql = NULL;
	char *json_str = NULL;
	result_data_t result_data;
	memset(&result_data, 0, sizeof(result_data_t));


	retCode = mysql_connect(&conn, &mysql_connect_info);
	if (!retCode) {
		select_sql = "select * from df_monitor_collection";
		retCode = mysql_select(&conn, select_sql, &result_data);

		if (!retCode) {
			json_str =	mysql_result_data_convert_json(&result_data);

			evhttp_add_header(evhttp_request_get_output_headers(req), "Content-type", "application/json");
			evbuffer_add_printf(buf, "%s", json_str);

			free(json_str);
			free_result_data(&result_data);
			mysql_close(&conn);
			return 0;
		}
	}

	return -1;
}


//添加
int collect_insert_request_cb(struct evhttp_request *req, void *arg, struct evbuffer *buf)
{
	int retCode = -1;
	struct evkeyvalq *post_params_ptr = calloc(sizeof(struct evkeyvalq), 1);
	char *url = NULL, *pattern = NULL;
	char *escape_url = NULL, *escape_pattern = NULL;
	char modify_time[20] = {0};

	MYSQL conn;
	memset(&conn, 0, sizeof(MYSQL));
	const char insert_template_sql[] = "insert into `df_monitor_collection` (`url`, `pattern`, `modify_time`) values('%s', '%s', '%s')";
	char *insert_sql = NULL;
	char *json_str = NULL;
	unsigned long result = 0l;


	if (!is_x_www_form_urlencoded(req)) {
		goto done;
	}

	//获取参数
	retCode = postParams(req, post_params_ptr);
	if (retCode) {
		goto done;
	}

	url = postParam(post_params_ptr, "url");
	pattern = postParam(post_params_ptr, "pattern");
	if (url == NULL || pattern == NULL) {
		retCode = -1;
		goto done;
	}

	getDate(modify_time);

	LOG(LOG_INFO, "url:%s, pattern:%s, modify_time:%s", url, pattern, modify_time);

	//连接数据库
	retCode = mysql_connect(&conn, &mysql_connect_info);

	if(retCode) {
		goto close;
	}

	escape_url = cap_mysql_escape_string(&conn, url, strlen(url));
	escape_pattern = cap_mysql_escape_string(&conn, pattern, strlen(pattern));

	insert_sql = calloc(strlen(insert_template_sql) + strlen(escape_url) + strlen(escape_pattern) + strlen(modify_time) + 1, 1);
	sprintf(insert_sql, insert_template_sql, escape_url, escape_pattern, modify_time);

	retCode = mysql_execute(&conn, insert_sql, &result);
	if (retCode) {
		goto close;
	}

#ifdef DEBUG
	LOG(LOG_DEBUG, "mysql_execute insert id:%lu", result);
#endif

	//成功
	json_str = sucOutputByJson();
	evhttp_add_header(evhttp_request_get_output_headers(req), "Content-type", "application/json");
	evbuffer_add_printf(buf, "%s", json_str);
	free(json_str);

close:
	mysql_close(&conn);
done:
	if (post_params_ptr)
		evhttp_clear_headers(post_params_ptr);
	if (url)
		free(url);
	if (pattern)
		free(pattern);
	if (escape_url)
		free(escape_url);
	if (escape_pattern)
		free(escape_pattern);
	if (insert_sql)
		free(insert_sql);
	
return retCode;
}

/**
 *  修改
 *  post fields: id url pattern
 */
int collect_modify_request_cb(struct evhttp_request *req, void *arg, struct evbuffer *buf)
{
	int retCode = -1;
	struct evkeyvalq *post_params_ptr = calloc(sizeof(struct evkeyvalq), 1);
	char *url = NULL, *pattern = NULL, *id = NULL;
	int id_int = 0;

	MYSQL conn;
	memset(&conn, 0, sizeof(MYSQL));
	const char update_template_sql[] = "update `df_monitor_collection` set `url` = '%s', `pattern` = '%s' where id = %d";
	char *update_sql = NULL;
	char *json_str = NULL;
	unsigned long result = 0l;

	if (!is_x_www_form_urlencoded(req)) {
		goto done;
	}

	//获取参数
	retCode = postParams(req, post_params_ptr);
	if (retCode) {
		goto done;
	}

	id = postParam(post_params_ptr, "id");
	url = postParam(post_params_ptr, "url");
	pattern = postParam(post_params_ptr, "pattern");
	if (id == NULL || url == NULL || pattern == NULL) {
		retCode = -1;
		goto done;
	}


	LOG(LOG_INFO, "id: %s, url:%s, pattern:%s", id, url, pattern);

	//连接数据库
	retCode = mysql_connect(&conn, &mysql_connect_info);

	if(retCode) {
		goto close;
	}

	update_sql = calloc(strlen(update_template_sql) + strlen(id) + strlen(url) + strlen(pattern) + 1, 1);
	id_int = atoi(id);
	sprintf(update_sql, update_template_sql, url, pattern, id_int);

	retCode = mysql_execute(&conn, update_sql, &result);
	if (retCode) {
		goto close;
	}

#ifdef DEBUG
	LOG(LOG_DEBUG, "mysql_execute effected row nums:%lu", result);
#endif

	//成功
	json_str = sucOutputByJson();
	evhttp_add_header(evhttp_request_get_output_headers(req), "Content-type", "application/json");
	evbuffer_add_printf(buf, "%s", json_str);
	free(json_str);

close:
	mysql_close(&conn);
done:
	if (post_params_ptr)
		evhttp_clear_headers(post_params_ptr);
	if (url)
		free(url);
	if (pattern)
		free(pattern);
	if (update_sql)
		free(update_sql);

return retCode;
}

int collect_del_request_cb(struct evhttp_request *req, void *arg, struct evbuffer *buf)
{
	int retCode = -1;
	struct evkeyvalq *query_params_ptr = calloc(sizeof(struct evkeyvalq), 1);
	char *id = NULL;
	int id_int = 0;

	MYSQL conn;
	memset(&conn, 0, sizeof(MYSQL));
	const char delete_template_sql[] = "delete from df_monitor_collection where id = %d";
	char *delete_sql = NULL;
	char *json_str = NULL;
	unsigned long result = 0l;

	if (!is_get_request(req)) {
		goto done;
	}

	retCode = getParams(req, query_params_ptr);

	if (retCode) {
		goto done;
	}

	id = getParam(query_params_ptr, "id");
	if (id == NULL) {
		goto done;
	}
	LOG(LOG_INFO, "id:%s", id);
	id_int = atoi(id);

	retCode = mysql_connect(&conn, &mysql_connect_info);

	if(retCode) {
		goto done;
	}

	delete_sql = calloc(strlen(delete_template_sql) + strlen(id) + 1, 1);
	sprintf(delete_sql, delete_template_sql, id_int);

	retCode = mysql_execute(&conn, delete_sql, &result);
	if (retCode) {
		goto done;
	}

#ifdef DEBUG
	LOG(LOG_DEBUG, "mysql_execute effected row nums:%lu", result);
#endif

	//成功
	json_str = sucOutputByJson();
	evhttp_add_header(evhttp_request_get_output_headers(req), "Content-type", "application/json");
	evbuffer_add_printf(buf, "%s", json_str);
	free(json_str);

done:
	if (query_params_ptr)
		evhttp_clear_headers(query_params_ptr);
	if (id)
		free(id);
	if (delete_sql)
		free(delete_sql);

	mysql_close(&conn);

return retCode;
}

//测试匹配
int collect_match_request_cb(struct evhttp_request *req, void *arg, struct evbuffer *buf)
{
	int retCode = -1;
	struct evkeyvalq *query_params_ptr = calloc(1, sizeof(struct evkeyvalq));
	char *id = NULL;
	int id_int = 0;

	MYSQL conn;
	memset(&conn, 0, sizeof(MYSQL));
	result_data_t result_data;
	memset(&result_data, 0, sizeof(result_data_t));
	mysql_field_value_t *pointer = NULL;

	const char select_template_sql[] = "select * from df_monitor_collection where id = %d";
	char *select_sql = NULL;
	char *json_str = NULL;
	int j = 0;
	char *url = NULL, *pattern = NULL;

	if (!is_get_request(req)) {
		goto done;
	}

	//获取参数
	retCode = getParams(req, query_params_ptr);

	if (retCode) {
		goto done;
	}

	id = getParam(query_params_ptr, "id");
	if (id == NULL) {
		retCode = -1;
		goto done;
	}
	LOG(LOG_INFO, "id:%s", id);
	id_int = atoi(id);

	//连接数据库
	retCode = mysql_connect(&conn, &mysql_connect_info);

	if(retCode) {
		goto close;
	}

	//组装sql语句
	select_sql = calloc(strlen(select_template_sql) + strlen(id) + 1, 1);
	sprintf(select_sql, select_template_sql, id_int);

	//查询
	retCode = mysql_select(&conn, select_sql, &result_data);
	if (retCode) {
		goto close;
	}

	//获取值
	pointer = *(result_data.data);
	for (j = 0; j < result_data.columns; j++) {
		if (strcasecmp(pointer->next->fieldName, "url") == 0) {
			url = pointer->next->fieldValue;
		} else if (strcasecmp(pointer->next->fieldName, "pattern") == 0) {
			pattern = pointer->next->fieldValue;
		}
		pointer = pointer->next;
	}

	//正则匹配
	int flag = 0;
	flag = collect_preg_match(url, pattern);


	//update match
	retCode = collect_update_match(&conn, id, flag);
	if (retCode) {
		goto close;
	}


	//成功
	json_str = flag ? sucOutputByJson() : errOutputByJson("not match");
	evhttp_add_header(evhttp_request_get_output_headers(req), "Content-type", "application/json");
	evbuffer_add_printf(buf, "%s", json_str);
	free(json_str);
close:
	mysql_close(&conn);
done:
	if (query_params_ptr)
		evhttp_clear_headers(query_params_ptr);
	if (id)
		free(id);
	if (select_sql)
		free(select_sql);

	free_result_data(&result_data);

return retCode;
}

//更新match
static int collect_update_match(MYSQL *conn_ptr, const char *id, int flag)
{
	int id_int = atoi(id);
	char *update_sql = NULL;

	char modify_time[20] = {0};
	getDate(modify_time);

	unsigned long result = 0l;
	int retCode = -1;


	char *update_template_sql = "update `df_monitor_collection` set `match` = %d, `modify_time` = '%s' where id = %d";
	update_sql = calloc(strlen(update_template_sql) + 1 + strlen(modify_time) + strlen(id) + 1, 1);

	sprintf(update_sql, update_template_sql, flag, modify_time, id_int);

	retCode = mysql_execute(conn_ptr, update_sql, &result);

#ifdef DEBUG
	LOG(LOG_DEBUG, "mysql_execute effected row nums:%lu", result);
#endif

	free(update_sql);

	return retCode;
}


//1 表示匹配到    0表示没有匹配到
static int collect_preg_match(char *url, char *pattern)
{
	regmatch_t pm[2];
	memset(pm, 0, sizeof(pm));
	int flag = 0;

	long dataLen = 0L;
	content_info_t dataInfo;
	memset(&dataInfo, 0, sizeof(content_info_t));
	dataLen = get_content_len(url, (setExtraCurlOpt)NULL, 1, &dataInfo);

	flag = preg_match(pattern, dataInfo.contents, pm, array_count(pm));
	free(dataInfo.contents);

	return flag;
}

/*
 * 批量match
 * 0 成功  -1 失败
 */
int collect_batch_match(void)
{
	MYSQL conn;
	memset(&conn, 0, sizeof(MYSQL));
	int retCode = -1;
	char *select_sql = NULL;
	result_data_t result_data;
	memset(&result_data, 0, sizeof(result_data_t));
	int i = 0;
	mysql_field_value_t *pointer = NULL;
	char *id = NULL, *url = NULL, *pattern = NULL;
	int flag = 0;


	retCode = mysql_connect(&conn, &mysql_connect_info);

	if (retCode) {
		goto done;
	}

	select_sql = "select id, url, pattern from df_monitor_collection";
	retCode = mysql_select(&conn, select_sql, &result_data);

	if (retCode) {
		goto done;
	}

	for (i = 0; i < result_data.rows; i++) {
		pointer = *(result_data.data + i);

		//获取这些值
		id = pointer->next->fieldValue;
		url = pointer->next->next->fieldValue;
		pattern = pointer->next->next->next->fieldValue;

		flag = collect_preg_match(url, pattern);
		retCode = collect_update_match(&conn, id, flag);

		if (retCode) {
			goto done;
		}
	}

done:
	mysql_close(&conn);
	free_result_data(&result_data);

return retCode;
}




