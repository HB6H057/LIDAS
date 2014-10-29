# include "global.h"
# include "main.h"
# include "ghost.h"
# include "timer.h"
# include "listener.h"
# include "log.h"

static struct buf_log_h **log_lst;

void getime (char *ret)
{
   FILE *pipe;

   if ((pipe = popen ("date", "r")) == NULL)
      return;

   fgets (ret, 30, pipe);
   lida_breakline (ret);
   fclose (pipe);
}

void lidalog (int type, char *info)
{
   FILE *logfile;
   char runtime[30];
   getime (runtime);
   
   /*Open log file*/
   if ((logfile = fopen ("/var/log/lida/error.log", "a")) == NULL)
      return;

   switch (type)
   {
      case LIDA_ERR:{
         fprintf (logfile, "%d: [%s]   %s :%s\n", (int)getpid (), runtime, info, strerror (errno));
         fclose (logfile);
         break;
      }
      
      case LIDA_ACCESS:{
         fprintf (logfile, "%d: [%s]   %s\n", (int)getpid (), runtime, info);
         fclose (logfile);
         break;
      }
   }
}


struct buf_log_h *buf_log_init (struct sockaddr_in *opt, int fd)
{
   struct buf_log_h *h = (*(timer_data_lst + fd))->log_pool;
   char time[30]; 
   getime (time);
   char *addr = inet_ntoa ((struct in_addr)(opt->sin_addr));
   char *header = "======================================================================================\n";
   char *content = (char *)mem_pool_alloc (strlen(time)+strlen(addr)+strlen(header)+10, fd);

   sprintf (content, "%sClient:%s[%s]\n", header, addr, time);
   h = (struct buf_log_h *)mem_pool_alloc (sizeof (struct buf_log_h), fd);
   h->content = content;
   h->head = (struct buf_log_t *)mem_pool_alloc (sizeof (struct buf_log_t), fd);
   h->tail = h->head;
   
   return (h);
}


/*void request_log (struct http_request_t *h, int fd)
{
   struct buf_log_h *head = (*(timer_data_lst + fd))->log_pool;
   
   if (head == NULL)
      return;
   
   char *user_agent = (void *)hash_search ("User-Agent", h->list);
   char *host = (void *)hash_search ("Host", h->list);
   char *r_len = (void *)hash_search ("Content-Length", h->list);
   char *content = (char *)mem_pool_alloc (strlen(h->url)+strlen(user_agent)+strlen(host)+strlen(r_len), fd);

   sprintf (content, "Requested:%s\nUrl:%s\nLength:%s\nClient:%s\n\n", host, h->url, r_len, user_agent);
   head->tail->content = content;
   head->tail->next = (struct buf_log_t *)mem_pool_alloc (sizeof (struct buf_log_h), fd);
   head->tail = head->tail->next;
}*/


void err_log (int type, char *str, int fd)
{
    struct buf_log_h *head = (*(timer_data_lst + fd))->log_pool;
    
    if (head == NULL)
       return;
    
   char *x = (char *)mem_pool_alloc (strlen (str) + 9, fd);

   sprintf (x, "Waring: %s\n", str);
   head->tail->content = x;
   head->tail->next = (struct buf_log_t *)mem_pool_alloc (sizeof (struct buf_log_h), fd);
   head->tail = head->tail->next;
}


void proxy_post_log (char *url, char *ip, int fd)
{
    struct buf_log_h *h = (*(timer_data_lst + fd))->log_pool;
    char *x = (char *)mem_pool_alloc (strlen (url)+strlen (ip)+10);

    sprintf (x, "%s has been ported to proxy %s", url, ip);
    h->tail->content = x;
    h->tail->next = (struct buf_log_t *)mem_pool_alloc (sizeof (struct buf_log_h), fd);
    h->tail = h->tail->next;
}


void buf_log_destroy (struct buf_log_h *h)
{
   struct buf_log_t *p;
   
   if (lock_reg (log_fd, F_SETLKW, F_WRLCK,
                 0, SEEK_SET, 0) < 0)
      return;

   /*Record head log information*/
   if (write (log_fd, h->content, strlen (h->content)) < 0)
      return;

   while (h->head != h->tail){
      if (write (log_fd, h->head->content, 
                 strlen (h->head->content)) < 0)
         return;
      
      h->head = h->head->next;
   }

   write (log_fd, "DISCONNECTED===========",
          25);
   
   /*Unlock the log file*/
   lock_reg (log_fd, F_SETLKW, F_UNLCK, 0, SEEK_SET, 0);
}
