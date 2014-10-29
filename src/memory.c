# include "global.h"
# include "main.h"
# include "timer.h"
# include "listener.h"
# include "memory.h"

static struct mem_pool_head **pool_lst;


int mem_cache_creat (size_t size, struct mem_cache_head **p, int n)
{
   *p = malloc (sizeof (struct mem_cache_head) + n*size);
   
   /*Init head of cache block*/
   (*p)->size = size;
   (*p)->num = n;

   return (mem_cache_locate (*p, (*p) + sizeof(struct mem_cache_head), size));
}


int mem_cache_locate (struct mem_cache_head *head, void *addr, size_t size)
{
   int num = head->num;
   head->next = (struct mem_cache *)addr;
   struct mem_cache *p = (struct mem_cache *)addr, t;

   t.next = (void *)p + size;
   t.prev = NULL;
   memcpy (p, &t, sizeof (struct mem_cache));
   p = p->next;

   while (num--)
   {
      t.next = (void *)p + size;
      t.prev = (void *)p - size;
      memcpy (p, &t, sizeof (struct mem_cache));
      p = p->next;
   }

   return (head->num);
}


void *mem_cache_alloc (struct mem_cache_head *p)
{
   if (p == NULL)
      return NULL;
      
   struct mem_cache_head *m;
   struct mem_cache *a;
   
   if (!(p->num))
      return NULL;

   /*Update memory chain*/
   a = p->next;
   p->next = a->next;
   a->next->prev = (struct mem_cache *)p;
   p->num --;

   /*Clean the space of ptr area*/
   memset ((void *)a, 0, p->size);
   return ((void *)a);
}


void mem_cache_free (void *g, struct mem_cache_head *p)
{
   struct mem_cache t;

   /*Add freed space to void chain*/
   t.next = p->next;
   t.prev = (struct mem_cache *)p;
   memcpy (g, (void *)&t, sizeof (t));
   p->next = (struct mem_cache *)g;
   p->num++;
}


struct mem_pool_head *mem_pool_creat (int fd)
{
   struct mem_pool_head t, **h = &((*(timer_data_lst + fd))->pool);
   
   if ((*h = (struct mem_pool_head *)malloc (PAGE_SIZ)) == NULL)
      return NULL;

   /*Init head info for this pool block*/
   t.start = (void *)*h + sizeof (struct mem_pool_head);
   t.end = (void *)*h + PAGE_SIZ;
   t.next = NULL;
   t.extra = NULL;
   t.tail = NULL;
   memcpy ((void *)*h, (void *)&t, sizeof (struct mem_pool_head));

   return (*h);
}


struct mem_pool_head *mem_pool_extend ()
{
   struct mem_pool_head t, *ret = (struct mem_pool_head *)malloc (PAGE_SIZ);
   
   /*Init head info for this pool block*/
   t.start = (void *)ret + sizeof (struct mem_pool_head);
   t.end = (void *)ret + PAGE_SIZ;
   t.next = NULL;
   t.extra = NULL;
   t.tail = NULL;
   memcpy ((void *)ret, (void *)&t, sizeof (struct mem_pool_head));
   
   return (ret);
}


void *mem_pool_alloc (size_t size, int fd)
{
   void *ret;
   struct mem_pool_head *new, *p = (*(timer_data_lst + fd))->pool;
   
   /*Alloc large space*/
   if (size > POOL_SIZ)
   {
      ret = malloc (size);
      mem_pool_add (ret, size, fd);
      return ret;
   }

   /*Browse to find avalible block*/
   for (; p != NULL; p = p->next)
   {
      if ((p->end - p->start) >= size)
         break;
   }

   /*Not enough space, we need to alloc more*/
   if (p == NULL)
   {
      new = mem_pool_extend ();
      p->next = new;
      p = p->next;
   }

   /*Return the address*/
   ret = p->start;
   p->start += size;
   return ret;
}


void mem_pool_destroy (struct mem_pool_head *p)
{
   struct mem_pool_large *s;
   
   /*Free large spaces first*/
   for (s=p->extra; s!=NULL; s=s->next)
      free (s->start);

   if (p->next != NULL)
      mem_pool_destroy (p->next);

   free ((void *)p);
}


void mem_pool_add (void *p, size_t size, int fd)
{
   struct mem_pool_head *s = (*(timer_data_lst + fd))->pool;
   struct mem_pool_large *new;
   
   if (s->extra == NULL){
      s->extra = (struct mem_pool_large *)mem_pool_alloc (sizeof (struct mem_pool_large), fd);
      new = s->extra;
      s->tail = s->extra;
   }
   
   else {
      new = (struct mem_pool_large *)mem_pool_alloc (sizeof (struct mem_pool_large), fd);
      s->tail->next = new;
      s->tail = s->tail->next;
   }
   
   new->start = p;
   new->end = p + size;
}


/*struct mem_cache_head *mem_shared_init (size_t size, int n, int id, void *addr)
{
   struct mem_cache_head *pool;

   addr = shmat (id, addr, 0);

   if (addr == (void *)-1)
      return NULL;

   pool = (struct mem_pool_head *)addr;
   pool->num = n;
   pool->size = size;
   pool->block_next = NULL;
   mem_cache_locate (pool, addr + sizeof(struct mem_cache_head), size);

   return pool;
}*/



