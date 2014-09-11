/*
 * test_http.c
 *
 *  Created on: 2014-9-1
 *  Author: Administrator
 *
 *  gcc -g -Wall -fPIC -Wl,-rpath,/usr/local/libevent-2.0.21-stable/lib -L/usr/local/libevent-2.0.21-stable/lib -levent  -I/usr/local/libevent-2.0.21-stable/include -lcurl -lm test_http.c ../curl_http.c -o test_http
 */

#include "../curl_http.h"

char *docroot = "/home/test/lijianwei/bigfileupload/curl/test";

int main(int argc, char **argv)
{
	http_server("0.0.0.0", 9876);


	return 0;
}

