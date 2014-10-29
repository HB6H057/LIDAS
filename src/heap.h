#ifndef HEAP_H_
#define HEAP_H_
struct heap_t
{
   unsigned int key;  //key 
   void *data;	      //value
};

struct heap_h
{
   unsigned int size;	//大小
   int cnt;	//计数
   struct heap_t *p;  //第一个元素地址
};

int heap_init(struct heap_h *, unsigned int, void *, unsigned int key); //初始化
int heap_insert(void *, unsigned int, struct heap_h *);//插入
struct heap_t* heap_max_delete(struct heap_h *hHeap);    //删除大头
int heap_traversal(struct heap_h *hHeap, void (*handler)(void *data));  //遍历

#endif //HEAP_H__
