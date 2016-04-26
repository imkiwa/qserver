#include "qserver.h"

#define CALL(fn, arg...) \
  if (fn) fn(arg)

enum {
  BACKLOG = 5,
  MAX_EVENT = 40, MAX_POLL = 256,
};


static cb_data_t* cb_data_new()
{
  return (cb_data_t*) malloc(sizeof(cb_data_t));
}


static void cb_data_delete(cb_data_t *cb)
{
  if (cb) {
    free(cb);
  }
}

static int process_io(int ep, cb_table_t *callbacks, struct epoll_event *ev)
{
  cb_data_t *data = ev->data.ptr;
  if (!data) {
    return -1;
  }
  
  int fd = data->fd;
  ssize_t n = 0;
  struct epoll_event nev;
  
  nev.data.fd = fd;
  nev.data.ptr = data;
  nev.events = EPOLLET;
  
  if (ev->events & EPOLLIN) {
    if ((n = recv(fd, data->buffer, BUFFER_SIZE, 0)) == 0) {
      // connection disconnected
      printf(":: Connection disconnected, removing from epoll\n");
      close(fd);
      data->fd = 0;
      CALL(callbacks->on_close, data);
      cb_data_delete(data);
      epoll_ctl(ep, EPOLL_CTL_DEL, fd, NULL);
      return 0;
    }
    
    data->buffer_bytes = n;
    nev.events |= EPOLLOUT;
    epoll_ctl(ep, EPOLL_CTL_MOD, fd, &nev);
    
    CALL(callbacks->on_data, data);
  }
  
  if (ev->events & EPOLLOUT) {
    CALL(callbacks->on_reply, data);
    nev.events |= EPOLLIN;
    epoll_ctl(ep, EPOLL_CTL_MOD, fd, &nev);
  }
  
  return 0;
}


int sock_get(int port)
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


int sock_nonblock(int fd)
{
  int f = fcntl(fd, F_GETFL);
  f |= O_NONBLOCK;
  return fcntl(fd, F_SETFL, f);  
}


int server_shutdown(int opt)
{
  static int shutdown = 0;
  if (opt) {
    shutdown = opt;
  }
  
  return shutdown;
}


int server_running()
{
  return !server_shutdown(0);
}


int server_loop(int sock, cb_table_t *callbacks)
{
  int ep = epoll_create(MAX_POLL);
  if (ep < 0) {
    return -1;
  }
  
  int nfds = 0;
  struct epoll_event ev;
  struct epoll_event events[MAX_EVENT];
  
  memset(&ev, '\0', sizeof(ev));
  memset(events, '\0', sizeof(events));
  
  ev.data.fd = sock;
  ev.data.ptr = NULL;
  ev.events = EPOLLIN | EPOLLET;
  epoll_ctl(ep, EPOLL_CTL_ADD, sock, &ev);
  
  while (server_running()) {
    nfds = epoll_wait(ep, events, MAX_EVENT, 0);
    for (int i = 0; i < nfds; ++i) {
      cb_data_t *curdata = events[i].data.ptr;
      
      // New client
      if (!curdata) {
        cb_data_t *cb = cb_data_new();
        if (!cb) {
          fprintf(stderr, "W: cb_data_new() failed: %s\n", strerror(errno));
          continue;
        }
        
        memset(cb, '\0', sizeof(*cb));
        memset(&ev, '\0', sizeof(ev));
         
        int conn = accept(sock, (struct sockaddr*) &cb->addr, &cb->addr_len);
        if (conn < 0) {
          fprintf(stderr, "W: accept() failed, %s\n", strerror(errno));
          continue;
        }
        
        printf(":: New connection from: %s\n", inet_ntoa(cb->addr.sin_addr));
        cb->fd = conn;
        ev.data.fd = conn;
        ev.data.ptr = (void*) cb;
        ev.events = EPOLLIN | EPOLLET;
        epoll_ctl(ep, EPOLL_CTL_ADD, conn, &ev);
        CALL(callbacks->on_open, cb);

      // I/O Operations
      } else {
        if (process_io(ep, callbacks, events + i) < 0) {
          fprintf(stderr, "W: process_io() failed\n");
        } // if
      } // else
    } // for
  } // while
  
  close(ep);
  return 0;
}

