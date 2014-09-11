/**
 * 测试helper.c
 * gcc -g -Wall -lcurl -lm test_helper.c ../helper.c -o test_helper
 */
#include <assert.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include "../helper.h"

void test1()
{
	//getHostByUrl
	const char *url = "http://blog.sina.com.cn/s/blog_66d760810100sfml.html";
	char host[50] = {0};
	getHostByUrl(url, host, sizeof(host));
	assert(!strcmp(host, "blog.sina.com.cn"));
}

void test2()
{
	//get_content_len
	const char *url2 = "http://www.veryeast.cn";
	long contentLen = 0;
	content_info_t content_info;
	FILE *fp;
	fp = fopen("test.txt", "w");

	contentLen = get_content_len(url2, (setExtraCurlOpt)NULL, 1, &content_info);

	printf("contentLen: %ld\n", contentLen);

	printf("content info length:%f\n", content_info.contentLen);


	printf("contents len:%zd\n", strlen(content_info.contents));

	fputs(content_info.contents, fp);
	fclose(fp);
	free(content_info.contents);
}

void test3()
{
	//通过content length获取长度
	long imgLen = 0L;
	const char *imgUrl = "http://static.v.veimg.cn/image/MD.gif";
	imgLen = get_content_len_by_header(imgUrl);
	printf("img len:%ld\n", imgLen);
}


void test4()
{
	//preg_match
	const char *pattern = "<h1>\\s*?<i\\s*?id=\"hotelGrade\"\\s*?class=\"\"></i>\\s*?(\\S+)\\s*?</h1>";
	regmatch_t pm[2];
	char hotelName[20] = {0};
	int flag = 0;

	const char *url3 = "http://hotel.elong.com/beihai/02103235/";
	long dataLen = 0L;
	content_info_t dataInfo;
	dataLen = get_content_len(url3, (setExtraCurlOpt)NULL, 1, &dataInfo);

	flag = preg_match(pattern, dataInfo.contents, pm, array_count(pm));
	printf("preg_match:%d\n", flag);

	if (flag > 0) {
		strncpy(hotelName, dataInfo.contents + pm[1].rm_so, pm[1].rm_eo - pm[1].rm_so);
		printf("hotelName:%s\n", hotelName);
	}
	free(dataInfo.contents);
}


void test5()
{
	char p[30] = "   liJIanWei   ";
	char *s;
	s = p;


	printf(",ltrim:%s,\n", strtriml(s));
	printf(",rtrim:%s,\n", strtrimr(s));
	printf(",trim:%s,\n", strtrim(s));

	printf("strtolower:%s\n", strtolower(s));
	printf("strtoupper:%s\n", strtoupper(s));
}

//大小写
void test6()
{
	const char a[20] = "lijianwei";
	const char b[10] = "wei";
	const char c[10] = "li";
	const char d[10] = "weiyanping";
	const char e[10] = "l";

	printf("%d\n", strpos(a, b));
	printf("%d\n", strpos(a, c));
	printf("%d\n", strpos(a, d));
	printf("%d\n", strpos(a, e));


	char ff[30] = " liJIanwei ";
	printf("%s\n", strtoupper(strtrim(ff)));


}

void test7()
{
	//获取本机ip地址
	char buff[20];
	struct hostent * hostaddr;
	struct in_addr addr;

	gethostname(buff, sizeof(buff));  //获取本机名
	printf("hostname=%s\n", buff);


	hostaddr=gethostbyname(buff);   //获取本机ip地址
	memcpy(&addr, hostaddr->h_addr_list[0], hostaddr->h_length);

	fprintf(stderr, "local ip addres=%s\n", inet_ntoa(addr));

}

//测试获取host->ip
void test8()
{
	//获取host->ip
	struct hostent *he;
	struct in_addr addr;

	he = gethostbyname("www.baidu.com");
	assert(he != NULL);

	memcpy(&addr, he->h_addr_list[0], he->h_length);

	printf("addr:%s\n", inet_ntoa(addr));
}

//测试时间
void test9()
{
	char p[20] = {0};
	if (getDate(p) != -1)
		printf("now time:%s\n", p);
}

//测试ini
//静态库   gcc -g -Wall -fPIC  test_helper.c  ../helper.c -L../lib -Wl,-dn -liniparser  -Wl,-dy -lcurl -lm  -o test_helper
void test10()
{
	int daemon = 0;
	char *host;

	const char *ini_file_name = "../config.ini";
	dictionary  *ini_ptr =  parse_ini_file(ini_file_name);

	if (ini_ptr == NULL) {
		exit(-1);
	}

	daemon = iniparser_getboolean(ini_ptr, "system:daemon", 0);
	host = iniparser_getstring(ini_ptr, "mysql:host", NULL);

	printf("daemon:%d\n", daemon);
	printf("host:%s\n", host);

	iniparser_freedict(ini_ptr);
}

int main()
{

	//test1();
	//test2();
	//test3();
	//test4();
	//test5();
	//test6();
	//test7();
	//test8();
	//test9();
	test10();


	return 0;
}
