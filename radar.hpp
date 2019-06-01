#ifndef __INCLUDE_RADAR_HPP__
#define __INCLUDE_RADAR_HPP__

#include <fstream>
#include <future>
#include <iostream>
#include <thread>

namespace LaserRadar {
class Radar {
private:
  // server socket session
  int server_fd;

  // laser radar socket
  int laser_radar_socket;

  std::thread accept_thread;
  std::thread read_thread;

  std::promise<void> read_thread_signal;

  std::ofstream output_stream;

public:
  // constructor set output filename
  Radar(std::string);

  // running or not
  bool running = false;

  // listen port
  int port;

  // startup the radar things
  // -1 failure
  // 0 success
  int startup(int);

  // shutdown all of the radar things
  // -1 failure
  // 0 success
  int teardown();

  // send command to server
  // -1 failure
  // 0 success
  int sender(unsigned char[], unsigned int length);

  enum working_pattern {
    working_pattern_working,
    working_pattern_stop
  };
  int set_working_pattern(enum working_pattern);

  enum frequency_num {
    frequency_num_10,
    frequency_num_20,
    frequency_num_40,
  };
  int set_frequency(enum frequency_num);

  enum back_wave_num {
    back_wave_num_1,
    back_wave_num_2,
    back_wave_num_3,
    back_wave_num_4,
  };
  enum back_wave_filter {
    back_wave_filter_1,
    back_wave_filter_2,
    back_wave_filter_3,
    back_wave_filter_strong,
    back_wave_filter_last,
    back_wave_filter_strong_last,
    back_wave_filter_strong_first,
  };
  int set_back_wave(enum back_wave_num, enum back_wave_filter);

  int set_azimuth_angle(unsigned int start, unsigned int end);

  int set_pitch_angle(unsigned int start, unsigned int end);

  enum laser_energy_pattern {
    laser_energy_pattern_manual,
    laser_energy_pattern_automatic,
  };
  enum laser_energy_level {
    laser_energy_level_1,
    laser_energy_level_2,
    laser_energy_level_3,
    laser_energy_level_4,
    laser_energy_level_5,
    laser_energy_level_6,
    laser_energy_level_7,
    laser_energy_level_8,
  };
  int set_laser_energy(enum laser_energy_pattern, enum laser_energy_level);

  int set_network_info(std::string destination_address, unsigned int destination_port, std::string original_address, unsigned int original_port);

  enum high_voltage_pattern {
    high_voltage_pattern_temperature,
    high_voltage_pattern_software,
    high_voltage_pattern_minus,
    high_voltage_pattern_add,
  };
  int set_high_voltage(enum high_voltage_pattern, unsigned int high_voltage);

  int set_noise();

  int set_rotate_speed(unsigned int speed);

  int set_rotate_stop();
  int set_rotate_speed_raw(unsigned int speed);
  int set_rotate_begin();

  int set_laser_machine();
};
} // namespace LaserRadar

#endif // __INCLUDE_RADAR_HPP__
