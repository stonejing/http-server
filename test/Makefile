all:
	g++ -o test -C thread_local_none.cpp -pthread
tcpserver:
	g++ tcpserver.cpp -o tcpserver.exe -lws2_32

tcpclient:
	g++ tcpclient.cpp -o tcpclient.exe -lws2_32

selectmode:
	g++ selectmode.cpp -o selectmode.exe -lws2_32
selectmodel:
	g++ selectmodel.cpp -o selectmodel.exe -lws2_32
	./selectmodel.exe