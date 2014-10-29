# ifndef STACK_C_
# define STACK_C_

struct stack_t
{
   void **data;
   int cnt;
};


void stack_init (struct stack_t **h, int max)
{
   *h = malloc (sizeof (struct stack_t));
   (*h)->data = calloc (max, sizeof (void *));
   (*h)->cnt = 0;
}


int stack_push (struct stack_t *h, void *d)
{
   if (h == NULL)
      return -1;

   *(h->data + h->cnt) = d;
   h->cnt++;
   return 0;
}


void *stack_pop (struct stack_t *h)
{
   if ((h == NULL) || (h->cnt == 0))
      return NULL;

   h->cnt--;
   return (*(h->data + h->cnt));
}

# endif