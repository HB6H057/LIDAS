# include <stdio.h>

int main ()
{
   FILE *fp;
   fp = fopen ("config.txt", "r");
   char name[20], sex[20];
   fscanf (fp, "name=%s\nsex=%s", name, sex);
   puts (name);
   puts (sex);

   return 1;
}
