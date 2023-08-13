import socket
import base64
import threading

def handle_client(client_socket, target_host, target_port, username, password):
    request = client_socket.recv(4096).decode()
    first_line = request.split('\n')[0]
    url = first_line.split(' ')[1]

    if not url.startswith("http://"):
        url = f"http://{url}"

    http_pos = url.find("://")
    if http_pos == -1:
        raise ValueError("Invalid URL")

    url = url[(http_pos + 3):]
    port_pos = url.find(":")

    webserver_pos = url.find("/")
    if webserver_pos == -1:
        webserver_pos = len(url)

    webserver = ""
    port = -1

    if port_pos == -1 or webserver_pos < port_pos:
        port = 80
        webserver = url[:webserver_pos]
    else:
        port = int((url[(port_pos + 1):])[:webserver_pos - port_pos - 1])
        webserver = url[:port_pos]

    auth_header = f"Proxy-Authorization: Basic {base64.b64encode(f'{username}:{password}'.encode()).decode()}\r\n"
    if auth_header not in request:
        client_socket.send("HTTP/1.1 407 Proxy Authentication Required\r\nProxy-Authenticate: Basic realm=\"Proxy\"\r\n\r\n".encode())
        client_socket.close()
        return

    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.connect((webserver, port))
    print(request)
    server_socket.send(request.encode())

    while True:
        response = server_socket.recv(4096)
        print(response)
        if not response:
            break
        client_socket.send(response)

    server_socket.close()
    client_socket.close()

def proxy_server(proxy_port, target_host, target_port, username, password):
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind(("0.0.0.0", proxy_port))
    server_socket.listen(5)

    print(f"Proxy server listening on port {proxy_port}...")

    while True:
        client_socket, addr = server_socket.accept()
        print(f"Accepted connection from {addr[0]}:{addr[1]}")
        client_thread = threading.Thread(target=handle_client, args=(client_socket, target_host, target_port, username, password))
        client_thread.start()

def main():
    proxy_port = 8000
    target_host = "www.example.com"
    target_port = 80
    username = "stonejing"
    password = "stonejing"

    proxy_server(proxy_port, target_host, target_port, username, password)

if __name__ == "__main__":
    # python -m http.server 3333
    main()
