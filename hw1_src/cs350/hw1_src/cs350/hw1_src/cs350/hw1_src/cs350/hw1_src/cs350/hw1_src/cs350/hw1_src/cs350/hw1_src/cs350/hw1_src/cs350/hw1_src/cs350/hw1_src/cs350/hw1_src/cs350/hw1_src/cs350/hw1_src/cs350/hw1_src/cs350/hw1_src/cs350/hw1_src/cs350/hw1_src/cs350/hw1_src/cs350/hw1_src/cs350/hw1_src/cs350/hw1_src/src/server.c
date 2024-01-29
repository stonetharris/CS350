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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

/* Include struct definitions and other libraries that need to be
 * included by both client and server */
#include "common.h"
#include <inttypes.h>
#include <arpa/inet.h>

#define BACKLOG_COUNT 100
#define USAGE_STRING				\
	"Missing parameter. Exiting.\n"		\
	"Usage: %s <port_number>\n"

static inline uint64_t get_clocks2(void) {
    uint32_t __clocks_hi, __clocks_lo;
    __asm__ __volatile__ ("rdtsc" : "=a" (__clocks_lo), "=d" (__clocks_hi));
    return (((uint64_t)__clocks_hi) << 32) | ((uint64_t)__clocks_lo);
}

double get_elapsed_busywait2(int wait_time_seconds, int wait_time_nanos){
    struct timespec start_time, current_time, delay;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    delay.tv_sec = wait_time_seconds;
    delay.tv_nsec = wait_time_nanos;
    struct timespec end_time = start_time;
    timespec_add(&end_time, &delay);
    uint64_t start, end;
    start = get_clocks2();
    do {
        clock_gettime(CLOCK_MONOTONIC, &current_time);
    } while (timespec_cmp(&end_time, &current_time) > 0);
    end = get_clocks2();
    return end - start;
}

/* Main function to handle connection with the client. This function
 * takes in input conn_socket and returns only when the connection
 * with the client is interrupted. */
static void handle_connection(int conn_socket) {
    struct request client_request;
    struct response server_response;
    ssize_t bytes_received, bytes_sent;
    struct timespec receipt_timestamp, completion_timestamp;
    while (1) {
        bytes_received = recv(conn_socket, &client_request, sizeof(client_request), 0);
        printf("bytes have been received \n");
        if (bytes_received < 0) {
            printf("ERROR: Could not read from socket.");
            break;
        }
        if (bytes_received == 0) {
            break;
        }

        clock_gettime(CLOCK_MONOTONIC, &receipt_timestamp);
        uint64_t busyWaitTime =  get_elapsed_busywait2(client_request.request_duration.tv_sec, client_request.request_duration.tv_nsec);
        clock_gettime(CLOCK_MONOTONIC, &completion_timestamp);
        server_response.request_id = client_request.request_id;
        write(conn_socket, &server_response, sizeof(server_response));
        printf("R%llu:%lf,%lf,%lf,%lf\n",
               client_request.request_id,
               (double)client_request.sent_timestamp.tv_sec + (double)client_request.sent_timestamp.tv_nsec / 1e9,
               (double)client_request.request_duration.tv_sec + (double)client_request.request_duration.tv_nsec / 1e9,
               (double)receipt_timestamp.tv_sec + (double)receipt_timestamp.tv_nsec / 1e9,
               (double)completion_timestamp.tv_sec + (double)completion_timestamp.tv_nsec / 1e9);
        if (busyWaitTime == 0) {
            printf("Busy wait error");
            break;
        }
    }
    close(conn_socket);
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

    /* Ready to handle the new connection with the client. */
    handle_connection(accepted);

    close(sockfd);
    return EXIT_SUCCESS;

}