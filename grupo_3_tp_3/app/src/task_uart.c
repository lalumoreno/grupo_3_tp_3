#include "task_uart.h"
#include "main.h"
#include "cmsis_os.h"
#include <string.h>
#include <stdio.h>
#include "board.h"

#define TASK_UART_STACK_SIZE 128
#define TASK_UART_PRIORITY tskIDLE_PRIORITY
#define UART_QUEUE_LENGTH 50

extern UART_HandleTypeDef huart2;

static QueueHandle_t uart_queue = NULL;

static void uart_handle_error(const char *error_msg);
static void uart_task(void *argument);
static bool uart_queue_init(void);

static void uart_task(void *argument) {
    char *msg;

    while (1) {
        if (xQueueReceive(uart_queue, &msg, portMAX_DELAY) == pdTRUE) {
            HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
            vPortFree(msg);  // Liberar memoria después de enviar el mensaje
        }
    }
}

static bool uart_queue_init(void) {
    if (uart_queue != NULL) {
        uart_log("URT - Cola uart_queue ya inicializada\r\n");
        return true;  // Ya está inicializada
    }

    uart_queue = xQueueCreate(UART_QUEUE_LENGTH, sizeof(char *));
    if (uart_queue == NULL) {
        return false;
    }

    uart_log("URT - Cola uart_queue creada\r\n");
    return true;
}

bool uart_task_init(void) {

    if(!uart_queue_init()) {
        return false;
    }

    // Crear tarea UART
    BaseType_t status = xTaskCreate(uart_task, "uart_task", TASK_UART_STACK_SIZE, NULL, TASK_UART_PRIORITY, NULL);
    if (status != pdPASS) {
        return false;  // Error al crear la tarea
    }

    uart_log("URT - Tarea uart_task creada \r\n");
    return true;
}

void uart_log(const char *msg) {
    if (msg == NULL || uart_queue == NULL) {
        return;
    }

    size_t len = strnlen(msg, UART_MSG_MAX_LEN - 1);
    char *copy = pvPortMalloc(len + 1);
    if (copy == NULL) {
        return;
    }

    strncpy(copy, msg, len);
    copy[len] = '\0';

    BaseType_t sent = xQueueSend(uart_queue, &copy, pdMS_TO_TICKS(100));
    if (sent != pdPASS) {
        vPortFree(copy);  // Liberar memoria si no se pudo enviar
    }
}
