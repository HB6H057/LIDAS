# ifndef SND_H_
# define SND_H_

struct http_response_h
{
   char *version;
   char *status;
   char *content_type;
   char *content_length;
   char *date;
   char *mtime;
};

int post_loop (int, void *, size_t, struct event_data *);
void post_file (char *, char *, char *, int, struct event_data *);
int post_header (struct http_response_h *, int);
void error_post (int, int);
int stream_to_socket (FILE *, int, struct event_data *);

# endif
