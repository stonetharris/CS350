/*******************************************************************************
* Multi-Threaded FIFO Server Implementation w/ Queue Limit
*
* Description:
*     A server implementation designed to process client requests in First In,
*     First Out (FIFO) order. The server binds to the specified port number
*     provided as a parameter upon launch. It launches multiple threads to
*     process incoming requests and allows to specify a maximum queue size.
*
* Usage:
*     <build directory>/server -q <queue_size> -w <workers> <port_number>
*
* Parameters:
*     port_number - The port number to bind the server to.
*     queue_size  - The maximum number of queued requests
*     workers     - The number of workers to start to process requests
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
	"Usage: %s -q <queue size> -w <number of threads> <port_number>\n"

/* 4KB of stack for the worker thread */
#define STACK_SIZE (4096)

/* Mutex needed to protect the threaded printf. DO NOT TOUCH */
sem_t * printf_mutex;

/* Synchronized printf for multi-threaded operation */
#define sync_printf(...)			\
	do {					\
		sem_wait(printf_mutex);		\
		printf(__VA_ARGS__);		\
		sem_post(printf_mutex);		\
	} while (0)

/* START - Variables needed to protect the shared queue. DO NOT TOUCH */
sem_t * queue_mutex;
sem_t * queue_notify;
/* END - Variables needed to protect the shared queue. DO NOT TOUCH */

struct request_meta {
	struct request request;

	/* ADD REQUIRED FIELDS */
    struct timespec rec_timestamp;
    uint8_t rejected;
};

struct queue {
	/* ADD REQUIRED FIELDS */
    struct request_meta *array;
    int front;
    int rear;
    int count;
    int maxSize;
};

struct connection_params {
	/* ADD REQUIRED FIELDS */
    int queue_size;
    int num_threads;
};

struct worker_params {
	/* ADD REQUIRED FIELDS */
    struct queue *the_queue;
    volatile int *worker_done;
    int thread_id;
    int conn_socket;
    int native_thread_id;
};

/* Helper function to perform queue initialization */
void queue_init(struct queue * the_queue, size_t queue_size)
{
	/* IMPLEMENT ME !! */
    the_queue->array = (struct request_meta *)malloc(queue_size * sizeof(struct request_meta));
    the_queue->front = 0;
    the_queue->rear = -1;
    the_queue->count = 0;
    the_queue->maxSize = queue_size;
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
	/* Make sure that the queue is not full */
	if (the_queue->count == the_queue->maxSize) {
		/* What to do in case of a full queue */
        retval = -1;

		/* DO NOT RETURN DIRECTLY HERE. The
		 * sem_post(queue_mutex) below MUST happen. */
	} else {
		/* If all good, add the item in the queue */
		/* IMPLEMENT ME !!*/
//        sync_printf("Adding to queue %d \n", the_queue->rear);
        the_queue->rear = (the_queue->rear + 1) % the_queue->maxSize;
        the_queue->array[the_queue->rear] = to_add;
        the_queue->count++;

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
    if (the_queue->count == 0) {
        // handle???? right now returning empty meta i think
        memset(&retval, 0, sizeof(retval));
    } else {
//        sync_printf("Getting from queue %d \n", the_queue->front);
        retval = the_queue->array[the_queue->front];
        the_queue->front = (the_queue->front + 1) % the_queue->maxSize;
        the_queue->count--;
    }

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
    for (size_t i = 0; i < the_queue->count; ++i) {
        size_t index = (the_queue->front + i) % the_queue->maxSize;
        printf("R%llu,", the_queue->array[index].request.req_id);
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
    sync_printf("");

	/* Okay, now execute the main logic. */
	while (!*params->worker_done) {
		/* IMPLEMENT ME !! Main worker logic. */
        struct request_meta request_meta = get_from_queue(params->the_queue);
        struct timespec start_timestamp, completion_timestamp;
        struct response server_response;
        uint64_t busyWaitTime = get_elapsed_busywait(request_meta.request.req_length.tv_sec, request_meta.request.req_length.tv_nsec);
        // Capture the completion timestamp.
        clock_gettime(CLOCK_MONOTONIC, &completion_timestamp);
        server_response.req_id = request_meta.request.req_id;
        server_response.rejected = 0;
        write(params->conn_socket, &server_response, sizeof(server_response));

        // Print the request details.
        sync_printf("T%d R%llu:%lf,%lf,%lf,%lf,%lf\n",
               params->thread_id,
               request_meta.request.req_id,
               TSPEC_TO_DOUBLE(request_meta.request.req_timestamp),
               TSPEC_TO_DOUBLE(request_meta.request.req_length),
               TSPEC_TO_DOUBLE(request_meta.rec_timestamp),
               TSPEC_TO_DOUBLE(start_timestamp),
               TSPEC_TO_DOUBLE(completion_timestamp));

		dump_queue_status(params->the_queue);
	}

	return EXIT_SUCCESS;
}

/* This function will start the worker thread wrapping around the
 * clone() system call*/
int start_worker(void * params, void * worker_stack)
{
	/* IMPLEMENT ME !! */
    int thread_pid;
    thread_pid = clone(worker_main, worker_stack + STACK_SIZE, CLONE_THREAD | CLONE_VM | CLONE_SIGHAND | CLONE_FS | CLONE_FILES | CLONE_SYSVSEM, params);
//    sync_printf("after clone, thread pid: %d\n", thread_pid);
    return thread_pid;
}

/* Main function to handle connection with the client. This function
 * takes in input conn_socket and returns only when the connection
 * with the client is interrupted. */
void handle_connection(int conn_socket, struct connection_params connection_params)
{
//    sync_printf("Number of threads: %d \n", connection_params.num_threads);
	struct request * req;
	struct queue * the_queue;
	size_t in_bytes;
    struct worker_params arr[connection_params.num_threads];
    the_queue = (struct queue*)malloc(sizeof(struct queue));
    queue_init(the_queue, connection_params.queue_size);
    int worker_done = 0;

	/* IMPLEMENT ME!! Write a loop to start and initialize all the
	 * worker threads ! */
    for(int i = 0; i < connection_params.num_threads; i++) {
//        sync_printf("Worker ID: %d \n", i);
        void * worker_stack = malloc(STACK_SIZE);
        arr[i].the_queue = the_queue;
        arr[i].thread_id = i;
        arr[i].worker_done = &worker_done;
        arr[i].conn_socket = conn_socket;
        arr[i].native_thread_id = start_worker(&arr[i], worker_stack);
    }

	/* We are ready to proceed with the rest of the request
	 * handling logic. */

	req = (struct request *)malloc(sizeof(struct request));

	do {
		/* IMPLEMENT ME: Receive next request from socket. */
        in_bytes = recv(conn_socket, req, sizeof(struct request), 0);

        /* Don't just return if in_bytes is 0 or -1. Instead
         * skip the response and break out of the loop in an
         * orderly fashion so that we can de-allocate the req
         * and resp varaibles, and shutdown the socket. */

        // have to handle if queue is full or not??
        if(in_bytes <= 0) {
            if(in_bytes == 0) {
                sync_printf("Client closed connection\n");
            } else {
                perror("recv");
            }
            break;
        }
        if (in_bytes > 0) {
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
                sync_printf("X%llu:%lf,%lf,%lf\n",
                       r.request.req_id,
                       TSPEC_TO_DOUBLE(r.request.req_timestamp),
                       TSPEC_TO_DOUBLE(r.request.req_length),
                       TSPEC_TO_DOUBLE(r.rec_timestamp));
            }
        }

		/* IMPLEMENT ME: Attempt to enqueue or reject request! */
	} while (in_bytes > 0);

	/* IMPLEMENT ME!! Write a loop to gracefully terminate all the
	 * worker threads ! */
    for(int i = 0; i < connection_params.num_threads; i++) {
        waitpid(arr[i].native_thread_id, NULL, 0);
    }

	free(the_queue);

	free(req);
	shutdown(conn_socket, SHUT_RDWR);
	close(conn_socket);
    sync_printf("INFO: Client disconnected.\n");
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
//	conn_params...
	/* 2. Detect the -w parameter and set aside the number of threads to launch */
//	conn_params...
	/* 3. Detect the port number to bind the server socket to (see HW1 and HW2) */
//	socket_port = ...

    if (argc > 5) {
        socket_port = strtol(argv[5], NULL, 10);
        printf("INFO: setting server port as: %d\n", socket_port);
    } else {
        ERROR_INFO();
        fprintf(stderr, USAGE_STRING, argv[0]);
        return EXIT_FAILURE;
    }

    while ((opt = getopt(argc, argv, "q:w:")) != -1) {

        if (opt == 'q') {
            conn_params.queue_size = strtol(optarg, NULL, 10);
        }
        if (opt == 'w') {
            conn_params.num_threads = strtol(optarg, NULL, 10);
        }
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

	/* Initilize threaded printf mutex */
	printf_mutex = (sem_t *)malloc(sizeof(sem_t));
	retval = sem_init(printf_mutex, 0, 1);
	if (retval < 0) {
		ERROR_INFO();
		perror("Unable to initialize printf mutex");
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
