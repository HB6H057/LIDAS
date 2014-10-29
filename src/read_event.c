# include "global.h"
# include "http_parser.h"
# include "timer.h"
# include "listener.h"
# include "log.h"
# include "main.h"
# include "memory.h"
# include "thread.h"
# include "read_event.h"


struct http_request_t *read_event (int fd, struct event_data *t, void *addr)
{
   char *head = (char *)malloc (PAGE_SIZ*2);
   void *buf = (void *)head, *new;
   size_t buf_siz = 0, rest = PAGE_SIZ*2;
   struct http_request_t *p;
   
   /*Get the request head*/
   while ((buf_siz = read (fd, buf, rest)) > 0)
   {
      buf+=buf_siz;
      rest-=buf_siz;
      
      if (!rest)
         break;
   }

   /*Reset head to  memory pool*/
   if (strlen (head) <= POOL_SIZ){
      new = (char *)mem_pool_alloc (buf_siz, fd);
      memcpy (new, head, strlen (head));
      free (head);
      head = new;
   }
   
   /*Protect the original data*/
   memcpy (addr, head, PAGE_SIZ*2);
   
   if ((p = http_parser (head, fd)) == NULL)
      return NULL;
   
   if ((buf_siz = get_content_len (p->list, p->offset)) != 0)
      t->f_tmp = write_to_stream (buf_siz, fd, t);
    else t->f_tmp = NULL;

    t->tmp = head;
    return p;
}


struct config_info *host_search (struct http_attrihash *l[])
{
   char *name = (char *)hash_search ("host", l, 29, crc32_port);
   return ((struct config_info *)hash_search (name, host_lst, host_hash_n, crc32_port));
}


char *file_search (char *method, struct config_info *q, char *url, int fd)
{
   char *t, *path;
   
   /*We only handle get and head method here*/
   if (strcmp (method, "GET") && strcmp (method, "HEAD"))
      return NULL;
      
   struct stat info;
   char *file = strtok_r (url, "?", &t);

   if (t != NULL)
     return NULL;

   if (strcmp (path, "/") == 0)
      file = q->frontpage;

   path = (char *)mem_pool_alloc (strlen (q->hostname) + strlen (file) +2, fd);
   sprintf (path, "%s%s", q->hostname, file);

   if (stat (path, &info) < 0)
      return NULL;

   //key_t fkey = ftok (path, 0);
   //request_cache_proxy (fkey);

   return (path);
}


size_t get_content_len (struct http_attrihash *t[], size_t offset)
{
   size_t s;
   char *r = (char *)hash_search ("Content-Length", t, 29, crc32_port);

   if (r == NULL)
      return 0;

   s = atoi (r) - offset;
   return s;
}


FILE *write_to_stream (size_t size, int fd, struct event_data *obj)
{
   char fname[15];
   FILE *fp;
   char buf[PAGE_SIZ];
   size_t buf_siz, sum = 0;
   int re_cnt = 10, err;

   sprintf (fname, "/var/lida/tmp/Buffer_%d.tmp", fd);
   fp = fopen (fname, "w+");

WR_START:
   while (re_cnt--){
      while ((buf_siz = read (fd, (void *)buf, PAGE_SIZ)) > 0)
      {
         sum+=buf_siz;
         fwrite ((const void *)buf, buf_siz, 1, fp);
      }

      if (buf_siz < 0)
         goto FAILED;

      if (sum >= size)
         goto WR_END;

      else {
         /*Waiting for epoll*/
	 if (thread_event_wait (EPOLLIN, fd, obj) == 0)
	    goto WR_START;
	 else goto FAILED;
      }
   }
   
FAILED:
   error_post (fd, 503);
   return NULL;

WR_END:
   fflush (fp);
   rewind (fp);
   return fp;
}
