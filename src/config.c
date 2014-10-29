# include "global.h"
# include "config.h"
# include "main.h"
# include "ghost.h"
# include "memory.h"
# include "http_parser.h"
# include "log.h"


static int host_hash_n;
static int proxy_group_hash_n;
static struct http_attrihash **proxy_lst;
int proxy_lst_len;
struct http_attrihash **host_lst;
struct mem_cache_head *proxy_node_cache;
struct mem_cache_head *host_cache;
static struct server_basic true_type;

xmlDocPtr lida_docheck (char *path, char *headstr)
{
   xmlDocPtr  doc;
   xmlNodePtr cur;
   
   doc = xmlReadFile (path, "UTF-8", XML_PARSE_RECOVER);
   if (doc == NULL)
   {
      printf ("xml read failed");
      longjmp (env, 1);
   }
   
   cur = xmlDocGetRootElement (doc);
   if (cur == NULL)
   {
      printf ("Rootpoint missing");
      xmlFreeDoc (doc);
      longjmp (env, 1);
   }
   
   else if (xmlStrcmp (cur->name, headstr) != 0)
   {
      printf ("Invaild config format:%s\n", headstr);
      xmlFreeDoc (doc);
      longjmp (env, 1);
   }
   
   return doc;
}


size_t xml_get_max (xmlXPathObjectPtr r)
{
   size_t ret = 0;
   char *str = (char *)xmlGetProp (r->nodesetval->nodeTab[0], "max");
   char *h = str, x[10];
   
   while (*str)
      str++;
      
   switch (*(--str))
   {
      case ('K'): ret = (size_t)1024*atoi (strncpy (x, h, str - h)); break;
      case ('M'): ret = (size_t)1024*1024*atoi (strncpy (x, h, str - h)); break;
      case ('G'): ret = (size_t)1024*1024*1024*atoi (strncpy (x, h, str - h)); break;
      default : ret = (size_t)atoi (strncpy (x, h, str - h));
   }
   
   return ret;
}


time_t xml_get_time (xmlXPathObjectPtr r)
{
   time_t ret = 0;
   char *str = (char *)xmlGetProp (r->nodesetval->nodeTab[0], "timeout");
   char *h = str, x[10];
   
   while (*str)
      str++;
      
   switch (*(--str))
   {
      case 'm': ret = (time_t)60*atol (str); break;
      case 'h': ret = (time_t)60*60*atol (str); break;
      default : ret = (time_t)atol (str); break;
   }
   
   return ret;
}


void lida_read_default ()
{
   xmlXPathContextPtr context;
   xmlXPathObjectPtr  result;
   char *type;
   
   xmlDocPtr configfile = lida_docheck ("./default.xml", "default");
   context = xmlXPathNewContext (configfile);
   
   result = xmlXPathEvalExpression ((const xmlChar *)"//port", context);
   true_type.port = atoi (xmlNodeGetContent (result->nodesetval->nodeTab[0]));
   
   result = xmlXPathEvalExpression ((const xmlChar *)"//listener", context);
   true_type.listener = atoi (xmlNodeGetContent (result->nodesetval->nodeTab[0]));
   
   result = xmlXPathEvalExpression ((const xmlChar *)"//connect", context);
   true_type.connect = atoi (xmlNodeGetContent (result->nodesetval->nodeTab[0]));
   
   result = xmlXPathEvalExpression ((const xmlChar *)"//keep-alive", context);
   type = (char *)xmlGetProp (result->nodesetval->nodeTab[0], "type");
   if (!strcmp (type, "min"))
      true_type.keep_alive = (time_t)60*atoi (xmlNodeGetContent (result->nodesetval->nodeTab[0]));
   else true_type.keep_alive = (time_t)atoi (xmlNodeGetContent (result->nodesetval->nodeTab[0]));
   
   result = xmlXPathEvalExpression ((const xmlChar *)"//max-thread", context);
   true_type.max_thread = atoi (xmlNodeGetContent (result->nodesetval->nodeTab[0]));
   
   /*Configure cache options here*/
   /*result = xmlXPathEvalExpression ((const xmlChar *)"//cache-options", context);
   type = (char *)xmlGetProp (result->nodesetval->nodeTab[0], "timestamp");
   true_type.cache.timestamp = (time_t)atol (type);
   
   result = xmlXPathEvalExpression ((const xmlChar *)"//mem-cache", context);
   true_type.cache.mem_max = xml_get_max (result);
   true_type.cache.mem_timeout = xml_get_time (result);
   
   result = xmlXPathEvalExpression ((const xmlChar *)"//disk-cache", context);
   true_type.cache.disk_max = xml_get_max (result);
   true_type.cache.disk_timeout = xml_get_time (result);*/
   
END_DEFAULT:
   xmlXPathFreeObject(result);
   xmlFreeDoc(configfile);
   xmlCleanupParser(); 
}


struct proxy_group *search_proxy_group (char *id)
{
   return ((struct proxy_group *)hash_search (id, host_lst, proxy_group_hash_n, crc32_port));
}


struct proxy_pass *xml_get_active (xmlNodePtr cur)
{
   xmlNodePtr act = cur->xmlChildrenNode;
   int cnt = xml_getlen (cur);
   proxy_group_hash_n = cnt*2;
   struct proxy_pass *ret = (struct proxy_pass *)calloc (cnt, sizeof (struct proxy_pass));
   char *tid, *x;
   int i;
   
   for (i=0; i<cnt; act = act->next, i++)
   {
      (ret + i)->match = (char *)xmlGetProp (act, "match");
      /*x = (char *)xmlGetProp (act, "cache");
      if (strcmp (x, "on"))
         (ret + i)->cache_flag = 0;
      else (ret + i)->cache_flag = 1;*/
      
      tid = xmlNodeGetContent (act);
      (ret + i)->proxy = search_proxy_group (tid);
   }
   
   return ret;
}


void lida_read_hosts ()
{
   int i;
   xmlXPathContextPtr context;
   xmlNodePtr cur;
   xmlXPathObjectPtr  status, hostname, path, frontpage, proxy_pass;
   struct config_info *p;

   xmlDocPtr configfile = lida_docheck ("./vhost.xml", "vhost");
   cur = xmlDocGetRootElement(configfile);
   cur = cur->xmlChildrenNode;
   context = xmlXPathNewContext(configfile);
   int cnt = xml_getlen (cur);
   host_hash_n = cnt*2;
   host_lst = (struct http_attrihash **)calloc (cnt, sizeof (void *));
   
   hostname = xmlXPathEvalExpression((const xmlChar*)"//hostname", context);
   path = xmlXPathEvalExpression((const xmlChar*)"//path", context);
   frontpage = xmlXPathEvalExpression((const xmlChar*)"//frontpage", context);
   proxy_pass = xmlXPathEvalExpression((const xmlChar*)"//proxy-pass", context);
   
   host_lst = calloc (cnt, sizeof (void *));
   for (i=0; i<cnt; i++, cur = cur->next)
   {
      p = (struct config_info *)malloc (sizeof (struct config_info));   
      p->hostname = xmlNodeGetContent (hostname->nodesetval->nodeTab[i]);
      p->path = (char *)xmlNodeGetContent (path->nodesetval->nodeTab[i]);
      p->frontpage = (char *)xmlNodeGetContent (frontpage->nodesetval->nodeTab[i]);
      p->active = xml_get_active (proxy_pass->nodesetval->nodeTab[i]);
      
      hash_set (p->hostname, (void *)p, host_lst, host_hash_n, 0, crc32_port);
   }
         
   xmlXPathFreeObject(hostname);
   xmlXPathFreeObject(path);
   xmlXPathFreeObject(frontpage);
   xmlXPathFreeObject(proxy_pass);
   xmlFreeDoc(configfile);
   xmlCleanupParser();  
}


int xml_getlen (xmlNodePtr n)
{
   int cnt;
   
   for (cnt=0; n != NULL; cnt++)
      n = n->next;
      
   return cnt;
}


void load_proxy ()
{ 
   xmlDocPtr doc = lida_docheck ("proxy.xml", "root");
   xmlNodePtr cur = xmlDocGetRootElement(doc);
   int node_cnt = 0;
   
   cur = cur->xmlChildrenNode;
   proxy_lst_len = xml_getlen (cur)*2;
   proxy_lst = (struct http_attrihash **)calloc (proxy_lst_len, sizeof (void *));
   struct proxy_group *new;
   
   while (cur != NULL)
   {
      new = (struct proxy_group *)malloc (sizeof (struct proxy_group));
      new->id = xmlGetProp (cur, "id");
      hash_set (new->id, (void *)new, proxy_lst, proxy_group_hash_n, 0, crc32_port);
      
      node_cnt = xml_getlen (cur->xmlChildrenNode);
      mem_cache_creat (sizeof (struct proxy_node), &proxy_node_cache, node_cnt);
      load_proxy_nodes (new, cur, proxy_node_cache);
      
      cur = cur->next;
   }
   
   xmlFreeDoc (doc);
   xmlCleanupParser();
}


void load_proxy_nodes (struct proxy_group *p, xmlNodePtr cur, struct mem_cache_head *pool)
{
   char *addr, *l;
   xmlNodePtr node = cur->xmlChildrenNode;
   struct proxy_node *new;
   int cnt = xml_getlen (node);
   if (heap_init (&(p->nodes), (unsigned int)cnt, NULL, 0) < 0){
      lidalog (LIDA_ACCESS, "Failed to add proxy nodes due to heap error");
      exit (1);
   }
   
   while (node != NULL)
   {
      new = (struct proxy_node *)mem_cache_alloc (pool);
      new->weight = atoi ((char *)xmlGetProp (node, "weight"));
      addr = (char *)xmlNodeGetContent (node);
      new->ip = strtok_r (addr, ":", &l);
      new->port = atoi (l);
      
      if (heap_insert ((void *)new, (unsigned int)new->weight, &(p->nodes)) < 0){
         lidalog (LIDA_ACCESS, "Faild to add proxy nodes due to heap error");
         exit (1);
      }

      node = node->next;
   }
}
