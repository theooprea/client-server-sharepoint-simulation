#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"
#include <netinet/tcp.h>
#include <iostream>
#include <string>

using namespace std;

void usage(char *file) {
	fprintf(stderr, "Usage: %s server_address server_port\n", file);
	exit(0);
}

int main(int argc, char **argv) {
    int sockfd, n, ret;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];
    char httpbuffer[HTTPBUFLEN];
    char aux[BUFLEN];

    fd_set read_fds;
    fd_set tmp_read_fds;

    FD_ZERO(&read_fds);
	FD_ZERO(&tmp_read_fds);

    if (argc < 3) {
		usage(argv[0]);
	}

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "socket");

	int enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1) {
        perror("setsocketopt");
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[2]));
	ret = inet_aton(argv[1], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

    ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

    FD_SET(STDIN_FILENO, &read_fds);
	FD_SET(sockfd, &read_fds);

	int max_fd = STDIN_FILENO > sockfd ? STDIN_FILENO + 1 : sockfd + 1;

    while (true) {
        tmp_read_fds = read_fds;
        int rc = select(max_fd, &tmp_read_fds, NULL, NULL, NULL);
        DIE(rc < 0, "select");

        if (FD_ISSET(STDIN_FILENO, &tmp_read_fds)) {
            memset(buffer, 0, BUFLEN);
            memset(httpbuffer, 0, HTTPBUFLEN);
			fgets(buffer, BUFLEN, stdin);
            buffer[strlen(buffer) - 1] = '\0';

            if (strcmp(buffer, "quit") == 0) {
                break;
            }

            if (strncmp(buffer, "get", 3) == 0) {
                memcpy(aux, buffer, BUFLEN);
                char *p = strtok(aux, " \n");
                char *nume_resursa = strtok(NULL, " \n");

                if (nume_resursa == NULL) {
                    printf("No nume_resursa\n");
                    continue;
                }

                sprintf(httpbuffer, "GET %s HTTP/1.1\r\n\r\n", nume_resursa);

                n = send(sockfd, httpbuffer, strlen(httpbuffer), 0);
			    DIE(n < 0, "send");
            }

            else if (strncmp(buffer, "login", 5) == 0) {
                memcpy(aux, buffer, BUFLEN);
                char *p = strtok(aux, " \n");
                char *user = strtok(NULL, " \n");

                if (user == NULL) {
                    printf("No user\n");
                    continue;
                }

                char *password = strtok(NULL, " \n");

                if (password == NULL) {
                    printf("No password\n");
                    continue;
                }

                sprintf(httpbuffer, "POST / HTTP/1.1\r\n\r\nuser=%s&password=%s", user, password);

                n = send(sockfd, httpbuffer, strlen(httpbuffer), 0);
			    DIE(n < 0, "send");
            }

            else if (strncmp(buffer, "put", 3) == 0) {
                memcpy(aux, buffer, BUFLEN);
                char *p = strtok(aux, " \n");
                char *nume_resursa = strtok(NULL, " \n");

                if (nume_resursa == NULL) {
                    printf("No nume_resursa\n");
                    continue;
                }

                char *continut_resursa = strtok(NULL, " \n");

                if (continut_resursa == NULL) {
                    printf("No continut_resursa\n");
                    continue;
                }

                sprintf(httpbuffer, "PUT %s HTTP/1.1\r\n\r\n%s", nume_resursa, continut_resursa);

                n = send(sockfd, httpbuffer, strlen(httpbuffer), 0);
			    DIE(n < 0, "send");
            }

            else if (strncmp(buffer, "delete", 6) == 0) {
                memcpy(aux, buffer, BUFLEN);
                char *p = strtok(aux, " \n");
                char *nume_resursa = strtok(NULL, " \n");

                if (nume_resursa == NULL) {
                    printf("No nume_resursa\n");
                    continue;
                }

                sprintf(httpbuffer, "DELETE %s HTTP/1.1\r\n\r\n", nume_resursa);

                n = send(sockfd, httpbuffer, strlen(httpbuffer), 0);
			    DIE(n < 0, "send");
            }

            else {
                printf("Invalid command\n");
            }
        }

        if (FD_ISSET(sockfd, &tmp_read_fds)) {
            memset(httpbuffer, 0, HTTPBUFLEN);

            n = recv(sockfd, httpbuffer, HTTPBUFLEN, 0);
			DIE(n < 0, "recv");
            
            if (n == 0) {
                break;
            }

            cout << httpbuffer << endl;
        }
    }

    close(sockfd);
    return 0;
}