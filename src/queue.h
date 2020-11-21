#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "types.h"

typedef struct OrderQueue {
	order_t** orderArray;
   int front;
   int rear;
   int itemCount;
   int max;
}OrderQueue;

OrderQueue* init_order_queue(int max) {
   OrderQueue* oq = malloc(sizeof(OrderQueue));
   oq->orderArray = malloc(max * sizeof(order_t*));
   oq->max = max;
	oq->front = 0;
   oq->rear = -1;
   oq->itemCount = 0;
   return oq;
}

bool isEmpty(OrderQueue* oq) {
   return oq->itemCount == 0;
}

bool isFull(OrderQueue* oq) {
   return oq->itemCount == oq->max;
}

int size(OrderQueue* oq) {
   return oq->itemCount;
}  

void insert(OrderQueue* oq, order_t* data) {
   if(!isFull(oq)) {
      if(oq->rear == oq->max - 1) {
         oq->rear = -1;            
      }       

      oq->orderArray[++oq->rear] = data;
      oq->itemCount++;
   }
}

order_t* removeData(OrderQueue* oq) {
   order_t* data = oq->orderArray[oq->front++];
	
   if(oq->front == oq->max) {
      oq->front = 0;
   }
	
   oq->itemCount--;
   return data;  
}