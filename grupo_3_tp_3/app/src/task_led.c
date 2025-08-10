#include <string.h>
#include "task_led.h"
#include "main.h"
#include "cmsis_os.h"
#include "task_uart.h"
#include "board.h"

#define LED_TASK_STACK_SIZE 128
#define LED_TASK_PRIORITY tskIDLE_PRIORITY
#define LED_TASK_PERIOD_MS 50

typedef enum
{
	LED_STATE_STANDBY,
	LED_STATE_RED,
	LED_STATE_GREEN,
	LED_STATE_BLUE
} led_state_t;

TaskHandle_t task_led_handle = NULL;
QueueHandle_t led_queue;

static void leds_off();
static void led_red_on();
static void led_green_on();
static void led_blue_on();

static void leds_off()
{
	HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED_BLUE_PORT, LED_BLUE_PIN, GPIO_PIN_RESET);
	uart_log("LED - LEDs apagados\r\n");
}

static void led_red_on()
{
	HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_SET);
}

static void led_green_on()
{
	HAL_GPIO_WritePin(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_PIN_SET);
}

static void led_blue_on()
{
	HAL_GPIO_WritePin(LED_BLUE_PORT, LED_BLUE_PIN, GPIO_PIN_SET);
}

// Inicializar la cola de un LED
void led_queue_init()
{
	if (led_queue != NULL)
	{
		uart_log("LED - Cola led_queue ya inicializada\r\n");
		return; // Ya está inicializada
	}

	// Crear cola de eventos del LED
	led_queue = xQueueCreate(NUM_LEDS, sizeof(led_event_t *));
	configASSERT(led_queue != NULL);
	if (led_queue == NULL)
	{
		uart_log("LED - Error: no se pudo crear la cola led_queue\r\n");
		while (1)
			;
	}

	uart_log("LED - Cola led_queue creada\r\n");
}

bool led_queue_add(led_event_t *event)
{

	BaseType_t sent = xQueueSend(led_queue, &event, portMAX_DELAY);
	if (sent != pdPASS)
	{
		uart_log("LED - Error agregando evento a la cola del LED\r\n");
		return false;
	}

	uart_log("LED - Evento led_event agregado a cola del LED\r\n");
	return true;
}

void led_process_event()
{
	led_event_t *led_event;

	if (led_queue != NULL)
	{
		if (xQueueReceive(led_queue, (void *)&led_event, 0) == pdTRUE)
		{
			leds_off(); // Apagar todos los LEDs al inicio
			switch (led_event->type)
			{
			case LED_EVENT_RED:
				uart_log("LED - Encender LED Rojo\r\n");
				led_red_on();
				break;
			case LED_EVENT_GREEN:
				uart_log("LED - Encender LED Verde\r\n");
				led_green_on();
				break;
			case LED_EVENT_BLUE:
				uart_log("LED - Encender LED Azul\r\n");
				led_blue_on();
				break;
			default:
				uart_log("LED - Estado Desconocido\r\n");
				break;
			}

			uart_log("LED - Evento led_event procesado \r\n");
			if (led_event->callback_process_completed != NULL)
			{
				led_event->callback_process_completed(led_event);
			}
			else
			{
				uart_log("LED - led_event callback vacio\r\n");
			}
		}

	}
}

static void led_task(void *argument)
{
	while (1)
	{
		led_process_event();
		vTaskDelay(LED_TASK_PERIOD_MS);
	}
}

bool led_task_init()
{
	// Crear cola de LEDs
	// TODO validar resultados para continuar
	led_queue_init();

	if (task_led_handle == NULL)
	{
		BaseType_t status = xTaskCreate(led_task, "led_task", LED_TASK_STACK_SIZE, NULL,
										LED_TASK_PRIORITY, &task_led_handle);
		configASSERT(status == pdPASS);
		uart_log("LED - Tarea led_task creada\r\n");
	}

	return true;
}
