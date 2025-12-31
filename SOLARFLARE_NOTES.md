# Solarflare ef_vi Notes

## Overview

Our firm uses **Solarflare ef_vi** for ultra-low-latency networking in production. This document bridges the gap between the DPDK examples in this project and actual ef_vi development.

## Why Learn DPDK When We Use ef_vi?

The concepts are **nearly identical**:
- Both bypass the kernel
- Both use polling (no interrupts)
- Both require dedicated CPU cores
- Both use huge pages for memory
- Both provide zero-copy packet access

**DPDK is used here because:**
- You don't need expensive Solarflare hardware to learn
- DPDK documentation is more extensive
- Concepts transfer directly to ef_vi
- Many HFT firms use both technologies

## API Comparison

### Initialization

**DPDK:**
```c
rte_eal_init(argc, argv);
mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS, ...);
```

**ef_vi:**
```c
ef_driver_open(&dh);
ef_pd_alloc(&pd, dh, ifindex, EF_PD_DEFAULT);
ef_vi_alloc_from_pd(&vi, dh, &pd, dh, -1, -1, -1, NULL, -1, 0);
```

### Packet Reception

**DPDK:**
```c
uint16_t nb_rx = rte_eth_rx_burst(port_id, queue_id, bufs, BURST_SIZE);
for (int i = 0; i < nb_rx; i++) {
    uint8_t *pkt = rte_pktmbuf_mtod(bufs[i], uint8_t*);
    process_packet(pkt, bufs[i]->data_len);
    rte_pktmbuf_free(bufs[i]);
}
```

**ef_vi:**
```c
ef_eventq_poll(&vi, events, MAX_EVENTS);
for (int i = 0; i < n_ev; i++) {
    if (EF_EVENT_TYPE(events[i]) == EF_EVENT_TYPE_RX) {
        ef_vi_receive_get(&vi, events[i].rx.rq_id, &pkt_addr, &len);
        process_packet(pkt_addr, len);
        ef_vi_receive_post(&vi, pkt_addr, buf_id);
    }
}
```

### Packet Transmission

**DPDK:**
```c
uint16_t nb_tx = rte_eth_tx_burst(port_id, queue_id, tx_bufs, nb_pkts);
```

**ef_vi:**
```c
ef_vi_transmit(&vi, base, len, rq_id);
ef_vi_transmit_push(&vi);
```

## Key Differences

| Feature | DPDK | Solarflare ef_vi |
|---------|------|------------------|
| **Latency** | 500-2000 ns | 200-400 ns |
| **Hardware** | Many vendors | Solarflare only |
| **API Complexity** | More abstraction | Lower level |
| **Documentation** | Extensive | Good but smaller |
| **Cost** | Free software | Expensive NICs |
| **Industry Use** | Widespread | HFT/Finance focused |

## Performance Expectations

### DPDK (This Project's Benchmarks)
- Median latency: ~1-2 μs
- P99 latency: ~3-4 μs
- Good for learning

### Solarflare ef_vi (Production at Our Firm)
- Median latency: ~200-400 ns
- P99 latency: ~500-800 ns
- **2-5x faster than DPDK**

## Getting Started with ef_vi (When You Have Hardware)

### 1. Install Solarflare Drivers

```bash
# Download from Solarflare website (requires login)
# Install EnterpriseOnload package
tar xf onload-<version>.tgz
cd onload-<version>/scripts
./onload_install
```

### 2. Verify Installation

```bash
# Check driver
lsmod | grep sfc

# Check NIC
onload_stackdump lots

# Test performance
ef_vi_test
```

### 3. Basic ef_vi Program Structure

```c
#include <etherfabric/vi.h>
#include <etherfabric/pd.h>
#include <etherfabric/memreg.h>

int main() {
    ef_driver_handle dh;
    ef_pd pd;
    ef_vi vi;
    
    // 1. Open driver
    ef_driver_open(&dh);
    
    // 2. Allocate protection domain
    ef_pd_alloc(&pd, dh, ifindex, EF_PD_DEFAULT);
    
    // 3. Allocate virtual interface
    ef_vi_alloc_from_pd(&vi, dh, &pd, dh, -1, -1, -1, NULL, -1, 0);
    
    // 4. Register memory for DMA
    ef_memreg_alloc(&memreg, dh, &pd, dh, buffer, buffer_size);
    
    // 5. Post receive buffers
    for (int i = 0; i < N_BUFS; i++)
        ef_vi_receive_init(&vi, buf_addr[i], i);
    
    // 6. Main poll loop
    ef_event events[EF_VI_EVENT_POLL_MIN_EVS];
    while (running) {
        int n_ev = ef_eventq_poll(&vi, events, EF_VI_EVENT_POLL_MIN_EVS);
        for (int i = 0; i < n_ev; i++) {
            // Process events...
        }
    }
    
    return 0;
}
```

### 4. Compile

```bash
gcc -o my_app my_app.c -L/usr/lib64/onload/lib -lciapp1 -lciul1 -lpthread
```

## Common ef_vi Patterns in HFT

### 1. UDP Multicast Receiver (Order Books)

```c
// Subscribe to multicast group
struct ip_mreqn mreq = {
    .imr_multiaddr.s_addr = inet_addr("239.1.1.1"),
    .imr_address.s_addr = INADDR_ANY,
    .imr_ifindex = ifindex
};
ef_vi_filter_add(&vi, dh, EF_FILTER_FLAG_MCAST_LOOP_RECEIVE,
                 NULL, 0, &mreq.imr_multiaddr, 12345, NULL, 0);
```

### 2. Timestamping

```c
// Hardware timestamping for latency measurement
ef_vi_receive_get_timestamp(&vi, &ts_sec, &ts_nsec);
```

### 3. TX with Warm-up

```c
// "Warm up" the TX path before critical section
ef_vi_transmit_init(&vi, addr, len, rq_id);
ef_vi_transmit_push(&vi);
```

## Optimization Tips for ef_vi

1. **CPU Pinning**
   ```bash
   taskset -c 2 ./my_app  # Pin to core 2
   ```

2. **Huge Pages** (same as DPDK)
   ```bash
   echo 512 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
   ```

3. **Interrupt Coalescing Disabled**
   ```bash
   ethtool -C eth0 rx-usecs 0 tx-usecs 0
   ```

4. **Use `ef_vi_transmit_alt_*` for Lowest Latency**
   - Alternative TX API with even lower latency
   - Used for order submission

5. **Busy-Wait Variants**
   ```c
   ef_eventq_poll(&vi, events, EF_VI_EVENT_POLL_MIN_EVS | EF_VI_POLL_SPIN);
   ```

## Debugging ef_vi

### Check Event Queue

```bash
onload_stackdump lots
```

### Packet Capture

```bash
# ef_vi doesn't work with tcpdump directly
# Use hardware packet capture or mirroring
```

### Performance Analysis

```bash
# Check RX/TX statistics
ethtool -S eth0

# Check for drops
ef_vi_stats_query(&vi, &stats);
```

## Resources

### Official Documentation
- Solarflare EnterpriseOnload User Guide
- ef_vi API Reference (comes with installation)
- `/usr/share/doc/onload/`

### Example Code
- `/usr/share/doc/onload/examples/`
- Look for `efvi_receive.c` and `efvi_transmit.c`

### Support
- Solarflare support portal (requires customer login)
- Internal firm documentation
- Ask senior developers on the team

## When You Transition from DPDK to ef_vi

**What Transfers Directly:**
- ✓ Polling concepts
- ✓ Memory management principles
- ✓ CPU isolation strategies
- ✓ Performance tuning mindset
- ✓ Debugging approaches

**What's Different:**
- API is lower-level (less abstraction)
- Event-based model vs burst model
- Hardware-specific optimizations
- Tighter integration with NIC features

**Estimated Learning Time:**
- If you understand DPDK: 1-2 weeks to be productive
- If you start from scratch: 4-6 weeks

## Bottom Line

**This DPDK project teaches you:**
- ✓ Kernel bypass fundamentals
- ✓ Polling architecture
- ✓ Memory management
- ✓ Performance optimization

**When you use ef_vi at work:**
- You'll recognize all the concepts
- The API is different but principles are the same
- You'll achieve even better latency
- Most effort will be in production-specific optimizations

**Think of DPDK as your training wheels** - once you understand it, ef_vi is just a faster, more specialized version of the same ideas.

