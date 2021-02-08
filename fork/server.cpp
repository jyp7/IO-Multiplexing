#include <arpa/inet.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <cstring>
#include <iostream>

sockaddr_in SockAddress(int port, std::string ip) {
  sockaddr_in res;
  res.sin_family = AF_INET;
  res.sin_port = htons(port);
  inet_pton(AF_INET, ip.c_str(),
            &res.sin_addr);  // res.sin_addr.s_addr = htonl(INADDR_ANY);
  bzero(&(res.sin_zero), 8);
  return res;
}

int ListenFd(int port, std::string ip, int backlog = 5) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) {
    std::cout << "errno: " << strerror(errno) << '\n';
    return -1;
  }
  int reuse = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
    std::cout << "errno: " << strerror(errno) << '\n';
    return -1;
  }
  sockaddr_in address = SockAddress(port, ip);
  if (bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1) {
    std::cout << "errno: " << strerror(errno) << '\n';
    return -1;
  }
  if (listen(fd, backlog) == -1) {
    std::cout << "errno: " << strerror(errno) << '\n';
    return -1;
  }
  return fd;
}

int AcceptFd(int listen_fd) {
  sockaddr_in client;
  socklen_t client_address_length = sizeof(client);
  int accept_fd = accept(listen_fd, reinterpret_cast<sockaddr*>(&client),
                         &client_address_length);
  if (accept_fd == -1) {
    std::cout << "errno: " << strerror(errno) << '\n';
    return -1;
  }
  std::cout << "receive connect request from " << inet_ntoa(client.sin_addr)
            << ':' << ntohs(client.sin_port) << '\n';
  return accept_fd;
}

void Send(int accept_fd, std::string s) {
  send(accept_fd, s.c_str(), strlen(s.c_str()), 0);
}

bool PrintReceiveMessage(int fd) {
  char buf[1024] = {};
  int n = recv(fd, buf, sizeof(buf) - 1, 0);
  if (n <= 0) {
    return false;
  }
  std::cout << n << " bytes message: " << buf << '\n';
  return true;
}

int main() {
  int listen_fd = ListenFd(12345, "0.0.0.0");
  assert(listen_fd != -1);
  while (true) {
    int accept_fd = AcceptFd(listen_fd);
    assert(accept_fd != -1);
    pid_t p = fork();
    if (p == -1) {
      std::cout << "errno: " << strerror(errno) << '\n';
    } else if (p == 0) {
      std::cout << "this is child, "
                << "child-parent: " << getpid() << " " << getppid() << '\n';
      Send(accept_fd, "welcome to join");
      PrintReceiveMessage(accept_fd);
    } else {
      std::cout << "this is parent, "
                << "child-parent: " << p << " " << getpid() << '\n';
    }
    close(accept_fd);
  }
  close(listen_fd);
}