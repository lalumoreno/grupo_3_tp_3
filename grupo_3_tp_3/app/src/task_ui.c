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

static void callback_process_completed(void *context);
static void ui_cleanup_ui_resources();
static void ui_destroy_task();
static void ui_task(void *argument);
static void ui_set_priority(button_event *event, pq_item_t *item);
static void ui_queue_init();

static void callback_process_completed(void *context)
{
	led_event_t *event = (led_event_t *)context;
	vPortFree(event);
	uart_log("UUI - Memoria led_event liberada desde callback\r\n");
}

static void ui_cleanup_ui_resources()
{
	// Liberar la cola de eventos si es necesario
	if (ui_queue != NULL)
	{
		vQueueDelete(ui_queue);
		ui_queue = NULL;
		uart_log("UII - Cola ui_queue eliminada\r\n");
	}
}

static void ui_destroy_task()
{
	// Liberar recursos si es necesario
	ui_cleanup_ui_resources();
	task_ui_handle = NULL;
	vTaskDelete(NULL);
	uart_log("UUI - Tarea ui_task eliminada\r\n");
}

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
		uart_log("UUI - Evento de prioridad: ALTA\r\n");
		item->priority = PRIORIDAD_ALTA;
		break;
	case BUTTON_TYPE_SHORT:
		uart_log("UUI - Evento de prioridad: MEDIA\r\n");
		item->priority = PRIORIDAD_MEDIA;
		break;
	case BUTTON_TYPE_LONG:
		uart_log("UUI - Evento de prioridad: BAJA\r\n");
		item->priority = PRIORIDAD_BAJA;
		break;
	default:
		return;
	}
}

static void ui_task(void *argument)
{
	button_event *button_event;

	while (1)
	{
		// Procesar eventos de la cola ui_queue
		if (xQueueReceive(ui_queue, &button_event, pdMS_TO_TICKS(100)) == pdPASS)
		{
			// Interpretar estado del botón y crear mensaje con prioridad
			pq_item_t item;
			ui_set_priority(button_event, &item);
			pq_push(&item);

			bool valid_event = true;
			led_event_t *led_event = (led_event_t *)pvPortMalloc(
				sizeof(led_event_t));

			// TODO remove this
			if (led_event != NULL)
			{
				char msg[64];
				sprintf(msg, "UUI - Memoria led_event alocada: %d\r\n",
						sizeof(*led_event));
				uart_log(msg);

				led_event->callback_process_completed = callback_process_completed;
				led_event->callback_context = led_event;

				switch (button_event->type)
				{
				case BUTTON_TYPE_PULSE:
					uart_log("UUI - Boton tipo PULSO - activar LED rojo\r\n");
					led_event->type = LED_EVENT_RED;
					break;

				case BUTTON_TYPE_SHORT:
					uart_log("UUI - Boton tipo CORTO - activar LED verde\r\n");
					led_event->type = LED_EVENT_GREEN;
					break;

				case BUTTON_TYPE_LONG:
					uart_log("UUI - Boton tipo LARGO - activar LED azul\r\n");
					led_event->type = LED_EVENT_BLUE;
					break;

				default:
					uart_log("UUI - tipo de pulsacion no reconocido\r\n");
					valid_event = false;
					break;
				}

				if (valid_event)
				{
					if (!led_queue_add(led_event))
					{
						uart_log("UUI - Error al agregar evento a la cola del LED\r\n");
						vPortFree(led_event);
						uart_log("UUI - Memoria de led_event liberada \r\n");
					}
				}

				uart_log("UUI - Evento button_event procesado \r\n");
				if (button_event->callback_process_completed != NULL)
				{
					button_event->callback_process_completed(button_event);
				}
				else
				{
					uart_log("UUI - button_event callback vacio\r\n");
				}
			}
			else
			{
				uart_log("UUI - Memoria insuficiente\r\n");
			}

		}

		vTaskDelay(pdMS_TO_TICKS(TASK_PERIOD_MS));
	}
}

static void ui_queue_init()
{
	if (ui_queue != NULL)
	{
		uart_log("UUI - Cola ui_queue ya inicializada\r\n");
		return; // Ya está inicializada
	}

	/* Crear cola de eventos del botón */
	ui_queue = xQueueCreate(UI_EVENT_QUEUE_LENGTH, sizeof(button_event *));
	configASSERT(ui_queue != NULL);
	if (ui_queue == NULL)
	{
		uart_log("UUI - Error: no se pudo crear ui_queue\r\n");
		while (1)
			;
	}

	uart_log("UUI - Cola ui_queue creada\r\n");
}

bool ui_task_init()
{ 
	// Crear cola de boton
	ui_queue_init(); //TODO validate result

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
