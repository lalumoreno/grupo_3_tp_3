#include "main.h"
#include "cmsis_os.h"
#include "task_button.h"
#include "task_uart.h"
#include "board.h"

void app_init(void)
{
	/* Crear tareas del sistema */
	uart_task_init();
	button_task_init();

	/* Enviar mensaje por UART */
	uart_log("APP - App init\r\n");
#ifdef _F429ZI_
	uart_log("APP - Board F429ZI\r\n");
#endif
}
