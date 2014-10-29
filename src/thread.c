# include "global.h"
# include "main.h"
# include "timer.h"
# include "listener.h"
# include "proxy.h"
# include "memory.h"
# include "thread.h"
# include "read_event.h"
# include "queue_chain.h"
# include "ghost.h"
# include "http_parser.h"
# include "snd.h"
# include "response.h"
# include "log.h"

# define HUGE_THREAD_POOL 

pthread_key_t thread_key;
static pthread_once_t init_done = PTHREAD_ONCE_INIT;

jmp_buf thread_env;
jmp_buf thread_init_env;
unsigned timer_flag;
pthread_mutex_t thread_mutex;
struct thread_pool_h *thread_pool;

int thread_pool_init (int cnt, struct thread_pool_h *ret)
{
   /*Init thread pool queue*/
   struct queue_chain_ctl *thread_pool_q = malloc (sizeof (struct queue_chain_ctl));
   thread_pool_q->head = NULL;
   thread_pool_q->tail = NULL;
   ret->h = thread_pool_q;

   struct thread_data_t *d;
   struct mem_cache_head *pool, *q_pool;
   int err;
   char msg[30];

   /*Init memory caches*/
   if (cnt > 500){
      mem_cache_creat (sizeof (struct queue_chain_t), &q_pool, cnt);
      mem_cache_creat (sizeof (struct thread_data_t), &pool, cnt);
      
      ret->pool = pool;
      ret->q_pool = q_pool;
   }

   /*Init thread mutex*/
   if (err = pthread_mutex_init (&thread_mutex, NULL)){
      lidalog (LIDA_ACCESS, strerror (err));
      return (-1);
   }

   while (cnt--)
   {
      d = (struct thread_data_t *)malloc (sizeof (struct thread_data_t));
      d->tmp_flag = 0;
      d->master = ret;

      if (err = pthread_create (&(d->id), NULL, thread_trigger, (void *)d)){
         lidalog (LIDA_ACCESS, strerror (err));
	 goto FAILED;
      }
      
      queue_chain_in ((void *)d, ret->h, ret->q_pool);
   }
   
   return (0);

FAILED:
   sprintf (msg, "Not enough thread created, rested %d", cnt);
   lidalog (LIDA_ACCESS, msg);
   return (-1);
}


void thread_key_init ()
{
   int err;

   while (err = pthread_key_create (&thread_key, NULL)){
      /*Wait for avalible resource*/
      if (err == EAGAIN)
         continue;
      else {
         lidalog (LIDA_ACCESS, "Memory error");
	 pthread_exit ((void *)-1);
      }
   }
}


void *thread_trigger (void *arg)
{
   int err;
   jmp_buf *p_env;

   /*Select signal mask*/
   sigset_t sigset;

   sigemptyset (&sigset);
   sigaddset (&sigset, SIGALRM);
   sigaddset (&sigset, SIGUSR1);
   sigaddset (&sigset, SIGUSR2);
   sigaddset (&sigset, SIGINT);
   sigaddset (&sigset, SIGCONT);
   sigaddset (&sigset, SIGCHLD);
   
   if (err = pthread_sigmask (SIG_BLOCK, &sigset, NULL)){
      lidalog (LIDA_ACCESS, strerror (err));
      pthread_exit ((void *)-1);
   }

   /*Set up signal handler*/
   struct sigaction act;

   act.sa_handler = thread_reset;

   if (sigaction (SIGHUP, &act, NULL) < 0){
      lidalog (LIDA_ERR, "sigaction");
      pthread_exit ((void *)-1);
   }

   /*Active thread data key*/
   pthread_once (&init_done, thread_key_init);

   while (err = pthread_setspecific (thread_key, (const void *)&thread_env)){
      if (err == ENOMEM)
         continue;
      else {
         lidalog (LIDA_ACCESS, "thread key error exiting");
	 pthread_exit ((void *)-1);
      }
   }

   if ((p_env = (jmp_buf *)pthread_getspecific (thread_key)) == NULL){
      err_log (LIDA_ACCESS, "Thread trigger init failed due to jmp_buf error", 
               ((struct thread_data_t *)arg)->data->fd);
      pthread_exit ((void *)-1);
   }

RESTART:
   /*Restart here*/
   setjmp (*p_env);

   /*Wait for action signals*/
   int sig;
   sigset_t wait_mask;

   sigemptyset (&wait_mask);
   sigaddset (&wait_mask, SIGCONT);
   sigaddset (&wait_mask, SIGINT);

   while (!(err = sigwait (&wait_mask, &sig))){
      switch (sig)
      {
         case SIGCONT: func_redirect (((struct thread_data_t *)arg)->data); goto THREAD_RET;
	 case SIGINT: {
	    mem_cache_free (arg, ((struct thread_data_t *)arg)->master->pool); 
	    raise (SIGCHLD);
	    pthread_exit ((void *)-1);
	 }
	 default: continue;
      }
   }

   lidalog (LIDA_ACCESS, strerror (err));
   pthread_exit ((void *)-1);
   
THREAD_RET:
   if (((struct thread_data_t *)arg)->tmp_flag)
      pthread_exit (NULL);
   else {
      queue_chain_in (arg, ((struct thread_data_t *)arg)->master->h, 
                      ((struct thread_data_t *)arg)->master->q_pool);
      goto RESTART;
   }
}


void thread_reset (int signo)
{
   jmp_buf *p_env = (jmp_buf *)pthread_getspecific (thread_key);

   if (p_env == NULL){
      lidalog (LIDA_ACCESS, "thread cannot reset");
      raise (SIGCHLD);
      pthread_exit ((void *)-1);
   }

   longjmp (*p_env, 1);
}


pthread_t thread_pool_alloc (struct thread_pool_h *pool, struct event_data *q)
{
   int err;
   pthread_t thid;

   setjmp (thread_init_env);

   struct thread_data_t *d;   
START:
   d = (struct thread_data_t *)queue_chain_out (pool->h, pool->q_pool);
   
   /*We can create a tmporiry thread here*/
   if (d == NULL){
      if ((thid = tmp_thread_port (q)) < 0)
         goto FAILED;
      else return thid;
   }

   else d->data = q;
   
   /*Wake up handler thread*/
   if (err = pthread_kill (d->id, SIGCONT)){
      if (err == ESRCH){
	 free (d);
	 goto START;
      }
      else goto FAILED;
   }

   return (d->id);

FAILED:
   err_log (LIDA_ACCESS, "Failed to handle current request due to thread problems", q->fd);
   error_post (q->fd, 503);
   return -1;
}


pthread_t tmp_thread_port (struct event_data *data)
{
   int err;
   struct thread_data_t *d = malloc (sizeof (struct thread_data_t));

   d->tmp_flag = 1;
   d->data = data;

   if (err = pthread_create (&(d->id), NULL, thread_trigger, (void *)d)){
      /*Re alloc from thread pool due to system limit*/
      if (err == EAGAIN)
         longjmp (thread_init_env, 1);
      else return -1;
   }

   return (d->id);
}


void func_redirect (void *d)
{
   struct thread_data_t *data = (struct thread_data_t *)d;
   struct http_request_t *r;
   struct cache_info_t *c;
   struct config_info *q;
   size_t f_siz;
   char *path, tmp_pool[PAGE_SIZ*2];
   int err;

   switch (data->data->type)
   {
      case COMMON_RDEVENT: {
         r = read_event (data->data->fd, data->data, (void *)tmp_pool);

	 if ((q = host_search (r->list)) == NULL){
	    error_post (data->data->fd, 400);
	    return;
	 }

	 data->data->host = q;
	 data->data->request = r;
         
	 //if (c = cache_search (MEM_CACHE, MEM_CACHE, 
	                       //q->hostname, r->url) == NULL)
	 //{
	    if ((path = file_search (r->method, q, r->url, data->data->fd)) == NULL)
            {
	       error_post (data->data->fd, 404);
	       return;
            }

	    post_file (hash_search ("Content-type", r->list, 29, crc32_port), 
	               r->method, path, data->data->fd, data->data);
	               
	    struct epoll_event m;
	    m.events = EPOLLIN | EPOLLRDHUP;
	    m.data.ptr = (void *)data->data;
	    data->data->type = COMMON_RDEVENT;
	    
	    if (epoll_ctl (epfd, EPOLL_CTL_MOD, data->data->fd, &m) < 0)
	       err_log (LIDA_ERR, "epoll_ctl", data->data->fd);
            
	    return;
	 //}

	 /*if (response_head (data->data->fd, c->size, 
	    \l                 get_mime (r->list), data->data) < 0)
	 {err = 500; goto FAILED;}*/

	 /*if (cache_to_client (data->data->fd, c->query
	                      c->next, data->data) < 0)
         {err = 500; goto FAILED;}*/
      }

      case PROXY_RDEVENT: {
         proxy_to_client (data->data, (void *)tmp_pool); 
         ret_proxy_fd (data->data->pfd, data->data->node);
         
         /*We need to delete proxy event after handled*/
         if (epoll_ctl (epfd, EPOLL_CTL_DEL, data->data->pfd, NULL) < 0)
            err_log (LIDA_ERR, "epoll_ctl", data->data->fd);
            
         return;
      }

      case PROXY_RECONNECT: {
         re_connect (data->data->node, data->data->fd);
         ret_proxy_fd (data->data->fd, data->data->node);
	 return;
      }
      
      default : return;
   }
}


int thread_event_wait (int option, int fd, struct event_data *obj)
{
   int err;
   struct epoll_event m;
   m.events = option | EPOLLHUP | EPOLLRDHUP;
   m.data.ptr = (void *)obj;

   /*Change epoll mode we cared about*/
   if (epoll_ctl (epfd, EPOLL_CTL_MOD, fd, &m) < 0)
      err_log (LIDA_ERR, "epoll_ctl", obj->fd);
   
   int sig;
   sigset_t wait_mask;
   
   sigemptyset (&wait_mask);
   sigaddset (&wait_mask, SIGCONT);
   sigaddset (&wait_mask, SIGINT);
   
   while (!(err = sigwait (&wait_mask, &sig))){
      switch (sig)
      {
         case SIGCONT: return 0;
	 case SIGINT: raise (SIGCHLD); pthread_exit ((void *)-1);
	 default: continue;
      }
   }
   
   err_log (LIDA_ACCESS, strerror (err), obj->fd);
   raise (SIGCHLD);
   pthread_exit ((void *)-1);
}
