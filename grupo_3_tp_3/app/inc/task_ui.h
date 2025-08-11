#ifndef INC_TASK_UI_H_
#define INC_TASK_UI_H_

#include "task_button.h"  // para button_type_t
#include <stdbool.h>

/* Declaración de la tarea de Interfaz de Usuario */
bool ui_task_init();
bool ui_queue_send(button_event *event);

#endif /* INC_TASK_UI_H_ */
