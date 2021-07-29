build: server.cpp tcp_client.cpp
	g++ server.cpp -o server
	g++ tcp_client.cpp -o tcp_client
server: server.cpp
	g++ server.cpp -o server
tcp_client: tcp_client.cpp
	g++ tcp_client.cpp -o tcp_client
clean: server tcp_client
	rm server tcp_client
