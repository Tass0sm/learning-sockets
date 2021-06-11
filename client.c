#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define PORT "3490"
/* max number of bytes we can get at once */
#define MAXDATASIZE 100

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*) sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

int connect_to_server(char *link, char *port)
{
  int socket_fd, status;
  struct addrinfo address_hints;
  struct addrinfo *address_results, *p;  // will point to the results
  char s[INET6_ADDRSTRLEN];

  memset(&address_hints, 0, sizeof address_hints); // make sure the struct is empty
  address_hints.ai_family = AF_UNSPEC;             // don't care IPv4 or IPv6
  address_hints.ai_socktype = SOCK_STREAM;         // TCP stream sockets

  status = getaddrinfo(link, port, &address_hints, &address_results);

  if (status != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    return -1;
  }

  // loop through all the results and connect to the first we can
  for (p = address_results; p != NULL; p = p->ai_next) {
    socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

    if (socket_fd == -1) {
      perror("client: socket");
      continue;
    }

    if (connect(socket_fd, p->ai_addr, p->ai_addrlen) == -1) {
      close(socket_fd);
      perror("client: connect");
      continue;
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "client: failed to connect\n");
    return -1;
  }

  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *) p->ai_addr), s, sizeof s);
  printf("client: connecting to %s\n", s);

  /* all done with this structure */
  freeaddrinfo(address_results);

  return socket_fd;
}

int main(int argc, char *argv[])
{
  int socket_fd, bytes_read;
  char buf[MAXDATASIZE];

  socket_fd = connect_to_server("127.0.0.1", "3490");

  if (socket_fd == -1) {
    perror("connect_to_server");
    return 1;
  }


  bytes_read = recv(socket_fd, buf, MAXDATASIZE - 1, 0);

  if (bytes_read == -1) {
    perror("recv");
    return 1;
  }

  buf[bytes_read] = '\0';
  printf("client: received '%s'\n",buf);
  close(socket_fd);

  return 0;
}
