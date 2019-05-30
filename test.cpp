#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <radar.hpp>
#include <spdlog/spdlog.h>

using namespace LaserRadar;

int main(int argc, char const *argv[]) {
  Radar radar("out.raw");
  radar.startup(12301);
  int sock = 0;
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    spdlog::error("Create socket error");
    return -1;
  }

  struct sockaddr_in serv_addr;
  memset(&serv_addr, '0', sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port   = htons(12301);

  if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
    spdlog::error("Invalid address not supported");
    return -1;
  }
  std::this_thread::sleep_for(std::chrono::seconds(1));
  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    spdlog::error("Connection failed");
    return -1;
  }

  std::string hello = "Hello";
  send(sock, hello.c_str(), strlen(hello.c_str()), 0);

  std::thread client_read_thread = std::thread(
      [](int *socket) {
        while (true) {
          unsigned char *buffer = (unsigned char *)malloc(1024 * sizeof(unsigned char *));
          memset(buffer, 0, 1024 * sizeof(unsigned char *));
          ssize_t valread = recv(*socket, buffer, 1024, 0);
          if (valread > 0) {
            std::cout << std::dec << "Client got msg length: " << valread << std::endl;
            std::ofstream output_stream;
            output_stream.open("client.raw", std::ios::out | std::ios::app | std::ofstream::binary);
            output_stream.write((char *)&buffer[0], valread);
            output_stream.flush();
            output_stream.close();
            std::cout << "Client got command: ";
            for (int index = 0; index < valread; index++) {
              if ((unsigned int)(buffer[index]) < 10) {
                std::cout << "0";
              }
              std::cout << std::hex << std::uppercase << (unsigned int)(buffer[index]);
            }
            std::cout << std::endl;
          }
        }
      },
      &sock);

  client_read_thread.detach();

  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    radar.set_network_info("192.168.0.1", 3000, "192.168.0.2", 3000);
  }

  return 0;
}
