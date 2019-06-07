#include <chrono>
#include <cstdlib>
#include <cstring>
#include <future>
#include <iostream>
#include <string>
#include <thread>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <radar.hpp>
#include <spdlog/spdlog.h>

namespace LaserRadar {
Radar::Radar(std::string filename, std::string ip, int port) {
  spdlog::info("Initialize LaserRadar...");
  radar_ip   = ip;
  radar_port = port;
  output_stream.open(filename, std::ios::out | std::ios::app | std::ofstream::binary);
}

int Radar::startup() {
  spdlog::info("LaserRadar Starting...");
  int recv_fd = 0;
  if ((recv_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == 0) {
    spdlog::error("Create socket error");
    return -1;
  }

  struct sockaddr_in serv_addr;

  bzero(&serv_addr, sizeof(struct sockaddr_in));

  serv_addr.sin_family      = AF_INET;
  serv_addr.sin_port        = htons(8080);
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  // inet_pton(AF_INET, "192.168.0.55", &serv_addr.sin_addr);

  std::future<void> futureObj = read_thread_signal.get_future();

  if (bind(recv_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) {
    spdlog::error("bind error");
  };

  read_thread = std::thread(
      [=](std::future<void> futureObj, std::ofstream output_stream) {
        unsigned int last_deg              = 0;
        unsigned char *pack_radar_frame    = (unsigned char *)malloc(10);
        unsigned int pack_radar_frame_size = 0;
        while (futureObj.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout) {
          size_t length         = 1500 * sizeof(unsigned char);
          unsigned char *buffer = (unsigned char *)malloc(length);
          bzero(buffer, length);
          socklen_t addrLen = sizeof(struct sockaddr_in);
          ssize_t valread   = recvfrom(recv_fd, buffer, length, 0, (struct sockaddr *)&serv_addr, &addrLen);
          if (valread > 17) {
            if (udp_frame_callback != nullptr) {
              udp_frame_callback(buffer, valread);
            }
            unsigned int deg = buffer[16] | buffer[17] << 8; // 将小端数据换算成 uint
            if (!start_radar_frame) {                        // 雷达可能处于一个角度非零的状态，寻找零点
              if (deg == 0) {                                // 确定是 零点 直接开始
                start_radar_frame = true;
                last_deg          = deg;
              } else {
                if (deg >= last_deg) { // 角度持续增大中
                  last_deg = deg;
                  continue;
                } else { // 角度突然变小，即使不是零点，应该也在零点附近
                  last_deg          = deg;
                  start_radar_frame = true;
                }
              }
            } else { // 已经得到有序的开始，无需寻找零点
              if (deg >= last_deg) {
                pack_radar_frame_size += valread; // 计算需要重新分配的空间
                pack_radar_frame = (unsigned char *)realloc(pack_radar_frame, pack_radar_frame_size);
                // 将新得到的数据存入新申请的空间中，必须要确定开始复制的位置
                memcpy(pack_radar_frame + (pack_radar_frame_size - valread), buffer, valread);
                last_deg = deg;
              } else {
                // 可能需要补充雷达有微小的角度来回抖动，TODO：deg <<< last_deg 再进入此处
                // std::cout << std::dec << deg << " " << last_deg << " round " << pack_radar_frame_size << std::endl;
                last_deg = deg;
                if (radar_frame_callback != nullptr) {
                  radar_frame_callback(pack_radar_frame, pack_radar_frame_size); // 回调给外部一圈的雷达数据
                }
                pack_radar_frame_size = 0;
                free(pack_radar_frame);
                pack_radar_frame = (unsigned char *)malloc(10);
              }
            }
            total_frame++;
            if (store_radar_frame) { // 存储数据到文件
              output_stream.write((char *)&buffer[0], valread);
              output_stream.flush();
            }
          }
          free(buffer);
        }
      },
      std::move(futureObj), std::move(output_stream));

  read_thread.detach();

  return 0;
}

void Radar::analysis_start() {
  analysis_start_time = std::chrono::steady_clock::now();
}

void Radar::analysis_stop() {
  total_frame = 0;

  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

  uint64_t count = std::chrono::duration_cast<std::chrono::milliseconds>(end - analysis_start_time).count();
  std::cout.precision(1);

  // std::cout << "total: " << total_frame << " frame" << std::endl;
  std::cout << "duration: " << std::fixed << count / 1000 * 1.0 << " s" << std::endl;
  std::cout << "frame per second: " << std::fixed << total_frame / count * 1.0 << " frame/s" << std::endl;
  total_frame = 0;
}

int Radar::teardown() {
  read_thread_signal.set_value();
  output_stream.close();
  return 0;
}

int Radar::sender(unsigned char command[], unsigned int length) {
  int sock      = socket(AF_INET, SOCK_DGRAM, 0);
  const int opt = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt)) < 0) {
    spdlog::error("Error in setting Broadcast option");
    return -1;
  }

  struct sockaddr_in sender_addr;
  sender_addr.sin_family      = AF_INET;
  sender_addr.sin_port        = htons(radar_port);
  sender_addr.sin_addr.s_addr = inet_addr(radar_ip.c_str());

  socklen_t len = sizeof(struct sockaddr_in);

  std::cout << "Sending command: ";
  for (int index = 0; index < length; index++) {
    if ((unsigned int)(command[index]) < 10) {
      std::cout << "0";
    }
    std::cout << std::hex << std::uppercase << (unsigned int)(command[index]);
  }
  std::cout << std::endl;

  if (sendto(sock, command, length, 0, (sockaddr *)&sender_addr, len) != (ssize_t)length) {
    spdlog::error("Send msg failed");
    return -1;
  }
  return 0;
}

int Radar::set_working_pattern(enum working_pattern pattern) {
  unsigned char command[19] = {0};

  command[0] = 0xEE;
  command[1] = 0xAA;

  command[2] = 0x01;

  switch (pattern) {
  case working_pattern_working:
    command[3] = 0xAA;
    break;
  case working_pattern_stop:
    command[3] = 0x55;
    break;
  default:
    return -1;
  }
  return sender(command, 19);
}

int Radar::set_frequency(enum frequency_num num) {
  unsigned char command[19] = {0};

  command[0] = 0xEE;
  command[1] = 0xAA;

  command[2] = 0x02;

  switch (num) {
  case frequency_num_10:
    command[3] = 0x0A;
    break;
  case frequency_num_20:
    command[3] = 0x14;
    break;
  case frequency_num_40:
    command[3] = 0x28;
    break;
  default:
    return -1;
  }
  return sender(command, 19);
}
int Radar::set_back_wave(enum back_wave_num num, enum back_wave_filter filter) {
  unsigned char command[19] = {0};

  command[0] = 0xEE;
  command[1] = 0xAA;

  command[2] = 0x04;

  switch (num) {
  case back_wave_num_1:
    command[3] = 0x01;
    break;
  case back_wave_num_2:
    command[3] = 0x02;
    break;
  case back_wave_num_3:
    command[3] = 0x03;
    break;
  case back_wave_num_4:
    command[3] = 0x04;
    break;
  default:
    return -1;
  }
  switch (filter) {
  case back_wave_filter_1:
    command[4] = 0xE1;
    break;
  case back_wave_filter_2:
    command[4] = 0xE2;
    break;
  case back_wave_filter_3:
    command[4] = 0xE3;
    break;
  case back_wave_filter_strong:
    command[4] = 0xE4;
    break;
  case back_wave_filter_last:
    command[4] = 0xE5;
    break;
  case back_wave_filter_strong_last:
    command[4] = 0xE6;
    break;
  case back_wave_filter_strong_first:
    command[3] = 0xE7;
    break;
  default:
    return -1;
  }
  return sender(command, 19);
}

int Radar::set_azimuth_angle(unsigned int start, unsigned int end) {
  unsigned char command[19] = {0};

  command[0] = 0xEE;
  command[1] = 0xAA;

  command[2] = 0x08;

  unsigned char start_char[2];
  memcpy(start_char, (char *)&start, 2);
  command[3] = start_char[0];
  command[4] = start_char[1];

  unsigned char end_char[2];
  memcpy(end_char, (char *)&end, 2);
  command[5] = end_char[0];
  command[6] = end_char[1];
  return sender(command, 19);
}

int Radar::set_pitch_angle(unsigned int start, unsigned int end) {
  unsigned char command[19] = {0};

  command[0] = 0xEE;
  command[1] = 0xAA;

  command[2] = 0x10;

  unsigned char start_char[2];
  memcpy(start_char, (char *)&start, 2);
  command[3] = start_char[0];
  command[4] = start_char[1];

  unsigned char end_char[2];
  memcpy(end_char, (char *)&end, 2);
  command[5] = end_char[0];
  command[6] = end_char[1];
  return sender(command, 19);
}

int Radar::set_laser_energy(enum laser_energy_pattern pattern, enum laser_energy_level level) {
  unsigned char command[19] = {0};

  command[0] = 0xEE;
  command[1] = 0xAA;

  command[2] = 0x20;

  switch (pattern) {
  case laser_energy_pattern_manual:
    command[3] = 0xDA;
    break;
  case laser_energy_pattern_automatic:
    command[3] = 0xD5;
    break;
  default:
    break;
  }
  switch (level) {
  case laser_energy_level_1:
    command[4] = 0xC0;
    break;
  case laser_energy_level_2:
    command[4] = 0xC1;
    break;
  case laser_energy_level_3:
    command[4] = 0xC2;
    break;
  case laser_energy_level_4:
    command[4] = 0xC3;
    break;
  case laser_energy_level_5:
    command[4] = 0xC4;
    break;
  case laser_energy_level_6:
    command[4] = 0xC5;
    break;
  case laser_energy_level_7:
    command[4] = 0xC6;
    break;
  case laser_energy_level_8:
    command[4] = 0xC7;
    break;
  default:
    break;
  }
  return sender(command, 19);
}

int Radar::set_network_info(std::string destination_address, unsigned int destination_port, std::string original_address, unsigned int original_port) {
  unsigned char command[19] = {0};

  command[0] = 0xEE;
  command[1] = 0xAA;

  command[2] = 0x40;

  std::string delimiter = ".";

  int substr = 0;

  size_t pos = 0;
  std::string token;
  while ((pos = destination_address.find(delimiter)) != std::string::npos) {
    substr++;
    token = destination_address.substr(0, pos);
    try {
      command[2 + substr] = (unsigned char)(std::stoi(token));
    } catch (std::exception const &e) {
      spdlog::error("Got error: {}", e.what());
      return -1;
    }
    destination_address.erase(0, pos + delimiter.length());
  }
  if (substr != 3) {
    spdlog::error("Address is invalid: {}", substr);
    return -1;
  }
  substr++;
  try {
    command[2 + substr] = (unsigned char)(std::stoi(destination_address));
  } catch (std::exception const &e) {
    spdlog::error("Got error: {}", e.what());
    return -1;
  }

  unsigned char destination_port_char[2];
  memcpy(destination_port_char, (char *)&destination_port, 2);
  command[7] = destination_port_char[0];
  command[8] = destination_port_char[1];

  substr += 2;

  pos = 0;

  while ((pos = original_address.find(delimiter)) != std::string::npos) {
    substr++;
    token = original_address.substr(0, pos);
    try {
      command[2 + substr] = (unsigned char)(std::stoi(token));
    } catch (std::exception const &e) {
      spdlog::error("Got error: {}", e.what());
      return -1;
    }
    original_address.erase(0, pos + delimiter.length());
  }
  if (substr != 9) {
    spdlog::error("Address is invalid: {}", substr);
    return -1;
  }
  substr++;
  try {
    command[2 + substr] = (unsigned char)(std::stoi(original_address));
  } catch (std::exception const &e) {
    spdlog::error("Got error: {}", e.what());
    return -1;
  }

  unsigned char original_port_char[2];
  memcpy(original_port_char, (char *)&original_port, 2);
  command[13] = original_port_char[0];
  command[14] = original_port_char[1];

  return sender(command, 19);
}

int Radar::set_noise() { return 0; }

int Radar::set_rotate_stop() {
  unsigned char command[19] = {0};

  command[0] = 0x55;
  command[1] = 0xAA;

  command[2] = 0x01;

  unsigned char arg[16] = "ST;";
  memcpy(command + 3, arg, 16);

  return sender(command, 19);
}

int Radar::set_rotate_speed_raw(unsigned int speed) {
  unsigned char command[19] = {0};

  command[0] = 0x55;
  command[1] = 0xAA;

  command[2] = 0x01;

  unsigned char arg[16] = "";
  char *speed_char      = (char *)malloc(16 * sizeof(char));
  memset(speed_char, 0, 16 * sizeof(char));
  sprintf(speed_char, "JV=%u;", speed * 47360);

  memcpy(command + 3, speed_char, 16);

  return sender(command, 19);
}

int Radar::set_rotate_begin() {
  unsigned char command[19] = {0};

  command[0] = 0x55;
  command[1] = 0xAA;

  command[2] = 0x01;

  unsigned char arg[16] = "BG;";
  memcpy(command + 3, arg, 16);

  return sender(command, 19);
}

int Radar::set_rotate_speed(unsigned int speed) {
  if (set_rotate_stop() != 0) {
    return -1;
  };
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  if (set_rotate_speed_raw(speed) != 0) {
    return -1;
  };
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  if (set_rotate_begin() != 0) {
    return -1;
  }
  return 0;
}

} // namespace LaserRadar

// std::cout << "Server got command: ";

// if ((unsigned int)(buffer[16]) < 10) {
//   std::cout << "0";
// }
// std::cout << std::hex << std::uppercase << (unsigned int)(buffer[16]);
// if ((unsigned int)(buffer[17]) < 10) {
//   std::cout << "0";
// }
// std::cout << std::hex << std::uppercase << (unsigned int)(buffer[17]);
// for (int index = 0; index < valread; index++) {
//   if (index == 2) std::cout << " ";
//   if (index == 3) std::cout << " ";
//   if (index == 9) std::cout << " ";
//   if (index == 10) std::cout << " ";
//   if (index == 13) std::cout << " ";
//   if (index == 14) std::cout << " ";
//   if (index == 16) std::cout << " ";
//   if (index == 18) std::cout << " ";
//   if (index == 20) std::cout << " ";
//   if (index == 21) std::cout << " ";
//   if ((unsigned int)(buffer[index]) < 10) {
//     std::cout << "0";
//   }
//   std::cout << std::hex << std::uppercase << (unsigned int)(buffer[index]);
// }
// std::cout << std::endl;
