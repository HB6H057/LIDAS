# include "global.h"
# include "memory.h"
# include "queue.h"

void queue_init (struct lida_queue_h **h, int max, int fd)
{
   *h = fd?mem_pool_alloc (sizeof (struct lida_queue_h), fd):malloc (sizeof (struct lida_queue_h));
   (*h)->max = max;
   (*h)->head = 0;
   (*h)->tail = 0;
   (*h)->data = fd?mem_pool_alloc (sizeof(void *)*max, fd):calloc (max, sizeof (void *));
}


int queue_in (struct lida_queue_h *h, void *d)
{
   if (Q_IS_FULL(h))
      return -1;

   *(h->data + h->head) = d;

   if (h->head == (h->max - 1))
      h->head = 0;
   else h->head ++;
   
   return 0;
}


void *queue_out (struct lida_queue_h *h)
{
   if (h->tail == h->head)
      return NULL;

   void *ret = *(h->data + h->tail);

   if (h->tail == (h->max - 1))
      h->tail = 0;
   else h->tail++;
   
   return (ret);
}
