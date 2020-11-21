#ifndef TYPES_H
#define TYPES_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>
#include <semaphore.h>

// CLIENT_MIN_THINK_SEC sets the min. time in seconds in which a client waits
// before ordering.
#define CLIENT_MIN_THINK_SEC 1
int clientMaxThinkSec;

// CLERK_MIN_WAIT_SEC sets the min. time in seconds in which a clerk waits while
// after receiving a clients order.
#define CLERK_MIN_WAIT_SEC 1
int clerkMaxWaitSec;

// COOK_MIN_WAIT_SEC sets the min. time in seconds in which a cooker waits to finish 
// preparing an order.
#define COOK_MIN_WAIT_SEC 1
int cookMaxWaitSec;

#define NUM_COOK    1
int numClients, numClerks;

void parseArgs(int argc, char *argv[]) {
	if (argc < 6) {
		printf("not enough arguments, run with: ./dog numClients numClerks maxClientThink maxClerkAnnotate maxCookerWait \n");
		exit(1);
	}
	numClients        = atoi(argv[1]);
	numClerks         = atoi(argv[2]);
	clientMaxThinkSec = atoi(argv[3]);
	clerkMaxWaitSec   = atoi(argv[4]);
	cookMaxWaitSec    = atoi(argv[5]);
}

typedef struct {
	int client_id;
	int password_num;
} order_t;

typedef struct {
	int id;
} client_t;

void client_inform_order(order_t* od, int clerk_id);
void client_think_order();
void client_wait_order(order_t* od);

typedef struct {
	int id;
} clerk_t;

void clerk_create_order();
void clerk_annotate_order();

typedef struct TicketCaller {
	// Current password being called, visible by all customers and clerks.
	int* current_password;

	// Each clerk has a single order spot where clients deposit orders when called.
	order_t** clerks_order_spot;

	// Random generated passwords avaiable, "decremented" by clients consuming
	// from it.
	int* available_passwords;

	// Random generated passwords already retrieved by clients, clerks "withdrawn"
	// and call one by one.
	int* retrieved_passwords;

	// Indicates the next password index to be consumed from 'generated_passwords'
	// or 'available_passwords' arrays.
	int gen_cursor, avl_cursor;

}TicketCaller;

TicketCaller* init_ticket_caller() {
	TicketCaller* pc = malloc(sizeof(TicketCaller));
	pc->current_password = malloc(numClerks*sizeof(int));
	pc->clerks_order_spot = malloc(numClerks*sizeof(order_t*));

	for (int i = 0; i < numClerks; i++) {
		pc->current_password[i] = -1;
		pc->clerks_order_spot[i] = NULL;
	}

	pc->available_passwords = calloc(numClients, sizeof(int));
	pc->retrieved_passwords = calloc(numClients, sizeof(int));
	srand(time(NULL));
	int seq = 0;
	for (int i = 0; i < numClients; i++) {
		seq += rand() % 50 + 1;
		pc->available_passwords[i] = seq;
	}

	pc->gen_cursor = 0;
	pc->avl_cursor = 0;
	return pc;
}

int get_unique_ticket(TicketCaller* pc) {
	int pw = pc->available_passwords[pc->avl_cursor];

	// store retrieved password on retrieved array, utilizing the same cursor.
	pc->retrieved_passwords[pc->avl_cursor] = pw;
	pc->avl_cursor++;
	return pw;
}

int get_retrieved_ticket(TicketCaller* pc) {
	int pw;
	if (pc->gen_cursor < numClients) {
		pw = pc->retrieved_passwords[pc->gen_cursor];
		pc->gen_cursor++;

	} else {
		// all client passwords were called.
		pw = -1;
	}
	return pw;
}

int* show_current_tickets(TicketCaller* pc) {
	int* cpw = malloc(numClerks*sizeof(int));
	for (int i = 0; i < numClerks; i++)
		cpw[i] = pc->current_password[i];
	return cpw;
}

void set_current_ticket(TicketCaller* pc, int pw, int clerk_id) {
	pc->current_password[clerk_id] = pw;
}

void anounce_clerk_order(order_t* od) {
	printf("ATDT --[%d, %d]-------------->\n", od->client_id, od->password_num);
}

void anounce_cooker_order(order_t* od) {
	printf("<--------------[%d, %d]-- COZN\n", od->client_id, od->password_num);
}

#endif