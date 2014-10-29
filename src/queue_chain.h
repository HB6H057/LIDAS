# ifndef QUEUE_H_
# define QUEUE_H_

struct queue_chain_t
{
   struct queue_chain_t *prev;
   struct queue_chain_t *next;

   void *data;
};

struct queue_chain_ctl
{
   struct queue_chain_t *head;
   struct queue_chain_t *tail;
   
   int cnt;
};

void queue_chain_in (void *, struct queue_chain_ctl *, struct mem_cache_head *);
void *queue_chain_out (struct queue_chain_ctl *, struct mem_cache_head *);

# endif
