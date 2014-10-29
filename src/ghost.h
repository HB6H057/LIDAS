# ifndef GHOST_H_
# define GHOST_H_

void backend        (void);
int  passiveTCP     (void);
void lida_fork      (void);
void check_client   (int);
void read_exit      (int);
void lida_reboot    (int);

extern jmp_buf env;
// static int log_fd;

extern int msock;
// static int *listener_lst;
// static unsigned QUIT_flag = 0;

/*struct mem_cache_head *rbt_cache_pool;
struct mem_cache_head *cache_info_pool;
struct mem_cache_head *cache_shm_pool;
struct disk_pool_h *cache_shd_pool;

int shm_id[3];*/
# endif

