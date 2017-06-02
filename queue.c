#include "queue.h"
#include "macros.h"

int queue_count(queue_t *queue) {
//    DEBUG("queue=%p\n", queue);
    int count = 0;
    if (queue_empty(queue)) return count;
    for (queue_t *q = queue_next(queue); q != queue; q = queue_next(q), count++);// DEBUG("count=%i\n", count);
    return count;
}