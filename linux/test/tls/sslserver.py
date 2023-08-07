import socket
import ssl

def create_ssl_context(certfile, keyfile):
    context = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
    context.load_cert_chain(certfile, keyfile)
    return context

def handle_client(client_socket):
    request = client_socket.recv(1024)
    print(f"Received request: {request.decode()}")

    response = b"HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, world!"
    client_socket.sendall(response)
    client_socket.close()

def main():
    HOST = '127.0.0.1'
    PORT = 12345
    CERTFILE = 'server.crt'
    KEYFILE = 'server.key'

    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind((HOST, PORT))
    server_socket.listen(5)

    ssl_context = create_ssl_context(CERTFILE, KEYFILE)

    print(f"Server is listening on {HOST}:{PORT}...")
    while True:
        client_socket, client_addr = server_socket.accept()
        ssl_socket = ssl_context.wrap_socket(client_socket, server_side=True)

        print(f"Accepted connection from {client_addr[0]}:{client_addr[1]}")
        handle_client(ssl_socket)

if __name__ == "__main__":
    main()
