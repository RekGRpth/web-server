/* Copyright (c) 2013, Ben Noordhuis <info@bnoordhuis.nl>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <stddef.h>

typedef void *queue_t[2];

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
      queue_t* q = queue_head(h);                                               \
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
  }                                                                           \
  while (0)

#endif // _QUEUE_H_
