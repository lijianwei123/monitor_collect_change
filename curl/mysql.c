/*
 *	mysql操作方法
 *	@see http://dev.mysql.com/doc/refman/5.1/en/c-api-function-overview.html
 *	@see http://zetcode.com/db/mysqlc/
 *	@see mysql中文手册   mysql c api
 *
 *  Created on: 2014-8-29
 *  Author: lijianwei
 */

#include "mysql.h"

static void mysql_fill_data(MYSQL_RES *res_ptr, result_data_t *result_data_ptr);

//关闭数据库
static void close_mysql(int status,void *arg)
{
	LOG(LOG_INFO, "%s", "close");
	//mysql_close((MYSQL *)arg);
}


static const char *  mysql_error_with_if(MYSQL *conn_ptr)
{
	if(mysql_error(conn_ptr)[0] != '\0') {
		return mysql_error(conn_ptr);
	} else {
		return "no mysql error";
	}
}


inline void  mysql_connection_info_init(mysql_connect_info_t *mysql_connection_info_ptr)
{
	mysql_connection_info_ptr->host = "127.0.0.1";
	mysql_connection_info_ptr->port = 3306;
	mysql_connection_info_ptr->db = "test";
	mysql_connection_info_ptr->unix_socket = NULL;
	mysql_connection_info_ptr->user = "root";
	mysql_connection_info_ptr->pwd = "root";
	//mysql_connection_info_ptr->client_flag = CLIENT_MULTI_STATEMENTS; //通知服务器，客户端可能在单个字符串内发送多条语句（由‘;’隔开）。如果未设置该标志，将禁止多语句执行    参见 mysql  c api
}

/**
 * 链接数据库
 * 使用
 * MYSQL mysql
 * mysql_connect(&mysql, ...);
 * @param MYSQL *conn_ptr
 * @param mysql_connect_info_t *mysql_connect_info_ptr
 * @return  0 表示成功  -1表示失败
 */
int mysql_connect(MYSQL *conn_ptr, mysql_connect_info_t *mysql_connect_info_ptr)
{
	int retCode = -1;

	conn_ptr = mysql_init(conn_ptr);
	if (conn_ptr == NULL) {
		LOG(LOG_ERROR, "%s", "mysql_init error");
		return -1;
	}


	conn_ptr = mysql_real_connect(
			conn_ptr,
			mysql_connect_info_ptr->host,
			mysql_connect_info_ptr->user,
			mysql_connect_info_ptr->pwd,
			mysql_connect_info_ptr->db,
			mysql_connect_info_ptr->port,
			mysql_connect_info_ptr->unix_socket,
			mysql_connect_info_ptr->client_flag
	);

	
#ifdef DEBUG
	printf("mysql_connect_info_ptr info: %s,%s,%s\n", mysql_connect_info_ptr->host, mysql_connect_info_ptr->user, mysql_connect_info_ptr->pwd);
#endif

	if (conn_ptr == NULL) {
		LOG(LOG_ERROR, "mysql_real_connect error:%s", mysql_error_with_if(conn_ptr));
		return -1;
	}

	retCode = mysql_set_character_set(conn_ptr, "utf8");
	if (retCode != 0) {
		LOG(LOG_ERROR, "mysql_set_charset_name error:%s", mysql_error_with_if(conn_ptr));
	}


	on_exit(close_mysql, (void *)conn_ptr);

	return 0;
}

/**
 *	执行写操作
 *	@param MYSQL *conn_ptr
 *	@param const char *sql  会自动trim
 *	@param int result   添加  返回insert_id  其他返回影响行数
 *	@return 0 表示成功   -1表示失败
 */
int mysql_execute(MYSQL *conn_ptr, char *sql, unsigned long *result)
{
	sql = strtrim(sql);
#ifdef DEBUG
	printf("mysql_execute sql:%s\n", sql);
#endif

	int retCode = -1;
	int isInsert = strncasecmp(sql, "INSERT ", 6) == 0;

	retCode = mysql_query(conn_ptr, sql);

	if (retCode == 0) {
		//执行成功
		if (isInsert) {
			*result = (unsigned long)mysql_insert_id(conn_ptr);
		} else {
			*result = (unsigned long)mysql_affected_rows(conn_ptr);
		}
		return 0;
	} else {
		LOG(LOG_ERROR, "mysql_execute error:%d, %s", mysql_errno(conn_ptr), mysql_error_with_if(conn_ptr));
		return -1;
	}
}


/**
 * 执行读操作
 * @param char *sql  需要用mysql_real_escape_string
 * @param result_data_t *result_data_ptr
 * @return 0 or -1
 */
int mysql_select(MYSQL *conn_ptr, char *sql, result_data_t *result_data_ptr)
{

#ifdef DEBUG
	printf("mysql_execute sql:%s\n", sql);
#endif

	//过滤空格
	sql = strtrim(sql);


	int retCode = -1;
	MYSQL_RES *res_ptr; //结果集

	//与mysql_query的区别   可以查看mysql中文手册   mysql c api
	retCode = mysql_real_query(conn_ptr, sql, strlen(sql));

	if (retCode == 0) {
		res_ptr = mysql_store_result(conn_ptr);
		if (res_ptr == NULL) {
			//没有错误
			if (mysql_errno(conn_ptr) == 0) {
				LOG(LOG_INFO, "%s", "mysql_store_result empty");
			} else {
				LOG(LOG_ERROR, "mysql_store_result_error:%s", mysql_error_with_if(conn_ptr));
			}
			return -1;
		} else {
			//拷贝数据
			mysql_fill_data(res_ptr, result_data_ptr);

		}
		//释放结果集
		mysql_free_result(res_ptr);
		return 0;

	} else {
		LOG(LOG_ERROR, "mysql_select error:%s", mysql_error_with_if(conn_ptr));
		return -1;
	}
}


//释放result_data_t *result_data_ptr
void free_result_data(result_data_t *result_data_ptr)
{
	mysql_field_value_t **data = result_data_ptr->data;
	mysql_field_value_t *head, *current, *prev;

	int i = 0, j = 0;
	for (i = 0; i < result_data_ptr->rows; i++) {
		prev = head = *(data + i);
		for (j = 0; j < result_data_ptr->columns; j++) {
			current = prev->next;
			free(prev);

			free(current->fieldName);
			free(current->fieldValue);

			prev = current;
		}
		free(prev);
	}

	free(data);
}



//查询数据
static void mysql_fill_data(MYSQL_RES *res_ptr, result_data_t *result_data_ptr)
{
	//行数
	int rowsNum = mysql_num_rows(res_ptr);
	//列数
	int fieldsNum = mysql_num_fields(res_ptr);
	//字段名称
	MYSQL_FIELD *fieldNames = mysql_fetch_fields(res_ptr);
	//字段值长度
	unsigned long *fieldValueLengths;

	int i = 0, j;

	MYSQL_ROW row;
	result_data_ptr->data = calloc(rowsNum, sizeof(mysql_field_value_t *));
	mysql_field_value_t *head, *current, *prev;


	while ((row = mysql_fetch_row(res_ptr))) {
		fieldValueLengths = mysql_fetch_lengths(res_ptr);
		prev = head = calloc(1, sizeof(mysql_field_value_t));

		for (j = 0; j < fieldsNum; j++) {
			current =  calloc(1, sizeof(mysql_field_value_t));

			prev->next = current;
			prev = current;

			//字段名称
			current->fieldName = calloc(1, strlen(fieldNames[j].name) + 1);
			strcpy(current->fieldName, fieldNames[j].name);

			//非空值  NULL
			if (fieldValueLengths[j]) {
				//字段值
				current->fieldValue = calloc(1, fieldValueLengths[j] + 1);
				strcpy(current->fieldValue, row[j]);
			}
		}
		*(result_data_ptr->data + i) = head;
		i++;
	}

	result_data_ptr->rows = rowsNum;
	result_data_ptr->columns = fieldsNum;
}

/*
 *  mysql 数据集 result_data_t 转换为json输出
 *  @see https://github.com/kbranigan/cJSON   可以看看test文件夹中的测试用例
 *  @see http://cjson.sourceforge.net/
 *	@see my-detectserver-c/test/cJSON.test.c
 *	@return  char *  json字符串  使用完之后  记得  free(char *)
 */
char * mysql_result_data_convert_json(result_data_t *result_data_ptr)
{
	int i, j;
	cJSON *root, *fld;
	mysql_field_value_t *pointer;

	char *json_str;


	root = cJSON_CreateArray();
	for (i = 0; i < result_data_ptr->rows; i++) {
		cJSON_AddItemToArray(root, fld = cJSON_CreateObject());
		pointer = *(result_data_ptr->data + i);
		for (j = 0; j < result_data_ptr->columns; j++) {
			j == 0 ? cJSON_AddNumberToObject(fld, pointer->next->fieldName, atof(pointer->next->fieldValue))
				   : cJSON_AddStringToObject(fld, pointer->next->fieldName, pointer->next->fieldValue);
			pointer = pointer->next;
		}
	}
	json_str = cJSON_Print(root);
	cJSON_Delete(root);

	return json_str;
}

