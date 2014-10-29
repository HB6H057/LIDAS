# include "main.h"
# include "queue.h"

int timer_init (struct timer_head *h)
{
   if ((h->b = calloc (true_type.connect, sizeof (lida_timer_t)) == NULL);
      return ERROR;
      
   h->max = true_type.connect - 1;
   return TRUE;
}


int lida_timer_add (struct timer_head *h, time_t s, int fd)
{
   time_t time;
   unsigned a = 0, b = 0;
   
   /*The first client socket*/
   if (h->head == h->tail)
   {
      h->head++;
      h->b->data = fd;
      h->b->real = time (&t);
      h->b->delta = listen_queue.b->real;
      
      alarm (s);
      return 1;
   }
   
   switch (h->head)
   {
      case (h->tail-1): {
         lidalog (LIDA_ACCESS, "Listen queue is full");
         return -1;
      }
      
      case (h->max): {
         if (h->tail == 0){
            lidalog (LIDA_ACCESS, "Listen queue is full");
            return -1;
         }
         else {
            a = 1;
            break;
         }
      }
      
      case (0): b = 1;
   }
   
   (h->b + h->head)->data = fd;
   (h->b + h->head)->real = time (&t);
   
   if (b)
   {
      (h->b + h->head)->delta = (h->b + h->head)->real - (h->b + h->max)->real;
      b = 0;
   }
   
   else (h->b + h->head)->delta = (h->b + h->head)->real - (h->b + h->head - 1)->real;
   
   if (a)
      h->head = 0;
   else h->head++;
   
   return 1;
}


void lida_timer_del (struct timer_head *h, void (*cleaner) (uint64_t d))
{
   (*cleaner) (h->b->data);
   
   if (h->tail == h->max)
      h->tail = 0;
      
   else h->tail++;
   
   if (h->tail == h->head)
      return;
   
   if ((h->b + h->tail)->delta == 0)
      lida_timer_del (h, cleaner);
      
   else alarm ((h->b + h->tail)->delta);
}


void lida_timer_stack_del (struct timer_head *h)
{
   if (h->head == 0)
      h->head = h->max;
   
   else h->head--;
}


int timer_is_full (struct timer_head *h)
{
   switch (h->head)
   {
      case (h->tail - 1): return 0;
      case (h->max): {
         if (h->tail == 0)
            return 0;
      }
      default : return 1;
   }
}
