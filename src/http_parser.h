# ifndef HTTP_PARSER_H_
# define HTTP_PARSER_H_


struct http_request_t
{
   char *method;
   char *url;
   char *protocal;

   struct http_attrihash **list;
   int list_siz;

   size_t offset;
   void *request_body;
};

struct http_attrihash 
{
   char *content;
   void *attri;
   struct http_attrihash *next;
};

unsigned int lida_hashkey (char *);
void *hash_conflictsearch (unsigned int, char *, struct http_attrihash **);
struct http_request_t *http_parser (char *, int);
int http_response_parser (char *, int);
int method_test (char *);
int protocal_test (char *);
void hash_conflictset (char *, void *, unsigned int, struct http_attrihash **, int);
void hash_set (char *, void *, struct http_attrihash **, int, int, unsigned int (*) (char *, int));
void *hash_search (char *, struct http_attrihash **, int, unsigned int (*) (char *, int));
void hash_ergodic (int, struct http_attrihash **, void (*) (void *));
unsigned int crc32_port (char *, int);



extern char *method_lst[];

# endif
