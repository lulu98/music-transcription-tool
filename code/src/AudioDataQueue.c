/**
@file AudioDataQueue.c
@author Lukas Graber
@date 30 May 2019
@brief Implementiation of functions to organise captured audio data in a queue.
**/
#include <stdio.h>
#include <stdlib.h>

#include "../include/Structures.h"

#define SUCCESS 0   ///< operation succeeded
#define ERR_INVAL 1 ///< error message 1
#define ERR_NOMEM 2 ///< error message 2

#define FALSE 0 ///< defines boolean value false
#define TRUE 1 ///< defines boolean value true

/**
While the front of the queue is still pointing to a queue node, then remove the node and point to the next node.
At some point there are no other nodes in the queue and the front is pointing to NULL due to the back of the queue
pointing to NULL.
**/
int freeAudioDataQueue(AudioDataQueue *queue){
  if (queue == NULL) {
    return ERR_INVAL;
  }
  while (queue->front != NULL) {
    struct queue_node *node = queue->front;
    queue->front = node->next;
    free(node);
  }
  //free(queue);
  return SUCCESS;
}

/**
While the front of the queue is still pointing to a queue node, then remove the node and point to the next node.
At some point there are no other nodes in the queue and the front is pointing to NULL due to the back of the queue
pointing to NULL.
**/
int queue_destroy(AudioDataQueue *queue) {
  if (queue == NULL) {
    return ERR_INVAL;
  }
  while (queue->front != NULL) {
    struct queue_node *node = queue->front;
    queue->front = node->next;
    free(node);
  }
  //free(queue);
  return SUCCESS;
}

/**
The queue is empty if it is not initialised (queue==NULL) or the front is pointing to no node (queue->front==NULL).
**/
int queue_empty(AudioDataQueue *queue) {
  if (queue == NULL || queue->front == NULL) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
Sets all the properties of the queue to their initial values. The front as well as the back of the queue point to NULL.
**/
void initAudioDataQueue(AudioDataQueue *queue){
  //queue = malloc(sizeof(*queue));
  queue->size = 0;
  if (queue == NULL) {
    return;
  }
  queue->front = queue->back = NULL;
}

/**
Sets all the properties of the queue to their initial values. The front as well as the back of the queue point to NULL.
The function returns the constructed queue.
**/
AudioDataQueue *queue_new(void) {
  AudioDataQueue *queue = malloc(sizeof(*queue));
  queue->size = 0;
  if (queue == NULL) {
    return NULL;
  }
  queue->front = queue->back = NULL;
  return queue;
}

/**
The function removes the first node to which front points to, reduces its size and lets the front point to the next node.
**/
AudioCapturePoint *queue_dequeue(AudioDataQueue *queue) {
  if (queue == NULL || queue->front == NULL) {
    return NULL;
  }
  struct queue_node *node = queue->front;
  AudioCapturePoint *data = node->data;
  queue->front = node->next;
  if (queue->front == NULL) {
    queue->back = NULL;
  }
  free(node);
  queue->size--;
  return data;
}

/**
First a new node is created that will be the new back of the queue. Hence it needs to point to NULL. The current back
is pointing to the new element and the size of the queue gets increased.
**/
int queue_enqueue(AudioDataQueue *queue, AudioCapturePoint *data) {
  if (queue == NULL) {
    return ERR_INVAL;
  }
  struct queue_node *node = malloc(sizeof(*node));
  if (node == NULL) {
    return ERR_NOMEM;
  }
  node->data = data;
  node->next = NULL;
  if (queue->back == NULL) {
    queue->front = queue->back = node;
  } else {
    queue->back->next = node;
    queue->back = node;
  }
  queue->size++;
  return SUCCESS;
}
