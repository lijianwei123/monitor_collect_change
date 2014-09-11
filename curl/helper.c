#include "helper.h"

//写header
static size_t get_content_len_write_header_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
#ifdef DEBUG
	printf("%s\n", "get_content_len_write_header_data");
#endif
	return fwrite(ptr, size, nmemb, (FILE *)stream);
}


static size_t get_content_len_write_body_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
	content_info_t *content_info_ptr = (content_info_t *)stream;
	size_t realSize = size * nmemb;

#ifdef DEBUG
	printf("content_info_ptr contentLen:%f\n", content_info_ptr->contentLen);
#endif

	content_info_ptr->contents = realloc(content_info_ptr->contents, content_info_ptr->contentLen + realSize + 1);
	if (content_info_ptr->contents == NULL) {
		LOG(LOG_ERROR, "%s", "calloc error");
		return 0;
	}

	memcpy(&(content_info_ptr->contents[(long)content_info_ptr->contentLen]), ptr, realSize);

	content_info_ptr->contentLen += realSize;
	content_info_ptr->contents[(long)content_info_ptr->contentLen] = 0;

#ifdef DEBUG
	printf("content_info_ptr contentLen:%f\n", content_info_ptr->contentLen);
#endif

	return realSize;
}

/**
 * @see curl http://curl.haxx.se/libcurl/c/curl_easy_setopt.html
 * @desc  获取content-length  因为使用的变长参数  注意传参顺序
 * @param setExtraCurlOpt setOptCallback  设置一些特殊选项
 * @param int by_data  0   通过head请求获取content-length 可能返回0或者不准确的数字    1通过真实获取数据获取
 * @param content_info_t *content_info_ptr  获取的网页内容、长度  内容 分配在堆上，注意free
 * @return  long
 */
long get_content_len(const char *url, ...)
{
	CURL *curl;
	CURLcode res;
	struct curl_slist *headers = NULL;
	char host[200] = {0};
	int status = 0;
	char headerBuffer[256] = {0};
	double contentLen = 0.0;
	FILE *fp = NULL;



	//获取变长参数
	int by_data = 0;
	setExtraCurlOpt setOptCallback = NULL;
	content_info_t *content_info_ptr = NULL;


	va_list args;
	va_start(args, url);
	setOptCallback = va_arg(args, setExtraCurlOpt);
	by_data = va_arg(args, int);

#ifdef DEBUG
	printf("by_data:%d\n", by_data);
#endif

	content_info_ptr = va_arg(args, content_info_t *);
	va_end(args);

	curl = curl_easy_init();
	if (curl != NULL) {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);

		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		//redirect nums
		curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 3L);

		curl_easy_setopt(curl, CURLOPT_TIMEOUT, CURL_TIMEOUT);

		if (by_data == 0) {
			//通过head
			//curl_easy_setopt(curl, CURLOPT_HEADER, 1L);  如果设置为0  表示body中要包含header 但是又不输出body
			curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);


			curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, get_content_len_write_header_data);
			fp = fopen("/dev/null", "w");
			if (fp == NULL) {
				perror("fopen error");
				return 0;
			}
			curl_easy_setopt(curl, CURLOPT_HEADERDATA, fp);

		} else if (by_data == 1) {
			//获取body  CURLOPT_HEADER 默认就是0 可以不设置
			curl_easy_setopt(curl, CURLOPT_HEADER, 0L);

			//通过get data获取content len
			memset(content_info_ptr, 0, sizeof(content_info_t));
			content_info_ptr->contents = calloc(1, sizeof(char));


			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, get_content_len_write_body_data);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, content_info_ptr);
		}

		//set header
		headers = curl_slist_append(headers, "User-Agent:Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/36.0.1985.143 Safari/537.36");
		status = getHostByUrl(url, host, sizeof(host));
#ifdef DEBUG
	printf("host:%s, status:%d\n", host, status);
#endif
		if (status == 0) {
			sprintf(headerBuffer, "%s%s", "Host: ", host);
			headers = curl_slist_append(headers, headerBuffer);
		}
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		//设置特殊选项
		if (setOptCallback != NULL)
			(*setOptCallback)(curl);

		res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			LOG(LOG_ERROR, "%s", curl_easy_strerror(res));
			return 0;
		}

		long retcode = 0L;
		res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE , &retcode);
#ifdef DEBUG
		printf("curl response code:%ld\n", retcode);
#endif
		if (res == CURLE_OK && retcode == 200) {

			if (by_data == 0) {
				//获取header中的content-length  很多是没有返回content-length  很多是不准确的content-length
				res = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &contentLen);
#ifdef DEBUG
		printf("curl content length:%f\n", contentLen);
#endif
				if (res != CURLE_OK || contentLen == -1) {
					return 0;
				}
			} else {
				contentLen = content_info_ptr->contentLen;
			}
		}
	}
	if (fp != NULL) fclose(fp);
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);

	return contentLen == 0 ? 0 : (long)ceil(contentLen);
}

//获取host
int getHostByUrl(const char *url, char *host, size_t hostLen)
{
	if (strlen(url) <= 0) { return -1;}

	//通过正则获取host
	const char pattern[] = "^(\\w+):\\/\\/([^/:]+)(:\\d*)?([^# ]*)";

	regex_t reg;
	regmatch_t pm[5];
	int z = 0, nmatch = 5;
	char ebuf[128] = {0};

	memset(&reg, 0, sizeof(reg));
	memset(pm, 0, sizeof(pm));

	z = regcomp(&reg, pattern, REG_EXTENDED|REG_ICASE|REG_NEWLINE);
	if (z != 0) {
		regerror(z, &reg, ebuf, sizeof(ebuf));
		fprintf(stderr, "pattern compile error:%s\n", ebuf);
		return -1;
	}

	z = regexec(&reg, url, nmatch, pm, 0);
	if (z == REG_NOMATCH) {return -1;}

	if (z != 0) {
		regerror(z, &reg, ebuf, sizeof(ebuf));
		fprintf(stderr, "regexec fail:%s\n", ebuf);
		return -1;
	}

	memset(host, 0, hostLen);
	strncpy(host,  url + pm[2].rm_so, pm[2].rm_eo - pm[2].rm_so);

	regfree(&reg);
	return 0;
}

/**
 * 	@see http://man7.org/linux/man-pages/man3/regexec.3.html
 * 	@see http://blog.chinaunix.net/uid-24733806-id-3846957.html
 *	@desc 正则匹配  1 表示匹配成功   0表示匹配失败
 *	@param  const char * pattern 正则表达式  不需要带  /
 *	@param  const char * subject 内容
 *	@param  regmatch_t * pm_ptr 匹配位置  rm_so 开始位置   rm_eo 结束位置
 *	@param  nmatch pm_ptr    元素个数    或者  子匹配和全匹配个数
 *	@return 1  表示匹配成功    0 表示未匹配到
 */
int preg_match(const char *pattern, const char *subject, regmatch_t *pm_ptr, int nmatch)
{
	regex_t reg;
	int z = 0;
	char ebuf[128] = {0};

	memset(&reg, 0, sizeof(regex_t));
	memset(pm_ptr, 0, sizeof(regmatch_t) * nmatch);

	z = regcomp(&reg, pattern, REG_EXTENDED|REG_ICASE|REG_NEWLINE);
	if (z != 0) {
		regerror(z, &reg, ebuf, sizeof(ebuf));
		LOG(LOG_ERROR, "pattern compile error:%s", ebuf);
		return 0;
	}

	z = regexec(&reg, subject, nmatch, pm_ptr, 0);
	if (z == REG_NOMATCH) {
#ifdef DEBUG
		printf("%s: reg_nomatch\n", pattern);
#endif
		return 0;
	}

	if (z != 0) {
		regerror(z, &reg, ebuf, sizeof(ebuf));
		LOG(LOG_ERROR, "pattern compile error:%s", ebuf);
		return 0;
	}

	regfree(&reg);
	return 1;
}

//rtrim
char *strtrimr(char *pstr)
{
	int i;
	i = strlen(pstr) - 1;
	while((i >=0) && isspace(pstr[i])){
		pstr[i--] = '\0';
	}
	return pstr;
}

//ltrim
char *strtriml(char *pstr)
{
	int i=0,j;
	j = strlen(pstr) - 1;
	while(isspace(pstr[i]) && (i <= j))
		i++;
	if(0 < i)
		strcpy(pstr, &pstr[i]);
	return pstr;
}

//trim
char *strtrim(char *pstr)
{
	char *p;
	p = strtrimr(pstr);
	return strtriml(p);
}

//小写
char *strtolower(char *str)
{
	int i = 0;
	do{
		str[i] = tolower(str[i]);
	}while(str[++i] != '\0');

	return str;
}

//大写
char *strtoupper(char *str)
{
	int i = 0;
	do{
		str[i] = toupper(str[i]);
	}while(str[++i] != '\0');

	return str;
}

/**
 *	查找字符串     >=0  or -1
 *	@param const char *haystack 待查找字符串
 *	@param const char *needle 搜索字符串
 */
int strpos(const char *haystack, const char *needle)
{
	char *p = strstr(haystack, needle);
	return p == NULL ? -1 : p - haystack;
}

//返回形如  2014-09-02 19:12:25 当前时间    -1 表示失败  其他成功
int getDate(char *date)
{
	time_t timep;
	struct tm *p;

	//取得当前时间
	time(&timep);

	p = localtime(&timep);
	return sprintf(date, "%d-%02d-%02d %02d:%02d:%02d", (1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
}

/*
 * 解析ini  注意使用  iniparser_freedict(ini_ptr) 释放内存
 * @param   const char *ini_file_name ini文件
 * @return  dictionary  *  or null
 */
void *parse_ini_file(const char *ini_file_name)
{
	dictionary  *ini_ptr = NULL;
	ini_ptr = iniparser_load(ini_file_name);
	if (ini_ptr == NULL) {
		LOG(LOG_ERROR, "parse ini file %s error", ini_file_name);
	}

	return ini_ptr;
}

/*
 * 成功json
 * @return char * 注意使用free释放内存
 */
char * sucOutputByJson(void)
{
	cJSON *root;
	char *json_str;

	root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "status", 1);
	json_str = cJSON_Print(root);
	cJSON_Delete(root);

	return json_str;
}

/*
 * 错误json
 * @return char * 注意使用free释放内存
 */
char * errOutputByJson(const char *msg)
{
	cJSON *root;
	char *json_str;

	root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "status", 0);
	cJSON_AddStringToObject(root, "errMsg", msg);
	json_str = cJSON_Print(root);
	cJSON_Delete(root);

	return json_str;
}

/**
 * 设置daemon
 * 也可以系统自带的daemon函数    @see http://man7.org/linux/man-pages/man3/daemon.3.html
 * daemon(0, 0);
 */

void setDaemon()
{
	pid_t pid;

	pid = fork();
	if (pid < 0) {
		LOG(LOG_ERROR, "%s", "fork error");
		exit(EXIT_FAILURE);
	}
	if (pid > 0) {
			exit(EXIT_SUCCESS);
	}
	//子进程
	if (setsid() == -1) {
		LOG(LOG_ERROR, "setsid error:%s", strerror(errno));
		exit(EXIT_FAILURE);
	}
	chdir("/");
	int null_fd = open("/dev/null", O_RDONLY|O_WRONLY);
	dup2(STDIN_FILENO, null_fd);
	dup2(STDOUT_FILENO, null_fd);
	dup2(STDERR_FILENO, null_fd);
	close(null_fd);
}

