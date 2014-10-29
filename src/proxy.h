# ifndef PROXY_H_
# define PROXY_H_

struct proxy_group *match_proxy (char *, struct config_info *, int);
void post_to_proxy (struct event_data *, struct config_info *);
void proxy_sock_wake (int, struct proxy_node *);
int slaveTCP (char *, short);
void proxy_heap_handler (void *);
void proxy_hash_handler (void *);
void proxy_init (void);
int re_connect (struct proxy_node *, int);
int load_balance (struct proxy_group *, struct proxy_node **);
void ret_proxy_fd (int, struct proxy_node *);
int proxy_mute_post (void *, struct event_data *);
int proxy_to_client (struct event_data *, void *);

# define COMMON_TIME_OUT 13
# define PROXY_TIME_OUT 14
# define WRITE_ERROR 15

# endif
