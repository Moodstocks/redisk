#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#define REDIS_SET_EXAMPLE "*3\r\n$3\r\nSET\r\n$5\r\nmykey\r\n$7\r\nmyvalue\r\n"
#define RECVBUF 1024


struct hostent *he;
struct sockaddr_in server;
int sockfd, opt=1;

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "USAGE: redis-cli-test PORT\n");
    exit(1);
  }
  int port = atoi(argv[1]);
  he = gethostbyname("localhost");
  memcpy(&server.sin_addr, he->h_addr_list[0], he->h_length);
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
  connect(sockfd, (struct sockaddr *)&server, sizeof(server));
  send(sockfd, REDIS_SET_EXAMPLE, strlen(REDIS_SET_EXAMPLE), 0);
  char buf[RECVBUF];
  recv(sockfd,buf,RECVBUF,0);
  printf("%s\n",buf);
  int i;
  for (i=0;i<4;++i) buf[i] = 'f';
  sleep(1);
  send(sockfd, REDIS_SET_EXAMPLE, strlen(REDIS_SET_EXAMPLE), 0);
  recv(sockfd,buf,RECVBUF,0);
  printf("%s\n",buf);
  close(sockfd);
  return 0;
}

