#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "types.h"
#include "queue.h"

TicketCaller* pc; // Ticket Caller
OrderQueue* queue; // Fila de pedidos
order_t** pedidos; // Balcão de pedidos

// Semáforo de senhas
sem_t sem_clerk_ps;

// Semáforos individuais
sem_t* sem_client;
sem_t* sem_clerk;

// Mutex de operações na fila
pthread_mutex_t mutex_queue;

// Mutex de operações no Ticket Caller
pthread_mutex_t mutex_client, mutex_clerk;

// Cliente passa o pedido para o atendente
void client_inform_order(order_t* od, int clerk_id) {
	pc->clerks_order_spot[clerk_id] = od; // Atualiza pedido atual do atendente
	sem_post(&sem_clerk[clerk_id]); // Libera o atendente para continuar
}

// Cliente decide pedido
void client_think_order() {
	sleep(rand() % (clientMaxThinkSec + CLIENT_MIN_THINK_SEC) + CLIENT_MIN_THINK_SEC);
}

// Cliente espera o pedido ficar pronto
void client_wait_order(order_t* od) {
	sem_wait(&sem_client[od->client_id]); // Aguarda liberação pelo cozinheiro
	pedidos[od->client_id] = NULL; // Retira o pedido do balcão
	free(od); // Libera espaço de memória do pedido
}

// Atendente passa pedido para a fila
void clerk_create_order(order_t* od) {
	pthread_mutex_lock(&mutex_queue);
	insert(queue, od); // Insere na fila
	pthread_mutex_unlock(&mutex_queue);
}

// Atendente anota pedido
void clerk_annotate_order() {
	sleep(rand() % (clerkMaxWaitSec + CLERK_MIN_WAIT_SEC) + CLERK_MIN_WAIT_SEC);
}

// Cozinheito prepara pedido
void cooker_wait_cook_time() {
	sleep(rand() % (cookMaxWaitSec + COOK_MIN_WAIT_SEC) + COOK_MIN_WAIT_SEC);
}

// Cliente
void* client(void *args) {
	client_t* cl = (client_t*) args;
	// Retira o ticket e libera os atendentes para trabalhar
	pthread_mutex_lock(&mutex_client);
	int pw = get_unique_ticket(pc);
	pthread_mutex_unlock(&mutex_client);
	sem_post(&sem_clerk_ps);

	while(true) {
		for(int i = 0; i < numClerks; i++) {
			// Verifica se ticket está sendo chamado
			if(pc->current_password[i] == pw) {
				client_think_order();

				// Cria o pedido
				order_t* order = malloc(sizeof(order));
				order->client_id = cl->id;
				order->password_num = pw;
				
				// Informa o pedido ao atendente e espera ficar pronto
				client_inform_order(order, i);
				client_wait_order(order);
				return NULL;
			}
		}
	}
}

// Atendente
void* clerk(void *args) {
	clerk_t* ck = (clerk_t*) args;

	while(true) {
		// Aguarda cliente pegar ticket e pega um ticket já retirado
		sem_wait(&sem_clerk_ps);
		pthread_mutex_lock(&mutex_clerk);
		int pw = get_retrieved_ticket(pc);
		pthread_mutex_unlock(&mutex_clerk);

		// Se não há mais clientes, retorna
		if(pw < 0)
			return NULL;

		// Anuncia ticket e espera o cliente
		set_current_ticket(pc, pw, ck->id);
		sem_wait(&sem_clerk[ck->id]);

		// Anota e anuncia o pedido
		clerk_annotate_order();
		anounce_clerk_order(pc->clerks_order_spot[ck->id]);

		// Passa o pedido para o cozinheiro
		clerk_create_order(pc->clerks_order_spot[ck->id]);
	}
}

// Cozinheiro
void* cooker(void *args) {
	int num_plates = 0;

	while(num_plates < numClients) {
		// Se há pedidos na fila...
		if(!isEmpty(queue)) {
			order_t* order = removeData(queue); // Retira pedido da fila

			// Cozinha, adiciona o pedido ao balcão e anuncia
			cooker_wait_cook_time();
			pedidos[order->client_id] = order;
			anounce_cooker_order(order);

			// Libera o cliente e incrementa a qtd de pratos feitos
			sem_post(&sem_client[order->client_id]);
			num_plates++;
		}
	}

	return NULL;
}

int main(int argc, char *argv[]) {
	parseArgs(argc, argv);
	queue = init_order_queue(numClients);
	pc = init_ticket_caller();
	pedidos = malloc(sizeof(order_t *) * numClients);

	// Criação das threads
	pthread_t clients_threads[numClients], clerks_threads[numClerks], cooker_threads[NUM_COOK];
	// Vetor de clientes e atendentes
	client_t* cl[numClients];
	clerk_t* ck[numClerks];

	// Inicialização dos semáforos
	sem_clerk = malloc(numClerks * sizeof(sem_t));
	sem_client = malloc(numClients * sizeof(sem_t));
	sem_init(&sem_clerk_ps, 0, 0);

	// Inicialização dos mutex
	pthread_mutex_init(&mutex_queue, NULL);
	pthread_mutex_init(&mutex_client, NULL);
	pthread_mutex_init(&mutex_clerk, NULL);

	// Inicialização das threads de cliente
	for (int i = 0; i < numClients; i++) {
		sem_init(&sem_client[i], 0, 0);
		cl[i] = malloc(sizeof(client_t));
		cl[i]->id = i;
		pthread_create(&clients_threads[i], NULL, client, (void *) cl[i]);
	}

	// Inicialização das threads de atendente
	for (int i = 0; i < numClerks; i++) {
		sem_init(&sem_clerk[i], 0, 0);
		ck[i] = malloc(sizeof(clerk_t));
		ck[i]->id = i;
		pthread_create(&clerks_threads[i], NULL, clerk, (void *) ck[i]);
	}

	// Inicialização das threads de cozinheiro
	for (int i = 0; i < NUM_COOK; i++) {
		pthread_create(&cooker_threads[i], NULL, cooker, NULL);
	}

  // Espera término das threads de cliente
	for (int i = 0; i < numClients; i++) {
		pthread_join(clients_threads[i], NULL);
		free(cl[i]);
	}

	// Finaliza as threads de atendente
	for (int i = 0; i < numClerks; i++) {
		sem_post(&sem_clerk_ps);
	}

  // Espera término das threads de atendente
	for (int i = 0; i < numClerks; i++) {
		pthread_join(clerks_threads[i], NULL);
		free(ck[i]);
	}
	
	// Espera término das threads de cozinheiro
	for (int i = 0; i < NUM_COOK; i++) {
		pthread_join(cooker_threads[i], NULL);
	}

	// Libera memória
	free(queue);
	free(pc);
	free(pedidos);

	sem_destroy(&sem_clerk_ps);
	sem_destroy(sem_client);
	sem_destroy(sem_clerk);
	
	pthread_mutex_destroy(&mutex_queue);
	pthread_mutex_destroy(&mutex_client);
	pthread_mutex_destroy(&mutex_clerk);

	return 0;
}
