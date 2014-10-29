# include "global.h"
# include "main.h"
# include "memory.h"
# include "config.h"
# include "listener.h"
# include "http_parser.h"
# include "queue.h"
# include "heap.h"
# include "thread.h"
# include "log.h"
# include "snd.h"
# include "proxy.h"


/*Use perl to handle regexp*/
struct proxy_group *match_proxy (char *url, struct config_info *q, int fd)
{
   FILE *pipe;
   struct proxy_pass *t;

   for (t=q->active; t!=NULL; t++){
      char ret[3], r[strlen(url) + strlen(t->match) + 11];
      sprintf (r, "./match.pl %s %s", url, t->match);

      if ((pipe = popen (r, "r")) == NULL){
         err_log (LIDA_ERR, "popen", fd);
	 continue;
      }

      fgets (ret, 3, pipe);
      fclose (pipe);

      if (strcmp (ret, "yes"))
         continue;
      else return (t->proxy);
   }

   return NULL;
}


void post_to_proxy (struct event_data *obj, struct config_info *q)
{
   struct proxy_group *t;
   struct event_data *new;
   struct epoll_event m;

   if ((t = match_proxy (obj->request->url, q, obj->fd)) == NULL){
      error_post (obj->fd, 400);
      return;
   } 

   obj->group = t;

   struct proxy_node *g;
   int pfd = 0;

F_CON:
   /*Get pthread lock to alloc proxy fd*/
   pthread_mutex_lock(&pth_lock);
   pfd = load_balance (t, &g);
   pthread_mutex_unlock (&pth_lock);

   obj->pfd = pfd;
   obj->node = g;
   
   /*Send first 8k data*/
   obj->type = PROXY_WREVENT;       
   
   if (post_loop (obj->pfd, obj->tmp, 8192, obj) < 0){
      proxy_sock_wake (obj->pfd, obj->node);
      goto F_CON;
   }
   /*Send disk buffer*/
   if (stream_to_socket (obj->f_tmp, pfd, obj) < 0){
      proxy_sock_wake (obj->pfd, obj->node);
      goto F_CON;
   }

EVENT_ADD:
   /*Add the proxy-pass event to epoll queue*/
   new = (struct event_data *)mem_pool_alloc (sizeof (struct event_data), obj->fd);
   
   new->fd = obj->fd;
   new->pfd = pfd;
   new->node = g;
   new->tmp = obj->tmp;
   new->f_tmp = obj->f_tmp;

   /*if (t->cache_flag)
      new->cache_flag = 1;
   else new->cache_flag = 0;*/
   
   m.events = EPOLLIN | EPOLLRDHUP;
   m.data.ptr = (void *)new;

   if (epoll_ctl (epfd, EPOLL_CTL_ADD, pfd, &m) < 0)
   {
      err_log (LIDA_ERR, "epoll_ctl", new->fd);
      goto SND_FAIL;
   }

   return;

SND_FAIL:
   err_log (LIDA_ACCESS, "server not avalible", obj->fd);
   error_post (obj->fd, 503);

   if (pfd)
   {
      pthread_mutex_lock(&pth_lock);
      ret_proxy_fd (pfd, g);
      pthread_mutex_unlock (&pth_lock);
   }
}


void proxy_sock_wake (int pfd, struct proxy_node *g)
{
   pthread_mutex_lock (&thread_mutex);
   ret_proxy_fd (pfd, g);
   pthread_mutex_unlock (&thread_mutex);

   /*Call a thread to wake up backend */
   struct event_data *new_obj = (struct event_data *)malloc (sizeof (struct event_data));
   new_obj->type = PROXY_RECONNECT;
   new_obj->fd = pfd;
   new_obj->node = g;

   thread_pool_alloc (thread_pool, new_obj);
}


int slaveTCP (char *ip, short port)
{
   int sock;
   struct sockaddr_in option;

   if ((sock = socket (AF_INET, SOCK_STREAM, 0)) < 0)
      ERR_RET ("socket");

   option.sin_family = AF_INET;
   option.sin_port = htons (port);

   if ((option.sin_addr.s_addr = inet_addr (ip)) == INADDR_NONE)
   {
      char e[30];
      sprintf (e, "Proxy ip invaild: %s", ip);
      lidalog (LIDA_ACCESS, e);
      return -1;
   }

   if (connect (sock, (struct sockaddr *)&option, sizeof (struct sockaddr_in)) < 0)
      ERR_RET ("connect");

   return sock;
}


void proxy_heap_handler (void *d)
{
   int sock;
   struct proxy_node *p = d;
   int cnt = p->weight;
   char msg[30];
   
   queue_init (&(p->fds), p->weight, 0);
   
   while ((sock = slaveTCP (p->ip, p->port)) && cnt--){
      if (sock < 0){
         sprintf (msg, "Failed to access proxy %s:%d", p->ip, p->port);
         lidalog (LIDA_ACCESS, msg);
         break;
      }
      
      queue_in (p->fds, (void *)sock);
  }
}


void proxy_hash_handler (void *d)
{
   heap_ergodic (((struct proxy_group *)d)->nodes, proxy_heap_handler);
}


void proxy_init ()
{
   hash_ergodic (proxy_lst_len, proxy_lst, proxy_hash_handler);
}


int re_connect (struct proxy_node *p, int pfd)
{
   struct sockaddr_in opt;
   char e[30];

   opt.sin_family = AF_INET;
   opt.sin_port = htons (p->port);

   if ((opt.sin_addr.s_addr = inet_addr (p->ip)) == INADDR_NONE)
   {
      sprintf (e, "Proxy ip invaild: %s", p->ip);
      lidalog (LIDA_ACCESS, e);
      return -1;
   }
   
   /*If error occurs, try 5 times*/
   int cnt = 5;
   
   while (connect (pfd, (struct sockaddr *)&opt, sizeof (struct sockaddr_in)) < 0 && cnt--)
      lidalog (LIDA_ERR, "connect");

   if (cnt)
      return pfd;

   else return -1;
}


int load_balance (struct proxy_group *g, struct proxy_node **p)
{
   struct heap_t *n;
   int t;

   pthread_mutex_lock (&thread_mutex);
   
   /*Alloc a node*/
   n = heap_max_delete (&(g->nodes));
   *p = (struct proxy_node *)(n->data);
   t = (int)queue_out ((*p)->fds);
   
   /*Return this node to max-heap*/
   heap_insert ((void *)n, (unsigned int)(((struct proxy_node *)(n->data))->weight), &(g->nodes));
   
   pthread_mutex_unlock (&thread_mutex);
   
   return (t);
}


void ret_proxy_fd (int fd, struct proxy_node *g)
{
   queue_in (g->fds, (void *)fd);
}


int proxy_mute_post (void *addr, struct event_data *obj)
{
   /*Send response data*/
   void *p = addr;
   size_t buf_siz, rest = strlen ((char *)addr);

   while (buf_siz = write (obj->fd, p, rest)){
      if (buf_siz < 0){
         if (buf_siz == EAGAIN){
	    obj->type = COMMON_WREVENT;

	    if (thread_event_wait (EPOLLOUT, obj->fd, obj))
	       return COMMON_TIME_OUT;
	 }
	 else return WRITE_ERROR;
      }

      p+=buf_siz;
      rest-=buf_siz;

      if (!rest)
         break;
   }

   /*Start mute transport*/
   char buf[PAGE_SIZ];
   void *tmp;
   
   while (buf_siz = read (obj->pfd, (void *)buf, PAGE_SIZ)){
      if (buf_siz < 0){
         if (buf_siz == EAGAIN){
	    obj->type = PROXY_RDEVENT;

	    if (thread_event_wait (EPOLLOUT, obj->fd, obj))
	       return PROXY_TIME_OUT;
	 }
	 else return WRITE_ERROR;
      }

      rest = buf_siz;
      tmp = buf;

      while (buf_siz = write (obj->fd, tmp, rest)){
         if (buf_siz < 0){
            if (buf_siz == EAGAIN){
	       obj->type = COMMON_WREVENT;

	       if (thread_event_wait (EPOLLOUT, obj->fd, obj))
	          return COMMON_TIME_OUT;
	    }
	    else return WRITE_ERROR;
         }
      }
   }

   return 0;
}


int proxy_to_client (struct event_data *obj, void *addr)
{
   void *ptr = malloc (8192);
   void *head = ptr;
   size_t buf_siz, rest = 8192;

   /*Receive first 8KB data*/
   while (buf_siz = read (obj->pfd, ptr, rest)){
      if (buf_siz < 0){
         if (buf_siz == EAGAIN){
	    if (thread_event_wait (EPOLLIN, obj->pfd, obj) != 0)
	       goto CLEAN;
            else return (-1);
	 }
	 else return (-1);
      }

      ptr+=buf_siz;
      rest-=buf_siz;

      if (!rest)
         break;
   }

   /*Shifting memory pool*/
   if ((8192-rest) > MEM_POOL_SIZ){
      ptr = malloc (8192 - rest);
      mem_pool_add (ptr, 8192 - rest, obj->fd);
   }
   else ptr = mem_pool_alloc (8192 - rest, obj->fd);

   memcpy (ptr, head, 8192 - rest);
   free (head);

   /*Filter response header*/
   memcpy (addr, ptr, 50);         //Protect original data
   
   switch (http_response_parser ((char *)addr, obj->fd))
   {
      case 400: 
      case 502:
      case 504:
      case 408:
      case 503: goto CLEAN;

      case 200: break;
      default: goto CLEAN;
   }

   /*Response to client without streaming*/
   switch (proxy_mute_post (ptr, obj))
   {
      case COMMON_TIME_OUT: {
         if (epoll_ctl (epfd, EPOLL_CTL_DEL, obj->fd, NULL) < 0){
	    err_log (LIDA_ERR, "epoll_ctl", obj->fd);
	 }

         timer_cleaner (obj->fd, timer_master_pool); 
      }

      case PROXY_TIME_OUT: {
         if (epoll_ctl (epfd, EPOLL_CTL_DEL, obj->pfd, NULL) < 0)
	    err_log (LIDA_ERR, "epoll_ctl", obj->fd);
	 return (-1);
      }

      case WRITE_ERROR: return (-1);
      default: return (-1);
   }

CLEAN:
   /*Delete this proxy-pass event from epoll*/
   if (epoll_ctl (epfd, EPOLL_CTL_DEL, obj->pfd, NULL) < 0){
      err_log (LIDA_ERR, "epoll_ctl", obj->fd);
      return (-1);
   }
   
   /*Restart reverse proxy*/
   post_to_proxy (obj, obj->host);
   return 1;
}
