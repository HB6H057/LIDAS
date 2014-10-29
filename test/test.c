# include <stdio.h>
# include <unistd.h>
# include <stdlib.h>

int main ()
{
   int *a = (int *)malloc (sizeof (int));
   
   printf ("%ld\n", (long)a);
   
   int pid = fork ();
   
   if (pid == 0)
      printf ("%ld\n", (long)a);
      
    else waitpid ();
}
