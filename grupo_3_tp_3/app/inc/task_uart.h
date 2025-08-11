#ifndef TASK_UART_H_
#define TASK_UART_H_

#include <stdbool.h>

#define UART_MSG_MAX_LEN 64

bool uart_task_init(void);
void uart_log(const char *msg);

#endif /* TASK_UART_H_ */
