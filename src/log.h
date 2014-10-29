# ifndef LOG_H_
# define LOG_H_

# define LIDA_ERR 44
# define LIDA_ACCESS 45

# define ERR_EXIT(A) {lidalog (LIDA_ERR, A); exit (1);}
# define ERR_RET(A) {lidalog (LIDA_ERR, A); return -1;}

void lidalog (int, char *);
void getime (char *);
struct buf_log_h *buf_log_init (struct sockaddr_in *, int);
//void request_log (struct http_request_t *, int);
void err_log (int, char *, int);
void proxy_post_log (char *, char *, int);
void buf_log_destroy (struct buf_log_h *);


struct buf_log_t
{
   char *content;
   struct buf_log_t *next;
};

struct buf_log_h
{
   char *content;
   struct buf_log_t *head, *tail;
};

// static struct buf_log_h **log_lst;

# endif
