.PHONY: all

all:
	g++ -o straydog.exe main.cpp straydog.cpp proxyserver.cpp shadowsocks.cpp crypto.cpp \
					eventloop.cpp \
		-lws2_32 \
		-L"D:/Program\ Files (x86)\msys64\mingw64\lib" -lcrypto -lssl \
		-pthread
	straydog.exe