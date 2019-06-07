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

unsigned char *hexstr_to_char(const char *hexstr) {
  size_t len          = strlen(hexstr);
  size_t final_len    = len / 2;
  unsigned char *chrs = (unsigned char *)malloc((final_len + 1) * sizeof(*chrs));
  for (size_t i = 0, j = 0; j < final_len; i += 2, j++)
    chrs[j] = (hexstr[i] % 32 + 9) % 25 * 16 + (hexstr[i + 1] % 32 + 9) % 25;
  chrs[final_len] = '\0';
  return chrs;
}

int main(int argc, char *argv[]) {
  char *output = "out.raw";
  char *ip     = "192.168.0.10";
  char *port   = "8080";
  int option_char;
  while ((option_char = getopt(argc, argv, "o:a:p:h")) != -1) {
    switch (option_char) {
    case 'a':
      ip = optarg;
      break;
    case 'p':
      port = optarg;
      break;
    case 'o':
      output = optarg;
      break;
    case 'h':
      printf("%s %s\n", argv[0], "usage:");
      printf("%-5s\t%s\n", "-h", "show this help information");
      printf("%-5s\t%s\n", "-o", "the radar frame output");
      printf("%-5s\t%s\n", "-a", "show laser radar's ip");
      printf("%-5s\t%s\n\n", "-p", "set laser radar's port");
      return 0;
    default:
      break;
    }
  }

  Radar radar(output, std::string(ip), atoi(port)); // 初始化

  radar.startup(); // 连接雷达

  while (true) {
    std::cout << "command > ";
    std::string command;
    std::cin >> command;
    std::cout << std::endl;
    if (command == "help") {
      printf("%-10s%s\n", "send", "send specified command to laser radar");
      printf("%-10s%s\n", "sendstr", "send specified command to laser radar");
      printf("%-10s%s\n", "working", "send working mode");
      printf("%-10s%s\n", "quit", "kill this programe");
      printf("%-10s%s\n", "working", "send working mode");
    } else if (command == "send") {
      std::string param;
      std::cin >> param;
      if (param.length() % 2 != 0) {
        std::cout << "invalid params" << std::endl;
      }
      unsigned char *p = hexstr_to_char(param.c_str());
      radar.sender(p, strlen((char *)p));
    } else if (command == "store") {
      std::string param;
      std::cin >> param;
      if (param.length()) {
        if (param == "start") {
          std::cout << "start" << std::endl;
        } else if (param == "stop") {
          std::cout << "stop" << std::endl;
        } else {
          std::cout << "invalid command" << std::endl;
        }
      }
    } else if (command == "sendstr") {
      std::string param;
      std::cin >> param;
      if (param.length()) {
        radar.sender((unsigned char *)param.c_str(), (unsigned int)param.length());
      }
    } else if (command == "quit") {
      std::cout << "Bye" << std::endl;
      radar.teardown();
      return EXIT_SUCCESS;
    } else {
      std::cout << "invalid command" << std::endl;
    }
  }

  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  radar.teardown(); // 断开连接

  return EXIT_SUCCESS;
}

// char c1[] = ";UM=5";
// radar.sender((unsigned char *)c1, strlen((char *)(c1)));
// std::this_thread::sleep_for(std::chrono::milliseconds(200));
// char c2[] = ";MO=1";
// radar.sender((unsigned char *)c2, strlen((char *)(c2)));
// std::this_thread::sleep_for(std::chrono::milliseconds(200));
// char c3[] = ";XQ##FindZero(658);";
// radar.sender((unsigned char *)c3, strlen((char *)(c3)));
