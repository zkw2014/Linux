/************************************************************************
  > File: queue.c
  > By: zkw
  > Description: 
 ************************************************************************/

#include <stdio.h>
#include <assert.h>
#include "def.h"

void queue_init(queue_t *queue_ptr)
{
	queue_ptr->head_ = queue_ptr->tail_ = NULL;
	queue_ptr->size_ = 0;
	queue_ptr->is_empty_ = true;
}

void queue_destroy(queue_t *queue_ptr)
{
	while (!queue_ptr->is_empty_) {
		queue_pop(queue_ptr);
	}
	assert(queue_ptr->is_empty_);
}

void queue_push(queue_t *queue_ptr, FUNC data, int parameter)
{
	//在堆上开辟节点空间
	task_t *temp = (task_t *)malloc(sizeof (task_t));
	if (NULL == temp) {
		fprintf(stderr, "malloc");
		exit(EXIT_FAILURE);
	}
	bzero(temp, sizeof(task_t));
	temp->data_ = data;
	temp->parameter_ = parameter;

	if (queue_ptr->is_empty_) { //队列空时，单独处理
		queue_ptr->head_ = queue_ptr->tail_ = temp;
		queue_ptr->is_empty_ = false;
	} else {
		queue_ptr->tail_->next_ = temp;
		queue_ptr->tail_ = temp;
	}
	queue_ptr->size_++; 
}

void queue_pop(queue_t *queue_ptr)
{
	assert(!queue_ptr->is_empty_);
	if (1 == queue_ptr->size_) {
		free(queue_ptr->head_);
		queue_ptr->head_ = queue_ptr->tail_ = NULL;
		queue_ptr->is_empty_ = true;
	} else {
		task_t *temp = queue_ptr->head_;
		while (temp->next_ != queue_ptr->tail_)
			temp = temp->next_;
		free(queue_ptr->tail_);
		queue_ptr->tail_ = temp;
		temp = NULL;
	}
	queue_ptr->size_--;
}

task_t queue_top(queue_t queue)
{
	assert(!queue.is_empty_);
	return *(queue.head_);
}

bool queue_is_empty(queue_t queue)
{
	return (0 == queue.size_);
}

unsigned int get_queue_size(queue_t queue)
{
	return queue.size_;
}
