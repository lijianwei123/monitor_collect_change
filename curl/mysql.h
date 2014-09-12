/*
 * mysql.h
 *
 *  Created on: 2014-8-29
 *  Author: lijianwei
 */

#ifndef MYSQL_H_
#define MYSQL_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include <mysql/mysql.h>
#include "log.h"
#include "helper.h"
#include "../cJSON.h"



typedef struct{
	char *host;
	unsigned int port;
	char *user;
	char *pwd;
	char *db;
	char *unix_socket;
	unsigned long client_flag;
} mysql_connect_info_t;

typedef struct mysql_field_value{
	//字段
	char *fieldName;
	char *fieldValue;
	struct mysql_field_value *next;
} mysql_field_value_t;


typedef struct {
	mysql_field_value_t **data;
	unsigned int rows;
	unsigned short columns;
} result_data_t;

//链接数据库
int mysql_connect(MYSQL *conn_ptr, mysql_connect_info_t *mysql_connect_info_ptr);

//执行写操作
int mysql_execute(MYSQL *conn_ptr, char *sql, unsigned long *result);

//执行读操作
int mysql_select(MYSQL *conn_ptr, char *sql, result_data_t *result_data_ptr);

//释放资源
void free_result_data(result_data_t *result_data_ptr);

//初始连接信息
inline void  mysql_connection_info_init(mysql_connect_info_t *mysql_connection_info_ptr);

//mysql 数据集 result_data_t 转换为json输出
char * mysql_result_data_convert_json(result_data_t *result_data_ptr);

//cap_mysql_escape_string
char *cap_mysql_escape_string(MYSQL *conn_ptr, char *from, unsigned long length);

#endif /* MYSQL_H_ */
