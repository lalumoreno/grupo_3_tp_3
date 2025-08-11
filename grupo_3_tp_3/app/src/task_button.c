#include "cmsis_os.h"
#include "main.h"
#include "task_button.h"
#include "task_uart.h"
#include "board.h"
#include "task_ui.h"
#include "task_led.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#define TASK_BUTTON_STACK_SIZE 128
#define TASK_BUTTON_PRIORITY (tskIDLE_PRIORITY + 2)
#define TASK_PERIOD_MS 50
#define BUTTON_PULSE_TIMEOUT 200
#define BUTTON_SHORT_TIMEOUT 1000
#define BUTTON_LONG_TIMEOUT 2000

/* Estados internos del botón */
typedef enum
{
	BUTTON_STATE_IDLE,
	BUTTON_STATE_PRESSED
} button_state_t;

typedef struct
{
	uint32_t counter;
	button_state_t state;
} button_info_t;

TaskHandle_t task_button_handle = NULL;

static button_info_t button_info;

static void button_task(void *argument);
static button_event button_process_state(bool is_pressed);
static void callback_process_completed(void *context);
static bool is_button_pressed(void);

/* Clasifica la pulsación */
static button_event button_process_state(bool is_pressed)
{

	button_event event = {BUTTON_TYPE_NONE, 0};

	if (is_pressed)
	{
		uart_log("BTN - Estado del boton: PRESIONADO\r\n");
		button_info.counter += TASK_PERIOD_MS;
		button_info.state = BUTTON_STATE_PRESSED;
	}
	else
	{
		if (button_info.state == BUTTON_STATE_PRESSED)
		{
			event.duration = button_info.counter;

			if (button_info.counter >= BUTTON_LONG_TIMEOUT)
			{
				event.type = BUTTON_TYPE_LONG;
			}
			else if (button_info.counter >= BUTTON_SHORT_TIMEOUT)
			{
				event.type = BUTTON_TYPE_SHORT;
			}
			else if (button_info.counter >= BUTTON_PULSE_TIMEOUT)
			{
				event.type = BUTTON_TYPE_PULSE;
			}
			else
			{
				event.type = BUTTON_TYPE_NONE;
			}
		}

		button_info.counter = 0;
		button_info.state = BUTTON_STATE_IDLE;
	}

	return event;
}

static void callback_process_completed(void *context)
{
	button_event *event = (button_event *)context;
	vPortFree(event);
	//uart_log("BTN - Memoria bnt_event liberada desde callback\r\n");
}

// Leer el estado del botón
static bool is_button_pressed(void)
{
	GPIO_PinState state = HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN);
#ifdef _F429ZI_
	return (state == GPIO_PIN_SET);
#else
	return (state == GPIO_PIN_RESET);
#endif
}

/* Tarea del botón (modo polling) */
static void button_task(void *argument)
{
	button_info.counter = 0;
	button_info.state = BUTTON_STATE_IDLE;

	while (1)
	{
		bool is_pressed = is_button_pressed();
		button_event temp_event = button_process_state(is_pressed); // TODO add event as parameter

		if (temp_event.type != BUTTON_TYPE_NONE)
		{
			// Asignar memoria para el evento
			button_event *bnt_event = (button_event *)pvPortMalloc(
				sizeof(button_event));

			if (bnt_event != NULL)
			{
				//uart_log("BTN - Memoria bnt_event alocada\r\n");

				*bnt_event = temp_event;
				bnt_event->callback_process_completed = callback_process_completed;
				bnt_event->callback_context = bnt_event;

				// Enviar evento a la cola
				if (!ui_queue_send(bnt_event))
				{
					vPortFree(bnt_event); // Liberar si no se pudo enviar
					//uart_log("BTN - Memoria de bnt_event liberada \r\n");
				}
			}
			else
			{
				uart_log("BTN - Memoria insuficiente\r\n");
			}
		}

		vTaskDelay(pdMS_TO_TICKS(TASK_PERIOD_MS));
	}
}

void button_task_init(void)
{
	if(task_button_handle != NULL)
	{
		uart_log("BTN - Tarea button_task ya inicializada\r\n");
		return; // Ya está creada
	}

	BaseType_t status = xTaskCreate(button_task, "button_task", TASK_BUTTON_STACK_SIZE, NULL, TASK_BUTTON_PRIORITY, &task_button_handle);
	configASSERT(status == pdPASS);
	if (status != pdPASS)
	{
		uart_log("BTN - Error: No se pudo crear la tarea Button\r\n");
	}
	else
	{
		uart_log("BTN - Tarea button_task creada \r\n");
	}
}