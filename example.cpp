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
  Radar radar("out.raw"); // 初始化

  radar.startup(); // 连接雷达

  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    if (radar.running) {
      spdlog::info("sending a command");
      if (radar.set_working_pattern(radar.working_pattern_working) == 0) { // 发送命令
        spdlog::info("sending a command");
      } else {
        spdlog::error("send command with error");
      };
    } else {
      spdlog::error("radar have not connected the driver udp server yet");
    }
  }

  radar.teardown(); // 断开连接
}
