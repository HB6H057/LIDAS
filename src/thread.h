# ifndef THREAD_H_
# define THREAD_H_

struct thread_data_t
{
   pthread_t id;
   unsigned tmp_flag;
   struct thread_pool_h *master;
   struct event_data *data;
};

struct thread_pool_h
{
   struct queue_chain_ctl *h;
   struct mem_cache_head *pool;
   struct mem_cache_head *q_pool;
};

extern unsigned timer_flag;
extern pthread_mutex_t thread_mutex;
extern struct thread_pool_h *thread_pool;

int thread_pool_init (int, struct thread_pool_h *);
void thread_key_init (void);
void *thread_trigger (void *);
void thread_reset (int);
pthread_t thread_pool_alloc (struct thread_pool_h *, struct event_data *);
pthread_t tmp_thread_port (struct event_data *);
void func_redirect (void *);
int thread_event_wait (int, int, struct event_data *);

# endif
