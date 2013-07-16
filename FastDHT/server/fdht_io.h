/**
* Copyright (C) 2008 Happy Fish / YuQing
*
* FastDFS may be copied only under the terms of the GNU General
* Public License V3, which may be found in the FastDFS source kit.
* Please visit the FastDFS Home Page http://www.csource.org/ for more detail.
**/

//fdht_io.h

#ifndef _FDHT_IO_H
#define _FDHT_IO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <event.h>
#include "fdht_define.h"
#include "task_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

void recv_notify_read(int sock, short event, void *arg);
int send_add_event(struct task_info *pTask);

#ifdef __cplusplus
}
#endif

#endif

