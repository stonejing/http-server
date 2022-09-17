.PHONY: thread_test	pointer

thread_test:
	g++ -o thread_test -C thread_test.cpp
openssl_test:
	g++ -o openssl_test -C openssl_test.cpp \ 
		-LD:/Program\ Files\ (x86)\msys64\mingw64\lib -lcrypto -lssl

pointer:
	g++ -o pointer_test pointer_test.cpp