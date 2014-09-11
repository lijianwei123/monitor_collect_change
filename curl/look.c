/**
 * 监控数据采集元素变化
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>

#include "helper.h"
#include "mysql.h"
#include "curl_http.h"

void show_help()
{
	char *b = "--------------------------------------------------------------------------------------------------\n"
			  "Monitoring collection element changes"
			  "\n\n"
			   "-c 	<config_file>  config file name\n"
			   "-h	print this help and exit\n"
			   "\n\n"
			   "Please visit \"https://github.com/lijianwei123\" for more help information.\n\n"
			   "--------------------------------------------------------------------------------------------------\n";
		fprintf(stderr, b, strlen(b));
}

void freeMem()
{
	free(mysql_connect_info.host);
	free(mysql_connect_info.user);
	free(mysql_connect_info.pwd);
	free(mysql_connect_info.db);

	free_request_cb_list(request_cb_complex_head);
}

void signalHandler(int signalNo)
{
	LOG(LOG_INFO, "signal no:%d", signalNo);


	freeMem();

	exit(0);
}


//信号处理
void registerSignal()
{
	//ctrl + c
	signal(SIGINT, signalHandler);

	//kill pkill  要求程序正常退出  本信号能被阻塞、处理和忽略
	signal(SIGTERM, signalHandler);

	//SIGQUIT  ctrl + \ 默认的处理方式是直接退出    会产生 core dumped 文件
	signal(SIGQUIT, SIG_IGN);

	//kill -9  本信号不能被阻塞、处理和忽略
	//signal(SIGKILL, signalHandler);

	//段错误
	signal(SIGSEGV, signalHandler);
}

//数据库连接信息
mysql_connect_info_t mysql_connect_info;


int main(int argc, char *argv[])
{
	//配置文件
	const char *config_file_name;

	//初始化连接信息
	mysql_connection_info_init(&mysql_connect_info);

	//daemon
	int daemon = 0;

	//http
	char *ip = NULL;
	int port = 0;


	//获取选项
	int ch;
	opterr = 0;

	if (argc < 2) {
		show_help();
		exit(EXIT_FAILURE);
	}

	while((ch = getopt(argc, argv, "c:h")) != -1) {
		switch(ch) {
			case 'c':
				config_file_name = optarg;
			break;

			case 'h':
				show_help();
				exit(EXIT_SUCCESS);
			break;

			case '?':
			default:
				show_help();
				exit(EXIT_SUCCESS);
			break;
		}
	}

	if (access(config_file_name, F_OK) < 0) {
		LOG(LOG_ERROR, "%s don't exist!", config_file_name);
		exit(EXIT_FAILURE);
	}

	dictionary  *ini_ptr =  parse_ini_file(config_file_name);

	if (ini_ptr == NULL) {
		LOG(LOG_ERROR, "%s", "parse_ini_file error");
		exit(EXIT_FAILURE);
	}

	daemon = iniparser_getboolean(ini_ptr, "system:daemon", 0);

	mysql_connect_info.host = strdup(iniparser_getstring(ini_ptr, "mysql:host", "localhost"));
	mysql_connect_info.user = strdup(iniparser_getstring(ini_ptr, "mysql:user", "root"));
	mysql_connect_info.pwd = strdup(iniparser_getstring(ini_ptr, "mysql:pwd", "root"));
	mysql_connect_info.db = strdup(iniparser_getstring(ini_ptr, "mysql:db", "test"));

	ip = strdup(iniparser_getstring(ini_ptr, "http:ip", "127.0.0.1"));
	port = iniparser_getint(ini_ptr, "http:port", 8000);

	iniparser_freedict(ini_ptr);

	if (daemon) {
		setDaemon();
		LOG(LOG_INFO, "%s", "daemon start");
	}

	//http 服务启动
	setup_http_server(ip, port);
	free(ip);

	add_request_cb(collect_list_request_cb, "/collect/list");
	add_request_cb(collect_del_request_cb, "/collect/del");
	add_request_cb(collect_match_request_cb, "/collect/match");
	add_request_cb(collect_insert_request_cb, "/collect/insert");
	add_request_cb(collect_modify_request_cb, "/collect/modify");


	start_http_server();

	freeMem();

	return 0;
}

