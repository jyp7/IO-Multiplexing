#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

sockaddr_in SockAddress(int port, std::string ip) {
  sockaddr_in res;
  bzero(&res, sizeof(res));
  res.sin_family = AF_INET;
  res.sin_port = htons(port);
  inet_pton(AF_INET, ip.c_str(), &res.sin_addr);
  return res;
}

int Connect(int port, std::string ip) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) {
    std::cout << "errno: " << strerror(errno) << '\n';
    return -1;
  }
  sockaddr_in address = SockAddress(port, ip);
  if (connect(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) ==
      -1) {
    std::cout << "errno: " << strerror(errno) << '\n';
    return -1;
  }
  return fd;
}

bool PrintReceiveMessage(int fd) {
  char buf[1024] = {};
  int n = recv(fd, buf, sizeof(buf) - 1, 0);
  if (n <= 0) {
    false;
  }
  std::cout << n << " bytes message: " << buf << '\n';
  return true;
}

void Send(int fd, std::string s) { send(fd, s.c_str(), strlen(s.c_str()), 0); }

int main() {
  int fd = Connect(12345, "0.0.0.0");
  assert(fd != -1);
  Send(fd, "hello");
  std::vector<pollfd> pollfds(1);
  pollfds[0].fd = fd;
  pollfds[0].events = POLLIN;
  while (true) {
    constexpr int timeout = 1000;
    int ret = poll(pollfds.data(), pollfds.size(), timeout);
    if (ret == -1) {
      std::cout << "errno: " << strerror(errno) << '\n';
      break;
    }
    if (ret == 0) {
      std::cout << "timeout" << '\n';
      continue;
    }
    if (pollfds.front().revents & POLLIN) {
      if (PrintReceiveMessage(fd)) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        Send(fd, "got your message");
      } else {
        break;
      }
    }
  }
  close(fd);
}