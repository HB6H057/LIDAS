# include "main.h"
# include "timer.h"

int lida_timer_add (struct mem_cache_head *p, struct timer_head *h, struct event_data *d)
{
   if (!h->max){
      lidalog (LIDA_ACCESS, "timer queue is full");
      return -1;
   }

   struct timer_t *t = mem_cache_alloc (p);

   /*If failed we alloc memory directly*/
   if (t == NULL)
      t = malloc (malloc (sizeof (struct timer_t)));

   t->data = d;
   t->time = time (&(t->time));
   t->next = NULL;
   t->prev = (h->head == NULL)?(h->head = mem_cache_alloc (p), h->tail = h->head):h;

   h->head->next = t;
   h->head = t;

   h->max--;

   return 1;
}


void lida_timer_del (struct timer_head *h, struct mem_cache_head *p, 
                     time_t t, void (*cleaner) (struct event_data *))
{
   if ((t - h->tail->time) < true_type.keep_alive)
      return;

   (*cleaner) (h->tail->data, p);

   h->tail = h->tail->next;
   mem_cache_free (h->tail->prev, p);

   if (h->tail == NULL){
      h->max++;
      return;
   }

   else lida_timer_del (h, p, t, cleaner);
}


void timer_event_del (struct timer_head *h, int fd, int pfd, void (*cleaner) (struct event_data *))
{
   struct timer_t *i;
   
   for (i = h->tail; i != h->head; i = i->next)
   {
      if (fd == i->data->fd || pfd == i->data->pfd)
      {
         (*cleaner) (i->data);
         h->max++;
         
         return;
      }
   }
}


void lida_timer_stack_del (struct timer_head *h, struct mem_cache_head *p)
{
   h->head = h->head->prev;
   mem_cache_free (h->head->next);
}
