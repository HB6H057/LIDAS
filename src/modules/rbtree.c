# include "rbtree.h"


struct rbtree_t *rbt_rewind (struct rbtree_t *n)
{
   while (n->father != NULL)
      n = n->father;

   return n;
}


struct rbtree_t *get_uncle (struct rbtree_t *p)
{
   if (p->father == NULL)
      return NULL;

   if (p->father->father == NULL)
      return NULL;

   if (p->father == p->father->father->l_child)
      return (p->father->father->r_child);

   else return (p->father->father->l_child);
}


struct rbtree_t *sibling (struct rbtree_t *n)
{
   if (n == n->father->l_child)
      return (n->father->r_child);

   else return (n->father->l_child);
}


void rotat_left (struct rbtree_t *p)
{
   struct rbtree_t *r = p->r_child;

   p->r_child = r->l_child;
   p->r_child->father = p;

   if (p->father != NULL){
      if (p == p->father->l_child)
         p->father->l_child = r;

      else p->father->r_child = r;

      r->father = p->father;
   }

   r->l_child = p;
   p->father = r;

   r->color = RBT_BLACK;
   p->color = RBT_RED;
}


void rotat_right (struct rbtree_t *p)
{
   struct rbtree_t *r = p->l_child;

   p->l_child = r->r_child;
   p->l_child->father = p;

   if (p->father != NULL)
   {
      if (p == p->father->l_child)
         p->father->l_child = r;
      
      else p->father->r_child = r;

      r->father = p->father;
   }

   r->r_child = p;
   p->father = r;

   r->color = RBT_BLACK;
   p->color = RBT_RED;
}


int rbtree_insert (struct rbtree_t **h, int key, void *data, struct mem_cache_head *pool, unsigned type)
{
   struct rbtree_t *new = mem_cache_alloc (pool);

   /*Set up new node*/
   new->color = RBT_RED;
   new->nil = 0;
   new->type = type;
   new->key = key;
   new->data = data;

   new->r_child = mem_cache_alloc (pool);
   new->l_child = mem_cache_alloc (pool);
   new->r_child->color = RBT_BLACK;
   new->l_child->color = RBT_BLACK;
   new->r_child->nil = 1;
   new->l_child->nil = 1;

   /*Insert at root node*/
   if (*h == NULL){
      *h = new;
      (*h)->color = RBT_BLACK;
      (*h)->father = NULL;
      (*h)->l_child->father = (*h);
      (*h)->r_child->father = (*h);

      return 0;
   }

   struct rbtree_t *p = *h;

   while (!(p->nil))
   {
      if (p->key == key)
         return -1;

      if (key > p->key)
         p = p->r_child;
      
      else if (key < p->key)
         p = p->l_child;
   }

   new->father = p->father;
   memcpy (p, new);
   new->l_child->father = p;
   new->r_child->father = p;
   mem_cache_free (new, pool);
   insert_case1 (p);

   *h = rbt_rewind (p);
   return 0;
}


void insert_case1 (struct rbtree_t *n)
{
   if (n->father->color == RBT_BLACK)
      return;

   else insert_case2 (n);
}


void insert_case2 (struct rbtree_t *n)
{
   if (get_uncle(n) != NULL && get_uncle(n)->color == RBT_RED)
   {
      n->father->color = RBT_BLACK;
      n->father->color = RBT_BLACK;
      
      if (n->father->father != NULL){
         if (n->father->father->father == NULL){
            n->father->father->color = RBT_BLACK;
	    return;
         }
	 
	 n->father->father->color = RBT_RED;
	 insert_case1 (n->father->father);
      }
   }

   else insert_case3 (n);
}


void insert_case3 (struct rbtree_t *n)
{
   if (n == n->father->r_child && n->father == n->father->father->l_child)
   {
      rotat_left (n->father);
      n = n->l_child;
   }

   else if (n == n->father->l_child && n->father == n->father->father->r_child)
   {
      rotat_right (n->father);
      n = n->r_child;
   }

   insert_case4 (n);
}


void insert_case4 (struct rbtree_t *n)
{
   n->father->color = RBT_BLACK;
   n->father->father->color = RBT_RED;

   if (n == n->father->left && n->father == n->father->father->l_child)
      rotat_right (n->father->father);

   else rotat_left (n->father->father);
}


void *rbtree_search (unsigned int key, struct rbtree_t *h, int cmd)
{
   struct rbtree_t *p = h;

   while (!(p->nil))
   {
      if (key == p->key)
         goto RET;
      
      if (key > p->key)
         p = p->r_child;
      
      else if (key < p->key)
         p = p->l_child;
   }

   return NULL;

RET:
   switch (cmd)
   {
      case RBT_SEARCH : return (p->data);
      case RBT_DEL    : return ((void *)p);
      default : return NULL;
   }
}


int rbtree_delete (int key, struct rbtree_t **h, 
                   void (*cleaner) (void *, struct mem_cache_head *), 
                   struct mem_cache_head *u_pool, struct mem_cache_head *pool)
{
   struct rbtree_t *t;
   struct rbtree_t *p = (struct rbtree_t *)rbtree_search (key, *h, 1);

   if (p == NULL)
      return (-1);

DEL_START:

   if (p->l_child->nil && p->r_child->nil)
   {
      (*cleaner) (p->data, u_pool);

      /*Set current node to nil*/
      p->color = RBT_BLACK;
      p->nil = 1;
      p->data = NULL;
      
      mem_cache_free ((void *)p->l_child, pool);
      mem_cache_free ((void *)p->r_child, pool);
      p->l_child = NULL;
      p->r_child = NULL;
      
      return 0;
   }

   else if (p->l_child->nil ^ p->r_child->nil)
   {
      t = p->l_child->nil?p->r_child:p->l_child;
      (*cleaner) (p->data, u_pool);
      mem_cache_free (p->l_child->nil?p->l_child:p->r_child, pool);
      
      /*Replace current node*/
      if (p->father == NULL)
         t->father = NULL;
      else {
         t->father = p->father;

	 if (p == p->father->l_child)
	    p->father->l_child = t;
	 else p->father->r_child = t;
      }
      
      if (p->color == RBT_BLACK){
         if (t->color == RBT_RED)
            t->color = RBT_BLACK;
         else delete_case1 (t);
      }
      
      mem_cache_free (p, pool);
      *h = rbt_rewind (t);
      return 0;
   }

   /*Current node has two child nodes*/
   else {
      (*cleaner) (p->data, u_pool);
      
      /*Find replace node*/
      for (t = p->l_child; !t->l_child->nil; t = t->r_child);
      p->data = t->data;
      p->key = t->key;

      /*Restart delete step*/
      p = t;
      goto DEL_START;
   }
}


void delete_case1 (struct rbtree_t *n)
{
   if (n->father != NULL)
      delete_case2 (n);
}


void delete_case2 (struct rbtree_t *n)
{
   struct rbtree_t *s = sibling (n);

   if (s->color == RBT_RED)
   {
      n->father->color = RBT_RED;
      n->color = RBT_BLACK;

      if (n == n->father->l_child)
         rotat_left (n->father);
      else rotat_right (n->father);
   }

   delete_case3 (n);
}


void delete_case3 (struct rbtree_t *n)
{
   struct rbtree_t *s = sibling (n);

   if ((n->father->color == RBT_BLACK) &&
       (s->color == RBT_BLACK) &&
       (s->l_child->color == RBT_BLACK) &&
       (s->r_child->color == RBT_BLACK)){
          s->color = RBT_RED;
	  delete_case1 (n->father);
       }

    else delete_case4 (n);
}


void delete_case4 (struct rbtree_t *n)
{
   struct rbtree_t *s = sibling (n);

   if ((n->father->color == RBT_RED) &&
       (s->color == RBT_BLACK) &&
       (s->l_child->color == RBT_BLACK) &&
       (s->r_child->color == RBT_BLACK)){
          s->color = RBT_RED;
	  n->father->color = RBT_BLACK;
       }

    else delete_case5 (n);
}


void delete_case5 (struct rbtree_t *n)
{
   struct rbtree_t *s = sibling (n);

   if (s->color == RBT_BLACK){
      if ((n == n->father->l_child) &&
          (s->r_child->color == RBT_BLACK) &&
	  (s->l_child->color == RBT_RED)){
	     s->color = RBT_RED;
	     s->l_child->color = RBT_BLACK;
             rotat_right (s);
	  }

       else if ((n == n->father->r_child) &&
                (s->l_child->color == RBT_BLACK) &&
		(s->r_child->color == RBT_RED)){
		   s->color = RBT_RED;
		   s->r_child->color = RBT_BLACK;
		   rotat_left (s);
		}
   }

   delete_case6 (n);
}


void delete_case6 (struct rbtree_t *n)
{
   struct rbtree_t *s = sibling (n);

   s->color = n->father->color;
   n->father->color = RBT_BLACK;

   if (n == n->father->l_child){
      s->r_child->color = RBT_BLACK;
      rotat_left (n->father);
   }

   else {
      s->l_child->color = RBT_BLACK;
      rotat_right (n->father);
   }
}


void rbtree_ergodic (struct rbtree_t *h, int (*handler) (void *, struct mem_cache_head *), struct mem_cache_head *pool, struct stack_t *ptr)
{
   if ((h == NULL) || h->nil)
      return;

   if ((*handler) (h->data, pool))
      stack_push (&(h->key), ptr);

   /*Recursional ergodic*/
   rbtree_ergodic (h->l_child, handler, pool);
   rbtree_ergodic (h->r_child, handler, pool);
}


void rbtree_stack_del (struct stack_t *s, struct rbtree_t *h, 
                       void (*cleaner) (void *, struct mem_cache_head *), 
                       struct mem_cache_head *u_pool, struct mem_cache_head *pool)
{
   int *k;

   while ((k = (int *)stack_pop (s)) != NULL)
      rbtree_delete (*k, &h, cleaner, u_pool, pool);
}
