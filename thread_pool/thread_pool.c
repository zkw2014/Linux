/************************************************************************
  > File: thread_pool.c
  > By: zkw
  > Description: 
 ************************************************************************/

#include "def.h"

void pool_init(thread_pool_t *pool_ptr, unsigned int thread_nums)
{
	bzero(pool_ptr, sizeof(thread_pool_t));
	//开辟线程数组空间
	pool_ptr->thread_nums_ = thread_nums;
	pool_ptr->threads_ = (pthread_t *)malloc(sizeof (pthread_t) * thread_nums);
	if (NULL == pool_ptr->threads_) {
		fprintf(stderr, "malloc");
		exit(EXIT_FAILURE);
	}
	//队列
	queue_init( &(pool_ptr->queue_) );
	//互斥锁
	int temp = pthread_mutex_init(&pool_ptr->mutex_lock_, NULL);
	if (0 != temp) {
		fprintf(stderr, "pthread_mutex_init");
		exit(EXIT_FAILURE);
	}
	//条件锁
	temp = pthread_cond_init(&pool_ptr->cond_lock_, NULL);
	if (0 != temp) {
		fprintf(stderr, "pthread_cond_init");
		exit(EXIT_FAILURE);
	}
	//状态
	pool_ptr->is_started_ = false;
}

void pool_destroy(thread_pool_t *pool_ptr)
{
	if (pool_ptr->is_started_) //停止线程池
		pool_stop(pool_ptr);
	//释放线程数组空间
	free(pool_ptr->threads_);
	//销毁队列
	queue_destroy(&pool_ptr->queue_);
	//互斥锁
	int temp = pthread_mutex_destroy(&pool_ptr->mutex_lock_);
	if (0 != temp) {
		fprintf(stderr, "pthread_mutex_destroy");
		exit(EXIT_FAILURE);
	}
	//条件锁
	temp = pthread_cond_destroy(&pool_ptr->cond_lock_);
	if (0 != temp) {
		fprintf(stderr, "pthread_cond_destroy");
		exit(EXIT_FAILURE);
	}
	//状态
	pool_ptr->is_started_ = false;
}

void *thread_func(void *arg)
{
	thread_pool_t *pool_ptr = (thread_pool_t *)arg;
	printf("NO:[%lu]thread start\n", pthread_self());
	//加锁
	int temp = pthread_mutex_lock(&(pool_ptr->mutex_lock_));
	if (0 != temp) {
		fprintf(stderr, "pthread_mutex_lock");
		exit(EXIT_FAILURE);
	}

	while (pool_ptr->is_started_) { //判断线程池是否已经开启
		if (!(pool_ptr->queue_.is_empty_)) { //队列不空，去执行任务
			task_t task = queue_top(pool_ptr->queue_);
			task.data_(&(task.parameter_));
			queue_pop(&pool_ptr->queue_);
		} else { //队列空了,线程去等待
			temp = pthread_cond_wait(&pool_ptr->cond_lock_, &pool_ptr->mutex_lock_);
			if (0 != temp) {
				fprintf(stderr, "pthread_cond_wait");
				exit(EXIT_FAILURE);
			}
		}
	}
	//解锁
	temp = pthread_mutex_unlock(&pool_ptr->mutex_lock_);
	if (0 != temp) {
		fprintf(stderr, "pthread_mutex_lock");
		exit(EXIT_FAILURE);
	}
	//线程退出
	printf("NO:[%lu]thread exit\n", pthread_self());
	pthread_exit(NULL);
}

void pool_start(thread_pool_t *pool_ptr)
{
	//先设置状态
	pool_ptr->is_started_ = true;
	//启动线程群
	int ix = 0;
	for (; ix != pool_ptr->thread_nums_; ++ix) {
		int temp = pthread_create( &(pool_ptr->threads_[ix]), NULL, thread_func, pool_ptr );
		if (0 != temp) {
			fprintf(stderr, "pthread_create in pool_start");
			exit(EXIT_FAILURE);
		}
	}
}

void pool_stop(thread_pool_t *pool_ptr)
{
	//先设置状态
	pool_ptr->is_started_ = false;
	//激活所有线程,以便线程群依次退出
	int temp = pthread_cond_broadcast(&pool_ptr->cond_lock_);
	if (0 != temp) {
		fprintf(stderr, "pthread_create");
		exit(EXIT_FAILURE);
	}
	//回收线程群
	int ix = 0;
	for (; ix != pool_ptr->thread_nums_; ++ix) {
		int temp = pthread_join(pool_ptr->threads_[ix], NULL);
		if (0 != temp) {
			fprintf(stderr, "pthread_join");
			exit(EXIT_FAILURE);
		}
	}
}

bool pool_is_running(const thread_pool_t *pool_ptr)
{
	return pool_ptr->is_started_;
}

void add_task_to_pool(thread_pool_t *pool_ptr, FUNC data, int parameter)
{
	//先加互斥锁
	int temp = pthread_mutex_lock(&pool_ptr->mutex_lock_);
	if (0 != temp) {
		fprintf(stderr, "pthread_mutex_lock");
		exit(EXIT_FAILURE);
	}
	//将任务添加到队列中
	queue_push(&pool_ptr->queue_, data, parameter);
	//激活阻塞在条件队列中的线程
	temp = pthread_cond_signal(&pool_ptr->cond_lock_);
	if (0 != temp) {
		fprintf(stderr, "pthread_cond_signal");
		exit(EXIT_FAILURE);
	}
	//解锁
	temp = pthread_mutex_unlock(&pool_ptr->mutex_lock_);
	if (0 != temp) {
		fprintf(stderr, "pthread_mutex_lock");
		exit(EXIT_FAILURE);
	}
}
