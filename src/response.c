# include "global.h"
# include "thread.h"
# include "timer.h"
# include "http_parser.h"
# include "listener.h"
# include "log.h"
# include "response.h"

int reponse_head (int fd, size_t len, char *mime,
                   struct event_data *obj)
{
   size_t size, rest;
   char buf[1024], *ptr, date[30];
   getime (date);

   sprintf (buf, "HTTP/1.1 200 OK\r\n\
                  Server: LIDA/1.1\r\n\
		  Date: %s\r\n\
		  Content-Length: %d\r\n\
		  Content-type: %s\r\n\r\n", date, len, mime);

   rest = strlen (buf);
   while (size = write (fd, ptr, rest))
   {
      ptr+=size;
      rest-=size;

      if (size < 0){
         if (errno == EAGAIN){
	    obj->type = COMMON_WREVENT;

	    if (thread_event_wait (EPOLLOUT, fd, obj) == 0)
	       continue;
	    else return -1;
	 }
	 else return -1;
      }
   }

   return 0;
}


char *get_mime (struct http_attrihash *l[])
{
   return ((char *)hash_search ("Content-type", l, 29, crc32_port));
}



