#include "qserver-extension.h"

enum {
  BACKLOG = 5,
};

int qserverext_sock_get(int port)
{
  static int fd = -1;
  if (port < 0) {
    return -1;
  }
  
  if (fd >= 0) {
    return fd;  
  }
  
  fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (fd < 0) {
    return -1;  
  }
  
  struct sockaddr_in addr;
  memset(&addr, '\0', sizeof(addr));
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  
  if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
    close(fd);
    fd = -1;
	}
	
  if (listen(fd, BACKLOG) < 0) {
    close(fd);
    fd = -1;
  }
  
  return fd;
}


int qserverext_sock_nonblock(int fd)
{
  int f = fcntl(fd, F_GETFL);
  f |= O_NONBLOCK;
  return fcntl(fd, F_SETFL, f);  
}
