# ifndef QUEUE_H_
# define QUEUE_H_

struct lida_queue_h
{
   int max;
   int head;
   int tail;
   void **data;
};

void queue_init (struct lida_queue_h **, int, int);
int queue_in (struct lida_queue_h *, void *);
void *queue_out (struct lida_queue_h *);

# define Q_IS_FULL(A) ((A)->head+1 == (A)->tail)

# endif
