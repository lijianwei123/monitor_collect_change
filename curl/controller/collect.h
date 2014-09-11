/*
 * 	采集
 *
 *  Created on: 2014-9-7
 *  Author: lijianwei
 */

#ifndef COLLECT_H_
#define COLLECT_H_

#include "common.h"

//列表
int collect_list_request_cb(struct evhttp_request *, void *, struct evbuffer *);

//删除
int collect_del_request_cb(struct evhttp_request *, void *, struct evbuffer *);

//添加
int collect_insert_request_cb(struct evhttp_request *req, void *arg, struct evbuffer *buf);

//修改
int collect_modify_request_cb(struct evhttp_request *req, void *arg, struct evbuffer *buf);

//测试匹配
int collect_match_request_cb(struct evhttp_request *req, void *arg, struct evbuffer *buf);

//批量匹配  定时任务
int collect_batch_match(void);

#endif /* COLLECT_H_ */
