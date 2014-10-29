# include "global.h"
#include "heap.h"
/*
 struct heap_t
{
   unsigned int key;  //key 
   void *data;	      //value
};

struct heap_h
{
   int size;	//大小
   int cnt;	//计数
   struct heap_t *p;  //第一个元素地址
};
 
 
 */


/*
 hHeap:堆头
 size ：堆大小
 */
int heap_init(struct heap_h *hHeap, unsigned int size, void *data, unsigned int key) //初始化
{
  //初始化堆头
  hHeap->size	= size;
  hHeap->cnt	= 1;
  hHeap->p	= NULL;
  //为堆体申请空间
  hHeap->p = (struct heap_t *)calloc(size + 1, sizeof(struct heap_t)); 
  if (hHeap->p == NULL)
  {
    puts("Head Body create faild.\n");
    return 1;
  }
  
  hHeap->p[1].key = key;
  hHeap->p[1].data = data;
  
  return 0;
}

/*
 data	:数组数据（任意类型）
 key	：键值
 hHeap	:堆头
 */
int heap_insert(void *data, unsigned int key, struct heap_h *hHeap)
{
  unsigned int i;
  if (hHeap->size <= hHeap->cnt)  //判断堆是否满了
  {
    puts("Heap is full!\n");
    return 1;
  }
  
  for(i = ++hHeap->cnt; hHeap->p[i/2].key < key; i /= 2 )
    hHeap->p[i] = hHeap->p[i/2];
  
  hHeap->p[i].key = key;
  hHeap->p[i].data = data;
  
  return 0;
}

struct heap_t* heap_max_delete(struct heap_h *hHeap)    //删除大头
{
    int i, Child;
    struct heap_t* MinElement, LastElement;
 
    if( hHeap->cnt == 0 )
    {
        puts("Queue is empty.");
        return NULL;
    }
    
    MinElement = &(hHeap->p[1]);
    LastElement = hHeap->p[hHeap->cnt--];
 
    for( i = 1; i*2 <= hHeap->cnt; i = Child )
    {
        /* Find max child. */
        Child = i*2;
        if( Child != hHeap->cnt && hHeap->p[Child+1].key > hHeap->p[Child].key )
            Child++;
 
        /* Percolate one level. */
        if( LastElement.key > hHeap->p[Child].key )
            hHeap->p[i] = hHeap->p[Child];
        else
            break;
    }
    hHeap->p[i] = LastElement;
    return MinElement;
}

/*
hHeap	:堆头
handler	:回调函数
 */
int heap_traversal(struct heap_h *hHeap, void (*handler)(void *data))     //遍历
{
  int i;
  struct heap_t *pHeap_t = NULL;
  pHeap_t = hHeap->p;
  if (hHeap == NULL) //error
    return 1;
  
  for (i = 1; i <= hHeap->cnt; ++i)
  {
    handler(pHeap_t[i].data);
  }
  
  return 0;
}
