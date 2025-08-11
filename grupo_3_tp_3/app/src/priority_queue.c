#include "priority_queue.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task_uart.h"

typedef struct {
    pq_item_t *buffer;
    uint32_t capacity;
    uint32_t count;
    uint32_t head;
    uint32_t tail;
    uint32_t timestamp_counter;
    SemaphoreHandle_t mutex;
} pq_control_t;

static pq_control_t pq;

bool pq_init(uint32_t max_items) {

    if(pq.buffer) {
        uart_log("PRQ - Cola de prioridad ya inicializada\r\n");
        return true; // Ya está inicializada
    }

    if (max_items <= 0) {
        uart_log("PRQ - Tamaño de cola inválido\r\n");
        return false;
    }

    pq.buffer = pvPortMalloc(sizeof(pq_item_t) * max_items);
    if (!pq.buffer) return false;

    pq.capacity = max_items;
    pq.count = 0;
    pq.head = 0;
    pq.tail = 0;
    pq.timestamp_counter = 0;

    pq.mutex = xSemaphoreCreateMutex();
    uart_log("PRQ - Cola de prioridad inicializada\r\n");
    return (pq.mutex != NULL);
}

bool pq_push(const pq_item_t *item) {
    if (!item) return false;
    xSemaphoreTake(pq.mutex, portMAX_DELAY);

    pq_item_t new_item = *item;
    new_item.timestamp = pq.timestamp_counter++;

    if (pq.count >= pq.capacity) {
        uart_log("PRQ - Cola llena, descartando el elemento más antiguo\r\n");
        // Descartar el más antiguo
        pq.head = (pq.head + 1) % pq.capacity;
        pq.count--;
    }

    pq.buffer[pq.tail] = new_item;
    pq.tail = (pq.tail + 1) % pq.capacity;
    pq.count++;

    xSemaphoreGive(pq.mutex);

    uart_log("PRQ - Elemento añadido a la cola de prioridades\r\n");
    return true;
}

bool pq_pop(pq_item_t *out_item) {
    if (!out_item) return false;
    xSemaphoreTake(pq.mutex, portMAX_DELAY);

    if (pq.count == 0) {
        xSemaphoreGive(pq.mutex);
        return false;
    }

    // Buscar elemento más prioritario y más antiguo
    int index = -1;
    for (uint32_t i = 0; i < pq.count; i++) {
        uint32_t idx = (pq.head + i) % pq.capacity;
        if (index == -1) {
            index = idx;
        } else {
            if (pq.buffer[idx].priority > pq.buffer[index].priority ||
               (pq.buffer[idx].priority == pq.buffer[index].priority &&
                pq.buffer[idx].timestamp < pq.buffer[index].timestamp)) {
                index = idx;
            }
        }
    }

    *out_item = pq.buffer[index];

    // Compactar cola (eliminar hueco)
    for (uint32_t i = index; i != pq.tail; i = (i + 1) % pq.capacity) {
        uint32_t next = (i + 1) % pq.capacity;
        pq.buffer[i] = pq.buffer[next];
    }

    pq.tail = (pq.tail + pq.capacity - 1) % pq.capacity;
    pq.count--;

    xSemaphoreGive(pq.mutex);

    uart_log("PRQ - Elemento obtenido de la cola de prioridades\r\n");
    return true;
}

bool pq_is_empty(void) {
    xSemaphoreTake(pq.mutex, portMAX_DELAY);
    bool empty = (pq.count == 0);
    xSemaphoreGive(pq.mutex);

    if (empty)  {
        uart_log("PRQ - Cola de prioridades vacía\r\n");
    } else {
        uart_log("PRQ - Cola de prioridades no está vacía\r\n");
    }

    return empty;
}

void pq_deinit(void) {
    vPortFree(pq.buffer);
    vSemaphoreDelete(pq.mutex);
}
