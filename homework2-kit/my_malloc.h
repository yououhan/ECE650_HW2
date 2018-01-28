#ifndef __MY_MALLOC_H__
#define __MY_MALLOC_H__
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
struct Node_t {
  void * start_address;
  void * end_address;
  //  size_t size;
  struct Node_t * next;
  struct Node_t * prev;
  struct Node_t * next_block;  
};
typedef struct Node_t Node;
//First Fit malloc/free
void *ff_malloc(size_t size);
void ff_free(void *ptr);
//Best Fit malloc/free
void *bf_malloc(size_t size);
void bf_free(void *ptr);
unsigned long get_data_segment_size(); //in bytes
unsigned long get_data_segment_free_space_size(); //in bytes 
void addToLinkedList(Node * prev, Node * toBeAdded);
void deleteFromLinkedList(Node * toBeDeleted);
void initNode(Node * node, Node * prev, size_t size);
void initListHead();
Node * initNewAllocatedNode(Node * prev, size_t size);
//global variable to store the freed memory list
Node * allocatedListHead = NULL;
void * heapTop = NULL;
#endif
