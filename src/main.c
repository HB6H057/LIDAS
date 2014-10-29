# include "global.h"
# include "main.h"
# include "ghost.h"


int main (int argc, char *argv[])
{
   puts ("LIDA is activated");

   /*Go to daemon mode directly*/
   port_to_ghost ();
}


void port_to_ghost ()
{
   pid_t pid;
   
   if ((pid = fork ()) < 0)
   {
      perror ("fork");
      exit (1);
   }
   
   else if (pid == 0)
      backend ();
   
   else {
      puts ("ghost to start");
      exit (0);
   }
}


void lida_breakline (char *str)
{
   char *p = str;
   
   while (*p)
      {
         if ((*p == '\r') || (*p == '\n'))
            *p = '\0';
         p++;
      }
}
