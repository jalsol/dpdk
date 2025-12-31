#!/bin/bash
# run_benchmark.sh - Automated benchmark runner for comparing kernel vs DPDK

set -e

# Configuration
MULTICAST_GROUP="239.1.1.1"
PORT=12345
DURATION=30
RATE=50000

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

print_header() {
    echo -e "\n${GREEN}===================================================${NC}"
    echo -e "${GREEN}$1${NC}"
    echo -e "${GREEN}===================================================${NC}\n"
}

print_info() {
    echo -e "${YELLOW}[INFO]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

check_prerequisites() {
    print_header "Checking Prerequisites"
    
    # Check if running as root
    if [ "$EUID" -ne 0 ]; then 
        print_error "Please run as root (needed for network configuration)"
        exit 1
    fi
    
    # Check if Python3 is available
    if ! command -v python3 &> /dev/null; then
        print_error "Python3 is required"
        exit 1
    fi
    
    # Check if kernel receiver is built
    if [ ! -f "kernel_receiver" ]; then
        print_info "Building kernel receiver..."
        make
    fi
    
    print_info "All prerequisites met"
}

run_single_feed_kernel() {
    print_header "Scenario 1: Single Feed - Kernel Driver"
    
    print_info "Starting kernel receiver..."
    ./kernel_receiver $MULTICAST_GROUP $PORT > kernel_single.log 2>&1 &
    RECEIVER_PID=$!
    sleep 2
    
    print_info "Starting feed simulator (rate: $RATE msg/sec, duration: ${DURATION}s)..."
    python3 feed_simulator.py -g $MULTICAST_GROUP -p $PORT -r $RATE -d $DURATION
    
    sleep 2
    print_info "Stopping receiver..."
    kill -INT $RECEIVER_PID 2>/dev/null || true
    wait $RECEIVER_PID 2>/dev/null || true
    
    print_info "Results saved to kernel_single.log"
    tail -10 kernel_single.log
}

run_single_feed_dpdk() {
    print_header "Scenario 1: Single Feed - DPDK"
    
    print_info "NOTE: DPDK receiver requires separate setup"
    print_info "Please run the DPDK receiver manually:"
    echo "  sudo ../dpdk_example/udp_multicast_receiver -l 1 -n 4"
    echo ""
    print_info "Press Enter when DPDK receiver is ready, or Ctrl+C to skip..."
    read -r
    
    print_info "Starting feed simulator..."
    python3 feed_simulator.py -g $MULTICAST_GROUP -p $PORT -r $RATE -d $DURATION
    
    print_info "DPDK results available in DPDK receiver output"
}

run_multiple_feeds_kernel() {
    print_header "Scenario 2: Multiple Feeds - Kernel Driver"
    
    COMBINED_RATE=$((RATE * 4))
    
    # Start receivers for 4 feeds
    for i in 1 2 3 4; do
        GROUP="239.1.1.$i"
        print_info "Starting receiver for feed $i (${GROUP})..."
        ./kernel_receiver $GROUP $PORT > kernel_multi_feed${i}.log 2>&1 &
        eval "RECEIVER${i}_PID=$!"
    done
    
    sleep 2
    
    # Start simulators for 4 feeds
    print_info "Starting feed simulators (combined rate: $COMBINED_RATE msg/sec)..."
    for i in 1 2 3 4; do
        GROUP="239.1.1.$i"
        SYMBOL=$([ $i -eq 1 ] && echo "AAPL" || [ $i -eq 2 ] && echo "MSFT" || [ $i -eq 3 ] && echo "GOOGL" || echo "AMZN")
        python3 feed_simulator.py -g $GROUP -p $PORT -s $SYMBOL -r $RATE -d $DURATION &
        eval "SIMULATOR${i}_PID=$!"
    done
    
    # Wait for simulators to finish
    wait $SIMULATOR1_PID $SIMULATOR2_PID $SIMULATOR3_PID $SIMULATOR4_PID 2>/dev/null || true
    
    sleep 2
    
    # Stop receivers
    print_info "Stopping receivers..."
    for i in 1 2 3 4; do
        eval "PID=\$RECEIVER${i}_PID"
        kill -INT $PID 2>/dev/null || true
        wait $PID 2>/dev/null || true
    done
    
    print_info "Results saved to kernel_multi_feed*.log"
    for i in 1 2 3 4; do
        echo -e "\n${YELLOW}Feed $i:${NC}"
        tail -5 kernel_multi_feed${i}.log
    done
}

run_multiple_feeds_dpdk() {
    print_header "Scenario 2: Multiple Feeds - DPDK"
    
    print_info "NOTE: For multiple feeds with DPDK, configure RSS (Receive Side Scaling)"
    print_info "or run multiple DPDK instances with different core assignments"
    echo ""
    print_info "Example multi-queue setup not automated in this script"
    print_info "See DPDK RSS documentation for advanced configuration"
}

cleanup() {
    print_info "Cleaning up any remaining processes..."
    pkill -f feed_simulator.py 2>/dev/null || true
    pkill -f kernel_receiver 2>/dev/null || true
}

trap cleanup EXIT

main() {
    print_header "HFT Order Book Feed Benchmark Suite"
    
    check_prerequisites
    
    echo ""
    echo "Select benchmark to run:"
    echo "  1) Single Feed - Kernel Driver"
    echo "  2) Single Feed - DPDK (manual)"
    echo "  3) Multiple Feeds - Kernel Driver"
    echo "  4) Multiple Feeds - DPDK (manual)"
    echo "  5) Run all automated tests (1 & 3)"
    echo "  q) Quit"
    echo ""
    read -p "Enter choice: " choice
    
    case $choice in
        1)
            run_single_feed_kernel
            ;;
        2)
            run_single_feed_dpdk
            ;;
        3)
            run_multiple_feeds_kernel
            ;;
        4)
            run_multiple_feeds_dpdk
            ;;
        5)
            run_single_feed_kernel
            run_multiple_feeds_kernel
            ;;
        q|Q)
            print_info "Exiting..."
            exit 0
            ;;
        *)
            print_error "Invalid choice"
            exit 1
            ;;
    esac
    
    print_header "Benchmark Complete"
}

main

