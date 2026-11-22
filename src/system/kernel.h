#ifndef KERNEL_H
#define KERNEL_H

#define STACK_DEPTH_SMALL   (configMINIMAL_STACK_SIZE)
#define STACK_DEPTH_MID     (configMINIMAL_STACK_SIZE*2)
#define STACK_DEPTH_LARGE   (configMINIMAL_STACK_SIZE*4)

#define TASK_PRI_HIGHEST    31
#define TASK_PRI_HIGH       24
#define TASK_PRI_MID        16
#define TASK_PRI_LOW        8
#define TASK_PRI_LOWEST     1

/**
 * RTOS資源生成を行う関数。
 * タスクの開始は行わない。
**/
bool kernel_init(void);

#endif