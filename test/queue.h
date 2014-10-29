typedef struct timer_body
{
   uint64_t data;
   time_t delta, real;
}lida_timer_t;  

struct timer_head
{
   lida_timer_t *b;
   int head, tail, max;
}; 
