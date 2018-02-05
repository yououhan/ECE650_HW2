#ifndef __MY_MALLOC_H__
#define __MY_MALLOC_H__
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include<pthread.h>

struct Node_t {
  struct Node_t * next;
  size_t size;
};
typedef struct Node_t Node;
//Thread Safe malloc/free: locking version
void *ts_malloc_lock(size_t size);
void ts_free_lock(void *ptr);
//Thread Safe malloc/free: non-locking version
void *ts_malloc_nolock(size_t size);
void ts_free_nolock(void *ptr);
//global variable to store the freed memory list
Node * head = NULL;//free list
void * heapTop = NULL;
pthread_rwlock_t  rwlock = PTHREAD_RWLOCK_INITIALIZER;
__thread Node * TLS_head = NULL;//free list
pthread_mutex_t sbrk_lock;
#endif
