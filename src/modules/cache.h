struct msg_cache_t
{
   long type;
   char fname[50];
};

struct cache_info_t
{
   unsigned mem_flag : 2;
   unsigned disk_flag : 2;
   unsigned local : 2;
   unsigned remote : 2;

   time_t time;
   char *query;

   size_t size;
   size_t rest;
   struct cache_chain_t *next;
};

struct cache_chain_t 
{
   struct cache_node_t *prev, *next;
   void *addr;
};

# define CACHE_REMOTE 1
# define CACHE_LOCAL  0

# define CACHE_BUFSIZ 32768-sizeof(struct cache_chain_t)

# define CACHE_CLIENT_ERR -11
# define NON_CACHE_MEM -22

# define RBT_MEM_CACHE 23
