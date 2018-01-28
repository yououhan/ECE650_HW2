#include "my_malloc.h"
#include <unistd.h>

extern Node * allocatedListHead;
extern void * heapTop;

void addToLinkedList(Node * prev, Node * toBeAdded) {
  Node * temp = prev->next;
  prev->next = toBeAdded;
  toBeAdded->next = temp;
  toBeAdded->prev = prev;
  if (temp != NULL) {
    temp->prev = toBeAdded;
  }
  toBeAdded->next_block = NULL;
  if (toBeAdded->next != NULL) toBeAdded->next_block = toBeAdded->next->next_block;
  temp = toBeAdded->prev;
  while (temp != NULL && temp->next_block == toBeAdded->next) {
    temp->next_block = toBeAdded;
    temp = temp->prev;
  }
}

void deleteFromLinkedList(Node * toBeDeleted) {
  Node * prev = toBeDeleted->prev;
  prev->next = toBeDeleted->next;
  if (toBeDeleted->next != NULL) {
    toBeDeleted->next->prev = prev;
  }
  prev = toBeDeleted->prev;
  while (prev != NULL && (prev->next_block == toBeDeleted->next_block || prev->next_block == toBeDeleted)) {
    prev->next_block = toBeDeleted->next;
    prev = prev->prev;
  }
}

Node * getPrevNode(Node * prevBlock) {
  Node * prev;
  if (prevBlock->next_block == NULL) {
    Node * curr = prevBlock;
    while (curr->next != NULL) {
      curr = curr->next;
    }
    prev = curr;
  } else {
    prev = prevBlock->next_block->prev;
  }
  return prev;
}
void initNode(Node * node, Node * prev, size_t size) {
  node->start_address = (void *)((size_t)node + (size_t)sizeof(Node));
  node->end_address = (void *)((size_t)node->start_address + size);
  addToLinkedList(prev, node);
}

void initListHead() {
  allocatedListHead= sbrk(sizeof(Node));
  allocatedListHead->next = NULL;
  allocatedListHead->prev = NULL;
  allocatedListHead->next_block = NULL;
  heapTop = allocatedListHead + 1;//exclusive
  allocatedListHead->start_address = heapTop;
  allocatedListHead->end_address = heapTop;
}

Node * initNewAllocatedNode(Node * prev, size_t size) {
  Node * newAllocatedNode = NULL;
  if (((size_t)heapTop - (size_t)prev->end_address) >= (size + sizeof(Node))) {
    newAllocatedNode = heapTop - size - sizeof(Node);
  } else {
    size_t increment = size + sizeof(Node) - ((size_t)heapTop - (size_t)prev->end_address);
    heapTop = (void *)((size_t)sbrk(increment) + increment);
    newAllocatedNode = (Node *)((size_t)heapTop - size - sizeof(Node));
  }
  return newAllocatedNode;
}

//First Fit malloc/free
void *ff_malloc(size_t size) {
  if (allocatedListHead == NULL) {
    initListHead();
  }
  Node * curr = allocatedListHead->next_block; 
  Node * prev = allocatedListHead;
  Node * newAllocatedNode = NULL;
  while (curr != NULL) {
    if (((size_t)curr - (size_t)curr->prev->end_address) >= (size + sizeof(Node))) {
      newAllocatedNode = (Node *)((size_t)curr - size - sizeof(Node));
      break;
    }
    prev = curr;
    curr = curr->next_block;      
  }
  prev = getPrevNode(prev);
  if (newAllocatedNode == NULL) {
    newAllocatedNode = initNewAllocatedNode(prev, size);
  }
  initNode(newAllocatedNode, prev, size);
  return (void *)(newAllocatedNode->start_address);
}

void ff_free(void *ptr) {
  if (ptr != NULL) {
    //printf("deleting %p\n", ptr);
    Node * toBeDeleted = (Node *)((size_t)ptr - (size_t)sizeof(Node));
    deleteFromLinkedList(toBeDeleted);
  }
}

//Best Fit malloc/free
void *bf_malloc(size_t size) {
  if (allocatedListHead == NULL) {
    initListHead();
  }
  Node * curr = allocatedListHead->next_block; 
  Node * prev = allocatedListHead;
  Node * newAllocatedNode = NULL;
  Node * bestFitPrev = NULL;
  size_t minSize = SIZE_MAX;
  while (curr != NULL) {
    size_t currSize = (size_t)curr - (size_t)curr->prev->end_address;
    if (currSize >= (size + sizeof(Node)) && currSize < minSize) {
	newAllocatedNode = (Node *)((size_t)curr - size - sizeof(Node));
	minSize = currSize;
	bestFitPrev = prev;
	if (currSize == (size + sizeof(Node))) break;
    }
    prev = curr;
    curr = curr->next_block;      
  }
  if (bestFitPrev == NULL) bestFitPrev = prev;
  bestFitPrev = getPrevNode(bestFitPrev);
  if (newAllocatedNode == NULL) {
    newAllocatedNode = initNewAllocatedNode(bestFitPrev, size);
  }
  initNode(newAllocatedNode, bestFitPrev, size);
  return (void *)(newAllocatedNode->start_address);
}

void bf_free(void *ptr) {
  ff_free(ptr);
}

unsigned long get_data_segment_size() {
  return (size_t)heapTop - (size_t)allocatedListHead;
}

unsigned long get_data_segment_free_space_size() {
  Node * curr = allocatedListHead;
  unsigned long dataSize = 0;
  size_t counter = 0;
  while (curr != NULL) {
    dataSize += curr->end_address - curr->start_address;
    curr = curr->next;
    counter++;
  }
  return (get_data_segment_size() - dataSize - counter * sizeof(Node));
}
