# ifndef MANAGER_H_
# define MANAGER_H_

// static pthread_t manager_id;
// static time_t task_timeout;

void timer_trigger (int);
void *manager_thread (void *);

# endif
