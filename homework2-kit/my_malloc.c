#include "my_malloc.h"
#include <unistd.h>
void initListHead_lock() {
  if (head == NULL) {
    head= sbrk(sizeof(Node));
    head->next = NULL;
    head->size = 0;
    heapTop = head + 1;//exclusive
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
  size_t increment = size + 2 * sizeof(Node);
  Node * newFreeNode = heapTop;
  heapTop = (void *)((size_t)sbrk(increment) + increment);
  newAllocatedNode = (Node *)((size_t)heapTop - size - sizeof(Node));
  newFreeNode->size = (size_t)newAllocatedNode - (size_t)newFreeNode - sizeof(Node);
  addToFreedList_lock(prev, newFreeNode);
  newAllocatedNode->size = size;
  return newAllocatedNode;
}

//Best Fit malloc/free
void *ts_malloc_lock(size_t size) {
  initListHead_lock();
  Node * prev = head;
  Node * curr = head->next;
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
    newAllocatedNode = allocateNewSpace_lock(prev, size);
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

void ts_free_lock(void *ptr) {
  if (ptr != NULL) {
    Node * newFreeNode = (Node *)((size_t)ptr - sizeof(Node));
    addToFreedList_lock(NULL, newFreeNode);
  }
}

void initListHead_nolock() {
  if (TLS_head == NULL) {
    pthread_mutex_lock(&sbrk_lock);
    TLS_head= sbrk(sizeof(Node));
    TLS_head->next = NULL;
    TLS_head->size = 0;
    heapTop = TLS_head + 1;//exclusive
    pthread_mutex_unlock(&sbrk_lock);
  }
}
  
void addToFreedList_nolock(Node * prev, Node * toBeAdded) {
  if (TLS_head == NULL) {
    initListHead_nolock();
  }
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
  Node * newAllocatedNode = NULL;
  size_t increment = size + 2 * sizeof(Node);
  pthread_mutex_lock(&sbrk_lock);
  Node * newFreeNode = heapTop;
  heapTop = (void *)((size_t)sbrk(increment) + increment);
  newAllocatedNode = (Node *)((size_t)heapTop - size - sizeof(Node));
  pthread_mutex_unlock(&sbrk_lock);
  newFreeNode->size = (size_t)newAllocatedNode - (size_t)newFreeNode - sizeof(Node);
  addToFreedList_nolock(prev, newFreeNode);
  newAllocatedNode->size = size;
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

unsigned long get_data_segment_size() {
  return (size_t)heapTop - (size_t)head;
}

unsigned long get_data_segment_free_space_size() {
  Node * curr = head;
  unsigned long dataSize = 0;
  while (curr != NULL) {
    dataSize += curr->size;
    curr = curr->next;
  }
  return dataSize;
}
