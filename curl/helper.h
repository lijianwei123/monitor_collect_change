/*
 *  helper.h
 *
 *  Created on: 2014-8-26
 *  Author: lijianwei
 */

#ifndef HELPER_H_
#define HELPER_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <curl/curl.h>
#include <regex.h>

#include <time.h>

#include "log.h"
#include "ini/iniparser.h"
#include "../cJSON.h"


#define CURL_TIMEOUT 10

//计算数组元素个数   注意array不能是个指针
#define array_count(array) (sizeof(array)/sizeof(array[0]))


#define get_content_len_by_header(url) get_content_len(url, (setExtraCurlOpt)NULL, 0, (content_info_t *)NULL)


//设置curl选项
typedef void (*setExtraCurlOpt)(CURL *curl);


typedef struct {
	char *contents;
	double contentLen;
} content_info_t;


//获取网页content length
long get_content_len(const char *url, ...);

//获取host
int getHostByUrl(const char *url, char *host, size_t hostLen);

int preg_match(const char *pattern, const char *subject, regmatch_t *pm_ptr, int nmatch);

//rtrim
char *strtrimr(char *pstr);

//ltrim
char *strtriml(char *pstr);

//trim
char *strtrim(char *pstr);

//小写
char *strtolower(char *str);

//大写
char *strtoupper(char *str);


//如果找到返回偏移量   没有返回  -1
int strpos(const char *haystack, const char *needle);

//返回形如  2014-09-02 19:12:25 当前时间
int getDate(char *date);

//解析ini
void *parse_ini_file(const char *ini_file_name);

//成功json
char * sucOutputByJson(void);

//错误json
char * errOutputByJson(const char *msg);

//设置daemon
void setDaemon();


#endif /* HELPER_H_ */
