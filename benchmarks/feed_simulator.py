#!/usr/bin/env python3
"""
UDP Multicast Feed Simulator for HFT Benchmarking

Simulates market data order book updates sent via UDP multicast.
"""

import socket
import struct
import time
import argparse
from dataclasses import dataclass
from typing import List

@dataclass
class OrderBookUpdate:
    """Represents a single order book update message"""
    timestamp: int  # nanoseconds since epoch
    symbol: str     # Stock symbol (4 chars)
    bid_price: float
    bid_size: int
    ask_price: float
    ask_size: int
    sequence: int

def create_order_book_message(update: OrderBookUpdate) -> bytes:
    """
    Create a binary order book update message
    
    Format (32 bytes):
    - timestamp: 8 bytes (uint64)
    - symbol: 4 bytes (4 chars)
    - bid_price: 4 bytes (float)
    - bid_size: 4 bytes (uint32)
    - ask_price: 4 bytes (float)
    - ask_size: 4 bytes (uint32)
    - sequence: 4 bytes (uint32)
    """
    symbol_bytes = update.symbol.encode('ascii')[:4].ljust(4, b'\x00')
    
    message = struct.pack(
        '!Q4sIIII',
        update.timestamp,
        symbol_bytes,
        int(update.bid_price * 100),  # Price in cents
        update.bid_size,
        int(update.ask_price * 100),
        update.ask_size,
        update.sequence
    )
    
    return message

class FeedSimulator:
    """Simulates UDP multicast market data feed"""
    
    def __init__(self, multicast_group: str, port: int, ttl: int = 2):
        self.multicast_group = multicast_group
        self.port = port
        self.ttl = ttl
        
        # Create UDP socket
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        self.sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, ttl)
        
        print(f"Feed simulator initialized: {multicast_group}:{port}")
    
    def send_update(self, update: OrderBookUpdate):
        """Send a single order book update"""
        message = create_order_book_message(update)
        self.sock.sendto(message, (self.multicast_group, self.port))
    
    def run_single_feed(self, symbol: str, rate: int, duration: int):
        """
        Run single feed simulation
        
        Args:
            symbol: Stock symbol
            rate: Messages per second
            duration: Duration in seconds
        """
        interval = 1.0 / rate
        sequence = 0
        start_time = time.time()
        
        print(f"\nStarting single feed: {symbol}")
        print(f"Rate: {rate} msg/sec, Duration: {duration} sec")
        print(f"Target: {multicast_group}:{self.port}")
        print("Press Ctrl+C to stop\n")
        
        try:
            while time.time() - start_time < duration:
                # Create order book update with varying prices
                timestamp_ns = int(time.time() * 1_000_000_000)
                base_price = 100.0 + (sequence % 100) * 0.01
                
                update = OrderBookUpdate(
                    timestamp=timestamp_ns,
                    symbol=symbol,
                    bid_price=base_price - 0.05,
                    bid_size=1000 + (sequence % 500),
                    ask_price=base_price + 0.05,
                    ask_size=1000 + ((sequence + 1) % 500),
                    sequence=sequence
                )
                
                self.send_update(update)
                sequence += 1
                
                # Display progress
                if sequence % rate == 0:
                    elapsed = time.time() - start_time
                    print(f"Sent {sequence} messages in {elapsed:.1f}s "
                          f"({sequence/elapsed:.0f} msg/sec)")
                
                # Sleep to maintain rate
                time.sleep(interval)
                
        except KeyboardInterrupt:
            print("\nStopping feed simulator...")
        
        elapsed = time.time() - start_time
        print(f"\nFinal stats: {sequence} messages in {elapsed:.1f}s "
              f"(avg: {sequence/elapsed:.0f} msg/sec)")
    
    def run_multiple_feeds(self, symbols: List[str], rate_per_feed: int, duration: int):
        """
        Run multiple feeds simulation (requires multiple simulator instances)
        
        This method demonstrates the concept. For true multiple feeds,
        run multiple instances of this script with different multicast groups.
        """
        print("For multiple feeds, run multiple instances of this script:")
        for i, symbol in enumerate(symbols):
            group = f"239.1.1.{i+1}"
            print(f"  python3 feed_simulator.py -g {group} -p {self.port} "
                  f"-s {symbol} -r {rate_per_feed} -d {duration}")

def main():
    parser = argparse.ArgumentParser(
        description='UDP Multicast Feed Simulator for HFT Benchmarking'
    )
    parser.add_argument('-g', '--group', default='239.1.1.1',
                       help='Multicast group address (default: 239.1.1.1)')
    parser.add_argument('-p', '--port', type=int, default=12345,
                       help='UDP port (default: 12345)')
    parser.add_argument('-s', '--symbol', default='AAPL',
                       help='Stock symbol (default: AAPL)')
    parser.add_argument('-r', '--rate', type=int, default=10000,
                       help='Messages per second (default: 10000)')
    parser.add_argument('-d', '--duration', type=int, default=60,
                       help='Duration in seconds (default: 60)')
    parser.add_argument('-m', '--multiple', action='store_true',
                       help='Show instructions for multiple feeds')
    
    args = parser.parse_args()
    
    simulator = FeedSimulator(args.group, args.port)
    
    if args.multiple:
        simulator.run_multiple_feeds(['AAPL', 'MSFT', 'GOOGL', 'AMZN'], 
                                     args.rate, args.duration)
    else:
        simulator.run_single_feed(args.symbol, args.rate, args.duration)

if __name__ == '__main__':
    main()

