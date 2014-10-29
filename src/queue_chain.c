# include "global.h"
# include "timer.h"
# include "memory.h"
# include "queue_chain.h"


void queue_chain_in (void *data, struct queue_chain_ctl *opt, struct mem_cache_head *pool)
{
   /*First add*/
   if (opt->head == NULL){
      opt->head = (pool == NULL)?malloc (sizeof (struct queue_chain_t)): mem_cache_alloc (pool);
      opt->tail = opt->head;

      opt->head->prev = NULL;
      opt->head->next = NULL;
      opt->head->data = data;

      opt->cnt = 1;
   }

   else {
      struct queue_chain_t *p = (pool == NULL)?malloc (sizeof (struct queue_chain_t)): mem_cache_alloc (pool);
      p->prev = opt->head;
      p->next = NULL;
      p->data = data;

      opt->head->next = p;
      opt->head = p;
      opt->cnt++;
   }
}


void *queue_chain_out (struct queue_chain_ctl *opt, struct mem_cache_head *pool)
{
   if ((opt->tail == NULL) || !opt->cnt)
      return NULL;

   opt->cnt--;
   void *ret = opt->tail->data;

   if ((opt->tail = opt->tail->next) == NULL)
      opt->head = NULL;

   if (pool == NULL)
      free (opt->tail->prev);
   else mem_cache_free (opt->tail->prev, pool);

   opt->tail->prev = NULL;
   return (ret);
}
