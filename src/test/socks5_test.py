import requests

proxies = {
    'http': 'socks5://127.0.0.1:5005',
    'https': 'socks5://127.0.0.1:5005'
}

print(requests.get('https://www.google.com', proxies=proxies).content)

