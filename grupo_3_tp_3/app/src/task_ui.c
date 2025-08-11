#include "cmsis_os.h"
#include "board.h"
#include "task_uart.h"
#include "task_led.h"
#include "task_ui.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "priority_queue.h"

#define UI_TASK_STACK_SIZE 128
#define UI_TASK_PRIORITY (tskIDLE_PRIORITY + 1)
#define UI_EVENT_QUEUE_LENGTH 10
#define TASK_PERIOD_MS 50

/* Cola de eventos del botón*/
QueueHandle_t ui_queue;
TaskHandle_t task_ui_handle = NULL;

static void ui_task(void *argument);
static void ui_set_priority(button_event *event, pq_item_t *item);
static bool ui_queue_init();

static void ui_set_priority(button_event *event, pq_item_t *item)
{
	if (event == NULL || item == NULL)
	{
		uart_log("UUI - Error: Puntero nulo en ui_set_priority\r\n");
		return;
	}

	switch (event->type)
	{
	case BUTTON_TYPE_PULSE:
		uart_log("UUI - Elemento de prioridad: ALTA\r\n");
		item->priority = PRIORIDAD_ALTA;
		break;
	case BUTTON_TYPE_SHORT:
		uart_log("UUI - Elemento de prioridad: MEDIA\r\n");
		item->priority = PRIORIDAD_MEDIA;
		break;
	case BUTTON_TYPE_LONG:
		uart_log("UUI - Elemento de prioridad: BAJA\r\n");
		item->priority = PRIORIDAD_BAJA;
		break;
	default:
		uart_log("UUI - Elemento de prioridad desconocida\r\n");
		item->priority = PRIORIDAD_NONE; // Asignar prioridad NONE si es desconocido
		return;
	}
}

static void ui_task(void *argument)
{
	button_event *button_event;
	pq_item_t item;

	while (1)
	{
		// Procesar eventos de la cola ui_queue
		if (xQueueReceive(ui_queue, &button_event, pdMS_TO_TICKS(100)) == pdPASS)
		{
			ui_set_priority(button_event, &item);
			if(!pq_push(&item))
			{
				uart_log("UUI - Error al agregar evento a la cola de prioridades\r\n");
				button_event->callback_process_completed(button_event);
				vTaskDelay(pdMS_TO_TICKS(TASK_PERIOD_MS));
				continue;
			}

			uart_log("UUI - Evento button_event procesado \r\n");
			button_event->callback_process_completed(button_event);
		}

		vTaskDelay(pdMS_TO_TICKS(TASK_PERIOD_MS));
	}
}

static bool ui_queue_init()
{
	if (ui_queue != NULL)
	{
		uart_log("UUI - Cola ui_queue ya inicializada\r\n");
		return true; // Ya está inicializada
	}

	/* Crear cola de eventos del botón */
	ui_queue = xQueueCreate(UI_EVENT_QUEUE_LENGTH, sizeof(button_event *));
	configASSERT(ui_queue != NULL);
	if (ui_queue == NULL)
	{
		uart_log("UUI - Error: no se pudo crear la cola ui_queue\r\n");
		return false; // Error al crear la cola
	}

	uart_log("UUI - Cola ui_queue creada\r\n");
	return true;
}

bool ui_task_init()
{
	// Crear cola de boton
	if(!ui_queue_init())
	{
		uart_log("UUI - Error al inicializar la cola ui_queue\r\n");
		return false; // Error al inicializar la cola
	}

	if (task_ui_handle != NULL)
	{
		uart_log("UUI - Tarea ui_task ya inicializada\r\n");
		return true; // Ya está inicializada
	}

	BaseType_t status = xTaskCreate(ui_task, "ui_task", UI_TASK_STACK_SIZE, NULL,
									UI_TASK_PRIORITY, &task_ui_handle);
	configASSERT(status == pdPASS);
	uart_log("UUI - Tarea ui_task creada\r\n");

	return true;
}

bool ui_queue_send(button_event *event)
{
	if (ui_queue == NULL)
	{
		uart_log("UUI - Cola de eventos del botón no inicializada\r\n");
		return false;
	}

	BaseType_t sent = xQueueSend(ui_queue, &event, portMAX_DELAY);
	if (sent != pdPASS)
	{
		uart_log("UUI - Error agregando evento a la cola ui_queue\r\n");
		return false;
	}

	uart_log("UUI - Evento bnt_event agregado a ui_queue\r\n");
	return true;
}
