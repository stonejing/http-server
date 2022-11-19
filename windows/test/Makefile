.PHONY: thread_test	pointer

thread_test:
	g++ -o thread_test -C thread_test.cpp
openssl_test:
	g++ -o openssl_test -C openssl_test.cpp \ 
		-LD:/Program\ Files\ (x86)\msys64\mingw64\lib -lcrypto -lssl

st:
	g++ -o st select_test_2.cpp -lws2_32 
	st.exe

pointer:
	g++ -o pointer_test pointer_test.cpp