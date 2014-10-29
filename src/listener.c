# include "global.h"
# include "main.h"
# include "config.h"
# include "timer.h"
# include "memory.h"
# include "listener.h"
# include "manager.h"
# include "ghost.h"
# include "queue_chain.h"
# include "queue.h"
# include "snd.h"
# include "log.h"
# include "thread.h"

struct event_data sock_data;
struct mem_cache_head *timer_master_pool;

unsigned event_flag = 0;
int listen_del_flag = 0;
static int proxy_cnt = 0; 
struct mem_cache_head *listener_mem_cache;
struct mem_cache_head *event_data_pool;
static int epfd;
pthread_mutex_t pth_lock;

struct rbtree_t *mem_cache_obj;
struct rbtree_t *disk_cache_obj;
char *err_str = "HTTP/1.1 %d NOTFOUND\r\nServer: LIDA/1.0\r\nContent-Type: text/html\r\nContent-Length: 112\r\n\r\n";

int listen_event (int sock, struct timer_opt *timer_obj)
{
   int client;
   struct sockaddr_in client_info;
   socklen_t alen = sizeof (struct sockaddr_in);
   
   client = accept (sock, (struct sockaddr *)&client_info, &alen);
            
   if (client < 0)
   {
      if (errno == EINTR)
         return 0;
      else ERR_RET ("accept");
   }
            
   /*Set new socket fd to non-block mode*/
   if (SET_NON_BLOCK (client) < 0)
      return -1;
      
   /*Add new socket fd to epoll list*/
   struct epoll_event tmp;
   struct event_data *new_obj_p = (struct event_data *)mem_cache_alloc (event_data_pool);

   new_obj_p->fd = client;
   new_obj_p->pfd = 0;
   new_obj_p->type = COMMON_RDEVENT;

   tmp.events = EPOLLIN | EPOLLRDHUP | EPOLLHUP;
   tmp.data.ptr = (void *)new_obj_p;

   if (epoll_ctl (epfd, EPOLL_CTL_ADD, client, &tmp) < 0){
      lidalog (LIDA_ERR, "epoll_ctl");
      goto END_ACCEPT;
   }
               
   /*Add new socket fd to keep-alive timeout queue*/
   if (timer_flag){
      if (timer_insert (timer_obj->t, client, 0, timer_obj->pool, &client_info) < 0)
         goto END_ACCEPT;
   }
              
END_ACCEPT:
   /*This process has the lock of master socket
   it must be released before next round*/
   event_flag = 0;
   struct flock l;

   /*Release the socket lock, others are waiting*/
   lida_lock_init (F_UNLCK, &l);
   
   if (fcntl (sock, F_SETLK, l) < 0)
      ERR_RET ("fcntl");
   
   if (epoll_ctl (epfd, EPOLL_CTL_DEL, sock, NULL) < 0)
      ERR_RET ("epoll_ctl");
}


void listener_kill (int sig)
{
   if (epoll_ctl (epfd, EPOLL_CTL_DEL, msock, NULL) < 0){
      lidalog (LIDA_ERR, "epoll_ctl");
      exit (1);
   }
   listen_del_flag = 1;
}


void listener_signal ()
{
   int err;
   
   /*Signal mask set up*/
   sigset_t sigset;
   
   sigemptyset (&sigset);
   sigaddset (&sigset, SIGALRM);
   
   if (err = pthread_sigmask (SIG_BLOCK, &sigset, NULL)){
      lidalog (LIDA_ACCESS, strerror (err));
      exit (1);
   }
   
   /*Install main thread's signal handler*/
   struct sigaction act[2];
   
   act[0].sa_handler = listener_kill;
   sigfillset (&act[0].sa_mask);
   
   if (sigaction (SIGINT, &act[0], NULL) < 0){
      lidalog (LIDA_ERR, "sigaction");
      exit (1);
   }
}


void lida_listener_proc (int sock, pid_t main_pid)
{
   int epoll_cnt, i, err;
   struct sockaddr_in in;
   struct timer_opt timer_obj;
   
   /*Init timer*/
   if (true_type.keep_alive > 0){
      timer_master_pool = timer_obj.pool;
      timer_flag = 1;

      mem_cache_creat (sizeof (timer_t), &(timer_obj.pool), 2*true_type.connect);
      queue_init (&(timer_obj.t), 2*true_type.keep_alive, 0);
      timer_data_lst = (struct timer_t **)calloc (2*true_type.connect, sizeof (void *));
   }
   else timer_flag = 0;

   /*Init thread pool*/
   if (pthread_mutex_init(&pth_lock, NULL) != 0)
      ERR_EXIT ("pthread_mutex_init");

   if (thread_pool_init (true_type.max_thread, thread_pool) < 0){
      lidalog (LIDA_ACCESS, "Failed to init thread pool");
      exit (1);
   }
      
   /*Start manager thread*/
   if (err = pthread_create (&manager_id, NULL, manager_thread, (void *)&timer_obj)){
      lidalog (LIDA_ACCESS, strerror (err));
      exit (1);
   }
   
   /*Register signal handler*/
   listener_signal ();

   /*Init cache settings*/
   /*rbt_cache_pool = mem_shared_init (sizeof (struct rbtree_t), 
                                     PAGE_SIZ*2/sizeof(struct rbtree_t), shm_id[0],
				     (void *)rbt_cache_pool);

   cache_info_pool = mem_shared_init (sizeof (struct cache_info_t),
                                      PAGE_SIZ*2/sizeof(struct cache_info_t),
				      shm_id[1], (void *)cache_info_pool);

   cache_shm_pool = mem_shared_init (32768, true_type.cache.mem_max/32768,
                                     shm_id[2], (void *)cache_shm_pool);*/

   /*Set up host socket epoll event*/
   struct epoll_event m, *t;
   mem_cache_creat (sizeof (struct event_data), &event_data_pool, true_type.connect);

   /*Listen socket data*/
   sock_data.fd = sock;
   sock_data.pfd = 0;
   sock_data.type = LISTEN_EVENT;
   
   if ((epfd = epoll_create (true_type.connect)) < 0);
      ERR_EXIT ("epoll");
      
   /*Init proxy sockets*/
   proxy_init ();
   
   while (1)
   {
      /*If listen queue is full, This process shouldn't get lock*/
      if ((!Q_IS_FULL (timer_obj.t)) ||
          (!listen_del_flag))
      {
         struct flock l;
	 lida_lock_init (F_RDLCK, &l);
         
	 /*This process have access to accept*/
         if (fcntl (sock, F_SETLK, l) >= 0)
         {
	    m.events = EPOLLIN | EPOLLHUP;
	    m.data.ptr = (void *)&sock_data;

            if (epoll_ctl (epfd, EPOLL_CTL_ADD, sock, &m) < 0)
               ERR_EXIT ("epoll_ctl");

            event_flag = 1;
         } 

         else if ((errno == EACCES) ||
	          (errno == EAGAIN))
		  event_flag = 0;
         else {
	    lidalog (LIDA_ERR, "fcntl");
	    exit (1);
	 }
      }
      
      /*Start listening*/
      if ((epoll_cnt = epoll_wait (epfd, t, true_type.connect, 0)) < 0)
         ERR_EXIT ("epoll_wait");

      /*No socket ready to read*/
      if (!epoll_cnt)
         continue;
      
      /*Handle epoll events*/
      for (i=0; i<epoll_cnt; i++)
      {  
         switch ((t+i)->events)
	 {
	    case EPOLLERR: lidalog (LIDA_ACCESS, "Connection failed due to epoll error");
	    case EPOLLHUP:
	    case EPOLLRDHUP: connect_destruct ((struct event_data *)((t+i)->data.ptr), sock, &timer_obj); continue;
	    case EPOLLIN: rd_event_redirect ((struct event_data *)((t+i)->data.ptr), &timer_obj); continue;
	    case EPOLLOUT: out_event_redirect ((struct event_data *)((t+i)->data.ptr), &timer_obj); continue;
	 }	
      }
   }
}


void request_destruct (int fd, struct mem_cache_head *pool, 
                       struct event_data *obj)
{
   int err;

   timer_cleaner (fd, pool);

   if (obj->tmp != NULL)
      free (obj->tmp);

   if (obj->f_tmp != NULL)
      fclose (obj->f_tmp);

   /*Return the proxy fd to proxy node*/
   queue_in (obj->node->fds, obj->pfd);
   obj->node->weight++;

   mem_cache_free ((void *)obj, event_data_pool);
}


void connect_destruct (struct event_data *obj, int master, struct timer_opt *timer)
{
   pthread_t thid;
   struct event_data *new;
   struct proxy_node *g;
   struct timer_t *tmp;
   int pfd, err;

   switch (obj->type)
   {
      case LISTEN_EVENT: {
         if (obj->fd == master){
	    lidalog (LIDA_ACCESS, "An error occurs to master socket, listener is exiting");
	    exit (1);
	 }
	 return;
      }

      case COMMON_RDEVENT:
      case COMMON_WREVENT: request_destruct (obj->fd, timer->pool, obj); return;

      case PROXY_RDEVENT:
      case PROXY_WREVENT: {
         /*Call a thread to handle TCP shake hands*/
         new = (struct event_data *)malloc (sizeof (struct event_data));
	 new->type = PROXY_RECONNECT;
	 new->fd = obj->pfd;
	 new->node = obj->node;

	 /*We don't concern the assciated fd here*/
         thid = thread_pool_alloc (thread_pool, new);

         if ((pfd = (int)queue_out (obj->node->fds)) <= 0){
	    if ((pfd = load_balance (obj->group, &g)) <= 0){
	       err_log (LIDA_ACCESS, "Respond failed due to proxy's status", obj->fd);
	       error_post (obj->fd, 503);

	       /*Clean the event data and timer data*/
	       request_destruct (obj->pfd, timer->pool, obj);
	       return;
	    }
	 }
	 break;
      }

      default: return;
   }

   /*Tell the handler thread to continue*/
   obj->pfd = pfd;
   thid = (*(timer_data_lst + obj->pfd))->thread_id;
   
   if (err = pthread_kill (SIGCONT, thid)){
      err_log (LIDA_ACCESS, strerror (err), obj->fd);
	       
      /*Do some clean works before calling another thread*/
      if (obj->f_tmp != NULL)
         rewind (obj->f_tmp);

      thid = thread_pool_alloc (thread_pool, obj);
      (*(timer_data_lst + obj->fd))->thread_id = thid;
   }
}


void rd_event_redirect (struct event_data *obj, struct timer_opt *timer_obj)
{
   pthread_t thid;
   char msg[30];
   int fd, err;

   switch (obj->type){
      case LISTEN_EVENT: {
         /*The listen event should be handled in main thread*/
         if (obj->fd == msock){
	    if (listen_event (msock, timer_obj) < 0)
	       lidalog (LIDA_ACCESS, "Failed to accept new connect");
	 }
	 else {
	    sprintf (msg, "Invailed listen socket %d", obj->fd);
	    lidalog (LIDA_ACCESS, "Invailed listen socket");
	 }

	 return;
      }

      case COMMON_RDEVENT: fd = obj->fd; break;
      case PROXY_RDEVENT: fd = obj->pfd; break;
            
      default : return;
   }

   /*Wake up blocking thread*/
   if ((*(timer_data_lst + fd))->thread_id > 0){
      if (err = pthread_kill ((*(timer_data_lst + fd))->thread_id, SIGCONT))
         err_log (LIDA_ACCESS, strerror (err), obj->fd);
      else return;
   }

   /*If failed to alloc wake thread we have to handle this in current thread*/
   if ((thid = thread_pool_alloc (thread_pool, obj)) < 0)
      func_redirect (obj);
   
   (*(timer_data_lst + fd))->thread_id = thid;

   /*Moditify epoll event*/
   struct epoll_event m;
   m.events = EPOLLRDHUP | EPOLLHUP;
   m.data.ptr = (void *)obj;

   if (epoll_ctl (epfd, EPOLL_CTL_MOD, fd, &m) < 0)
      err_log (LIDA_ERR, "epoll_ctl", fd);
}


void out_event_redirect (struct event_data *obj, struct timer_opt *timer)
{
   pthread_t thid;
   int err, fd;
   char msg[100];

   switch (obj->type)
   {
      case COMMON_WREVENT: fd = obj->fd; break;
      case PROXY_WREVENT: fd = obj->pfd; break;

      default: return;
   }

   /*Wake up handler thread*/
   if (err = pthread_kill (SIGCONT, (*(timer_data_lst + fd))->thread_id)){
      error_post (obj->fd, 503);
      err_log (LIDA_ACCESS, "The handler thread is lost", obj->fd);
      request_destruct (fd, timer->pool, obj);
      return;
   }

   struct epoll_event m;
   m.events = EPOLLRDHUP | EPOLLHUP;
   m.data.ptr = (void *)obj;

   if (epoll_ctl (epfd, EPOLL_CTL_MOD, fd, &m) < 0)
      err_log (LIDA_ERR, "epoll_ctl", fd);
}


/*Set up lock options*/
void lida_lock_init (int type, struct flock *lock)
{
   (*lock).l_type = type;
   (*lock).l_start = 0;
   (*lock).l_whence = SEEK_SET;
   (*lock).l_len = 0;
}


int lock_reg (int fd, int cmd, int type, off_t offset, int whence, off_t len)
{
   struct flock lock;

   lock.l_type = type;
   lock.l_start = offset;
   lock.l_whence = whence;
   lock.l_len = len;

   return (fcntl (fd, cmd, &lock));
}


pid_t lock_test (int fd, int type, off_t offset, int whence, off_t len)
{
   struct flock lock;
   lock.l_type = type;
   lock.l_start = offset;
   lock.l_whence = whence;
   lock.l_len = len;

   if (fcntl (fd, F_GETLK, &lock) < 0){
      lidalog (LIDA_ERR, "fcntl");
      return -1;
   }

   if (lock.l_type == F_UNLCK)
      return 0;
   else return (lock.l_pid);
}
