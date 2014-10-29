# include "global.h"
# include "memory.h"
# include "listener.h"
# include "main.h"
# include "log.h"
# include "snd.h"
# include "crc32.h"
# include "http_parser.h"

static unsigned parse_flag = 1;
static unsigned parse_start = 1;
char *method_lst[] = {"GET", "HEAD", "POST", "PUT", "DELETE", "TRACE", "CONNECT", "OPTIONS"};
struct http_request_t *http_parser (char *h, int fd)
{
   char *toklist, *capture, *content_cap, *compat;
   int hash_listcnt;
   size_t size = strlen (h);

   /*Init parser object*/
   struct http_request_t *t = (struct http_request_t *)mem_pool_alloc (sizeof (struct http_request_t), fd);
   t->list_siz = 29;
   t->list = mem_pool_alloc (sizeof(void *)*t->list_siz, fd);

   /*Test the method string*/
   t->method = strtok_r (h, " ", &toklist);
   if (method_test (t->method) < 0)
      return NULL;

   /*Get url*/
   t->url = strtok_r (NULL, " ", &toklist);
   if (toklist == NULL)
      return NULL;

   /*Get http version*/
   t->protocal = strtok_r (NULL, "\r\n", &toklist);
   if (protocal_test (t->protocal) < 0)
      return NULL;

   /*Parse the rest string*/
   while (*(toklist+1) != '\n')
   {
      capture = strtok_r (NULL, ": ", &toklist);
      content_cap = strtok_r (NULL, "\n", &toklist);
      
      /*Clear invaild chars*/
      while (*content_cap == ' ')
         content_cap++;
      lida_breakline (content_cap);
      
      hash_set (capture, (void *)content_cap, t->list, 29, fd, crc32_port);

      if (!*(toklist+1))
         return NULL;
   }
   
   toklist+=2;
   /*Here is the request body*/
   t->offset = strlen (toklist);
   t->request_body = toklist;
   
   return (t);
}


/*We only care status here*/
int http_response_parser (char *ptr, int fd)
{
   char *l, *str;

   str = strtok_r (ptr, " ", &l);
   str = strtok_r (NULL, " ", &l);

   return (atoi (str));
}


int method_test (char *m)
{
   int i;

   for (i=0; i<8; i++){
      if (strcmp (m, method_lst[i]) == 0)
         return TRUE;
   }

   return -1;
}


int protocal_test (char *p)
{
   if (strcmp (p, "HTTP/1.1") == 0)
      return TRUE;

   else if (strcmp (p, "HTTP/1.0") == 0)
      return TRUE;

   else return -1;
}


unsigned int lida_hashkey (char *ptr)
{
   unsigned int sum;
   
   while (*ptr)
   {
      sum+=*ptr;
      ptr++;
   }
   
   return (sum%29);
}


void hash_conflictset (char *key, void *content_cap, unsigned int cnt, struct http_attrihash *list[], int fd)
{
   struct http_attrihash *p = list[cnt];
   p = p->next;
   
   while (p != NULL)
      p = p->next;
      
   p = fd?mem_pool_alloc (sizeof (struct http_attrihash), fd): malloc (sizeof (struct http_attrihash));
   p->content = content_cap;
   p->attri = key;
}


void hash_set (char *key, void *content, struct http_attrihash *list[], 
               int len, int fd, unsigned int (*key_set) (char *, int))
{
   unsigned cnt = (*key_set) (key, len);
   
   if (list[cnt] == NULL)
   {
      list[cnt] = fd?mem_pool_alloc (sizeof (struct http_attrihash), fd): malloc (sizeof (struct http_attrihash));
      list[cnt]->attri = key;
      list[cnt]->content = content;
   }
   
   else hash_conflictset (key, content, cnt, list, fd);
}


void *hash_conflictsearch (unsigned int cnt, char *key, struct http_attrihash *t[])
{
   struct http_attrihash *p = t[cnt];
   p = p->next;
   
   while (p != NULL)
   {
      if (strcmp (p->attri, key) == 0)
         return (p->content);
      else p++;
   }
   
   return NULL;
}


void *hash_search (char *q, struct http_attrihash *t[], 
                   int len, unsigned int (*key_set) (char *, int))
{
   unsigned int cnt = (*key_set) (q, len);
   void *ret;
   
   if (t[cnt] == NULL)
      return NULL;
   
   if (strcmp (t[cnt]->attri, q) == 0)
      ret = t[cnt]->content;
      
   else ret = (void *)hash_confilctsearch (cnt, q, t);
   
   return ret;
}


void hash_ergodic (int len, struct http_attrihash *t[], void (*handler) (void *))
{
   int i;
   struct http_attrihash *p;
   
   for (i=0; i<len; i++)
   {
      if (t[i] == NULL)
         continue;
         
      (*handler) (t[i]->content);
      p = t[i]->next;
      
      while (p != NULL){
         (*handler) (p->content);
         p = p->next;
      }
   }
}


unsigned int crc32_port (char *q, int len)
{
   return (crc32(0, (void *)q, strlen (q)) % len);
}
