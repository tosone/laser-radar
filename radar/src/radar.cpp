#include <fstream>
#include <future>
#include <iostream>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <spdlog/spdlog.h>

#include <radar/UcharVector.h>
#include <ros/ros.h>
#include <std_msgs/UInt8.h>

int main(int argc, char **argv) {
  ros::init(argc, argv, "laser_radar");

  ros::NodeHandle n;

  ros::Publisher radar_pub = n.advertise<radar::UcharVector>("laser_radar_frame", 1000);

  std::promise<void> read_thread_signal;
  std::future<void> futureObj = read_thread_signal.get_future();

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

  if (bind(recv_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) {
    spdlog::error("bind error");
  };

  std::ofstream output_stream;
  output_stream.open("filename.raw", std::ios::out | std::ios::app | std::ofstream::binary);
  std::thread read_thread;
  read_thread = std::thread(
      [=](std::future<void> futureObj, std::ofstream output_stream) {
        radar::UcharVector msg;
        // std::this_thread::sleep_for(std::chrono::milliseconds(200));
        // std::cout << "1" << std::endl;
        unsigned int last_deg              = 0;
        unsigned char *pack_radar_frame    = (unsigned char *)malloc(10);
        unsigned int pack_radar_frame_size = 0;
        bool start_radar_frame             = false;
        while (true) {
          size_t length         = 1500 * sizeof(unsigned char);
          unsigned char *buffer = (unsigned char *)malloc(length);
          bzero(buffer, length);
          socklen_t addrLen = sizeof(struct sockaddr_in);
          std::cout << "1" << std::endl;
          ssize_t valread = recvfrom(recv_fd, buffer, length, 0, (struct sockaddr *)&serv_addr, &addrLen);
          if (valread > 17) {
            unsigned int deg = buffer[16] | buffer[17] << 8;
            if (!start_radar_frame) {
              if (deg == 0) {
                start_radar_frame = true;
              } else {
                if (deg >= last_deg) {
                  last_deg = deg;
                  continue;
                } else {
                  last_deg          = deg;
                  start_radar_frame = true;
                }
              }
            } else {
              if (deg >= last_deg) {
                pack_radar_frame_size += valread;
                pack_radar_frame = (unsigned char *)realloc(pack_radar_frame, pack_radar_frame_size);
                memcpy(pack_radar_frame, buffer, valread);
                last_deg = deg;
              } else {
                std::cout << std::dec << deg << " " << last_deg << " round " << pack_radar_frame_size << std::endl;
                last_deg = deg;
                // pack and send
                for (int index = 0; index < pack_radar_frame_size; index++) {
                  msg.val.push_back(pack_radar_frame[index]);
                }
                radar_pub.publish(msg);
                ros::spinOnce();
                msg.val.clear();
                pack_radar_frame_size = 0;
                free(pack_radar_frame);
                pack_radar_frame = (unsigned char *)malloc(10);
              }
            }
            output_stream.write((char *)&buffer[0], valread);
            output_stream.flush();
          }
          free(buffer);
        }
      },
      std::move(futureObj), std::move(output_stream));
  read_thread.detach();
  while (ros::ok()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
  return 0;
}
