/************************************************************************
  > File: main.c
  > By: zkw
  > Description: 用户只需将原料(parameter)和方法(user_func)添加到线程池，具体的实现交由线程池自行完成。
 ************************************************************************/

#include <stdio.h>
#include "def.h"

#define THREAD_NUMS 3 //线程池中的线程个数
#define PRODUCT_NUMS 5 //要生产的产品数量

void user_func(void *arg) //对原料的加工方法
{
	int *p = (int *)arg;
	int temp = *p;
	printf("%d * %d = %d\n", temp, temp, temp * temp);
}

int main(void)
{
	thread_pool_t pool;
	pool_init(&pool, THREAD_NUMS);
	pool_start(&pool);

	int times = 0;
	srand(time(NULL));
	while (1) {
		int parameter = rand()%10; //原料
		add_task_to_pool(&pool, user_func, parameter);
		sleep(1); //留出时间让线程群去完成任务
		++times;
		if (times == PRODUCT_NUMS)
			break;
	}

	pool_stop(&pool);
	pool_destroy(&pool);

	return 0;
}


