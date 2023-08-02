.PHONY: epoll_test libev_test select_test socks5_select tcp_udp_echo test_openssl test
epoll_test:
	g++ -o epoll_test -C epoll_http_server.cpp
libev_test:
	g++ -o libev_test -C libev_test.cpp -lev
socket_select_test:
	g++ -o socket_select_test -C socket_select_test.cpp
select_test:
	g++ -o select_test -C select_test.cpp
socks5_select:
	g++ -o socks5_select -C socks5_select.cpp -pthread -lssl -lcrypto -lmbedcrypto -lmbedtls
tcp_udp_echo:
	g++ -o tcp_udp_echo -C tcp_udp_echo.cpp
test_openssl:
	g++ -o test_openssl -C test_openssl.cpp -lssl -lcrypto -lmbedtls -lmbedcrypto
test:
	g++ -o test -C test.cpp -lssl -lcrypto -lmbedtls
clean:
	rm -f epoll_test
	rm -f http_parser
	rm -f libev_test
	rm -f select_test
	rm -f socket_select_test
	rm -f socks5_select_test
	rm -f tcp_udp_echo
	rm -f test
	rm -f test_openssl