#ifndef INC_TASK_LED_H_
#define INC_TASK_LED_H_

#include "cmsis_os.h"
#include <stdbool.h>

typedef enum {
	LED_EVENT_RED, LED_EVENT_GREEN, LED_EVENT_BLUE
} led_event_type_t;

typedef void (*callback_t)(void *context);

typedef struct {
	led_event_type_t type;
	callback_t callback_process_completed;
	void *callback_context;
} led_event_t;

typedef struct {
	QueueHandle_t queue;
} led_t;

#define NUM_LEDS 3

/* Declaraciones de las tareas para los LEDs */
void led_queue_init(led_t *led);
void led_process_event(led_t *led);
bool led_queue_add(led_t *leds, led_event_type_t event_type, led_event_t *event);

#endif /* INC_TASK_LED_H_ */
