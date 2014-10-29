# ifndef CONFIG_H_
# define CONFIG_H_

# include "heap.h"

struct proxy_pass
{
   char *match;
   unsigned cache_flag;
   struct proxy_group *proxy;
};

struct config_info
{
   char *hostname;
   char *path;
   char *frontpage;
   struct proxy_pass *active;
};

/*struct cache_options
{
   size_t mem_max;
   time_t mem_timeout;
   
   size_t disk_max;
   time_t disk_timeout;
};*/

struct server_basic
{
   int port;
   int listener;
   int connect;
   time_t keep_alive;
   int max_thread;
   //struct cache_options cache;
};

// static struct server_basic true_type;

struct proxy_node
{
   char *ip;
   unsigned short port;
   unsigned int weight;
   struct lida_queue_h *fds;
   struct proxy_node *next, *prev;
};

struct proxy_group
{
   char *id;
   struct heap_h nodes;
};


// static int host_hash_n;
// static int proxy_group_hash_n;
// static struct http_attrihash **proxy_lst;
extern int proxy_lst_len;
extern struct http_attrihash **host_lst;
extern struct mem_cache_head *proxy_node_cache;
extern struct mem_cache_head *host_cache;

xmlDocPtr lida_docheck (char *, char *);
size_t xml_get_max (xmlXPathObjectPtr);
time_t xml_get_time (xmlXPathObjectPtr);
void lida_read_default (void);
struct proxy_group *search_proxy_group (char *);
struct proxy_pass *xml_get_active (xmlNodePtr);
void lida_read_hosts (void);
int xml_getlen (xmlNodePtr);
void load_proxy (void);
void load_proxy_nodes (struct proxy_group *, xmlNodePtr, struct mem_cache_head *);

# endif
