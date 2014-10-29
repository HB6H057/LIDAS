# ifndef TIMER_H_
# define TIMER_H_

# include "stack.c"

struct timer_t
{
   int fd;
   int pfd;

   time_t time;
   pthread_t thread_id;
   struct mem_pool_head *pool;
   struct buf_log_h *log_pool;
};

struct timer_opt
{
   struct lida_queue_h *t;
   struct mem_cache_head *pool;
};

struct timer_pool_t
{
   time_t time;
   struct stack_t list;
};

extern truct timer_t **timer_data_lst;
extern struct mem_cache_head *timer_data_pool;

int timer_insert (struct lida_queue_h *, int, int, struct mem_cache_head *, struct sockaddr_in *);
void timer_cleaner (int, struct mem_cache_head *);
void timer_task (struct lida_queue_h *, time_t, struct mem_cache_head *);

# define HUGE_TIMER 1000

# endif
