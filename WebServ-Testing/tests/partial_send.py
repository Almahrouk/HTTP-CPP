#!/usr/bin/env python3
import socket
import time

def test_partial_write(host='localhost', port=8080):
    # Create socket with tiny receive buffer
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 1024)
    sock.connect((host, port))
    
    # Send request for large response
    sock.send(b"GET /index.html HTTP/1.1\r\nHost: localhost:8080\r\n\r\n")
    
    # Read VERY slowly to force partial writes on server side
    time.sleep(0.1)  # Small delay to start
    
    # Read byte by byte with delays
    total_received = 0
    while True:
        data = sock.recv(1)  # Read 1 byte at a time
        if not data:
            break
        total_received += 1
        if total_received % 100 == 0:
            time.sleep(0.05)  # Add delay every 100 bytes
    
    sock.close()

if __name__ == "__main__":
    test_partial_write()
