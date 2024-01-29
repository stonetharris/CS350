/*******************************************************************************
* Dual-Threaded FIFO Server Implementation w/ Queue Limit
* testing docker mount
* Description:
*     A server implementation designed to process client requests in First In,
*     First Out (FIFO) order. The server binds to the specified port number
*     provided as a parameter upon launch. It launches a secondary thread to
*     process incoming requests and allows to specify a maximum queue size.
*
* Usage:
*     <build directory>/server -q <queue_size> <port_number>
*
* Parameters:
*     port_number - The port number to bind the server to.
*     queue_size  - The maximum number of queued requests
*
* Author:
*     Renato Mancuso
*
* Affiliation:
*     Boston University
*
* Creation Date:
*     September 29, 2023
*
* Notes:
*     Ensure to have proper permissions and available port before running the
*     server. The server relies on a FIFO mechanism to handle requests, thus
*     guaranteeing the order of processing. If the queue is full at the time a
*     new request is received, the request is rejected with a negative ack.
*
*******************************************************************************/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sched.h>
#include <signal.h>

/* Needed for wait(...) */
#include <sys/types.h>
#include <sys/wait.h>

/* Needed for semaphores */
#include <semaphore.h>

/* Include struct definitions and other libraries that need to be
 * included by both client and server */
#include "common.h"

#define BACKLOG_COUNT 100
#define USAGE_STRING				\
	"Missing parameter. Exiting.\n"		\
	"Usage: %s -q <queue size> <port_number>\n"

/* 4KB of stack for the worker thread */
#define STACK_SIZE (4096)

/* START - Variables needed to protect the shared queue. DO NOT TOUCH */
sem_t * queue_mutex;
sem_t * queue_notify;
/* END - Variables needed to protect the shared queue. DO NOT TOUCH */

struct request_meta {
	struct request request;

	/* ADD REQUIRED FIELDS */
    uint8_t rejected;
    struct timespec rec_timestamp;
};

struct queue {
	/* ADD REQUIRED FIELDS */
    struct request_meta * items;
    size_t head, tail, size, max_size;
};

struct connection_params {
	/* ADD REQUIRED FIELDS */
    size_t queue_size;
};

struct worker_params {
	/* ADD REQUIRED FIELDS */
    struct queue * the_queue;
    int * worker_done;
    int conn_socket;
};

/* Helper function to perform queue initialization */
void queue_init(struct queue * the_queue, size_t queue_size)
{
	/* IMPLEMENT ME !! */
    the_queue->items = (struct request_meta *) malloc(queue_size * sizeof(struct request_meta));
    the_queue->head = 0;
    the_queue->tail = 0;
    the_queue->size = 0;
    the_queue->max_size = queue_size;
}

/* Add a new request <request> to the shared queue <the_queue> */
int add_to_queue(struct request_meta to_add, struct queue * the_queue)
{
	int retval = 0;
	/* QUEUE PROTECTION INTRO START --- DO NOT TOUCH */
	sem_wait(queue_mutex);
	/* QUEUE PROTECTION INTRO END --- DO NOT TOUCH */

	/* WRITE YOUR CODE HERE! */
	/* MAKE SURE NOT TO RETURN WITHOUT GOING THROUGH THE OUTRO CODE! */
//    printf("Inside add to queue \n");

	/* Make sure that the queue is not full */
	if (the_queue->size == the_queue->max_size) {
		/* What to do in case of a full queue */
		/* DO NOT RETURN DIRECTLY HERE */
//        printf("Inside if queue is full \n");
        retval = -1;
	} else {
		/* If all good, add the item in the queue */
		/* IMPLEMENT ME !!*/
//        printf("Inside queue is not full \n");
        the_queue->items[the_queue->tail] = to_add;
        the_queue->tail = (the_queue->tail + 1) % the_queue->max_size;
        the_queue->size++;

		/* QUEUE SIGNALING FOR CONSUMER --- DO NOT TOUCH */
		sem_post(queue_notify);
	}

	/* QUEUE PROTECTION OUTRO START --- DO NOT TOUCH */
	sem_post(queue_mutex);
	/* QUEUE PROTECTION OUTRO END --- DO NOT TOUCH */
	return retval;
}

/* Add a new request <request> to the shared queue <the_queue> */
struct request_meta get_from_queue(struct queue * the_queue)
{
	struct request_meta retval;
	/* QUEUE PROTECTION INTRO START --- DO NOT TOUCH */
	sem_wait(queue_notify);
	sem_wait(queue_mutex);
	/* QUEUE PROTECTION INTRO END --- DO NOT TOUCH */

	/* WRITE YOUR CODE HERE! */
	/* MAKE SURE NOT TO RETURN WITHOUT GOING THROUGH THE OUTRO CODE! */
    if (the_queue->size == 0) {
        // Return an invalid request_meta when the queue is empty. In practice, should handle this case properly.
        printf("get_from_queue: empty queue \n");
        retval.request.req_id = -1;
        return retval;
    }

    retval = the_queue->items[the_queue->head];
    the_queue->head = (the_queue->head + 1) % the_queue->max_size;
    the_queue->size--;

	/* QUEUE PROTECTION OUTRO START --- DO NOT TOUCH */
	sem_post(queue_mutex);
	/* QUEUE PROTECTION OUTRO END --- DO NOT TOUCH */
	return retval;
}

void dump_queue_status(struct queue * the_queue)
{
	/* QUEUE PROTECTION INTRO START --- DO NOT TOUCH */
	sem_wait(queue_mutex);
	/* QUEUE PROTECTION INTRO END --- DO NOT TOUCH */

	/* WRITE YOUR CODE HERE! */
	/* MAKE SURE NOT TO RETURN WITHOUT GOING THROUGH THE OUTRO CODE! */
    printf("Q:[");
    for (size_t i = 0; i < the_queue->size; ++i) {
        size_t index = (the_queue->head + i) % the_queue->max_size;
        printf("R%llu,", the_queue->items[index].request.req_id);
    }
    printf("]\n");

	/* QUEUE PROTECTION OUTRO START --- DO NOT TOUCH */
	sem_post(queue_mutex);
	/* QUEUE PROTECTION OUTRO END --- DO NOT TOUCH */
}

/* Main logic of the worker thread */
int worker_main (void * arg)
{
	struct timespec now;
	struct worker_params * params = (struct worker_params *)arg;

	/* Print the first alive message. */
	clock_gettime(CLOCK_MONOTONIC, &now);
	printf("[#WORKER#] %lf Worker Thread Alive!\n", TSPEC_TO_DOUBLE(now));

	/* Okay, now execute the main logic. */
//    printf("worker_done = %d \n", *params->worker_done);
	while (!*params->worker_done) {

		/* IMPLEMENT ME !! Main worker logic. */
//        printf("in worker main before get_from_queue \n");
        struct request_meta req_meta = get_from_queue(params->the_queue);
//        printf("in worker main after get_from_queue \n");
        if (req_meta.request.req_id != -1){
//            if (req_meta.rejected) {
//                printf("X%d:%lf,%d,%lf\n",
//                       req_meta.request.req_id,
//                       req_meta.request.req_timestamp,
//                       req_meta.request.req_length,
//                       req_meta.rec_timestamp
//            }
                struct timespec start_timestamp, completion_timestamp;
                struct response server_response;
                uint64_t busyWaitTime = get_elapsed_busywait(req_meta.request.req_length.tv_sec, req_meta.request.req_length.tv_nsec);

                // Capture the completion timestamp.
                clock_gettime(CLOCK_MONOTONIC, &completion_timestamp);
                server_response.req_id = req_meta.request.req_id;
                server_response.rejected = 0;
                write(params->conn_socket, &server_response, sizeof(server_response));
                // Print the request details.
                printf("R%llu:%lf,%lf,%lf,%lf,%lf\n",
                       req_meta.request.req_id,
                       TSPEC_TO_DOUBLE(req_meta.request.req_timestamp),
                       TSPEC_TO_DOUBLE(req_meta.request.req_length),
                       TSPEC_TO_DOUBLE(req_meta.rec_timestamp),
                       TSPEC_TO_DOUBLE(start_timestamp),
                       TSPEC_TO_DOUBLE(completion_timestamp));

        }
		dump_queue_status(params->the_queue);
	}
    printf("worker is done \n");

	return EXIT_SUCCESS;
}

/* This function will start the worker thread wrapping around the
 * clone() system call*/
int start_worker(void * params, void * worker_stack)
{
	/* IMPLEMENT ME !! */
//    printf("Starting worker\n");

    int thread_pid;
//    printf("before clone\n");
    thread_pid = clone(worker_main, worker_stack + STACK_SIZE, CLONE_THREAD | CLONE_VM | CLONE_SIGHAND | CLONE_FS | CLONE_FILES | CLONE_SYSVSEM, params);
//    printf("after clone\n");
    return thread_pid;
}

/* Main function to handle connection with the client. This function
 * takes in input conn_socket and returns only when the connection
 * with the client is interrupted. */
void handle_connection(int conn_socket, struct connection_params conn_params)
{
	struct request * req;
	struct queue * the_queue;
	size_t in_bytes;
    int worker_done = 0;

	/* The connection with the client is alive here. Let's get
	 * ready to start the worker thread. */
	void * worker_stack = malloc(STACK_SIZE);
	struct worker_params worker_params;
	int worker_id, res;

	/* Now handle queue allocation and initialization */
	/* IMPLEMENT ME !!*/
    the_queue = (struct queue*)malloc(sizeof(struct queue));
    queue_init(the_queue, conn_params.queue_size);
//    printf("Just initialized queue \n");

	/* Prepare worker_parameters */
    worker_params.the_queue = the_queue;
    worker_params.conn_socket = conn_socket;
    worker_params.worker_done = &worker_done;
	/* IMPLEMENT ME !!*/
	worker_id = start_worker(&worker_params, worker_stack);
//    printf("Just assigned worker ID and ran start_worker \n");

	if (worker_id < 0) {
		/* HANDLE WORKER CREATION ERROR */
        perror("Error, worker couldn't be created");
        exit(1);
	}

	printf("INFO: Worker thread started. Thread ID = %d\n", worker_id);

	/* We are ready to proceed with the rest of the request
	 * handling logic. */

	req = (struct request *)malloc(sizeof(struct request));
    ssize_t bytes_received;

	do {
		/* IMPLEMENT ME: Receive next request from socket. */
//        printf("before bytes received \n");
        bytes_received = recv(conn_socket, req, sizeof(struct request), 0);
//        printf("bytes have been received \n");

		/* Don't just return if in_bytes is 0 or -1. Instead
		 * skip the response and break out of the loop in an
		 * orderly fashion so that we can de-allocate the req
		 * and resp varaibles, and shutdown the socket. */

		/* IMPLEMENT ME: Attempt to enqueue or reject request! */
        if (bytes_received < 0) {
            printf("ERROR: Could not read from socket.");
            break;
        }
        if (bytes_received == 0) {
            break;
        }

        if (bytes_received > 0) {
            struct request_meta r;
            r.request = *req;
            r.rejected = 0;
            clock_gettime(CLOCK_MONOTONIC, &r.rec_timestamp);
            if (add_to_queue(r, the_queue) == -1) {
                struct response server_response;
                server_response.rejected = 1;
                r.rejected = 1;
                server_response.req_id = r.request.req_id;
                write(conn_socket, &server_response, sizeof(server_response));
                printf("X%llu:%lf,%lf,%lf\n",
                       r.request.req_id,
                       TSPEC_TO_DOUBLE(r.request.req_timestamp),
                       TSPEC_TO_DOUBLE(r.request.req_length),
                       TSPEC_TO_DOUBLE(r.rec_timestamp));
            }
        }

	} while (bytes_received > 0);

	/* Ask the worker thead to terminate */
	printf("INFO: Asserting termination flag for worker thread...\n");
	worker_done = 1;

	/* Just in case the thread is stuck on the notify semaphore,
	 * wake it up */
	sem_post(queue_notify);

	/* Wait for orderly termination of the worker thread */
	waitpid(-1, NULL, 0);
	printf("INFO: Worker thread exited.\n");
	free(worker_stack);
	free(the_queue);

	free(req);
	shutdown(conn_socket, SHUT_RDWR);
	close(conn_socket);
	printf("INFO: Client disconnected.\n");
}


/* Template implementation of the main function for the FIFO
 * server. The server must accept in input a command line parameter
 * with the <port number> to bind the server to. */
int main (int argc, char ** argv) {
	int sockfd, retval, accepted, optval, opt;
	in_port_t socket_port;
	struct sockaddr_in addr, client;
	struct in_addr any_address;
	socklen_t client_len;

	struct connection_params conn_params;

	/* Parse all the command line arguments */
	/* IMPLEMENT ME!! */
	/* PARSE THE COMMANDS LINE: */
	/* 1. Detect the -q parameter and set aside the queue size in conn_params */
	//conn_params...

	/* 2. Detect the port number to bind the server socket to (see HW1 and HW2) */
	//socket_port = ...
    if (argc > 3) {
        socket_port = strtol(argv[3], NULL, 10);
        printf("INFO: setting server port as: %d\n", socket_port);
    } else {
        ERROR_INFO();
        fprintf(stderr, USAGE_STRING, argv[0]);
        return EXIT_FAILURE;
    }

    opt = getopt(argc, argv, "q:");
    if (opt == 'q') {
        conn_params.queue_size = strtol(optarg, NULL, 10);
    }

	/* Now onward to create the right type of socket */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0) {
		ERROR_INFO();
		perror("Unable to create socket");
		return EXIT_FAILURE;
	}

	/* Before moving forward, set socket to reuse address */
	optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&optval, sizeof(optval));

	/* Convert INADDR_ANY into network byte order */
	any_address.s_addr = htonl(INADDR_ANY);

	/* Time to bind the socket to the right port  */
	addr.sin_family = AF_INET;
	addr.sin_port = htons(socket_port);
	addr.sin_addr = any_address;

	/* Attempt to bind the socket with the given parameters */
	retval = bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));

	if (retval < 0) {
		ERROR_INFO();
		perror("Unable to bind socket");
		return EXIT_FAILURE;
	}

	/* Let us now proceed to set the server to listen on the selected port */
	retval = listen(sockfd, BACKLOG_COUNT);

	if (retval < 0) {
		ERROR_INFO();
		perror("Unable to listen on socket");
		return EXIT_FAILURE;
	}

	/* Ready to accept connections! */
	printf("INFO: Waiting for incoming connection...\n");
	client_len = sizeof(struct sockaddr_in);
	accepted = accept(sockfd, (struct sockaddr *)&client, &client_len);

	if (accepted == -1) {
		ERROR_INFO();
		perror("Unable to accept connections");
		return EXIT_FAILURE;
	}

	/* Initialize queue protection variables. DO NOT TOUCH. */
	queue_mutex = (sem_t *)malloc(sizeof(sem_t));
	queue_notify = (sem_t *)malloc(sizeof(sem_t));
	retval = sem_init(queue_mutex, 0, 1);
	if (retval < 0) {
		ERROR_INFO();
		perror("Unable to initialize queue mutex");
		return EXIT_FAILURE;
	}
	retval = sem_init(queue_notify, 0, 0);
	if (retval < 0) {
		ERROR_INFO();
		perror("Unable to initialize queue notify");
		return EXIT_FAILURE;
	}
	/* DONE - Initialize queue protection variables */

	/* Ready to handle the new connection with the client. */
	handle_connection(accepted, conn_params);

	free(queue_mutex);
	free(queue_notify);

	close(sockfd);
	return EXIT_SUCCESS;
}