#include "queue.h"

int queue_count(queue_t *queue) {
    int count = 0;
    if (queue_empty(queue)) return count;
    for (queue_t *q = queue_next(queue); q != queue; q = queue_next(q)) count++;
    return count;
}
