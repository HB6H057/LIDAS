# include "listener.h"

struct timer_t 
{
   struct event_data *data;
   time_t time;
   struct timer_t next, prev;
};

struct timer_head
{
   int max;
   struct timer_t head, tail;
};
