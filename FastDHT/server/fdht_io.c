/**
* Copyright (C) 2008 Happy Fish / YuQing
*
* FastDFS may be copied only under the terms of the GNU General
* Public License V3, which may be found in the FastDFS source kit.
* Please visit the FastDFS Home Page http://www.csource.org/ for more detail.
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <event.h>
#include "shared_func.h"
#include "logger.h"
#include "global.h"
#include "ini_file_reader.h"
#include "sockopt.h"
#include "fdht_global.h"
#include "fdht_proto.h"
#include "task_queue.h"
#include "work_thread.h"
#include "fdht_io.h"

static void client_sock_read(int sock, short event, void *arg);
static void client_sock_write(int sock, short event, void *arg);

void recv_notify_read(int sock, short event, void *arg)
{
	int bytes;
	int incomesock;
	int result;
	struct thread_data *pThreadData;
	struct task_info *pTask;
	char szClientIp[IP_ADDRESS_SIZE];
	in_addr_t client_addr;

	while (1)
	{
		if ((bytes=read(sock, &incomesock, sizeof(incomesock))) < 0)
		{
			if (!(errno == EAGAIN || errno == EWOULDBLOCK))
			{
				logError("file: "__FILE__", line: %d, " \
					"call read failed, " \
					"errno: %d, error info: %s", \
					__LINE__, errno, STRERROR(errno));
			}

			break;
		}
		else if (bytes == 0)
		{
			break;
		}

		if (incomesock < 0)
		{
			struct timeval tv;
                        tv.tv_sec = 1;
                        tv.tv_usec = 0;
			pThreadData = g_thread_data + (-1 * incomesock - 1) % \
					g_max_threads;
			event_base_loopexit(pThreadData->ev_base, &tv);
			return;
		}

		client_addr = getPeerIpaddr(incomesock, \
				szClientIp, IP_ADDRESS_SIZE);
		if (g_allow_ip_count >= 0)
		{
			if (bsearch(&client_addr, g_allow_ip_addrs, \
					g_allow_ip_count, sizeof(in_addr_t), \
					cmp_by_ip_addr_t) == NULL)
			{
				logError("file: "__FILE__", line: %d, " \
					"ip addr %s is not allowed to access", \
					__LINE__, szClientIp);

				close(incomesock);
				continue;
			}
		}

		if (tcpsetnonblockopt(incomesock) != 0)
		{
			close(incomesock);
			continue;
		}

		pTask = free_queue_pop();
		if (pTask == NULL)
		{
			logError("file: "__FILE__", line: %d, " \
				"malloc task buff failed", \
				__LINE__);
			close(incomesock);
			continue;
		}

		pThreadData = g_thread_data + incomesock % g_max_threads;

		strcpy(pTask->client_ip, szClientIp);
		event_set(&pTask->ev_read, incomesock, EV_READ, \
				client_sock_read, pTask);
		if (event_base_set(pThreadData->ev_base, &pTask->ev_read) != 0)
		{
			free_queue_push(pTask);
			close(incomesock);

			logError("file: "__FILE__", line: %d, " \
				"event_base_set fail.", __LINE__);
			continue;
		}

		event_set(&pTask->ev_write, incomesock, EV_WRITE, \
				client_sock_write, pTask);
		if ((result=event_base_set(pThreadData->ev_base, \
				&pTask->ev_write)) != 0)
		{
			free_queue_push(pTask);
			close(incomesock);

			logError("file: "__FILE__", line: %d, " \
					"event_base_set fail.", __LINE__);
			continue;
		}

		if (event_add(&pTask->ev_read, &g_network_tv) != 0)
		{
			free_queue_push(pTask);
			close(incomesock);

			logError("file: "__FILE__", line: %d, " \
				"event_add fail.", __LINE__);
			continue;
		}
	}
}

int send_add_event(struct task_info *pTask)
{
	pTask->offset = 0;

	/* direct send */
	client_sock_write(pTask->ev_write.ev_fd, EV_WRITE, pTask);

	/*
	if ((result=event_add(&pTask->ev_write, &g_network_tv)) != 0)
	{
		close(pTask->ev_write.ev_fd);
		free_queue_push(pTask);

		logError("file: "__FILE__", line: %d, " \
				"event_add fail.", __LINE__);
		return result;
	}
	*/

	return 0;
}

static void client_sock_read(int sock, short event, void *arg)
{
	int bytes;
	int recv_bytes;
	struct task_info *pTask;

	pTask = (struct task_info *)arg;

	if (event == EV_TIMEOUT)
	{
		if (pTask->offset == 0 && \
			((FDHTProtoHeader *)pTask->data)->keep_alive)
		{
			if (event_add(&pTask->ev_read, &g_network_tv) != 0)
			{
				close(pTask->ev_read.ev_fd);
				free_queue_push(pTask);

				logError("file: "__FILE__", line: %d, " \
						"event_add fail.", __LINE__);
			}
		}
		else
		{
			logError("file: "__FILE__", line: %d, " \
				"client ip: %s, recv timeout, " \
				"recv offset: %d, expect length: %d", \
				__LINE__, pTask->client_ip, \
				pTask->offset, pTask->length);

			close(pTask->ev_read.ev_fd);
			free_queue_push(pTask);
		}

		return;
	}

	while (1)
	{
		if (pTask->length == 0) //recv header
		{
			recv_bytes = sizeof(FDHTProtoHeader) - pTask->offset;
		}
		else
		{
			recv_bytes = pTask->length - pTask->offset;
		}

		if (pTask->offset + recv_bytes > pTask->size)
		{
			char *pTemp;

			pTemp = pTask->data;
			pTask->data = realloc(pTask->data, \
				pTask->size + recv_bytes);
			if (pTask->data == NULL)
			{
				logError("file: "__FILE__", line: %d, " \
					"malloc failed, " \
					"errno: %d, error info: %s", \
					__LINE__, errno, STRERROR(errno));

				pTask->data = pTemp;  //restore old data

				close(pTask->ev_read.ev_fd);
				free_queue_push(pTask);
				return;
			}

			pTask->size += recv_bytes;
		}

		bytes = recv(sock, pTask->data + pTask->offset, recv_bytes, 0);
		if (bytes < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				if(event_add(&pTask->ev_read, &g_network_tv)!=0)
				{
					close(pTask->ev_read.ev_fd);
					free_queue_push(pTask);

					logError("file: "__FILE__", line: %d, "\
						"event_add fail.", __LINE__);
				}
			}
			else
			{
				logError("file: "__FILE__", line: %d, " \
					"client ip: %s, recv failed, " \
					"errno: %d, error info: %s", \
					__LINE__, pTask->client_ip, \
					errno, STRERROR(errno));

				close(pTask->ev_read.ev_fd);
				free_queue_push(pTask);
			}

			return;
		}
		else if (bytes == 0)
		{
			logDebug("file: "__FILE__", line: %d, " \
				"client ip: %s, recv failed, " \
				"connection disconnected.", \
				__LINE__, pTask->client_ip);

			close(pTask->ev_read.ev_fd);
			free_queue_push(pTask);
			return;
		}

		if (pTask->length == 0) //header
		{
			if (pTask->offset + bytes < sizeof(FDHTProtoHeader))
			{
				if (event_add(&pTask->ev_read, &g_network_tv)!=0)
				{
					close(pTask->ev_read.ev_fd);
					free_queue_push(pTask);

					logError("file: "__FILE__", line: %d, "\
						"event_add fail.", __LINE__);
				}

				pTask->offset += bytes;
				return;
			}

			pTask->length = buff2int(((FDHTProtoHeader *) \
						pTask->data)->pkg_len);
			if (pTask->length < 0)
			{
				logError("file: "__FILE__", line: %d, " \
					"client ip: %s, pkg length: %d < 0", \
					__LINE__, pTask->client_ip, \
					pTask->length);

				close(pTask->ev_read.ev_fd);
				free_queue_push(pTask);
				return;
			}

			pTask->length += sizeof(FDHTProtoHeader);
			if (pTask->length > g_max_pkg_size)
			{
				logError("file: "__FILE__", line: %d, " \
					"client ip: %s, pkg length: %d > " \
					"max pkg size: %d", __LINE__, \
					pTask->client_ip, pTask->length, \
					g_max_pkg_size);

				close(pTask->ev_read.ev_fd);
				free_queue_push(pTask);
				return;
			}
		}

		pTask->offset += bytes;
		if (pTask->offset >= pTask->length) //recv done
		{
			work_deal_task(pTask);
			return;
		}
	}

	return;
}

static void client_sock_write(int sock, short event, void *arg)
{
	int bytes;
	int result;
	struct task_info *pTask;

	pTask = (struct task_info *)arg;
	if (event == EV_TIMEOUT)
	{
		logError("file: "__FILE__", line: %d, " \
			"send timeout", __LINE__);

		close(pTask->ev_write.ev_fd);
		free_queue_push(pTask);

		return;
	}

	while (1)
	{
		bytes = send(sock, pTask->data + pTask->offset, \
				pTask->length - pTask->offset,  0);
		//printf("%08X sended %d bytes\n", (int)pTask, bytes);
		if (bytes < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				if (event_add(&pTask->ev_write, &g_network_tv) != 0)
				{
					close(pTask->ev_write.ev_fd);
					free_queue_push(pTask);

					logError("file: "__FILE__", line: %d, " \
						"event_add fail.", __LINE__);
				}
			}
			else
			{
				logError("file: "__FILE__", line: %d, " \
					"client ip: %s, recv failed, " \
					"errno: %d, error info: %s", \
					__LINE__, pTask->client_ip, \
					errno, STRERROR(errno));

				close(pTask->ev_write.ev_fd);
				free_queue_push(pTask);
			}

			return;
		}
		else if (bytes == 0)
		{
			logWarning("file: "__FILE__", line: %d, " \
				"send failed, connection disconnected.", \
				__LINE__);

			close(pTask->ev_write.ev_fd);
			free_queue_push(pTask);
			return;
		}

		pTask->offset += bytes;
		if (pTask->offset >= pTask->length)
		{
			if (((FDHTProtoHeader *)pTask->data)->keep_alive)
			{
				pTask->offset = 0;
				pTask->length  = 0;

				if ((result=event_add(&pTask->ev_read, \
						&g_network_tv)) != 0)
				{
					close(pTask->ev_read.ev_fd);
					free_queue_push(pTask);

					logError("file: "__FILE__", line: %d, "\
						"event_add fail.", __LINE__);
					return;
				}
			}
			else
			{
				close(pTask->ev_write.ev_fd);
				free_queue_push(pTask);
			}

			return;
		}
	}
}

