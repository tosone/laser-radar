#include <chrono>
#include <cstdlib>
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

int radar_frame_num = 0;

void callback(unsigned char *buffer, unsigned int size) {
  char command[100] = {0};
  char filename[15] = {0};
  sprintf(filename, "%d.raw", radar_frame_num);
  sprintf(command, "sh -c \"if [ ! -f out/%s ]; then rm out/%s; fi\"", filename, filename);
  if (system(command) != 0) {
    spdlog::error("cannot create output dir");
  }
  std::ofstream output_stream;
  output_stream.open(filename, std::ios::out | std::ios::app | std::ofstream::binary);
  output_stream.write((char *)&buffer[0], size);
  output_stream.flush();
  output_stream.close();
}

uint64_t udp_frame_num = 0;

void udp_callback(unsigned char *buffer, unsigned int size) {
  udp_frame_num++;
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
      printf("%-5s\t%s\n", "-h", "显示这些帮助信息");
      printf("%-5s\t%s\n", "-o", "指定 UDP 帧输出的文件名");
      printf("%-5s\t%s\n", "-a", "设置激光类的 ip 地址");
      printf("%-5s\t%s\n\n", "-p", "设置激光雷达的端口");
      return 0;
    default:
      break;
    }
  }

  if (system("sh -c \"if [ ! -d out ]; then mkdir out; fi\"") != 0) {
    spdlog::error("cannot create output dir");
  }

  Radar radar(output, std::string(ip), atoi(port)); // 初始化

  radar.radar_frame_callback = callback;
  radar.startup(); // 连接雷达

  radar.analysis_start();

  while (true) {
    std::cout << "> ";
    std::string command;
    std::cin >> command;
    if (command == "help") {
      printf("%-15s%s\n", "send", "发送指定的十六进制数到雷达。例如：send EEEE0101");
      printf("%-15s%s\n", "sendStr", "发送指定的字符串到雷达。例如：sendStr ;UM=5;");
      printf("%-15s%s\n", "getFrequency", "获取当前的帧频信息");
      printf("%-15s%s\n", "store", "是否存储当前的 UDP 帧数据");
      printf("%-15s%s\n", "quit", "结束程序");
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
          radar.store_radar_frame = true;
          radar_frame_num         = 0;
          udp_frame_num           = 0;
          radar.analysis_start();
        } else if (param == "stop") {
          radar.store_radar_frame = false;
          std::cout << "got radar frame num:" << radar_frame_num << std::endl;
          std::cout << "got udp frame num:" << udp_frame_num << std::endl;
          radar.analysis_stop();
        } else {
          std::cout << "invalid command" << std::endl;
        }
      }
    } else if (command == "getFrequency") {
      radar.analysis_stop();
    } else if (command == "sendStr") {
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
