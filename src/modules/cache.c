# include "cache.h"
# include "thread.h"

int msg_id;
struct stack_t *msg_thread;
struct rbtree_t *mem_cache_s;
struct rbtree_t *disk_cache_s;


void cache_proc (pid_t m_pid)
{
      
}


struct cache_info_t *cache_search (int type, char *host, char *query)
{
   struct rbtree_t *obj;
   char str[strlen(host) + strlen(query)];
   sprintf (str, "%s%s", host, query);

   switch (type){
      case MEM_CACHE: obj = mem_cache_obj; break;     
      case DISK_CACHE: obj = disk_cache_obj; break;
      default: return NULL;
   }

   return ((struct cache_info_t *)rbtree_search (crc32_port (str), obj, 0));
}


int cache_to_client (int fd, struct cache_chain_t *buf, struct event_data *obj)
{
   void *ptr;
   size_t size, rest;

   do{
      ptr = buf + sizeof (struct cache_chain_t);
      rest = CACHE_BUFSIZ;

   WR_START:
      while (((size = write (fd, ptr, rest)) >= 0) && rest)
      {
         ptr+=size;
	 rest-=size;
      }

      /*Might be blocked*/
      if (size < 0){
         if (errno == EAGAIN){
	    obj->type = COMMON_WREVENT;

	    if (thread_event_wait (EPOLLOUT, fd, obj) == 0)
	       goto WR_START;
	    else return -1;
	 }
	 else return -1;
      }
   }while ((buf = buf->next) != NULL);
}


void mem_cache_destroy (struct cache_chain_t *head)
{
   if (head->next != NULL)
      mem_cache_destroy (head->next);

   mem_cache_free ((void *)head, cache_shm_pool);
}


void disk_cache_destroy (struct cache_chain_t *head)
{
   if (head->next != NULL)
      mem_cache_destroy (head->next);

   disk_pool_free ((void *)head, cache_shd_pool);
}


void disk_cache_destroy (struct cache_chain_t *head)
{
   if (head->next != NULL)
      mem_cache_destroy (head->next);

   disk__free ();
}


int file_cache_to_client (int fd, char *file, size_t size, struct event_data *obj)
{
   FILE *fp;
   size_t rest = size, sur_siz;
   int num = size/CACHE_BUFSIZ + 1, ret, cnt = 0;
   struct cache_chain_t *tmp, *prev, *head;
   void *ptr = malloc (4096);

   /*Init cache infomation*/
   struct cache_info_t *new = (struct cache_info_t *)mem_cache_alloc (cache_info_pool);

   if (new == NULL){
      cnt = 0;
      goto POST_START;
   }

   new->local = 1;
   new->remote = 0;
   new->time = time (&(new->time));
   new->query = file;
   new->next = prev;
   new->size = size;

   if ((prev = (struct cache_chain_t *)mem_cache_alloc (cache_shm_pool)) == NULL){
      cnt = 0;
      goto POST_STRAT;
   }
   
   head = prev;
   prev->addr = (void *)tmp + sizeof (struct cache_chain_t);

   /*Pre alloc cache chain*/
   while (--num)
   {
      pthread_mutex_lock (&thread_lock);

      if ((tmp = (struct cache_chain_t *)mem_cache_alloc (cache_shm_pool)) == NULL){
         pthread_mutex_unlock (&thread_lock);
	 mem_cache_free ((void *)new, cache_info_pool);
	 mem_cache_destroy (head);
	 return NON_CACHE_MEM;
      }

      pthread_mutex_unlock (&thread_lock);
      
      prev->next = tmp;
      tmp->prev = prev;
      tmp->addr = (void *)tmp + sizeof (struct cache_chain_t);
      prev = tmp;
   }

POST_START:
   if ((fp = fopen (file, "r")) == NULL)
      return CACHE_CLIENT_ERR;

   while (!feof (fp))
   {
      cur_siz = fread ((cnt--)?head->addr:, CACHE_BUFSIZ, 1, fp);
      rest-=cur_siz;

      if (rest <= 0)
         break;
      
      head = head->next;
   }

   /*Add cached space to rbtree*/
   rbtree_insert (&mem_cache_obj, crc32_port (file), new, 
                  rbt_cache_pool, RBT_MEM_CACHE);
   fclose (fp);
   return 0;
}
