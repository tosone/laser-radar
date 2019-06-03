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
Radar::Radar(std::string filename) {
  spdlog::info("Initialize LaserRadar...");
  output_stream.open(filename, std::ios::out | std::ios::app | std::ofstream::binary);
}

int Radar::startup() {
  if (running) {
    return 0;
  }
  spdlog::info("LaserRadar Starting...");

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    spdlog::error("Create socket error");
    return -1;
  }

  struct sockaddr_in serv_addr;
  memset(&serv_addr, '0', sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port   = htons(8080);

  if (inet_pton(AF_INET, "192.168.0.10", &serv_addr.sin_addr) <= 0) {
    spdlog::error("Invalid address not supported");
    return -1;
  }

  // int opt = 1;
  // if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
  //   spdlog::error("Setting socket error");
  //   return -1;
  // }
  // struct sockaddr_in address;
  // address.sin_family      = AF_INET;
  // address.sin_addr.s_addr = INADDR_ANY;
  // address.sin_port        = htons(port);
  // if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
  //   spdlog::error("Binding socket error");
  //   return -1;
  // }
  // if (listen(server_fd, 3) < 0) {
  //   spdlog::error("Listen specified port error");
  //   return -1;
  // }

  // accept_thread = std::thread(
  //     [=](int server_fd, int *socket, struct sockaddr_in *address) {
  //       int addrlen = sizeof(address);
  //       if ((*socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
  //         spdlog::error("Got new connection error");
  //         running = false;
  //       }
  //       running = true;
  //       spdlog::info("Got new connection");
  //     },
  //     server_fd, &laser_radar_socket, &address);

  // accept_thread.detach();

  connect_thread = std::thread(
      [=]() {
        while (true) {
          if (connect(laser_radar_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            spdlog::error("Laser Radar connect failed，please check laser radar status");
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
          } else {
            break;
          }
        }
        running = true;
        spdlog::info("Laser Radar connected");
      });

  connect_thread.detach();

  std::future<void> futureObj = read_thread_signal.get_future();

  read_thread = std::thread(
      [](int *socket, std::future<void> futureObj, std::ofstream output_stream) {
        while (futureObj.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout) {
          unsigned char *buffer = (unsigned char *)malloc(1024 * sizeof(unsigned char *));
          memset(buffer, 0, 1024 * sizeof(unsigned char *));
          ssize_t valread = recv(*socket, buffer, 1024, 0);
          if (valread > 0) {
            std::cout << std::dec << "Server got msg length: " << valread << std::endl;
            output_stream.write((char *)&buffer[0], valread);
            output_stream.flush();
            std::cout << "Server got command: ";
            for (int index = 0; index < valread; index++) {
              if ((unsigned int)(buffer[index]) < 10) {
                std::cout << "0";
              }
              std::cout << std::hex << std::uppercase << (unsigned int)(buffer[index]);
            }
            std::cout << std::endl;
          }
          free(buffer);
        }
      },
      &laser_radar_socket, std::move(futureObj), std::move(output_stream));

  read_thread.detach();

  return 0;
}

int Radar::teardown() {
  read_thread_signal.set_value();
  output_stream.close();
  return 0;
}

int Radar::sender(unsigned char command[], unsigned int length) {
  if (!running) {
    spdlog::error("No such a socket to laser radar");
    return -1;
  }
  std::cout << "Sending command: ";
  for (int index = 0; index < length; index++) {
    if ((unsigned int)(command[index]) < 10) {
      std::cout << "0";
    }
    std::cout << std::hex << std::uppercase << (unsigned int)(command[index]);
  }
  std::cout << std::endl;

  if (send(laser_radar_socket, command, length, 0) != (ssize_t)length) {
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
