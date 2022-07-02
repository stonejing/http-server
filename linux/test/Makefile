.PHONY: openssl_test signal_test
openssl_test:
	g++ -o openssl_test -C openssl_test.cpp -lcrypto -lssl
signal_test:
	g++ -o signal_test -C signal_test.cpp
clean:
	rm -f openssl_test
	rm -f signal_test