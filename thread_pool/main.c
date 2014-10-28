/************************************************************************
  > File: main.c
  > By: zkw
  > Description: 
 ************************************************************************/

#include <stdio.h>
#include "def.h"

#define THREAD_NUMS 5

void user_func(void)
{
	printf("hello\n");
}

int main(void)
{
	thread_pool_t pool;
	pool_init(&pool, 5);
	pool_start(&pool);

	while (1) {
		add_task_to_pool(&pool, user_func);
		sleep(1);
	}

	pool_stop(&pool);
	pool_destroy(&pool);

	return 0;
}


