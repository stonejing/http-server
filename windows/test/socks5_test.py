import requests

proxies = {
    'http': 'socks5://127.0.0.1:6000',
    'https': 'socks5://127.0.0.1:6000'
}

print(requests.get("http://127.0.0.1:5150/").content)

# print(requests.get('https://www.google.com', proxies=proxies).content)

