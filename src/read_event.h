# ifndef READ_EVENT_H_
# define READ_EVENT_H_

# include "config.h"

struct http_request_t *read_event (int, struct event_data *, void *);
struct config_info *host_search (struct http_attrihash **);
char *file_search (char *, struct config_info *, char *, int);
size_t get_content_len (struct http_attrihash **, size_t);
FILE *write_to_stream (size_t, int, struct event_data *);

# endif
