#include "my_malloc.h"
#include <unistd.h>
void initListHead_lock() {
  if (head == NULL) {
    pthread_rwlock_wrlock(&rwlock);
    if (head == NULL) {
      head= sbrk(sizeof(Node));
      head->next = NULL;
      head->size = 0;
      heapTop = head + 1;//exclusive
    }
    pthread_rwlock_unlock(&rwlock);
  }
}

void addToFreedList_lock(Node * prev, Node * toBeAdded) {
  if (prev == NULL) {
    prev = head;
    Node * curr = head->next;
    while (curr != NULL && (size_t)curr < (size_t)toBeAdded) {
      prev = curr;
      curr = curr->next;
    }
  }
  Node * newAddedNode = NULL;
  if ((size_t)prev + prev->size + sizeof(Node) == (size_t)toBeAdded) {//combine with the previous node
    prev->size += toBeAdded->size + sizeof(Node);
    newAddedNode = prev;
  } else {
    toBeAdded->next = prev->next;
    prev->next = toBeAdded;
    newAddedNode = toBeAdded;
  }
  if ((size_t)newAddedNode + newAddedNode->size + sizeof(Node) == (size_t)newAddedNode->next) {//combine with the next node
    newAddedNode->size += newAddedNode->next->size + sizeof(Node);
    newAddedNode->next = newAddedNode->next->next;
  }
}

Node * allocateNewSpace_lock(Node * prev, size_t size) {
  Node * newAllocatedNode = NULL;
  size_t increment = size + sizeof(Node);
  //  Node * newFreeNode = heapTop;
  newAllocatedNode = (Node *)sbrk(increment);
  //  newFreeNode->size = increment;
  //  addToFreedList_lock(prev, newFreeNode);
  newAllocatedNode->size = size;
  return newAllocatedNode;
}

//Best Fit malloc/free
void *ts_malloc_lock(size_t size) {
  initListHead_lock();
  Node * minSizePrev = NULL;
  Node * newAllocatedNode = NULL;
  size_t minSize = SIZE_MAX;
  pthread_rwlock_rdlock(&rwlock);
  Node * prev = head;
  Node * curr = head->next;
  while (curr != NULL) {
    if (curr->size >= size && curr->size < minSize) {
      newAllocatedNode = curr;
      minSizePrev = prev;
      minSize = curr->size;
      if (minSize == size) {
	break;
      }
    }
    prev = curr;
    curr = curr->next;
  }
  pthread_rwlock_unlock(&rwlock);
  pthread_rwlock_wrlock(&rwlock);
  if (newAllocatedNode == NULL) {//if the free list has no data segment that satisfies the needs
    //if (prev == NULL) return ts_malloc_lock(size);
    //    while (prev->next != NULL) {prev = prev->next;}
    newAllocatedNode = allocateNewSpace_lock(prev, size);
  } else if (newAllocatedNode->size < size || minSizePrev->next != newAllocatedNode) {
    pthread_rwlock_unlock(&rwlock);
    return ts_malloc_lock(size);
  } else if (newAllocatedNode->size > size + sizeof(Node)) {
    newAllocatedNode->size -= size + sizeof(Node);
    newAllocatedNode = (Node *)((size_t)newAllocatedNode + newAllocatedNode->size + sizeof(Node));
    newAllocatedNode->size = size;
  } else {
    minSizePrev->next = newAllocatedNode->next;
  }
  newAllocatedNode->next = NULL;
  pthread_rwlock_unlock(&rwlock);
  return (void *) ((size_t)newAllocatedNode + sizeof(Node));  
}

void ts_free_lock(void *ptr) {
  if (ptr != NULL) {
    Node * newFreeNode = (Node *)((size_t)ptr - sizeof(Node));
    pthread_rwlock_wrlock(&rwlock);
    addToFreedList_lock(NULL, newFreeNode);
    pthread_rwlock_unlock(&rwlock);
  }
}

void initListHead_nolock() {
  if (TLS_head == NULL) {
    pthread_mutex_lock(&sbrk_lock);
    TLS_head= sbrk(sizeof(Node));
    pthread_mutex_unlock(&sbrk_lock);
    TLS_head->next = NULL;
    TLS_head->size = 0;
  }
}
  
void addToFreedList_nolock(Node * prev, Node * toBeAdded) {
  initListHead_nolock();
  if (prev == NULL) {
    prev = TLS_head;
    Node * curr = TLS_head->next;
    while (curr != NULL && (size_t)curr < (size_t)toBeAdded) {
      prev = curr;
      curr = curr->next;
    }
  }
  Node * newAddedNode = NULL;
  if ((size_t)prev + prev->size + sizeof(Node) == (size_t)toBeAdded) {//combine with the previous node
    prev->size += toBeAdded->size + sizeof(Node);
    newAddedNode = prev;
  } else {
    toBeAdded->next = prev->next;
    prev->next = toBeAdded;
    newAddedNode = toBeAdded;
  }
  if ((size_t)newAddedNode + newAddedNode->size + sizeof(Node) == (size_t)newAddedNode->next) {//combine with the next node
    newAddedNode->size += newAddedNode->next->size + sizeof(Node);
    newAddedNode->next = newAddedNode->next->next;
  }
}

Node * allocateNewSpace_nolock(Node * prev, size_t size) {
  size_t increment = size + sizeof(Node);
  pthread_mutex_lock(&sbrk_lock);
  //  Node * newFreeNode = sbrk(0);//find the top of heap 
  Node * newAllocatedNode = (void *)((size_t)sbrk(increment) + sizeof(Node));
  pthread_mutex_unlock(&sbrk_lock);
  //newFreeNode->size = (size_t)newAllocatedNode - (size_t)newFreeNode - sizeof(Node);
  newAllocatedNode->size = size;
  //addToFreedList_nolock(prev, newFreeNode);
  return newAllocatedNode;
}

void *ts_malloc_nolock(size_t size) {
  initListHead_nolock();
  Node * prev = TLS_head;
  Node * curr = TLS_head->next;
  Node * minSizePrev = NULL;
  Node * newAllocatedNode = NULL;
  size_t minSize = SIZE_MAX;
  while (curr != NULL) {
    if (curr->size >= size && curr->size < minSize) {
      //      curr->size -= size + sizeof(Node);
      //      newAllocatedNode = (Node *)((size_t)curr + curr->size + sizeof(Node));
      newAllocatedNode = curr;
      minSizePrev = prev;
      minSize = curr->size;
      if (minSize == size) {
	break;
      }
    }
    prev = curr;
    curr = curr->next;
  }
  if (newAllocatedNode == NULL) {//if the free list has no data segment that satisfies the needs
    newAllocatedNode = allocateNewSpace_nolock(prev, size);
  } else {
    if (minSize > size + sizeof(Node)) {
      newAllocatedNode->size -= size + sizeof(Node);
      newAllocatedNode = (Node *)((size_t)newAllocatedNode + newAllocatedNode->size + sizeof(Node));
      newAllocatedNode->size = size;
    } else {
      minSizePrev->next = newAllocatedNode->next;
    }
  }
  return (void *) ((size_t)newAllocatedNode + sizeof(Node));  
}

void ts_free_nolock(void *ptr) {
  if (ptr != NULL) {
    Node * newFreeNode = (Node *)((size_t)ptr - sizeof(Node));
    addToFreedList_nolock(NULL, newFreeNode);
  }
}
