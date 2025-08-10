#ifndef INC_TASK_UI_H_
#define INC_TASK_UI_H_

#include "task_button.h"  // para button_type_t
#include <stdbool.h>

/* Declaración de la tarea de Interfaz de Usuario */
void ui_task_create(int event_type);
bool ui_queue_send(button_event *event);
void ui_queue_init();

#endif /* INC_TASK_UI_H_ */
