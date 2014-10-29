# include "global.h"
# include "config.h"
# include "log.h"
# include "manager.h"
# include "timer.h"

static struct timer_opt *obj_tm;
static pthread_t manager_id;
static time_t task_timeout;


void timer_trigger (int signo)
{
   time_t cur;
   
   timer_task (obj_tm->t, time (&cur), obj_tm->pool); 
   alarm (task_timeout);
}


void *manager_thread (void *arg)
{
   int err;
   obj_tm = (struct timer_opt *)arg;
   
   /*Set up manager signal functions*/
   sigset_t sigset;

   sigemptyset (&sigset);
   sigaddset (&sigset, SIGUSR1);
   sigaddset (&sigset, SIGUSR2);
   sigaddset (&sigset, SIGINT);
   sigaddset (&sigset, SIGCHLD);

   if (err = pthread_sigmask (SIG_BLOCK, &sigset, NULL)){
      lidalog (LIDA_ACCESS, strerror (err));
      pthread_exit ((void *)-1);
   }

   /*Delete timeout events*/
   struct sigaction act;

   act.sa_handler = timer_trigger;

   if (sigaction (SIGALRM, &act, NULL) < 0){
      lidalog (LIDA_ERR, "sigaction");
      pthread_exit ((void *)-1);
   }

   /*Start timer*/
   task_timeout = (true_type.keep_alive/5)?true_type.keep_alive/5:1;
   alarm (task_timeout);
   
   while (1);
}
