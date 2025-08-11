#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include <stdint.h>
#include <stdbool.h>

/* Prioridades según el enunciado */
typedef enum {
    PRIORIDAD_BAJA = 0,
    PRIORIDAD_MEDIA,
    PRIORIDAD_ALTA,
    PRIORIDAD_NONE
} priority_t;

/* Estructura del mensaje */
typedef struct {
    priority_t priority;
    uint32_t timestamp;
} pq_item_t;

/* API del driver */
bool pq_init(uint32_t max_items);
bool pq_push(const pq_item_t *item);
bool pq_pop(pq_item_t *out_item);
bool pq_is_empty(void);
void pq_deinit(void);

#endif // PRIORITY_QUEUE_H
