#include "main.h"
#include "cmsis_os.h"
#include "task_button.h"
#include "task_uart.h"
#include "board.h"
#include "task_ui.h"
#include "task_led.h"
#include "priority_queue.h"

void app_init(void)
{
	// Inicializar cola de prioridades
	if (!pq_init(10))
	{
		uart_log("APP - Error al inicializar la cola de prioridades\r\n");
		return;
	}

	/* Crear tareas del sistema */
	uart_task_init();
	button_task_init();
	ui_task_init();
	led_task_init();

	/* Enviar mensaje por UART */
	uart_log("APP - App inicializada\r\n");
#ifdef _F429ZI_
	uart_log("APP - Board F429ZI\r\n");
#endif
}
