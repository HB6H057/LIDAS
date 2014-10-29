# include "global.h"
# include "ghost.h"
# include "config.h"
# include "listener.h"
//# include "cache.h"
# include "main.h"
# include "log.h"

jmp_buf env;
static int log_fd;

int msock;
static int *listener_lst;
static unsigned QUIT_flag = 0;

void backend ()
{
   pid_t sid;
   int proc_info, i, fd;
   pid_t pid;

   /*Init settings*/
   if (setjmp (env) != 0){
      lidalog (LIDA_ACCESS, "Error occurs during config");
      exit (1);
   }

   lida_read_default ();
   load_proxy ();
   lida_read_hosts ();

   /*Init daemon process*/
   umask (0);
   
   if ((sid = setsid ()) < 0)
      ERR_EXIT ("setsid");
   
   if (chdir ("/") < 0)
      ERR_EXIT ("chdir");
   
   close(STDIN_FILENO);
   close(STDOUT_FILENO);
   close(STDERR_FILENO);
   
   /*Init log file*/
   if ((log_fd = open ("/var/lida/log/access.log", O_WRONLY)) < 0)
      ERR_EXIT ("open");

   /*Init child processes*/
   lida_fork ();
   
   /*Register signal handlers*/
   if (signal (SIGCHLD, check_client) == SIG_ERR)
      ERR_EXIT ("signal");
   
   if (signal (SIGINT, lida_reboot) == SIG_ERR)
      ERR_EXIT ("signal");

   if (signal (SIGKILL, read_exit) == SIG_ERR)
      ERR_EXIT ("signal");

   /*Init shared memories and disks
   (void *)rbt_cache_pool = malloc (PAGE_SIZ*2);
   (void *)cache_info_pool = malloc (PAGE_SIZ*2);
   (void *)cache_shm_pool = malloc (true_type.cache_mem_max);

   if ((shm_id[0] = shmget (IPC_PRIVATE, PAGE_SIZ*2, IPC_CREAT)) < 0){
      lidalog (LIDA_ERR, "shmget");
      exit (1);
   }

   if ((shm_id[1] = shmget (IPC_PRIVATE, PAGE_SIZ*2, IPC_CREAT)) < 0){
      lidalog (LIDA_ERR, "shmget");
      exit (1);
   }

   if ((shm_id[2] = shmget (IPC_PRIVATE, true_type.cache.mem_max, IPC_CREAT)) < 0){
      lidalog (LIDA_ERR, "shmget");
      exit (1);
   }

   cache_shd_pool = disk_pool_creat (true_type.cache.disk_max);*/

   /*Enter loop to get signals*/
   while (1);
}


void lida_reboot (int sig)
{
   lida_read_default ();
   load_proxy ();
   lida_read_hosts ();

   /*Send all child processes SIGINT signal*/
   if (kill (0, SIGINT) < 0){
      lidalog (LIDA_ERR, "kill");
      exit (1);
   }

   /*Restart service*/
   lida_fork ();
}


void lida_fork ()
{
   int i, pid;
   int m = getpid ();
   
   listener_lst = (int *)calloc (true_type.listener, sizeof (int));

   /*Set up listen port*/
   if ((msock = passiveTCP ()) < 0)
      exit (1);
      
   /*Start file cache process
   pid = fork ();
   
   if (pid < 0)
      ERR_EXIT ("fork");
      
   else if (pid == 0)
      lida_cache_proc (m);*/
   /*Save cache pid for listeners
   else cache_pid = pid;*/
   
   /*Start listener process*/
   for (i=0; i<true_type.listener; i++)
   {
      pid = fork ();
      
      if (pid < 0){
         lidalog (LIDA_ERR, "fork");
         exit (1);
      }

      else if (pid == 0)
         listener_proc (msock, m);
         
      /*Record the listeners pids*/
      else *(listener_lst+i) = pid;
   }
}


int passiveTCP ()
{  
   int socket_fd;
   struct sockaddr_in options;
   
   if ((socket_fd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
   {
      lidalog (LIDA_ERR, "socket");
      return ERROR;
   }
   
   bzero (&options, sizeof (struct sockaddr_in));
   
   options.sin_family = AF_INET;
   options.sin_port = htons (true_type.port);
   options.sin_addr.s_addr = htonl (INADDR_ANY);
   
   if (bind (socket_fd, (struct sockaddr *)&options, sizeof (struct sockaddr_in)) < 0)
   {
      lidalog (LIDA_ERR, "bind");
      return ERROR;
   }
   
   if (listen (socket_fd, true_type.connect * true_type.listener) < 0)
   {
      lidalog (LIDA_ERR, "listen");
      return ERROR;
   }
   
   return socket_fd;
}


void check_client (int sig)
{
   int status, chid, pid, i;
   pid_t m = getpid ();
   
   /*Check which one is dead*/
   while ((chid = waitpid (0, &status, WNOHANG)) >= 0)
   {
      if (WIFSIGNALED (status) && !QUIT_flag){
        /*cache proc end*/
        /*if (chid == cache_pid)
	{
	   lidalog (LIDA_ACCESS, "cache proc end up unproper, restarting");
           pid = fork ();
           if (pid < 0){
              lidalog (LIDA_ERR, "fork");
              exit (1);
           }
           else if (pid == 0)
	      lida_cache_proc (m);
	   else cache_pid = pid;
	}*/

	//else
	//{
	   for (i=0; i<true_type.listener; i++)
	   {
	      if (*(listener_lst+i) == chid)
	      {
	         lidalog (LIDA_ACCESS, "listenser process died unproper, restart");
		 /*Resstart listener process*/
		 pid = fork ();
		 if (pid < 0){
		    lidalog (LIDA_ERR, "fork");
		    exit (1);
		 }
		 else if (pid == 0)
		    listener_proc (msock, m);
		 else 
		    *(listener_lst+i) = pid;
	      }
	   }
	//}
      }
   }
}


void read_exit (int sig)
{
   int i;
   
   /*Send all child processes SIGKILL signal*/
   if (kill (0, SIGKILL) < 0)
      lidalog (LIDA_ERR, "kill");

   lidalog (LIDA_ACCESS, "Exiting.........");

   for (i=0; i<true_type.listener; i++)
      waitpid (*(listener_lst+i), NULL, 0);
   lidalog (LIDA_ACCESS, "All listener is down");

   //waitpid (cache_pid, NULL, 0);
   exit (0);
}
