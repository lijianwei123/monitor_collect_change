/**
 * 监控数据采集元素变化
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>

#include "helper.h"
#include "mysql.h"
#include "curl_http.h"


//数据库连接信息
mysql_connect_info_t mysql_connect_info;
//子进程
pid_t batch_match_pid = 0;

int block = 1;

//当前根路径
char *docroot = NULL;


struct itimerval *ovalue = NULL;

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

	free(docroot);

	free_request_cb_list(request_cb_complex_head);
}

//处理普通信号
void signalHandler(int signalNo)
{
	LOG(LOG_INFO, "signal no:%d", signalNo);

	//干死子进程
	if (batch_match_pid > 0) {
		//发送信号
		kill(batch_match_pid, SIGUSR1);
		batch_match_pid = 0;
	}


	freeMem();

	exit(0);
}

//处理chld 信号
void signalChldHandler(int signalNo)
{
	LOG(LOG_INFO, "signal no:%d", signalNo);

	pid_t chldPid = 0;
	int status = 0;
	int exitCode;
	int flag;

	chldPid = waitpid(-1, &status, WNOHANG | WUNTRACED);

	if (chldPid < 0) {
		LOG(LOG_ERROR, "waitpid error: %s", strerror(errno));
	} else {
		//非正常结束  @see http://net.pku.edu.cn/~yhf/linux_c/

		printf("WIFEXITED:%d, WEXITSTATUS: %d, WIFSIGNALED: %d\n", WIFEXITED(status), WEXITSTATUS(status), WIFSIGNALED(status));

		if (!WIFEXITED(status)) {
			//返回exit状态码
			exitCode = WEXITSTATUS(status);
			//是否因为信号结束的
			flag = WIFSIGNALED(status);
		}
	}
}

//处理子进程接到的sigusr1信号
void childSigusr1Hanlder(int signalNo)
{
	LOG(LOG_INFO, "childSigusr1Hanlder sinal no:%d", signalNo);
	//取消定时任务
	block = 0;

	setitimer(ITIMER_PROF, ovalue, NULL);
}

//处理子进程 ITIMER_PROF信号
void childBatchMatchTimerHandler(int signalNo)
{
	if (!collect_batch_match()) {
		LOG(LOG_INFO, "%s", "success");
	} else {
		LOG(LOG_ERROR, "%s", "error");
	}
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
	//signal(SIGSEGV, signalHandler);
}

//开启子进程  批量match
void do_batch_match(int batch_match_interval)
{
	batch_match_pid = fork();

	if (batch_match_pid < 0) {
		LOG(LOG_ERROR, "%s", "fork error");
	} else if (batch_match_pid > 0) {
		//parent
		signal(SIGCHLD, signalChldHandler);

	} else if (batch_match_pid == 0) {
		//child

		struct sigaction act;
		memset(&act, 0, sizeof(act));

		act.sa_handler = childSigusr1Hanlder;
		//act.sa_mask  用来设置在处理该信号时暂时将sa_mask 指定的信号搁置
		sigfillset(&act.sa_mask);
		act.sa_flags |= SA_RESTART; //被信号中断的系统调用会自行重启

		if (sigaction(SIGUSR1, &act, 0) < 0) {
			LOG(LOG_ERROR, "sigaction error: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}

		//定时任务
		struct itimerval timer;
		timer.it_value.tv_sec = 3;
		timer.it_value.tv_usec = 0;
		timer.it_interval.tv_sec = batch_match_interval * 60;
		timer.it_interval.tv_usec = 0;

		signal(SIGPROF, childBatchMatchTimerHandler);
		setitimer(ITIMER_PROF, &timer, ovalue);

		while(block);
	}

}


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

	//batch_match
	int batch_match = 0;
	unsigned int batch_match_interval = 0;

	//log file
	char log_file[200] = {0};


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

	docroot = getcwd(NULL, 0);
#ifdef DEBUG
	LOG(LOG_DEBUG, "docroot:%s", docroot);
#endif

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
	batch_match = iniparser_getboolean(ini_ptr, "system:batch_match", 0);
	batch_match_interval = iniparser_getint(ini_ptr, "system:batch_match_interval", 0);
	strcpy(log_file, iniparser_getstring(ini_ptr, "system:log_file", ""));

	mysql_connect_info.host = strdup(iniparser_getstring(ini_ptr, "mysql:host", "localhost"));
	mysql_connect_info.user = strdup(iniparser_getstring(ini_ptr, "mysql:user", "root"));
	mysql_connect_info.pwd = strdup(iniparser_getstring(ini_ptr, "mysql:pwd", "root"));
	mysql_connect_info.db = strdup(iniparser_getstring(ini_ptr, "mysql:db", "test"));

	ip = strdup(iniparser_getstring(ini_ptr, "http:ip", "127.0.0.1"));
	port = iniparser_getint(ini_ptr, "http:port", 8000);

	iniparser_freedict(ini_ptr);

	if (daemon) {
		setDaemon(log_file);
		LOG(LOG_INFO, "%s", "daemon start");
	}

	//信号处理
	registerSignal();

	//batch_match
	if (batch_match) {
		do_batch_match(batch_match_interval);
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

