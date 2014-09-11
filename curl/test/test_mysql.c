/*
 * test_mysql.c
 *
 *  Created on: 2014-9-2
 *  Author: Administrator
 *
 *  makefile: gcc -g -Wall -Wl,-rpath,../lib test_mysql.c ../mysql.c ../helper.c ../../cJSON.c -L/usr/lib/mysql -lmysqlclient -L../lib -liniparser -lcurl -lm -o test_mysql -DDEBUG
 *  检测内存泄露：valgrind --tool=memcheck --leak-check=full ./test_mysql
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "../mysql.h"
#include "../helper.h"


void test1()
{
	mysql_connect_info_t  mysql_connect_info;
	MYSQL conn;
	int retCode = -1;

	//测试db连接
	mysql_connection_info_init(&mysql_connect_info);
	mysql_connect_info.host = "168.192.122.30";
	mysql_connect_info.user = "efoncheng";
	mysql_connect_info.pwd = "efoncheng";


	retCode = mysql_connect(&conn, &mysql_connect_info);

	assert(retCode == 0);


	retCode = mysql_select_db(&conn, "df_mixed");

	assert(retCode == 0);

	unsigned long result = 0l;

	//测试创建表
	/*
	char create_table_sql[] = "CREATE TABLE `df_monitor_collection` ("
			  "`id` int(11) unsigned NOT NULL AUTO_INCREMENT,"
			  "`url` varchar(200) NOT NULL,"
			  "`pattern` varchar(200) NOT NULL,"
			  "`add_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,"
			  "`modify_time` datetime NOT NULL,"
			  "PRIMARY KEY (`id`)"
			  ") ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=utf8 COMMENT='监控采集页面变动'";




	retCode = mysql_execute(&conn, create_table_sql, &result);
	assert(retCode == 0);
	printf("result:%lu\n", result);
	*/


	//测试插入数据
	/*
	const char insert_template_sql[] = "INSERT INTO `df_monitor_collection` (`url`, `pattern`, `modify_time`) "
				"VALUES ('http://www.veryeast.cn', '<div\\s*?class=\\\"autocomplete-wrap', '%s')";

	char date[20] = {0};
	getDate(date);

	char *insert_sql = calloc(strlen(insert_template_sql) + strlen(date) + 1, 1);
	sprintf(insert_sql, insert_template_sql, date);
	printf("insert_sql:%s\n", insert_sql);

	retCode = -1;
	retCode = mysql_execute(&conn, insert_sql, &result);
	assert(retCode == 0);
	printf("result:%lu\n", result);
	*/


	//查询
	result_data_t result_data;
	char select_sql[] = "select * from df_monitor_collection";
	retCode = mysql_select(&conn, select_sql, &result_data);
	printf("rows:%d, cols:%d\n", result_data.rows, result_data.columns);

	int i, j;
	//输出列
	mysql_field_value_t **data_ptr = result_data.data;
	mysql_field_value_t *prev = *data_ptr;

	for (j = 0; j < result_data.columns; j ++) {
		printf("%-30s", prev->next->fieldName);
		prev = prev->next;
	}
	printf("\n");

	//输出数据
	for (i = 0; i < result_data.rows; i++) {
		prev = *(data_ptr + i);
		for (j = 0; j < result_data.columns; j++) {
			printf("%-30s", prev->next->fieldValue);
			prev = prev->next;
		}
		printf("\n");
	}

	//测试下转换为json
	char *json_str;
	json_str =	mysql_result_data_convert_json(&result_data);
	printf("json_str:%s\n", json_str);
	free(json_str);

	free_result_data(&result_data);

	mysql_close(&conn);
}


int main(int argc, char **argv)
{
	test1();

	return 0;
}
