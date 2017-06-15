#ifndef _QUEUE_H_
#define _QUEUE_H_

//#include <stddef.h>

typedef void *queue_t[2];
int queue_count(queue_t *queue);

#define pointer_t queue_t
#define pointer_remove queue_remove
#define queue_put_pointer queue_insert_tail
#define pointer_init queue_init
#define pointer_data queue_data
#define queue_get_pointer queue_head

/* Private macros. */
#define queue_next(q)       (*(queue_t **) &((*(q))[0]))
#define queue_prev(q)       (*(queue_t **) &((*(q))[1]))
#define queue_prev_next(q)  (queue_next(queue_prev(q)))
#define queue_next_prev(q)  (queue_prev(queue_next(q)))

/* Public macros. */
#define queue_data(ptr, type, field)                                          \
  ((type *) ((char *) (ptr) - offsetof(type, field)))

/* Important note: mutating the list while queue_foreach is
 * iterating over its elements results in undefined behavior.
 */
#define queue_foreach(q, h)                                                   \
  for ((q) = queue_next(h); (q) != (h); (q) = queue_next(q))

#define queue_empty(q)                                                        \
  ((const queue_t *) (q) == (const queue_t *) queue_next(q))

#define queue_head(q)                                                         \
  (queue_next(q))

#define queue_init(q)                                                         \
  do {                                                                        \
    queue_next(q) = (q);                                                      \
    queue_prev(q) = (q);                                                      \
  }                                                                           \
  while (0)

#define queue_add(h, n)                                                       \
  do {                                                                        \
    queue_prev_next(h) = queue_next(n);                                       \
    queue_next_prev(n) = queue_prev(h);                                       \
    queue_prev(h) = queue_prev(n);                                            \
    queue_prev_next(h) = (h);                                                 \
  }                                                                           \
  while (0)

#define queue_split(h, q, n)                                                  \
  do {                                                                        \
    queue_prev(n) = queue_prev(h);                                            \
    queue_prev_next(n) = (n);                                                 \
    queue_next(n) = (q);                                                      \
    queue_prev(h) = queue_prev(q);                                            \
    queue_prev_next(h) = (h);                                                 \
    queue_prev(q) = (n);                                                      \
  }                                                                           \
  while (0)

#define queue_move(h, n)                                                      \
  do {                                                                        \
    if (queue_empty(h))                                                       \
      queue_init(n);                                                          \
    else {                                                                    \
      queue_t* q = queue_head(h);                                             \
      queue_split(h, q, n);                                                   \
    }                                                                         \
  }                                                                           \
  while (0)

#define queue_insert_head(h, q)                                               \
  do {                                                                        \
    queue_next(q) = queue_next(h);                                            \
    queue_prev(q) = (h);                                                      \
    queue_next_prev(q) = (q);                                                 \
    queue_next(h) = (q);                                                      \
  }                                                                           \
  while (0)

#define queue_insert_tail(h, q)                                               \
  do {                                                                        \
    queue_next(q) = (h);                                                      \
    queue_prev(q) = queue_prev(h);                                            \
    queue_prev_next(q) = (q);                                                 \
    queue_prev(h) = (q);                                                      \
  }                                                                           \
  while (0)

#define queue_remove(q)                                                       \
  do {                                                                        \
    queue_prev_next(q) = queue_next(q);                                       \
    queue_next_prev(q) = queue_prev(q);                                       \
    queue_init(q);                                                            \
  }                                                                           \
  while (0)

#endif // _QUEUE_H_
