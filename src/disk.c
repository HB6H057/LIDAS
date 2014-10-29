# include "main.h"
# include "disk.h"
# include "thread.h"

static struct disk_pool_head **dpool_lst;
int disk_cnt = 0;

struct disk_pool_h *disk_pool_creat (int num, int *ids, void **addrs)
{   
   pid_t pid;
   int err, fd, i = 0;
   struct disk_pool_h *ret = malloc (sizeof (struct disk_pool_h)), *new = ret;
   void *p;
   char name[15];
   struct stat statbuf;
   
   while (num--){
      if ((pid = vfork ()) < 0){
         lidalog (LIDA_ERR, "vfork");
         return NULL;
      }
      /*Alloc disk space*/
      else if (pid == 0){
         sprintf (name, "/var/lida/cache/%d.cache", num);
         execl ("/usr/bin/fallocate", "-l", "1G", name, NULL);
      }

      /*Load this area*/
      fd = open (name, O_RDWR | O_DSYNC);

      if (fstat (fd, &statbuf) < 0){
         lidalog (LIDA_ERR, "fstat");
         return  NULL;
      }

      if ((p = mmap (0, statbuf.st_size, PROT_READ | PROT_WRITE, 
           MAP_SHARED, fd, 0)) == MAP_FAILED){
	      lidalog (LIDA_ERR, "mmap");
	      return NULL;
	   }

      *(ids + i) = fd;
      *(addrs + i) = p;

      p = disk_pool_locate (ret, DISK_CELL, DISK_CELL);
      new->next = p;
      new = new->next;
      i++;
   }

   new = ret->next;
   free (ret);
   return new;
}


struct disk_pool_t *disk_pool_locate (void *addr, size_t size, int num)
{
   struct disk_pool_t *ret = addr, t;
   size_t space = sizeof (struct disk_pool_t);

   /*Set up pool spaces*/
   t.prev = NULL;
   t.next = addr + size;

   memcpy (addr, (void *)&t, space);
   msync (addr, space, MS_SYNC);
   addr += size;
   
   while (--num)
   {
      t.prev = addr - size;
      t.next = addr + size;
      memcpy (addr, (void *)&t, space);
      msync (addr, space, MS_SYNC);
      addr += size;
   }

   return ret;
}


void *disk_pool_alloc (struct disk_pool_h *p, int *ids, void **addrs)
{
   if (p == NULL)
      return NULL;

   struct disk_pool_t *ret;
   int i = 0, n;
   size_t offset;
 
   while (p != NULL){
      if (!(p->n)){
         p = p->block_next;
	 i++;
	 continue;
      }

      /*Pop the pool stack*/
      pthread_mutex_lock (&thread_lock);
      
      ret = p->next;

      if (lock_reg (*(ids+i), F_SETLK, F_WRLCK, 0, SEEK_SET, sizeof (struct disk_pool_h)) < 0)
         return NULL;
      p->next = ret->next;
      p->n--;
      lock_reg (*(ids+i), F_SETLK, F_UNLCK, 0, SEEK_SET, sizeof (struct disk_pool_h))
      
      if (lock_reg (*(ids+i), F_SETLK, F_WRLCK, (void *)(p->next) - *(addrs+i), SEEK_SET, 32768) < 0)
         return NULL;
      ret->next->prev = NULL;
      lock_reg (*(ids+i), F_SETLK, F_UNLCK, (void *)(p->next) - *(addrs+i), SEEK_SET, 32768);

      pthread_mutex_lock (&thread_lock);


      if (lock_reg (*(ids+i), F_SETLK, F_WRLCK, (void *)ret - *(addrs+i), SEEK_SET, 32768) < 0)
         return NULL;
      memset (ret, 0, p->size);
      lock_reg (*(ids+i), F_SETLK, F_UNLCK, (void *)ret - *(addrs+i), SEEK_SET, 32768);
      return (ret);
   }

   return NULL;
}


void disk_pool_free (void *g, struct disk_pool_h *head)
{
   struct disk_pool_t t;
   
   t.next = head->next;
   t.prev = NULL;
   memcpy (g, (void *)&t, sizeof (struct disk_pool_t));
   head->next = (struct disk_pool_t *)g;
   head->n++;
}
