//task_queue.c

#include <errno.h>
#include <pthread.h>
#include "task_queue.h"
#include "fdht_global.h"
#include "global.h"
#include "logger.h"
#include "shared_func.h"
#include "pthread_func.h"
#include "work_thread.h"

static struct task_queue_info g_free_queue;

static struct task_info *g_mpool = NULL;

static struct task_info *_queue_pop_task(struct task_queue_info *pQueue);
static int _task_queue_count(struct task_queue_info *pQueue);

int task_queue_init()
{
	struct task_info *pTask;
	struct task_info *pEnd;
	int alloc_size;
	int result;

	if ((result=init_pthread_lock(&(g_free_queue.lock))) != 0)
	{
		logError("file: "__FILE__", line: %d, " \
			"init_pthread_lock fail, program exit!", __LINE__);
		return result;
	}

	alloc_size = sizeof(struct task_info) * g_max_connections;
	g_mpool = (struct task_info *)malloc(alloc_size);
	if (g_mpool == NULL)
	{
		logError("file: "__FILE__", line: %d, " \
			"malloc %d bytes fail", \
			__LINE__, alloc_size);
		return errno != 0 ? errno : ENOMEM;
	}

	memset(g_mpool, 0, alloc_size);
	pEnd = g_mpool + g_max_connections;
	for (pTask=g_mpool; pTask<pEnd; pTask++)
	{
		pTask->size = g_min_buff_size;
		pTask->data = malloc(pTask->size);
		if (pTask->data == NULL)
		{
			task_queue_destroy();

			logError("file: "__FILE__", line: %d, " \
				"malloc %d bytes fail", \
				__LINE__, pTask->size);
			return errno != 0 ? errno : ENOMEM;
		}
	}

	g_free_queue.head = g_mpool;
	g_free_queue.tail = pEnd - 1;
	for (pTask=g_mpool; pTask<g_free_queue.tail; pTask++)
	{
		pTask->next = pTask + 1;
	}
	g_free_queue.tail->next = NULL;

	return 0;
}

void task_queue_destroy()
{
	struct task_info *pTask;
	struct task_info *pEnd;

	if (g_mpool == NULL)
	{
		return;
	}

	pEnd = g_mpool + g_max_connections;
	for (pTask=g_mpool; pTask<pEnd; pTask++)
	{
		if (pTask->data != NULL)
		{
			free(pTask->data);
			pTask->data = NULL;
		}
	}

	free(g_mpool);
	g_mpool = NULL;

	pthread_mutex_destroy(&(g_free_queue.lock));
}

struct task_info *free_queue_pop()
{
	return _queue_pop_task(&g_free_queue);;
}

int free_queue_push(struct task_info *pTask)
{
	char *new_buff;
	int result;

	pTask->length = 0;
	pTask->offset = 0;
	if (pTask->size > g_min_buff_size) //need thrink
	{
		new_buff = (char *)malloc(g_min_buff_size);
		if (new_buff == NULL)
		{
			logWarning("file: "__FILE__", line: %d, " \
				"malloc %d bytes fail", \
				__LINE__, g_min_buff_size);
		}
		else
		{
			free(pTask->data);
			pTask->size = g_min_buff_size;
			pTask->data = new_buff;
		}
	}

	if ((result=pthread_mutex_lock(&g_free_queue.lock)) != 0)
	{
		logError("file: "__FILE__", line: %d, " \
			"call pthread_mutex_lock fail, " \
			"errno: %d, error info: %s", \
			__LINE__, result, STRERROR(result));
	}

	pTask->next = g_free_queue.head;
	g_free_queue.head = pTask;
	if (g_free_queue.tail == NULL)
	{
		g_free_queue.tail = pTask;
	}

	if ((result=pthread_mutex_unlock(&g_free_queue.lock)) != 0)
	{
		logError("file: "__FILE__", line: %d, " \
			"call pthread_mutex_unlock fail, " \
			"errno: %d, error info: %s", \
			__LINE__, result, STRERROR(result));
	}

	return result;
}

int free_queue_count()
{
	return _task_queue_count(&g_free_queue);
}

static struct task_info *_queue_pop_task(struct task_queue_info *pQueue)
{
	struct task_info *pTask;
	int result;

	if ((result=pthread_mutex_lock(&(pQueue->lock))) != 0)
	{
		logError("file: "__FILE__", line: %d, " \
			"call pthread_mutex_lock fail, " \
			"errno: %d, error info: %s", \
			__LINE__, result, STRERROR(result));
		return NULL;
	}

	pTask = pQueue->head;
	if (pTask != NULL)
	{
		pQueue->head = pTask->next;
		if (pQueue->head == NULL)
		{
			pQueue->tail = NULL;
		}
	}

	if ((result=pthread_mutex_unlock(&(pQueue->lock))) != 0)
	{
		logError("file: "__FILE__", line: %d, " \
			"call pthread_mutex_unlock fail, " \
			"errno: %d, error info: %s", \
			__LINE__, result, STRERROR(result));
	}

	return pTask;
}

static int _task_queue_count(struct task_queue_info *pQueue)
{
	struct task_info *pTask;
	int count;
	int result;

	if ((result=pthread_mutex_lock(&(pQueue->lock))) != 0)
	{
		logError("file: "__FILE__", line: %d, " \
			"call pthread_mutex_lock fail, " \
			"errno: %d, error info: %s", \
			__LINE__, result, STRERROR(result));
		return 0;
	}

	count = 0;
	pTask = pQueue->head;
	while (pTask != NULL)
	{
		pTask = pTask->next;
		count++;
	}

	if ((result=pthread_mutex_unlock(&(pQueue->lock))) != 0)
	{
		logError("file: "__FILE__", line: %d, " \
			"call pthread_mutex_unlock fail, " \
			"errno: %d, error info: %s", \
			__LINE__, result, STRERROR(result));
	}

	return count;
}

