# include "global.h"
# include "log.h"
# include "memory.c"
# include "queue.h"
# include "timer.h"
# include "listener.h"

struct timer_t **timer_data_lst;
struct mem_cache_head *timer_data_pool;

int timer_insert (struct lida_queue_h *h, int fd, int pfd, 
                   struct mem_cache_head *pool, struct sockaddr_in *opt)
{
   struct timer_t *p = (struct timer_t *)mem_cache_alloc (pool);

   p->fd = fd;
   p->pfd = pfd;
   p->time = time (&(p->time));
   
   /*Init memory pool and log pool*/
   p->pool = mem_pool_creat (fd);
   p->log_pool = buf_log_init (opt, fd);

   if (queue_in (h, (void *)p) < 0){
      lidalog (LIDA_ACCESS, "listen queue is full");
      return (-1);
   }

   *(timer_data_lst + fd) = p;
   return 0;
}


void timer_cleaner (int fd, struct mem_cache_head *pool)
{
   struct timer_t *p = *(timer_data_lst + fd);
   
   /*Delete possible proxy event*/
   if (p->pfd)
      epoll_ctl (epfd, EPOLL_CTL_DEL, p->pfd, NULL);
   
   /*Reset Handler thread*/
   buf_log_destroy (p->log_pool);
   pthread_kill (p->thread_id, SIGHUP);
   
   /*Release memory pool*/
   if (p->pool != NULL)
      mem_pool_destroy (p->pool);
   
   *(timer_data_lst + fd) = NULL;
   mem_cache_free (p, pool);
   
   if (epoll_ctl (epfd, EPOLL_CTL_DEL, fd, NULL) < 0)
      lidalog (LIDA_ERR, "epoll_ctl");
}


void timer_task (struct lida_queue_h *h, time_t cur_time, struct mem_cache_head *pool)
{
   struct timer_t *p = (struct timer_t *)queue_out (h);
   
   /*The timer struct could be freed before*/
   if (p == NULL)
      goto CONT;

   if ((cur_time - p->time) > true_type.keep_alive){
      timer_cleaner (p->fd, pool);
      
      if ((h->head == h->tail) && listen_del_flag)
         exit (0);
   
   CONT:
      timer_task (h, cur_time, pool);
   }
   else queue_in (h, p);
}
