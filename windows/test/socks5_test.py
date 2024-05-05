import requests
import threading

proxies = {
    'http': 'socks5://127.0.0.1:6000',
    'https': 'socks5://127.0.0.1:6000'
}

def loop():
    for i in range(500):
        # print(requests.get("http://127.0.0.1:11380").content)
        requests.get("http://127.0.0.1:11380")

# a = []

# for i in range(10):
#     t = threading.Thread(target=loop)
#     t.start()
#     a.append(t)

# for i in range(10):
#     a[i].join()

print("loop over.")
# print(requests.get("http://127.0.0.1:5150/").content)
# for i in range(10):
#     print(requests.get("http://127.0.0.1:11380").content)

# print(requests.get('https://www.google.com', proxies=proxies).content)

# import socket

# # connect to 192.168.88.59:9998

# s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
# s.connect(('192.168.88.59', 9998))
# s.send(b'hello')
# print(s.recv(1024))
# s.close()
import requests

url = "http://217.142.236.86:8008"
urlg = "https://www.google.com"
res = requests.get(url)
print(res.content)