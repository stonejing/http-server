support_cipher = {}

OpenSSLStreamCrypto = 1
OpenSSLAeadCrypto = 2


ciphers_1 = {
    'aes-128-cfb': (16, 16, OpenSSLStreamCrypto),
    'aes-192-cfb': (24, 16, OpenSSLStreamCrypto),
    'aes-256-cfb': (32, 16, OpenSSLStreamCrypto),
    'aes-128-ofb': (16, 16, OpenSSLStreamCrypto),
    'aes-192-ofb': (24, 16, OpenSSLStreamCrypto),
    'aes-256-ofb': (32, 16, OpenSSLStreamCrypto),
    'aes-128-ctr': (16, 16, OpenSSLStreamCrypto),
    'aes-192-ctr': (24, 16, OpenSSLStreamCrypto),
    'aes-256-ctr': (32, 16, OpenSSLStreamCrypto),
    'aes-128-cfb8': (16, 16, OpenSSLStreamCrypto),
    'aes-192-cfb8': (24, 16, OpenSSLStreamCrypto),
    'aes-256-cfb8': (32, 16, OpenSSLStreamCrypto),
    'aes-128-cfb1': (16, 16, OpenSSLStreamCrypto),
    'aes-192-cfb1': (24, 16, OpenSSLStreamCrypto),
    'aes-256-cfb1': (32, 16, OpenSSLStreamCrypto),
    'bf-cfb': (16, 8, OpenSSLStreamCrypto),
    'camellia-128-cfb': (16, 16, OpenSSLStreamCrypto),
    'camellia-192-cfb': (24, 16, OpenSSLStreamCrypto),
    'camellia-256-cfb': (32, 16, OpenSSLStreamCrypto),
    'cast5-cfb': (16, 8, OpenSSLStreamCrypto),
    'idea-cfb': (16, 8, OpenSSLStreamCrypto),
    'rc2-cfb': (16, 8, OpenSSLStreamCrypto),
    'des-cfb': (8, 8, OpenSSLStreamCrypto),
    'rc4': (16, 0, OpenSSLStreamCrypto),
    'seed-cfb': (16, 16, OpenSSLStreamCrypto),
    # AEAD: iv_len = salt_len = key_len
    'aes-128-gcm': (16, 16, OpenSSLAeadCrypto),
    'aes-192-gcm': (24, 24, OpenSSLAeadCrypto),
    'aes-256-gcm': (32, 32, OpenSSLAeadCrypto),
    'aes-128-ocb': (16, 16, OpenSSLAeadCrypto),
    'aes-192-ocb': (24, 24, OpenSSLAeadCrypto),
    'aes-256-ocb': (32, 32, OpenSSLAeadCrypto),
}



