#include "cmsis_os.h"
#include "board.h"
#include "task_uart.h"
#include "task_led.h"
#include "task_ui.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#define UI_TASK_STACK_SIZE 128
#define UI_TASK_PRIORITY (tskIDLE_PRIORITY + 1)
#define UI_EVENT_QUEUE_LENGTH 10

/* Cola de eventos del botón*/
QueueHandle_t ui_queue;
TaskHandle_t task_ui_handle = NULL;

led_t leds[NUM_LEDS] = {
	{.queue = NULL}, // LED Rojo
	{.queue = NULL}, // LED Verde
	{.queue = NULL}  // LED Azul
};

static void callback_process_completed(void *context);
static void ui_cleanup_ui_resources();
static void ui_destroy_task();
static void ui_task(void *argument);

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

static void ui_task(void *argument)
{
	led_t *leds = (led_t *)argument;
	button_event *button_event;

	while (1)
	{
		// Procesar eventos de la cola ui_queue
		if (xQueueReceive(ui_queue, &button_event, pdMS_TO_TICKS(100)) == pdPASS)
		{
			bool valid_event = true;
			led_event_t *led_event = (led_event_t *)pvPortMalloc(
				sizeof(led_event_t));

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
					if (!led_queue_add(leds, led_event->type, led_event))
					{
						uart_log("UUI - Error al agregar evento a la cola del LED\r\n");
						vPortFree(led_event);
						uart_log("UUI - Memoria de led_event liberada \r\n");
					}
				}

				led_process_event(leds);

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
		else
		{
			// Verificar si la cola está vacía
			if (uxQueueMessagesWaiting(ui_queue) == 0)
			{
				// Si no hay eventos, destruir la tarea
				ui_destroy_task();
			}
		}
	}
}

void ui_queue_init()
{
	/* Crear cola de eventos del botón */
	ui_queue = xQueueCreate(UI_EVENT_QUEUE_LENGTH, sizeof(button_event *));
	configASSERT(ui_queue != NULL);
	if (ui_queue == NULL)
	{
		uart_log("UUI - Error: no se pudo crear la cola de botón\r\n");
		while (1)
			;
	}

	uart_log("UUI - Cola ui_queue creada\r\n");
}

void ui_task_create(int event_type)
{
	BaseType_t status;
	int led_type = event_type - 1;
	
	// Crear cola de LEDs
	led_queue_init(&leds[led_type]);

	// Crear cola de boton
	ui_queue_init();

	if (task_ui_handle == NULL)
	{		
		status = xTaskCreate(ui_task, "ui_task", UI_TASK_STACK_SIZE, (void *)leds,
							 UI_TASK_PRIORITY, &task_ui_handle);
		configASSERT(status == pdPASS);
		uart_log("BTN - Tarea ui_task creada\r\n");
	}
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

