# DPDK UDP Multicast Receiver

A high-performance UDP multicast receiver using DPDK for kernel-bypass networking, designed for HFT order book processing.

## Features

- **Kernel Bypass**: Direct NIC access via DPDK PMD
- **Poll Mode**: Continuous polling for minimum latency
- **Zero-Copy**: Direct packet access without memory copies
- **Latency Tracking**: Measures processing time per packet
- **Statistics**: Packet count, bytes, and average latency

## Prerequisites

### Install DPDK

```bash
# Install dependencies
sudo apt-get update
sudo apt-get install -y build-essential libnuma-dev python3-pip meson ninja-build

# Download DPDK
wget https://fast.dpdk.org/rel/dpdk-22.11.tar.xz
tar xf dpdk-22.11.tar.xz
cd dpdk-22.11

# Build DPDK
meson build
cd build
ninja
sudo ninja install
sudo ldconfig

# Set environment variable
export RTE_SDK=/path/to/dpdk-22.11
export RTE_TARGET=x86_64-native-linuxapp-gcc
```

### Configure Huge Pages

```bash
# Allocate 1024 huge pages (2GB total)
echo 1024 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages

# Mount hugetlbfs
sudo mkdir -p /mnt/huge
sudo mount -t hugetlbfs nodev /mnt/huge

# Make persistent (add to /etc/fstab)
echo "nodev /mnt/huge hugetlbfs defaults 0 0" | sudo tee -a /etc/fstab
```

### Bind NIC to DPDK

```bash
# Load UIO driver
sudo modprobe uio_pci_generic

# Identify your NIC
lspci | grep Ethernet

# Bind NIC to DPDK (example: 0000:01:00.0)
cd $RTE_SDK/usertools
sudo ./dpdk-devbind.py --bind=uio_pci_generic 0000:01:00.0

# Check status
sudo ./dpdk-devbind.py --status
```

## Building

### Using DPDK Make System (older DPDK)

```bash
export RTE_SDK=/path/to/dpdk
export RTE_TARGET=x86_64-native-linuxapp-gcc
make
```

### Using pkg-config (newer DPDK)

```bash
make build-pkgconfig
```

## Running

### Basic Usage

```bash
sudo ./udp_multicast_receiver -l 0-1 -n 4 -- -p 0x1
```

Parameters:
- `-l 0-1`: Use CPU cores 0 and 1
- `-n 4`: 4 memory channels
- `-p 0x1`: Port mask (port 0)

### With CPU Isolation

For best performance, isolate CPUs from the kernel:

```bash
# Add to kernel boot parameters (edit /etc/default/grub):
# GRUB_CMDLINE_LINUX="isolcpus=1,2,3"

# Update grub and reboot
sudo update-grub
sudo reboot

# Run with isolated cores
sudo ./udp_multicast_receiver -l 1 -n 4 -- -p 0x1
```

### Testing with Multicast Traffic

In another terminal, send UDP multicast packets:

```bash
# Using Python
python3 << EOF
import socket
import time

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 2)

while True:
    message = b"ORDER_BOOK_UPDATE:BID=100.50,ASK=100.55"
    sock.sendto(message, ("239.1.1.1", 12345))
    time.sleep(0.001)  # 1ms interval
EOF
```

## Performance Tuning

### 1. CPU Affinity

Pin the application to specific cores:

```bash
sudo taskset -c 1 ./udp_multicast_receiver -l 1 -n 4
```

### 2. Interrupt Affinity

Disable interrupts on isolated cores:

```bash
for i in /proc/irq/*/smp_affinity; do
    echo 1 | sudo tee $i  # Pin all IRQs to core 0
done
```

### 3. Kernel Parameters

```bash
# Disable CPU frequency scaling
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Disable C-states
sudo cpupower idle-set -D 0
```

## Expected Performance

Typical latency results:

| Configuration | Avg Latency | P99 Latency |
|--------------|-------------|-------------|
| Kernel driver | 10-15 μs | 25-50 μs |
| DPDK (this app) | 0.5-2 μs | 3-5 μs |

**Improvement**: 5-10x latency reduction!

## Architecture

```
┌─────────────────────────────────────────────┐
│         User Space Application              │
│  ┌──────────────────────────────────────┐   │
│  │  Main Processing Loop (lcore_main)   │   │
│  │                                      │   │
│  │  1. rte_eth_rx_burst() - Poll NIC   │   │
│  │  2. Parse Ethernet/IP/UDP headers   │   │
│  │  3. process_order_book_update()     │   │
│  │  4. rte_pktmbuf_free()              │   │
│  └──────────────────────────────────────┘   │
│                    ↕                        │
│  ┌──────────────────────────────────────┐   │
│  │     DPDK Poll Mode Driver (PMD)      │   │
│  └──────────────────────────────────────┘   │
└─────────────────────────────────────────────┘
                     ↕
┌─────────────────────────────────────────────┐
│          NIC Hardware (Direct Access)       │
└─────────────────────────────────────────────┘
```

## Troubleshooting

### No packets received

1. Check NIC binding: `sudo ./dpdk-devbind.py --status`
2. Verify huge pages: `cat /proc/meminfo | grep Huge`
3. Check multicast group on NIC
4. Disable firewall: `sudo iptables -F`

### Low performance

1. Enable CPU isolation
2. Use huge pages (2MB or 1GB)
3. Pin to dedicated cores
4. Disable frequency scaling

### Build errors

```bash
# Install missing dependencies
sudo apt-get install -y libnuma-dev

# Update pkg-config path
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig
```

## Code Overview

### Main Components

1. **port_init()**: Configure NIC with RX/TX queues
2. **lcore_main()**: Main polling loop on dedicated core
3. **process_order_book_update()**: Order book processing logic
4. **Statistics tracking**: Latency and throughput metrics

### Key DPDK APIs

- `rte_eal_init()`: Initialize DPDK environment
- `rte_pktmbuf_pool_create()`: Create packet buffer pool
- `rte_eth_rx_burst()`: Receive burst of packets (poll mode)
- `rte_pktmbuf_mtod()`: Get packet data pointer (zero-copy)
- `rte_rdtsc()`: Read CPU timestamp counter (for latency)

## Next Steps

- Implement full order book data structure
- Add multiple multicast group support
- Integrate with trading logic
- Add order transmission (TX path)
- Implement message parsing for real market data formats

## References

- [DPDK Documentation](https://doc.dpdk.org/)
- [DPDK Sample Applications](https://doc.dpdk.org/guides/sample_app_ug/)
- [DPDK Getting Started Guide](https://doc.dpdk.org/guides/linux_gsg/)

