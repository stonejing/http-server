import socket

def udp_client(host, port):
    # Create a UDP socket
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as client_socket:
        while True:
            # Get user input for the message
            message = input("Enter message (or 'exit' to quit): ")

            # Check if the user wants to exit
            if message.lower() == 'exit':
                break

            # Send the message to the server
            client_socket.sendto(message.encode('utf-8'), (host, port))
            print(f"Sent message to {host}:{port}")

            # Receive the echoed message from the server
            data, server_address = client_socket.recvfrom(1024)
            print(f"Received echoed message from {server_address}: {data.decode('utf-8')}")

if __name__ == "__main__":
    # Set the host and port of the UDP server
    server_host = "64.110.111.81"
    server_port = 8008

    udp_client(server_host, server_port)
