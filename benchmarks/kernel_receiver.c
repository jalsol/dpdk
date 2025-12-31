/*
 * kernel_receiver.c - Traditional kernel-based UDP multicast receiver
 * 
 * Receives UDP multicast order book updates using standard socket API.
 * Used as baseline for comparing against DPDK performance.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define MAX_BUFFER_SIZE 2048
#define NSEC_PER_SEC 1000000000ULL

static volatile int running = 1;

/* Statistics structure */
struct stats {
    uint64_t packets_received;
    uint64_t bytes_received;
    uint64_t total_latency_ns;
    uint64_t min_latency_ns;
    uint64_t max_latency_ns;
};

/* Get current timestamp in nanoseconds */
static inline uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * NSEC_PER_SEC + (uint64_t)ts.tv_nsec;
}

/* Signal handler */
static void signal_handler(int signum) {
    (void)signum;
    running = 0;
}

/* Process order book update message */
static void process_order_book(const uint8_t *data, size_t len, 
                               uint64_t recv_time_ns, struct stats *stats) {
    if (len < 32) {
        /* Invalid message */
        return;
    }
    
    /* Extract timestamp from message (first 8 bytes, network byte order) */
    uint64_t send_time_ns = 0;
    for (int i = 0; i < 8; i++) {
        send_time_ns = (send_time_ns << 8) | data[i];
    }
    
    /* Calculate latency */
    uint64_t latency_ns = recv_time_ns - send_time_ns;
    
    /* Update statistics */
    stats->packets_received++;
    stats->bytes_received += len;
    stats->total_latency_ns += latency_ns;
    
    if (stats->packets_received == 1 || latency_ns < stats->min_latency_ns) {
        stats->min_latency_ns = latency_ns;
    }
    if (latency_ns > stats->max_latency_ns) {
        stats->max_latency_ns = latency_ns;
    }
}

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in addr;
    struct ip_mreq mreq;
    uint8_t buffer[MAX_BUFFER_SIZE];
    struct stats stats = {0};
    
    const char *multicast_group = "239.1.1.1";
    int port = 12345;
    
    /* Parse command line arguments */
    if (argc >= 2) {
        multicast_group = argv[1];
    }
    if (argc >= 3) {
        port = atoi(argv[2]);
    }
    
    printf("Kernel-based UDP Multicast Receiver\n");
    printf("Multicast Group: %s\n", multicast_group);
    printf("Port: %d\n", port);
    printf("Press Ctrl+C to stop...\n\n");
    
    /* Install signal handler */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* Create UDP socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }
    
    /* Allow multiple sockets to bind to the same port */
    int reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
                   &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt SO_REUSEADDR");
        close(sockfd);
        return 1;
    }
    
    /* Bind to port */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    
    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sockfd);
        return 1;
    }
    
    /* Join multicast group */
    mreq.imr_multiaddr.s_addr = inet_addr(multicast_group);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    
    if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
                   &mreq, sizeof(mreq)) < 0) {
        perror("setsockopt IP_ADD_MEMBERSHIP");
        close(sockfd);
        return 1;
    }
    
    printf("Listening for multicast packets...\n");
    
    uint64_t last_report_time = get_timestamp_ns();
    uint64_t report_interval_ns = 5ULL * NSEC_PER_SEC;  /* Report every 5 seconds */
    
    /* Main receive loop */
    while (running) {
        ssize_t n = recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL);
        
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("recvfrom");
            break;
        }
        
        uint64_t recv_time = get_timestamp_ns();
        
        /* Process packet */
        process_order_book(buffer, n, recv_time, &stats);
        
        /* Print periodic statistics */
        if (recv_time - last_report_time >= report_interval_ns) {
            uint64_t avg_latency_ns = stats.total_latency_ns / stats.packets_received;
            double avg_latency_us = avg_latency_ns / 1000.0;
            double min_latency_us = stats.min_latency_ns / 1000.0;
            double max_latency_us = stats.max_latency_ns / 1000.0;
            
            printf("Packets: %lu, Avg Latency: %.2f μs, "
                   "Min: %.2f μs, Max: %.2f μs\n",
                   stats.packets_received, avg_latency_us, 
                   min_latency_us, max_latency_us);
            
            last_report_time = recv_time;
        }
    }
    
    /* Print final statistics */
    printf("\n=== Final Statistics ===\n");
    printf("Total Packets:     %lu\n", stats.packets_received);
    printf("Total Bytes:       %lu\n", stats.bytes_received);
    
    if (stats.packets_received > 0) {
        uint64_t avg_latency_ns = stats.total_latency_ns / stats.packets_received;
        double avg_latency_us = avg_latency_ns / 1000.0;
        double min_latency_us = stats.min_latency_ns / 1000.0;
        double max_latency_us = stats.max_latency_ns / 1000.0;
        
        printf("Average Latency:   %.2f μs\n", avg_latency_us);
        printf("Min Latency:       %.2f μs\n", min_latency_us);
        printf("Max Latency:       %.2f μs\n", max_latency_us);
    }
    
    /* Cleanup */
    close(sockfd);
    
    return 0;
}

