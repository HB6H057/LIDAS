# ifndef MEMORY_H_
# define MEMORY_H_

struct mem_cache_head
{
   size_t size;
   int num;
   struct mem_cache *next;
};

struct mem_cache 
{
   struct mem_cache *prev, *next;
};

struct mem_pool_large
{
   void *start, *end;
   struct mem_pool_large *next;
};

struct mem_pool_head
{
   void *start, *end;
   struct mem_pool_head *next;
   struct mem_pool_large *extra, *tail;
};

struct mem_chain_t
{
   size_t size;
   unsigned int key;
   void *offset;
   struct mem_buffer_t *prev, *next;
};

// static struct mem_pool_head **pool_lst;

# define MEM_POOL_SIZ PAGE_SIZ-sizeof(struct mem_pool_head)
# define POOL_SIZ PAGE_SIZ-sizeof(struct mem_pool_head)

int mem_cache_creat (size_t, struct mem_cache_head **, int);
int mem_cache_locate (struct mem_cache_head *, void *, size_t);
void *mem_cache_alloc (struct mem_cache_head *);
void mem_cache_free (void *, struct mem_cache_head *);
struct mem_pool_head *mem_pool_creat (int);
void *mem_pool_alloc (size_t, int);
void mem_pool_destroy (struct mem_pool_head *);
void mem_pool_add (void *, size_t, int);
//struct mem_cache_head *mem_shared_init (size_t, int, int, void);

# endif

