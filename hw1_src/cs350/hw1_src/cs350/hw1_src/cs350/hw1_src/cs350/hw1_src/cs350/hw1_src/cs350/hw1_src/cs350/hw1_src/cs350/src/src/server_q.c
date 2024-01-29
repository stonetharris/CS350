/*******************************************************************************
* Simple FIFO Order Server Implementation
*
* Description:
*     A server implementation designed to process client requests in First In,
*     First Out (FIFO) order. The server binds to the specified port number
*     provided as a parameter upon launch.
*
* Usage:
*     <build directory>/server <port_number>
*
* Parameters:
*     port_number - The port number to bind the server to.
*
* Author:
*     Renato Mancuso
*
* Affiliation:
*     Boston University
*
* Creation Date:
*     September 10, 2023
*
* Notes:
*     Ensure to have proper permissions and available port before running the
*     server. The server relies on a FIFO mechanism to handle requests, thus
*     guaranteeing the order of processing. For debugging or more details, refer
*     to the accompanying documentation and logs.
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
	"Usage: %s <port_number>\n"

/* 4KB of stack for the worker thread */
#define STACK_SIZE (4096)

/* START - Variables needed to protect the shared queue. DO NOT TOUCH */
sem_t * queue_mutex;
sem_t * queue_notify;
/* END - Variables needed to protect the shared queue. DO NOT TOUCH */

/* Max number of requests that can be queued */
#define QUEUE_SIZE 500

//struct request {
//    uint64_t request_id;
//    struct timespec sent_timestamp;
//    struct timespec request_duration;
//};

struct queue {
    struct request req [QUEUE_SIZE];
    int head, tail;
};

struct worker_params {
    struct queue *shared_queue;
    int conn_socket;
};

/* Add a new request <request> to the shared queue <the_queue> */
int add_to_queue(struct request to_add, struct queue * the_queue)
{
	int retval = 0;
	/* QUEUE PROTECTION INTRO START --- DO NOT TOUCH */
	sem_wait(queue_mutex);
	/* QUEUE PROTECTION INTRO END --- DO NOT TOUCH */

	/* WRITE YOUR CODE HERE! */
	/* MAKE SURE NOT TO RETURN WITHOUT GOING THROUGH THE OUTRO CODE! */
    if ((the_queue->tail+1)%QUEUE_SIZE == the_queue->head) {
        retval = -1;
    } else {
        the_queue->req[the_queue->tail] = to_add;
        the_queue->tail = (the_queue->tail+1)%QUEUE_SIZE;
    }

	/* QUEUE PROTECTION OUTRO START --- DO NOT TOUCH */
	sem_post(queue_mutex);
	sem_post(queue_notify);
	/* QUEUE PROTECTION OUTRO END --- DO NOT TOUCH */
	return retval;
}

/* Add a new request <request> to the shared queue <the_queue> */
struct request get_from_queue(struct queue * the_queue)
{
	struct request retval;
	/* QUEUE PROTECTION INTRO START --- DO NOT TOUCH */
	sem_wait(queue_notify);
	sem_wait(queue_mutex);
	/* QUEUE PROTECTION INTRO END --- DO NOT TOUCH */

	/* WRITE YOUR CODE HERE! */
	/* MAKE SURE NOT TO RETURN WITHOUT GOING THROUGH THE OUTRO CODE! */
    //if q empty
    if (the_queue->head == the_queue->tail) {
//        printf("Inside if queue is empty \n");
        fflush(stdout);
        retval;
    } else {
//        printf("Inside queue is not empty \n");
        retval = the_queue->req[the_queue->head];
        the_queue->head = (the_queue->head+1)%QUEUE_SIZE;
    }

	/* QUEUE PROTECTION OUTRO START --- DO NOT TOUCH */
	sem_post(queue_mutex);
	/* QUEUE PROTECTION OUTRO END --- DO NOT TOUCH */
	return retval;
}

/* Implement this method to correctly dump the status of the queue
 * following the format Q:[R<request ID>,R<request ID>,...] */
void dump_queue_status(struct queue * the_queue)
{
	int i;
	/* QUEUE PROTECTION INTRO START --- DO NOT TOUCH */
	sem_wait(queue_mutex);
	/* QUEUE PROTECTION INTRO END --- DO NOT TOUCH */

	/* WRITE YOUR CODE HERE! */
	/* MAKE SURE NOT TO RETURN WITHOUT GOING THROUGH THE OUTRO CODE! */
    //TODO?

	/* QUEUE PROTECTION OUTRO START --- DO NOT TOUCH */
	sem_post(queue_mutex);
	/* QUEUE PROTECTION OUTRO END --- DO NOT TOUCH */
}

/* Main logic of the worker thread */
/* IMPLEMENT HERE THE MAIN FUNCTION OF THE WORKER */
int worker_main(void *arg) {
    struct worker_params *params = (struct worker_params *) arg;
    struct request current_request;
    struct timespec start_timestamp, completion_timestamp;
    while (1) {
        printf("in worker main now \n");
        // Get a request from the shared queue.
        current_request = get_from_queue(params->shared_queue);
        printf("in worker main after get from queue \n");
        // Capture the start timestamp.
        clock_gettime(CLOCK_MONOTONIC, &start_timestamp);

        // Process the request (busywait for the desired duration).
        uint64_t busyWaitTime = get_elapsed_busywait(current_request.req_length.tv_sec, current_request.req_length.tv_nsec);

        // Capture the completion timestamp.
        clock_gettime(CLOCK_MONOTONIC, &completion_timestamp);

        // Print the request details.
        printf("R%llu:%lf,%lf,%lf,%lf,%lf\n",
               current_request.req_id,
               TSPEC_TO_DOUBLE(current_request.req_timestamp),
               TSPEC_TO_DOUBLE(current_request.req_length),
               TSPEC_TO_DOUBLE(current_request.rec_timestamp),
               TSPEC_TO_DOUBLE(start_timestamp),
               TSPEC_TO_DOUBLE(completion_timestamp));

        // Dump the current status of the queue.
        dump_queue_status(params->shared_queue);
    }
    return 0;
}

/* Main function to handle connection with the client. This function
 * takes in input conn_socket and returns only when the connection
 * with the client is interrupted. */
void handle_connection(int conn_socket)
{
    //now using socket request struct
	struct request_s * req;
	struct queue * the_queue;
	size_t in_bytes, bytes_received;

	/* The connection with the client is alive here. Let's
	 * initialize the shared queue. */
    the_queue = malloc(sizeof(struct queue));
    the_queue->tail = 0, the_queue->head = 0;
//    printf("queue initialized \n");

	/* IMPLEMENT HERE ANY QUEUE INITIALIZATION LOGIC */

	/* Queue ready to go here. Let's start the worker thread. */
    char *child_stack = malloc(STACK_SIZE);
    if (!child_stack) {
        perror("Failed to allocate stack for child thread");
        return;
    }

    /* IMPLEMENT HERE THE LOGIC TO START THE WORKER THREAD. */
    int thread_pid;
//    printf("before clone\n");
    struct worker_params *params = malloc(sizeof (struct worker_params));
    params->shared_queue = the_queue;
    thread_pid = clone(worker_main, child_stack + STACK_SIZE, CLONE_THREAD | CLONE_VM | CLONE_SIGHAND | CLONE_FS | CLONE_FILES | CLONE_SYSVSEM, params);
//    printf("after clone\n");
	/* We are ready to proceed with the rest of the request
	 * handling logic. */

	/* REUSE LOGIC FROM HW1 TO HANDLE THE PACKETS */

    //now using socket request struct
	req = (struct request_s *)malloc(sizeof(struct request_s));

	do {
		//in_bytes = recv(conn_socket, ... /* IMPLEMENT ME */);
		/* SAMPLE receipt_timestamp HERE */
//        printf("before bytes received \n");
        bytes_received = recv(conn_socket, req, sizeof(struct request_s), 0);
//        printf("bytes have been received \n");
        if (bytes_received < 0) {
            printf("ERROR: Could not read from socket.");
            break;
        }
        if (bytes_received == 0) {
            break;
        }

		/* Don't just return if in_bytes is 0 or -1. Instead
		 * skip the response and break out of the loop in an
		 * orderly fashion so that we can de-allocate the req
		 * and resp varaibles, and shutdown the socket. */
		if (in_bytes > 0) {
            struct request r;
            r.req_id = req->req_id;
            r.req_timestamp = req->req_timestamp;
            r.req_length = req->req_length;
            clock_gettime(CLOCK_MONOTONIC, &r.rec_timestamp);
//			add_to_queue(r, the_queue);
		}
	} while (in_bytes > 0);

	/* PERFORM ORDERLY DEALLOCATION AND OUTRO HERE */
    free(child_stack);
    free(req);
    free(the_queue);
}

/* Template implementation of the main function for the FIFO
 * server. The server must accept in input a command line parameter
 * with the <port number> to bind the server to. */
int main (int argc, char ** argv) {
	int sockfd, retval, accepted, optval;
	in_port_t socket_port;
	struct sockaddr_in addr, client;
	struct in_addr any_address;
	socklen_t client_len;

	/* Get port to bind our socket to */
	if (argc > 1) {
		socket_port = strtol(argv[1], NULL, 10);
		printf("INFO: setting server port as: %d\n", socket_port);
	} else {
		ERROR_INFO();
		fprintf(stderr, USAGE_STRING, argv[0]);
		return EXIT_FAILURE;
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
//    printf("about to initialize queue protection vars \n");
    fflush(stdout);
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
	/* DONE - Initialize queue protection variables. DO NOT TOUCH */

	/* Ready to handle the new connection with the client. */
	handle_connection(accepted);

	free(queue_mutex);
	free(queue_notify);

	close(sockfd);
	return EXIT_SUCCESS;
}