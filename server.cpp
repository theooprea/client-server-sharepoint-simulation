#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "helpers.h"
#include <netinet/tcp.h>
#include <iostream>
#include <string>
#include <queue>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <bits/stdc++.h>

using namespace std;

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

int main(int argc, char **argv) {
    int sockfd_tcp, sockfd_udp, newsockfd, portno;
	char buffer[BUFLEN], httpbuffer[HTTPBUFLEN], httpresponsebuffer[HTTPBUFLEN];
	struct sockaddr_in serv_addr_tcp, cli_addr;
	int n, ret;
	socklen_t clilen;

    unordered_set<int> connected_clients;
    unordered_set<string> files;
    unordered_set<int> authorized;

    fd_set read_fds;
	fd_set tmp_fds;
	int fdmax;

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

    if (argc < 2) {
        usage(argv[0]);
    }
    
    portno = atoi(argv[1]);
    DIE(portno == 0, "port");

    sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd_tcp < 0, "socket_tcp");

    memset((char*) &serv_addr_tcp, 0, sizeof(serv_addr_tcp));
    serv_addr_tcp.sin_family = AF_INET;
    serv_addr_tcp.sin_port = htons(portno);
    serv_addr_tcp.sin_addr.s_addr = INADDR_ANY;

    int enable_tcp = 1;
    if (setsockopt(sockfd_tcp, SOL_SOCKET, SO_REUSEADDR, &enable_tcp, sizeof(int)) == -1) {
        perror("setsocketopt");
        exit(1);
    }

    ret = bind(sockfd_tcp, (struct sockaddr *) &serv_addr_tcp, sizeof(struct sockaddr));
    DIE(ret < 0, "bind");

    ret = listen(sockfd_tcp, MAX_CLIENTS);
    DIE(ret < 0, "listen");

    FD_SET(STDIN_FILENO, &read_fds);
    FD_SET(sockfd_tcp, &read_fds);
    fdmax = sockfd_tcp;

    while(true) {
        tmp_fds = read_fds;

        ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
        DIE(ret < 0, "select\n");

        for (int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &tmp_fds)) {
                // Reading from STDIN
                if (i == 0) {
                    memset(buffer, 0, BUFLEN);
                    fgets(buffer, BUFLEN, stdin);
                    buffer[strlen(buffer) - 1] = '\0';

                    if (strcmp(buffer, "quit") == 0) {
                        for (auto socket_user : connected_clients) {
                            close(socket_user);
                        }
                        close(sockfd_tcp);
                        return 0;
                    }

                    if (strcmp(buffer, "status") == 0) {
                        cout << connected_clients.size() << " connected users" << endl;
                        cout << "Files:" << endl;
                        for (auto fisier : files) {
                            cout << fisier << endl;
                        }
                    }
                }
                // TCP connection
                else if (i == sockfd_tcp) {
                    clilen = sizeof(cli_addr);
					newsockfd = accept(sockfd_tcp, (struct sockaddr *) &cli_addr, &clilen);
					DIE(newsockfd < 0, "tcp accept\n");

                    connected_clients.insert(newsockfd);

                    FD_SET(newsockfd, &read_fds);
					if (newsockfd > fdmax) { 
						fdmax = newsockfd;
					}

                    printf("Noua conexiune de la %s, port %d, socket client %d\n",
							inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), newsockfd);

                }
                // Messages recieved on the TCP socket
                else {
                    memset(httpbuffer, 0, HTTPBUFLEN);
                    n = recv(i, httpbuffer, HTTPBUFLEN, 0);
					DIE(n < 0, "recv");

					if (n == 0) {
						printf("Socket-ul client %d a inchis conexiunea\n", i);
						close(i);
						
						FD_CLR(i, &read_fds);
                        connected_clients.erase(i);
                        authorized.erase(i);
					} else {
						if (strncmp(httpbuffer, "GET", 3) == 0) {
                            printf("GET Request\n");
                            string command = httpbuffer;

                            int pozinit = 4;
                            int pozfinal = command.find("HTTP/1.1");

                            string nume_resursa = command.substr(pozinit, pozfinal - pozinit - 1);

                            if (files.find(nume_resursa) != files.end()) {
                                ifstream fin(nume_resursa);

                                string continut;
                                fin >> continut;

                                memset(httpresponsebuffer, 0, HTTPBUFLEN);
                                sprintf(httpresponsebuffer, "HTTP/1.1 200 OK\r\n\r\n%s", continut.c_str());

                                n = send(i, httpresponsebuffer, strlen(httpresponsebuffer), 0);
			                    DIE(n < 0, "send");
                            }
                            else {
                                memset(httpresponsebuffer, 0, HTTPBUFLEN);
                                sprintf(httpresponsebuffer, "HTTP/1.1 404 Not Found\r\n\r\n");

                                n = send(i, httpresponsebuffer, strlen(httpresponsebuffer), 0);
			                    DIE(n < 0, "send");
                            }
                        }
                        else if (strncmp(httpbuffer, "POST", 4) == 0) {
                            printf("POST Request\n");
                            string command = httpbuffer;

                            int pozuser = command.find("user=");

                            int pozpassword = command.find("password=");

                            string user = command.substr(pozuser + 5, pozpassword - pozuser - 5 - 1);
                            string password = command.substr(pozpassword + 9, command.size() - pozpassword - 9);

                            if (user.compare("salut") == 0 && password.compare("scuze") == 0) {
                                memset(httpresponsebuffer, 0, HTTPBUFLEN);
                                sprintf(httpresponsebuffer, "HTTP/1.1 200 OK\r\n\r\n");

                                authorized.insert(i);

                                n = send(i, httpresponsebuffer, strlen(httpresponsebuffer), 0);
			                    DIE(n < 0, "send");
                            }
                            else {
                                memset(httpresponsebuffer, 0, HTTPBUFLEN);
                                sprintf(httpresponsebuffer, "HTTP/1.1 401 Unauthorized\r\n\r\n");

                                n = send(i, httpresponsebuffer, strlen(httpresponsebuffer), 0);
			                    DIE(n < 0, "send");
                            }
                        }
                        else if (strncmp(httpbuffer, "PUT", 3) == 0) {
                            printf("PUT Request\n");

                            if (authorized.find(i) != authorized.end()) {
                                string command = httpbuffer;

                                int pozinit = 4;
                                int pozfinal = command.find("HTTP/1.1\r\n\r\n");

                                string nume_resursa = command.substr(pozinit, pozfinal - pozinit - 1);
                                string continut_resursa = command.substr(pozfinal + 12, command.size() - pozfinal - 12);

                                files.insert(nume_resursa);

                                ofstream fout(nume_resursa);
                                fout << continut_resursa << endl;
                                fout.close();

                                memset(httpresponsebuffer, 0, HTTPBUFLEN);
                                sprintf(httpresponsebuffer, "HTTP/1.1 200 OK\r\n\r\n");

                                n = send(i, httpresponsebuffer, strlen(httpresponsebuffer), 0);
                                DIE(n < 0, "send");
                            }
                            else {
                                memset(httpresponsebuffer, 0, HTTPBUFLEN);
                                sprintf(httpresponsebuffer, "HTTP/1.1 401 Unauthorized\r\n\r\n");

                                n = send(i, httpresponsebuffer, strlen(httpresponsebuffer), 0);
			                    DIE(n < 0, "send");
                            }
                        }
                        else if (strncmp(httpbuffer, "DELETE", 6) == 0) {
                            printf("DELETE Request\n");

                            if (authorized.find(i) != authorized.end()) {
                                string command = httpbuffer;

                                int pozinit = 7;
                                int pozfinal = command.find("HTTP/1.1");

                                string nume_resursa = command.substr(pozinit, pozfinal - pozinit - 1);

                                if (files.find(nume_resursa) != files.end()) {
                                    remove(nume_resursa.c_str());

                                    files.erase(nume_resursa);

                                    memset(httpresponsebuffer, 0, HTTPBUFLEN);
                                    sprintf(httpresponsebuffer, "HTTP/1.1 200 OK\r\n\r\n");

                                    n = send(i, httpresponsebuffer, strlen(httpresponsebuffer), 0);
                                    DIE(n < 0, "send");
                                }
                                else {
                                    memset(httpresponsebuffer, 0, HTTPBUFLEN);
                                    sprintf(httpresponsebuffer, "HTTP/1.1 404 Not Found\r\n\r\n");

                                    n = send(i, httpresponsebuffer, strlen(httpresponsebuffer), 0);
                                    DIE(n < 0, "send");
                                }
                            }
                            else {
                                memset(httpresponsebuffer, 0, HTTPBUFLEN);
                                sprintf(httpresponsebuffer, "HTTP/1.1 401 Unauthorized\r\n\r\n");

                                n = send(i, httpresponsebuffer, strlen(httpresponsebuffer), 0);
			                    DIE(n < 0, "send");
                            }
                        }
                        else {
                            continue;
                        }
					}
                }
            }
        }
    }

    return 0;
}