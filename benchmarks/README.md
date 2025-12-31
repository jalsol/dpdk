# HFT Order Book Feed Benchmarks

Benchmark suite for comparing kernel driver vs DPDK performance in HFT order book processing scenarios.

## Overview

This suite provides tools to:
1. Simulate UDP multicast order book feeds
2. Measure latency with kernel-based receivers
3. Compare against DPDK receivers
4. Test single and multiple feed scenarios

## Components

### 1. Feed Simulator (`feed_simulator.py`)

Python script that generates UDP multicast order book updates.

**Usage:**
```bash
# Single feed, 10,000 msg/sec for 60 seconds
python3 feed_simulator.py -g 239.1.1.1 -p 12345 -r 10000 -d 60

# Different symbol and rate
python3 feed_simulator.py -g 239.1.1.2 -s MSFT -r 50000 -d 30

# Show instructions for multiple feeds
python3 feed_simulator.py --multiple
```

**Options:**
- `-g, --group`: Multicast group address (default: 239.1.1.1)
- `-p, --port`: UDP port (default: 12345)
- `-s, --symbol`: Stock symbol (default: AAPL)
- `-r, --rate`: Messages per second (default: 10000)
- `-d, --duration`: Duration in seconds (default: 60)
- `-m, --multiple`: Show instructions for multiple feeds

### 2. Kernel Receiver (`kernel_receiver.c`)

Traditional socket-based UDP multicast receiver for baseline measurements.

**Build:**
```bash
make
```

**Usage:**
```bash
# Basic usage
./kernel_receiver 239.1.1.1 12345

# Just multicast group (uses default port 12345)
./kernel_receiver 239.1.1.1
```

**Output:**
- Periodic statistics every 5 seconds
- Final statistics on exit (Ctrl+C)
- Latency measurements in microseconds

### 3. Automated Benchmark Suite (`run_benchmark.sh`)

Complete benchmark automation for both scenarios.

**Usage:**
```bash
sudo ./run_benchmark.sh
```

**Features:**
- Interactive menu for selecting tests
- Automated setup and teardown
- Result logging to separate files
- Support for single and multiple feed scenarios

## Benchmark Scenarios

### Scenario 1: Single Feed Processing

Tests processing of one order book feed at 50,000 msg/sec.

**Kernel Driver Test:**
```bash
# Terminal 1: Start receiver
./kernel_receiver 239.1.1.1 12345

# Terminal 2: Start simulator
python3 feed_simulator.py -g 239.1.1.1 -p 12345 -r 50000 -d 30
```

**DPDK Test:**
```bash
# Terminal 1: Start DPDK receiver (see dpdk_example/README.md)
cd ../dpdk_example
sudo ./udp_multicast_receiver -l 1 -n 4

# Terminal 2: Start simulator
cd ../benchmarks
python3 feed_simulator.py -g 239.1.1.1 -p 12345 -r 50000 -d 30
```

### Scenario 2: Multiple Feed Processing

Tests processing of four concurrent order book feeds (200,000 combined msg/sec).

**Kernel Driver Test:**
```bash
# Terminal 1-4: Start receivers
./kernel_receiver 239.1.1.1 12345 &
./kernel_receiver 239.1.1.2 12345 &
./kernel_receiver 239.1.1.3 12345 &
./kernel_receiver 239.1.1.4 12345 &

# Terminal 5-8: Start simulators
python3 feed_simulator.py -g 239.1.1.1 -s AAPL -r 50000 -d 30 &
python3 feed_simulator.py -g 239.1.1.2 -s MSFT -r 50000 -d 30 &
python3 feed_simulator.py -g 239.1.1.3 -s GOOGL -r 50000 -d 30 &
python3 feed_simulator.py -g 239.1.1.4 -s AMZN -r 50000 -d 30 &
```

**Or use the automated script:**
```bash
sudo ./run_benchmark.sh
# Select option 3 or 5
```

## Message Format

Order book update messages are 32 bytes:

```c
struct order_book_update {
    uint64_t timestamp;    // Nanoseconds since epoch
    char symbol[4];        // Stock symbol (e.g., "AAPL")
    uint32_t bid_price;    // Bid price in cents
    uint32_t bid_size;     // Bid quantity
    uint32_t ask_price;    // Ask price in cents
    uint32_t ask_size;     // Ask quantity
    uint32_t sequence;     // Message sequence number
};
```

## Metrics Collected

1. **Latency**: Time from message send to processing complete
   - Average (mean)
   - Minimum
   - Maximum
   - P50, P99, P99.9 (requires extended analysis)

2. **Throughput**: Messages processed per second

3. **Packet Loss**: Messages sent vs received

## Expected Results

### Single Feed (50,000 msg/sec)

| Metric | Kernel Driver | DPDK | Improvement |
|--------|---------------|------|-------------|
| Avg Latency | 10-15 μs | 1-2 μs | 6-8x |
| P99 Latency | 25-35 μs | 3-4 μs | 8-10x |
| CPU Usage | 35% (1 core) | 100% (1 core) | Dedicated |

### Multiple Feeds (200,000 msg/sec)

| Metric | Kernel Driver | DPDK | Improvement |
|--------|---------------|------|-------------|
| Avg Latency | 15-20 μs | 2-3 μs | 7-8x |
| P99 Latency | 40-50 μs | 4-5 μs | 10-12x |
| Packet Loss | 0-1% | 0% | Perfect |
| CPU Usage | 85% (2 cores) | 100% (1 core) | More efficient |

## Network Configuration

### Multicast Routing

Ensure multicast is enabled on your network interface:

```bash
# Check multicast support
ip link show

# Add multicast route
sudo route add -net 224.0.0.0 netmask 240.0.0.0 dev eth0
```

### Firewall

Disable firewall for testing:

```bash
sudo iptables -F
sudo iptables -X
```

### Verify Multicast

```bash
# Listen for multicast packets (requires socat)
socat UDP4-RECVFROM:12345,ip-add-membership=239.1.1.1:eth0,fork -

# In another terminal, send test packet
echo "test" | socat - UDP4-DATAGRAM:239.1.1.1:12345
```

## Troubleshooting

### No packets received

1. Check multicast routing: `route -n | grep 224`
2. Verify interface supports multicast: `ip link show | grep MULTICAST`
3. Check firewall: `sudo iptables -L`
4. Verify group membership: `netstat -g`

### High latency variance

1. Disable CPU frequency scaling
2. Isolate CPU cores from kernel
3. Disable unnecessary services
4. Use real-time kernel (optional)

### Packet loss

1. Increase socket buffer: `setsockopt(SO_RCVBUF)`
2. Reduce message rate
3. Use multiple receiver threads
4. Switch to DPDK

## Advanced Analysis

For detailed latency distribution analysis, collect raw timestamps and process with:

```python
import numpy as np

latencies = []  # Load from logs
p50 = np.percentile(latencies, 50)
p99 = np.percentile(latencies, 99)
p999 = np.percentile(latencies, 99.9)
print(f"P50: {p50:.2f} μs, P99: {p99:.2f} μs, P99.9: {p999:.2f} μs")
```

## Cleanup

```bash
# Kill all benchmark processes
pkill -f feed_simulator
pkill -f kernel_receiver

# Remove log files
rm -f *.log

# Clean build artifacts
make clean
```

## Next Steps

1. Implement latency histogram collection
2. Add P99/P99.9 percentile tracking
3. Create visualization scripts for results
4. Add support for different message sizes
5. Implement jitter analysis

