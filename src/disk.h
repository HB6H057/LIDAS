struct disk_pool_t
{
   struct disk_pool_t *prev, *next;
};

struct disk_pool_h
{
   int n;
   size_t size;
   struct disk_pool_t *next;
   struct disk_pool_h *block_next;
};

# define DISK_CELL 32768

// static struct disk_pool_head **dpool_lst;
extern int disk_cnt;
