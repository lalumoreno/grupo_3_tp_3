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

bool led_queue_add(led_event_t *event);
bool led_task_init();

#endif /* INC_TASK_LED_H_ */
