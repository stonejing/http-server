.PHONY: all select_test client_test event_select socks5_select
all:
	g++ block_echo_client.cpp -o client -lws2_32
	g++ block_echo_server.cpp -o server -lws2_32
select_test:
	g++ -o select_test -C select_test.cpp -lws2_32 -lwsock32
client_test:
	g++ -o client_test -C client_test.cpp -lws2_32
event_select:
	g++ -o event_select -C WSAEventSelect_tcp_server.cpp -lws2_32 
socks5_select:
	g++ -o socks5_select -C socks5_select.cpp -lws2_32
memory_test:
	g++ -o memory_test -C memory_test.cpp
clean:
	rm -f select_test.exe
	rm -f client_test.exe

# blocking_client_thread:
# !include <win32.mak>

# objs=bclient.obj resolve.obj
# all: bclient.exe

# .cpp.obj:
# 	$(cc) $(cdebug) $(cflags) $(cvarsmt) $*.cpp

# bclient.exe: $(objs) $(common_objs)
# 	$(link) $(linkdebug) $(conlflags) -out:bclient.exe $(objs) $(conlibsmt) ws2_32.lib

# clean:
# 	del *.obj
# 	del *.exe
# 	del *.pdb