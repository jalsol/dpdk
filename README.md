# Linux Drivers and Kernel-Bypass Networking for HFT

A comprehensive presentation and code examples covering Linux driver development, kernel-bypass networking with DPDK, and HFT-focused benchmarks for order book processing.

## Overview

This project demonstrates the journey from traditional Linux drivers to high-performance kernel-bypass networking, with a focus on High-Frequency Trading (HFT) applications. It includes:

- **Educational Presentation**: 40 slides covering driver basics through DPDK
- **Working Code Examples**: Character device driver, DPDK application, benchmarks
- **Real Benchmarks**: Single and multiple UDP multicast feed scenarios
- **Practical Guidance**: When and how to implement kernel-bypass solutions

**Note on Solarflare ef_vi**: While our firm uses Solarflare ef_vi in production, this presentation uses DPDK as a teaching tool since it's more accessible without specialized hardware. The concepts (polling, huge pages, CPU isolation, zero-copy) are identical and directly transferable to ef_vi development. A dedicated comparison slide explains the mapping between DPDK and ef_vi APIs.

## Target Audience

Intermediate developers with:
- Basic Linux/C programming knowledge
- Understanding of networking concepts
- Interest in low-latency systems and HFT

## Project Structure

```
dpdk/
├── presentation.md              # Main presentation (40 slides, 30-45 min)
├── SOLARFLARE_NOTES.md          # Bridging DPDK concepts to ef_vi
├── char_driver_example/         # Simple character device driver
│   ├── simple_chardev.c
│   ├── Makefile
│   └── README.md
├── dpdk_example/                # DPDK UDP multicast receiver
│   ├── udp_multicast_receiver.c
│   ├── Makefile
│   └── README.md
├── benchmarks/                  # Benchmark suite
│   ├── feed_simulator.py        # UDP multicast feed generator
│   ├── kernel_receiver.c        # Kernel driver baseline
│   ├── run_benchmark.sh         # Automated test runner
│   ├── Makefile
│   └── README.md
└── README.md                    # This file
```

## For ef_vi Users

**Using Solarflare ef_vi at your firm?** See [`SOLARFLARE_NOTES.md`](SOLARFLARE_NOTES.md) for:
- API comparison: DPDK ↔ ef_vi
- What concepts transfer directly
- Getting started with ef_vi when you have hardware
- Production optimization tips

## Quick Start

### 1. Character Device Driver

Learn Linux driver basics with a simple character device:

```bash
cd char_driver_example
make
make load
make test
make unload
```

See [char_driver_example/README.md](char_driver_example/README.md) for details.

### 2. DPDK Setup

Install and configure DPDK for kernel-bypass networking:

```bash
# Install DPDK (see dpdk_example/README.md for full instructions)
sudo apt-get install -y dpdk dpdk-dev

# Configure huge pages
echo 1024 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages

# Build DPDK example
cd dpdk_example
# Follow README.md for compilation
```

See [dpdk_example/README.md](dpdk_example/README.md) for complete setup.

### 3. Run Benchmarks

Compare kernel driver vs DPDK performance:

```bash
cd benchmarks
make
sudo ./run_benchmark.sh
```

See [benchmarks/README.md](benchmarks/README.md) for detailed instructions.

## Presentation

The main presentation (`presentation.md`) is 40 slides covering:

### Part 1: Introduction to Linux Drivers (Slides 1-10)
- Kernel architecture overview
- Character device driver basics
- Network driver essentials (real e1000 driver example)
- **Where is the RX handler?** (interrupt-driven vs sync)
- Network packet flow
- Performance bottlenecks

### Part 2: Kernel-Bypass Networking (Slides 11-21)
- Why kernel bypass for HFT
- DPDK architecture (PMD, huge pages, CPU isolation)
- DPDK vs Solarflare ef_vi comparison (our firm's stack)
- Code comparison: sockets vs DPDK
- Memory flow diagrams

### Part 3: HFT Order Book Processing Benchmarks (Slides 22-33)
- Test setup and configuration
- Scenario 1: Single feed processing
- Scenario 2: Multiple feed processing
- Results analysis and jitter metrics

### Part 4: Conclusions and Takeaways (Slides 34-40)
- When to use kernel bypass
- Development complexity and trade-offs
- Alternative technologies (Solarflare ef_vi, XDP, AF_XDP, io_uring)
- Q&A and resources

### Viewing the Presentation

**Option 1: Markdown Viewer**
```bash
# Use any markdown viewer
glow presentation.md
# or
mdless presentation.md
```

**Option 2: Convert to Slides (reveal.js)**
```bash
# Install pandoc
sudo apt-get install pandoc

# Convert to HTML slides
pandoc -t revealjs -s presentation.md -o presentation.html -V theme=night

# Open in browser
firefox presentation.html
```

**Option 3: Use Online Tools**
- Upload to https://slides.com/
- Use https://marp.app/ for Marp-based rendering
- Import into Google Slides or PowerPoint

## Key Results

Performance improvements achieved with DPDK vs traditional kernel drivers:

| Scenario | Metric | Kernel Driver | DPDK | Improvement |
|----------|--------|---------------|------|-------------|
| Single Feed | Median Latency | 12.3 μs | 1.8 μs | **6.8x** |
| Single Feed | P99 Latency | 28.4 μs | 3.1 μs | **9.2x** |
| Multiple Feeds | Median Latency | 16.1 μs | 2.2 μs | **7.3x** |
| Multiple Feeds | P99 Latency | 43.9 μs | 3.9 μs | **11.3x** |
| Jitter (Std Dev) | Single Feed | 8.7 μs | 0.8 μs | **10x better** |

**Summary**: 5-10x latency improvement with 10x more stable performance.

## Prerequisites

### For Character Driver Example
- Linux kernel headers: `sudo apt-get install linux-headers-$(uname -r)`
- Build tools: `sudo apt-get install build-essential`

### For DPDK Example
- DPDK libraries: `sudo apt-get install dpdk dpdk-dev`
- Or build from source (see dpdk_example/README.md)
- Huge pages support
- Compatible NIC (Intel X710, Mellanox ConnectX, etc.)

### For Benchmarks
- Python 3.x
- GCC compiler
- Root access (for network configuration)

## Learning Path

Recommended order for going through the materials:

1. **Read the presentation** (presentation.md)
   - Understand concepts before diving into code
   - 30-45 minute read

2. **Build and test character driver** (char_driver_example/)
   - Hands-on Linux driver basics
   - 15-30 minutes

3. **Install and configure DPDK** (dpdk_example/)
   - System setup and configuration
   - 1-2 hours (first time)

4. **Run DPDK example** (dpdk_example/)
   - See kernel bypass in action
   - 30 minutes

5. **Execute benchmarks** (benchmarks/)
   - Compare performance yourself
   - 1 hour

6. **Experiment and modify**
   - Change parameters, test scenarios
   - Ongoing learning

## Use Cases

This material is relevant for:

- **HFT Trading Systems**: Market data processing, order execution
- **Low-Latency Messaging**: Financial messaging, real-time communications
- **Network Function Virtualization**: Packet processing, routing
- **High-Performance Computing**: Distributed computing, data transfer
- **Research and Education**: Understanding kernel bypass techniques

## Troubleshooting

### Character Driver
- Module won't load: Check kernel logs with `dmesg`
- Permission denied: Device needs proper permissions or run as root
- Module in use: Unload with `rmmod` before rebuilding

### DPDK
- No huge pages: Configure with `echo 1024 > /sys/kernel/.../nr_hugepages`
- NIC not bound: Use `dpdk-devbind.py` to bind NIC to DPDK
- Build errors: Install missing dependencies (libnuma-dev)

### Benchmarks
- No packets received: Check multicast routing and firewall
- High latency: Disable CPU frequency scaling, isolate cores
- Socket errors: Run as root for network configuration

See individual README files for detailed troubleshooting.

## Performance Tuning

For optimal DPDK performance:

1. **CPU Isolation**
   ```bash
   # Add to /etc/default/grub:
   GRUB_CMDLINE_LINUX="isolcpus=1-3"
   sudo update-grub && sudo reboot
   ```

2. **Huge Pages**
   ```bash
   # Persistent huge pages
   echo "vm.nr_hugepages=1024" | sudo tee -a /etc/sysctl.conf
   ```

3. **CPU Frequency Scaling**
   ```bash
   # Set performance governor
   echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
   ```

4. **IRQ Affinity**
   ```bash
   # Pin interrupts to specific cores
   echo 1 | sudo tee /proc/irq/*/smp_affinity
   ```

See [dpdk_example/README.md](dpdk_example/README.md) for complete tuning guide.

## Contributing

This is an educational project. Improvements welcome:

- Enhanced benchmark scenarios
- Additional driver examples
- Performance optimizations
- Documentation improvements
- Bug fixes

## References

### Solarflare ef_vi
- [SOLARFLARE_NOTES.md](SOLARFLARE_NOTES.md) - Bridging guide in this repo
- Solarflare EnterpriseOnload Documentation (requires customer access)
- `/usr/share/doc/onload/` after installation

### DPDK
- [DPDK Official Documentation](https://doc.dpdk.org/)
- [DPDK Getting Started Guide](https://doc.dpdk.org/guides/linux_gsg/)
- [DPDK Sample Applications](https://doc.dpdk.org/guides/sample_app_ug/)

### Linux Kernel
- [Linux Device Drivers (LDD3)](https://lwn.net/Kernel/LDD3/)
- [Linux Kernel Documentation](https://www.kernel.org/doc/)
- [Network Driver Guide](https://wiki.linuxfoundation.org/)

### Performance
- [Brendan Gregg's Linux Performance](http://www.brendangregg.com/)
- [Mechanical Sympathy Blog](https://mechanical-sympathy.blogspot.com/)

### HFT and Low Latency
- Industry white papers and publications
- Conference talks (Strata, QCon, etc.)

## License

Educational use. Code examples provided as-is for learning purposes.

## Author

HFT Intern - Learning journey through Linux drivers and kernel-bypass networking

## Acknowledgments

- DPDK community for excellent documentation
- Linux kernel developers
- HFT community for pushing performance boundaries

---

**Ready to dive into sub-microsecond latency?** Start with the presentation, then work through the examples!

For questions or issues, refer to individual component READMEs or the presentation Q&A section.

