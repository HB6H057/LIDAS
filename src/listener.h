# ifndef LISTENER_H_
# define LISTENER_H_

# include "config.h"

struct event_data
{
   int fd;
   int pfd;
 
   unsigned type : 4; 
   unsigned cache_flag : 4;

   struct proxy_node *node; 
   struct proxy_group *group;

   void *tmp;
   FILE *f_tmp;
   //struct mem_cache_head *cache;
   struct config_info *host;
   struct http_request_t *request;
};

extern struct event_data sock_data;

extern struct mem_cache_head *timer_master_pool;

extern unsigned event_flag;
extern int listen_del_flag;

void listener_proc (int, int);
int lida_timer_init (void);
void lida_timer_add (int);
void lida_lock_init (int type, struct flock *lock);

//int shm_cache_id;
//struct mem_cache_head *shm_cache_pool;

//int *disk_cache_id_lst;
//struct disk_pool_h *disk_cache_pool;

// static int proxy_cnt = 0; 

extern struct mem_cache_head *listener_mem_cache;

// static int epfd;

extern pthread_mutex_t pth_lock;

extern struct rbtree_t *mem_cache_obj;
extern struct rbtree_t *disk_cache_obj;

# define COMMON_RDEVENT 11
# define PROXY_RDEVENT 12
# define COMMON_WREVENT 13
# define PROXY_WREVENT 14
# define LISTEN_EVENT 10
# define PROXY_RECONNECT 15

# define SET_NONBLOCK(A) fcntl(A, F_SETFL, fcntl (A, F_GETFL, 0) | O_NONBLOCK)   

extern char *err_str;


int listen_event (int, struct timer_opt *);
void listener_kill (int);
void listener_signal (void);
void lida_listener_proc (int, int);
void request_destruct (int, struct mem_cache_head *, 
                       struct event_data *);
void connect_destruct (struct event_data *, int, struct timer_opt *);
void rd_event_redirect (struct event_data *, struct timer_opt *);
void out_event_redirect (struct event_data *, struct timer_opt *);
//void lida_lock_init (int, struct flock *);
int lock_reg (int, int, int, off_t, int, off_t);
pid_t lock_test (int, int, off_t, int, off_t);

# endif








