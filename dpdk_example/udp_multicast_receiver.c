/*
 * udp_multicast_receiver.c - DPDK-based UDP multicast receiver
 * 
 * Demonstrates kernel-bypass networking for HFT order book processing
 * using DPDK Poll Mode Drivers (PMD).
 */

#include <stdint.h>
#include <inttypes.h>
#include <signal.h>
#include <getopt.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024
#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32

static volatile bool force_quit = false;

/* Statistics structure */
struct rx_stats {
    uint64_t packets;
    uint64_t bytes;
    uint64_t errors;
    uint64_t total_latency_cycles;
};

static struct rx_stats stats = {0};

/* Port configuration */
static const struct rte_eth_conf port_conf_default = {
    .rxmode = {
        .max_rx_pkt_len = RTE_ETHER_MAX_LEN,
    },
};

/*
 * Signal handler for clean exit
 */
static void signal_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        printf("\n\nSignal %d received, preparing to exit...\n", signum);
        force_quit = true;
    }
}

/*
 * Initialize a port with given configuration
 */
static int port_init(uint16_t port, struct rte_mempool *mbuf_pool) {
    struct rte_eth_conf port_conf = port_conf_default;
    const uint16_t rx_rings = 1, tx_rings = 1;
    uint16_t nb_rxd = RX_RING_SIZE;
    uint16_t nb_txd = TX_RING_SIZE;
    int retval;
    uint16_t q;
    struct rte_eth_dev_info dev_info;
    struct rte_eth_txconf txconf;

    if (!rte_eth_dev_is_valid_port(port))
        return -1;

    retval = rte_eth_dev_info_get(port, &dev_info);
    if (retval != 0) {
        printf("Error getting device info: %s\n", strerror(-retval));
        return retval;
    }

    /* Configure device */
    retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
    if (retval != 0)
        return retval;

    retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
    if (retval != 0)
        return retval;

    /* Allocate and set up RX queue */
    for (q = 0; q < rx_rings; q++) {
        retval = rte_eth_rx_queue_setup(port, q, nb_rxd,
                rte_eth_dev_socket_id(port), NULL, mbuf_pool);
        if (retval < 0)
            return retval;
    }

    /* Allocate and set up TX queue */
    txconf = dev_info.default_txconf;
    txconf.offloads = port_conf.txmode.offloads;
    for (q = 0; q < tx_rings; q++) {
        retval = rte_eth_tx_queue_setup(port, q, nb_txd,
                rte_eth_dev_socket_id(port), &txconf);
        if (retval < 0)
            return retval;
    }

    /* Start device */
    retval = rte_eth_dev_start(port);
    if (retval < 0)
        return retval;

    /* Enable promiscuous mode */
    retval = rte_eth_promiscuous_enable(port);
    if (retval != 0)
        return retval;

    return 0;
}

/*
 * Process UDP packet (simulate order book update)
 */
static inline void process_order_book_update(const uint8_t *payload, uint16_t len) {
    /* In real HFT system, this would:
     * 1. Parse market data message
     * 2. Update order book structure
     * 3. Trigger trading logic
     * 4. Generate orders if needed
     * 
     * For demo, we just simulate processing
     */
    (void)payload;
    (void)len;
    
    /* Simulate some processing */
    rte_pause();
}

/*
 * Main packet processing loop
 */
static void lcore_main(uint16_t port) {
    uint64_t start_cycles, end_cycles;
    
    printf("\nCore %u forwarding packets on port %u. [Ctrl+C to quit]\n",
            rte_lcore_id(), port);

    while (!force_quit) {
        struct rte_mbuf *bufs[BURST_SIZE];
        
        /* Receive burst of packets */
        const uint16_t nb_rx = rte_eth_rx_burst(port, 0, bufs, BURST_SIZE);
        
        if (unlikely(nb_rx == 0))
            continue;

        /* Process each packet */
        for (uint16_t i = 0; i < nb_rx; i++) {
            struct rte_mbuf *m = bufs[i];
            struct rte_ether_hdr *eth_hdr;
            struct rte_ipv4_hdr *ip_hdr;
            struct rte_udp_hdr *udp_hdr;
            uint8_t *payload;
            
            start_cycles = rte_rdtsc();
            
            /* Parse Ethernet header */
            eth_hdr = rte_pktmbuf_mtod(m, struct rte_ether_hdr *);
            
            /* Check if IP packet */
            if (eth_hdr->ether_type == rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4)) {
                ip_hdr = (struct rte_ipv4_hdr *)(eth_hdr + 1);
                
                /* Check if UDP packet */
                if (ip_hdr->next_proto_id == IPPROTO_UDP) {
                    udp_hdr = (struct rte_udp_hdr *)((uint8_t *)ip_hdr + 
                                                      sizeof(struct rte_ipv4_hdr));
                    payload = (uint8_t *)(udp_hdr + 1);
                    uint16_t payload_len = rte_be_to_cpu_16(udp_hdr->dgram_len) - 
                                           sizeof(struct rte_udp_hdr);
                    
                    /* Process order book update */
                    process_order_book_update(payload, payload_len);
                    
                    end_cycles = rte_rdtsc();
                    
                    /* Update statistics */
                    stats.packets++;
                    stats.bytes += m->pkt_len;
                    stats.total_latency_cycles += (end_cycles - start_cycles);
                }
            }
            
            /* Free packet buffer */
            rte_pktmbuf_free(m);
        }
    }
}

/*
 * Print statistics
 */
static void print_stats(void) {
    uint64_t hz = rte_get_tsc_hz();
    double avg_latency_ns = 0;
    
    if (stats.packets > 0) {
        avg_latency_ns = (double)(stats.total_latency_cycles * 1000000000ULL) / 
                         (hz * stats.packets);
    }
    
    printf("\n=== Final Statistics ===\n");
    printf("Total Packets:     %"PRIu64"\n", stats.packets);
    printf("Total Bytes:       %"PRIu64"\n", stats.bytes);
    printf("Errors:            %"PRIu64"\n", stats.errors);
    printf("Avg Latency:       %.2f ns\n", avg_latency_ns);
}

/*
 * Main function
 */
int main(int argc, char *argv[]) {
    struct rte_mempool *mbuf_pool;
    uint16_t portid = 0;
    unsigned lcore_id;
    int ret;

    /* Install signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Initialize DPDK EAL */
    ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

    argc -= ret;
    argv += ret;

    /* Check that there is a port to receive on */
    if (rte_eth_dev_count_avail() == 0)
        rte_exit(EXIT_FAILURE, "Error: no Ethernet ports detected\n");

    /* Create mbuf pool */
    mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS,
        MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

    if (mbuf_pool == NULL)
        rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

    /* Initialize port 0 */
    if (port_init(portid, mbuf_pool) != 0)
        rte_exit(EXIT_FAILURE, "Cannot init port %"PRIu16 "\n", portid);

    /* Get lcore for main processing */
    lcore_id = rte_lcore_id();
    
    printf("Starting UDP multicast receiver on lcore %u\n", lcore_id);
    printf("Press Ctrl+C to stop...\n");

    /* Run main processing loop */
    lcore_main(portid);

    /* Print statistics */
    print_stats();

    /* Clean shutdown */
    printf("\nStopping port %u...\n", portid);
    rte_eth_dev_stop(portid);
    rte_eth_dev_close(portid);

    /* Clean up EAL */
    rte_eal_cleanup();

    printf("Bye...\n");

    return 0;
}

