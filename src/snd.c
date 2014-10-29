# include "global.h"
# include "main.h"
# include "timer.h"
# include "memory.h"
# include "listener.h"
# include "http_parser.h"
# include "snd.h"
# include "log.h"


int post_loop (int fd, void *data, size_t size, 
               struct event_data *obj)
{
   void *p = data;
   size_t cur, rest = size;

   while (cur = write (fd, p, rest)){
      if (cur < 0){
         /*We need to add blocked writing to epoll queue*/
         if (cur == EAGAIN){
	    if (thread_event_wait (EPOLLOUT, fd, obj))
	       return -1;
	    else continue;
	 }
	 else return -1;
      }

      p+=cur;
      rest-=cur;

      if (!rest)
         break;
   }

   return 0;
}

void post_file (char *type, char *method, 
                char *path, int fd, struct event_data *obj)
{
   FILE *fp = fopen (path, "r");

   if (fp == NULL){
      error_post (fd, 404);
      return;
   }

   struct stat statbuf;
   if (stat (path, &statbuf) < 0){
      err_log (LIDA_ERR, "stat", fd);
      error_post (fd, 503);
      fclose (fp);
      return;
   }

   /*Set up response head*/
   struct http_response_h s;
   s.version = "HTTP/1.1";
   s.status = "200 OK";
   s.content_type = type;
   sprintf (s.content_length, "%ld", statbuf.st_size);
   s.date = mem_pool_alloc (30, fd);
   getime (s.date);
   s.mtime = ctime ((time_t *)&(statbuf.st_mtime));

   if (post_header (&s, fd) < 0){
      fclose (fp);
      return;
   }

   /*We only post header for 'HEAD' method*/
   if (strcmp (method, "HEAD") == 0){
      fclose (fp);
      return;
   }
   
   stream_to_socket (fp, fd, obj);
}


int post_header (struct http_response_h *r, int fd)
{
   size_t len = strlen (r->version) + strlen(r->status) +
   strlen (r->content_type) + strlen (r->content_length) +
   strlen (r->date) + strlen (r->mtime) + 100;

   char header[len];

   sprintf (header, "%s %s\r\nServer: LIDA/1.0\r\nContent-Type: %s\r\n\
		     Content-Length: %s\r\n\
		     Date: %s\r\n\
		     Last-Modified: %s\r\n\
		     \r\n", r->version, r->status, r->content_type, 
		     r->content_length, r->date, r->mtime);

   /*The response header should be posted within once*/
   if (write (fd, header, len) < 0)
      return -1;

   return 1;
}


void error_post (int fd, int status)
{
   char str[strlen (err_str) + 5];
   sprintf (str, err_str, status);
   write (fd, str, strlen (str));
}


int stream_to_socket (FILE *fp, int fd, struct event_data *obj)
{
   rewind (fp);

   char buf[PAGE_SIZ];
   size_t buf_siz;

   while ((buf_siz = fread (buf, PAGE_SIZ, 1, fp)) > 0){
      if (post_loop (fd, buf, buf_siz, obj) < 0){
         error_post (fd, 503);
	 return;
      }
   }
}
