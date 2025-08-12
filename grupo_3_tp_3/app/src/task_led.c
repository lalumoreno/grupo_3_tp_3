#include <string.h>
#include "task_led.h"
#include "main.h"
#include "cmsis_os.h"
#include "task_uart.h"
#include "board.h"
#include "priority_queue.h"

#define LED_TASK_STACK_SIZE 128
#define LED_TASK_PRIORITY tskIDLE_PRIORITY
#define LED_TASK_PERIOD_MS 50
#define LED_ON_MS 5000
#define LED_QUEUE_SIZE 10

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
static bool led_queue_init();
static void led_process_event();
static bool led_get_priority(pq_item_t *item);
static bool led_create_event(led_event_t *event, pq_item_t *item);

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

// Inicializar la cola de leds
static bool led_queue_init()
{
	if (led_queue != NULL)
	{
		uart_log("LED - Cola led_queue ya inicializada\r\n");
		return true; // Ya está creada
	}

	// Crear cola de eventos del LED
	led_queue = xQueueCreate(LED_QUEUE_SIZE, sizeof(led_event_t *));
	configASSERT(led_queue != NULL);
	if (led_queue == NULL)
	{
		uart_log("LED - Error: no se pudo crear la cola led_queue\r\n");
		return false; // Error al crear la cola
	}

	uart_log("LED - Cola led_queue creada\r\n");
	return true;
}

static bool led_get_priority(pq_item_t *item)
{
	if (item == NULL)
	{
		uart_log("LED - Error: Puntero nulo en led_get_priority\r\n");
		return false;
	}

	// Buscar elemento más prioritario y más antiguo
	if (pq_pop(item) == false)
	{
		// uart_log("LED - Error al obtener el elemento de la cola de prioridades\r\n");
		return false;
	}

	return true;
}

static bool led_create_event(led_event_t *event, pq_item_t *item)
{
	if (event == NULL || item == NULL)
	{
		uart_log("LED - Error: Puntero nulo en led_create_event\r\n");
		return false;
	}

	switch (item->priority)
	{
	case PRIORIDAD_BAJA:
		uart_log("LED - Elemento de prioridad BAJA\r\n");
		event->type = LED_EVENT_BLUE;
		break;
	case PRIORIDAD_MEDIA:
		event->type = LED_EVENT_GREEN;
		uart_log("LED - Elemento de prioridad MEDIA\r\n");
		break;
	case PRIORIDAD_ALTA:
		uart_log("LED - Elemento de prioridad ALTA\r\n");
		event->type = LED_EVENT_RED;
		break;
	default:
		uart_log("LED - Error: Prioridad desconocida en led_create_event\r\n");
		return false;
	}

	return true;
}

static void led_process_event()
{
	led_event_t *led_event;

	if (led_queue != NULL)
	{
		if (xQueueReceive(led_queue, (void *)&led_event, 0) == pdTRUE)
		{
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
			vTaskDelay(LED_ON_MS);
			leds_off(); // Apagar LEDs después de un tiempo
		}
	}
}

static void led_task(void *argument)
{
	led_event_t led_event;
	pq_item_t item;
	leds_off();

	while (1)
	{

		if (!led_get_priority(&item))
		{
			vTaskDelay(pdMS_TO_TICKS(LED_TASK_PERIOD_MS));
			continue; // Si no se pudo obtener evento, continuar
		}

		if (!led_create_event(&led_event, &item))
		{
			uart_log("LED - Error creando led_event\r\n");
			vTaskDelay(pdMS_TO_TICKS(LED_TASK_PERIOD_MS));
			continue;
		}

		if (!led_queue_add(&led_event))
		{
			vTaskDelay(pdMS_TO_TICKS(LED_TASK_PERIOD_MS));
			continue;
		}

		led_process_event();

		vTaskDelay(pdMS_TO_TICKS(LED_TASK_PERIOD_MS));
	}
}

bool led_queue_add(led_event_t *event)
{

	BaseType_t sent = xQueueSend(led_queue, &event, portMAX_DELAY);
	if (sent != pdPASS)
	{
		uart_log("LED - Error agregando evento a led_queue\r\n");
		return false;
	}

	uart_log("LED - Evento led_event agregado a led_queue\r\n");
	return true;
}

bool led_task_init()
{
	if (!led_queue_init())
	{
		return false;
	}

	if (task_led_handle != NULL)
	{
		uart_log("LED - Tarea led_task ya inicializada\r\n");
		return true; // Ya está creada
	}

	BaseType_t status = xTaskCreate(led_task, "led_task", LED_TASK_STACK_SIZE, NULL, LED_TASK_PRIORITY, &task_led_handle);
	configASSERT(status == pdPASS);
	if (status != pdPASS)
	{
		uart_log("LED - Error al crear la tarea led_task\r\n");
		return false; // Error al crear la tarea
	}

	uart_log("LED - Tarea led_task creada\r\n");
	return true;
}
