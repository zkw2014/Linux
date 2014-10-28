/************************************************************************
  > File: def.h
  > By: zkw
  > Description: 
 ************************************************************************/

#ifndef DEF_H
#define DEF_H

#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#define ERR_EXIT(m) \
	do { \
		perror(m); \
		exit(EXIT_FAILURE); \
	} while (0)

typedef enum {false, true} bool;
typedef void (*FUNC)(void);

typedef struct task
{
	FUNC data_;
	struct task *next_;
} task_t;

typedef struct
{
	task_t *head_;
	task_t *tail_;
	unsigned int size_;
	bool is_empty_;
} queue_t;

typedef struct
{
	unsigned int thread_nums_;
	pthread_t *threads_;
	queue_t queue_;
	pthread_mutex_t mutex_lock_;
	pthread_cond_t cond_lock_; //处理队列空的情况
	bool is_started_;
} thread_pool_t;

void queue_init(queue_t *queue_ptr);
void queue_destroy(queue_t *queue_ptr);
void queue_push(queue_t *queue_ptr, FUNC data);
void queue_pop(queue_t *queue_ptr);
task_t queue_top(queue_t queue);
bool queue_is_empty(queue_t queue);
unsigned int get_queue_size(queue_t queue);

void pool_init(thread_pool_t *pool_ptr, unsigned int thread_nums);
void pool_destroy(thread_pool_t *pool_ptr);
void pool_start(thread_pool_t *pool_ptr);
void pool_stop(thread_pool_t *pool_ptr);
bool pool_is_running(const thread_pool_t *pool_ptr);
void add_task_to_pool(thread_pool_t *pool_ptr, FUNC data);

#endif //DEF_H
