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
static void led_queue_init();
static void led_process_event();
static bool led_get_priority(pq_item_t *item);

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
static void led_queue_init()
{
	if (led_queue != NULL)
	{
		uart_log("LED - Cola led_queue ya inicializada\r\n");
		return; // Ya está inicializada
	}

	// Crear cola de eventos del LED
	led_queue = xQueueCreate(LED_QUEUE_SIZE, sizeof(led_event_t *));
	configASSERT(led_queue != NULL);
	if (led_queue == NULL)
	{
		uart_log("LED - Error: no se pudo crear la cola led_queue\r\n");
		while (1)
			;
	}

	uart_log("LED - Cola led_queue creada\r\n");
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
		//uart_log("LED - Error al obtener el elemento de la cola de prioridades\r\n");
		return false;
	}

	led_event_t *led_event = pvPortMalloc(sizeof(led_event_t));
	if (led_event == NULL)
	{
		uart_log("LED - Error: No se pudo asignar memoria para led_event\r\n");
		return false;
	}

	switch (item->priority)
	{
	case PRIORIDAD_ALTA:
		uart_log("LED - Evento de prioridad: ALTA - activar LED rojo\r\n");
		led_event->type = LED_EVENT_RED;
		break;
	case PRIORIDAD_MEDIA:
		uart_log("LED - Evento de prioridad: MEDIA - activar LED verde\r\n");
		led_event->type = LED_EVENT_GREEN;
		break;
	case PRIORIDAD_BAJA:
		uart_log("LED - Evento de prioridad: BAJA - activar LED azul\r\n");
		led_event->type = LED_EVENT_BLUE;
		break;
	default:
		uart_log("LED - Prioridad no reconocida\r\n");
		break;
	}

	return led_queue_add(led_event);
}

static void led_process_event()
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
			// Liberar memoria del evento
			vPortFree(led_event);

			vTaskDelay(LED_ON_MS);
			leds_off(); // Apagar LEDs después de un tiempo
		}
	}
}

static void led_task(void *argument)
{
	while (1)
	{
		pq_item_t item;
		if (!led_get_priority(&item))
		{
			continue; // Si no se pudo obtener evento de prioridad, continuar
		}

		led_process_event();
		vTaskDelay(LED_TASK_PERIOD_MS);
	}
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
