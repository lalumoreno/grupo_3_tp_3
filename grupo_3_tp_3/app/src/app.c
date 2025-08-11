#include "main.h"
#include "cmsis_os.h"
#include "task_button.h"
#include "task_uart.h"
#include "board.h"
#include "task_ui.h"
#include "task_led.h"
#include "priority_queue.h"

void app_error_handler()
{
	while (1)
	{
		// Bucle infinito para detener la ejecución
	}
}

void app_init(void)
{
	/* Crear tareas del sistema */
	if(!uart_task_init())
	{
		app_error_handler();
	}

	//TODO handle errors
	button_task_init();
	
	if(!ui_task_init())
	{
		uart_log("APP - Error al inicializar la tarea de UI\r\n");
		app_error_handler();
	}

	if(!led_task_init())
	{
		uart_log("APP - Error al inicializar la tarea de LED\r\n");
		app_error_handler();
	}

	// Inicializar cola de prioridades
	if (!pq_init(10))
	{
		uart_log("APP - Error al inicializar la cola de prioridades\r\n");
		app_error_handler();
	}

	/* Enviar mensaje por UART */
	uart_log("APP - App inicializada\r\n");
#ifdef _F429ZI_
	uart_log("APP - Board F429ZI\r\n");
#endif
}
