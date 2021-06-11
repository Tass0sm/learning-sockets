#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

/* the port users will be connecting to */
#define MY_PORT "3490"
/* how many pending connections queue will hold */
#define BACKLOG 10

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*) sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

int start_listening()
{
  int socket_fd, status, yes;
  struct addrinfo address_hints;
  struct addrinfo *address_results, *p;  // will point to the results

  memset(&address_hints, 0, sizeof address_hints); // make sure the struct is empty
  address_hints.ai_family = AF_UNSPEC;             // don't care IPv4 or IPv6
  address_hints.ai_socktype = SOCK_STREAM;         // TCP stream sockets
  address_hints.ai_flags = AI_PASSIVE;             // fill in my IP for me

  status = getaddrinfo(NULL, MY_PORT, &address_hints, &address_results);

  if (status != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    return -1;
  }

  /* Just assuming the first result is alright. */

  /* loop through all the results and bind to the first we can */
  for (p = address_results; p != NULL; p = p->ai_next) {
    socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

    if (socket_fd == -1) {
      perror("server: socket");
      continue;
    }

    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      perror("setsockopt");
      exit(1);
    }

    if (bind(socket_fd, p->ai_addr, p->ai_addrlen) == -1) {
      close(socket_fd);
      perror("server: bind");
      continue;
    }

    /* break once socket is successfully made and bound */
    break;
  }

  /* all done with this structure */
  freeaddrinfo(address_results);

  if (p == NULL) {
    fprintf(stderr, "server: failed to bind\n");
    exit(1);
  }

  if (listen(socket_fd, BACKLOG) == -1) {
    perror("listen");
    exit(1);
  }

  return socket_fd;
}

int main(int argc, char *argv[])
{
  struct sockaddr_storage their_addr;
  struct sigaction sa;
  socklen_t addr_size, sin_size;
  int socket_fd, connection_fd, bytes_read;
  char s[INET6_ADDRSTRLEN];


  char buffer[80];

  socket_fd = start_listening();

  /* reap all dead processes */
  sa.sa_handler = sigchld_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    exit(1);
  }

  printf("server: waiting for connections...\n");

  while (1) {
    sin_size = sizeof their_addr;
    connection_fd = accept(socket_fd, (struct sockaddr *) &their_addr, &sin_size);

    if (connection_fd == -1) {
      perror("accept");
      continue;
    }

    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *) &their_addr), s, sizeof s);
    printf("server: got connection from %s\n", s);

    /* parent doesn't need this */
    close(connection_fd);
  }

  return 0;
}
