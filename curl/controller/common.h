/*
 * common.h
 *
 *  Created on: 2014-9-9
 *      Author: Administrator
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/keyvalq_struct.h>
#include <event2/http_struct.h>
#include <event2/util.h>

#include "../mysql.h"
#include "../curl_http.h"


extern mysql_connect_info_t mysql_connect_info;

#endif /* COMMON_H_ */
