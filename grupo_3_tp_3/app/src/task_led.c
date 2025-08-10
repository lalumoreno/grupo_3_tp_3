#include <string.h>
#include "task_led.h"
#include "main.h"
#include "cmsis_os.h"
#include "task_uart.h"
#include "board.h"

typedef enum
{
	LED_STATE_STANDBY,
	LED_STATE_RED,
	LED_STATE_GREEN,
	LED_STATE_BLUE
} led_state_t;

TaskHandle_t task_led_handle = NULL;

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
void led_queue_init(led_t *led) {
    if (led->queue == NULL) {
        led->queue = xQueueCreate(5, sizeof(led_event_t *));
        configASSERT(led->queue != NULL);
        if (led->queue != NULL) {
            uart_log("LED - Cola de led creada \r\n");
        } else {
            uart_log("LED - Error al crear la cola led->queue \r\n");
        }
    }
}

bool led_queue_add(led_t *leds, led_event_type_t event_type, led_event_t *event)
{
	if (leds[event_type].queue == NULL)
	{
		uart_log("LED - Cola del LED no inicializada\r\n");
		return false;
	}

	BaseType_t sent = xQueueSend(leds[event_type].queue, &event, portMAX_DELAY);
	if (sent != pdPASS)
	{
		uart_log("LED - Error agregando evento a la cola del LED\r\n");
		return false;
	}

	uart_log("LED - Evento led_event agregado a cola del LED\r\n");
	return true;
}

void led_process_event(led_t *led)
{
	led_event_t *led_event;

	leds_off(); // Apagar todos los LEDs al inicio

	for (int i = 0; i < NUM_LEDS; i++)
	{
		if (led[i].queue != NULL)
		{
			if (xQueueReceive(led[i].queue, (void *)&led_event, 0) == pdTRUE)
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
				if (led_event->callback_process_completed != NULL)
				{
					led_event->callback_process_completed(led_event);
				}
				else
				{
					uart_log("LED - led_event callback vacio\r\n");
				}
			}
			
			vQueueDelete(led[i].queue);
			led[i].queue = NULL;
			uart_log("LED - Cola de led eliminada\r\n");
		}
	}
}

