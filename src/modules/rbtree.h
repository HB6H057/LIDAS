struct rbtree_t 
{
   struct rbtree_t *r_child;
   struct rbtree_t *l_child;
   struct rbtree_t *father;

   unsigned color : 2;
   unsigned nil : 2;
   unsigned type : 4;

   int key;
   void *data;
};

# define RBT_RED 1
# define RBT_BLACK 0

# define RBT_TIMER 0

# define RBT_SEARCH 0
# define RBT_DEL 1
