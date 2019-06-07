#ifndef __INCLUDE_RADAR_HPP__
#define __INCLUDE_RADAR_HPP__

#include <fstream>
#include <future>
#include <iostream>
#include <thread>

#pragma once

/** @example example.cpp
 * This is an example of how to use the LaserRadar::Radar class.
*/

/** LaserRadar 激光雷达的命名空间 */
namespace LaserRadar {
/** Radar 雷达 */
class Radar {
private:
  // server socket session
  // int server_fd;

  // laser radar socket
  // int laser_radar_socket;

  // std::thread accept_thread;
  // std::thread connect_thread;
  std::thread read_thread;

  std::promise<void> read_thread_signal;

  std::ofstream output_stream;

  bool start_radar_frame = false;

  std::string radar_ip;
  int radar_port;

public:
  /** 雷达构造函数，初始化接受的 UDP 内容文件
   * @param filename 从雷达接受的内容存入此文件中
  */
  Radar(std::string filename, std::string, int);

  /** 查看是否连接雷达成功 */
  bool running = false;

  /** 启动并连接雷达
   * @return 0 for success, -1 for failure
   */
  int startup();

  /** 断开连接雷达，并关闭文件输出
   * @return 0 for success, -1 for failure
   */
  int teardown();

  /** 发送自定义命令到雷达
   * @param command 命令内容
   * @param length command 长度
   * @return 0 for success, -1 for failure
   */
  int sender(unsigned char command[], unsigned int length);

  /** 工作模式 */
  enum working_pattern {
    /** 工作 */
    working_pattern_working,
    /** 停止 */
    working_pattern_stop
  };

  /** 设置雷达的工作模式。
   * @param pattern 工作模式
   * @return 0 for success, -1 for failure
  */
  int set_working_pattern(enum working_pattern pattern);

  /** 频率 */
  enum frequency_num {
    /** 10kHz */
    frequency_num_10,
    /** 20kHz */
    frequency_num_20,
    /** 40kHz */
    frequency_num_40,
  };
  /** 频率设置
   * @param frequency 频率设置
   * @return 0 for success, -1 for failure
  */
  int set_frequency(enum frequency_num frequency);

  /** 回波个数 */
  enum back_wave_num {
    /** 1 个 */
    back_wave_num_1,
    /** 2 个 */
    back_wave_num_2,
    /** 3 个 */
    back_wave_num_3,
    /** 4 个 */
    back_wave_num_4,
  };
  enum back_wave_filter {
    /** 只返回第 1 次回波 */
    back_wave_filter_1,
    /** 只返回第 2 次回波 */
    back_wave_filter_2,
    /** 只返回第 3 次回波 */
    back_wave_filter_3,
    /** 只返回最强回波 */
    back_wave_filter_strong,
    /** 只返回最后回波 */
    back_wave_filter_last,
    /** 返回最强和最后回波，如果最强和最后回波相同，则返回第二强回波 */
    back_wave_filter_strong_last,
    /** 返回最强和第一次回波，如果最强和第一次回波相同，则返回第二强回波 */
    back_wave_filter_strong_first,
  };
  /** 回波设置
   * @param num 回波个数设置
   * @param filter 回波过滤
   * @return 0 for success, -1 for failure
  */
  int set_back_wave(enum back_wave_num num, enum back_wave_filter filter);

  /** 方位角设置
   * @param start 起始角度
   * @param end 结束角度
   * @return 0 for success, -1 for failure
  */
  int set_azimuth_angle(unsigned int start, unsigned int end);

  /** 俯仰角设置
   * @param start 起始角度
   * @param end 结束角度
   * @return 0 for success, -1 for failure
   */
  int set_pitch_angle(unsigned int start, unsigned int end);

  /** 激光能量模式 */
  enum laser_energy_pattern {
    /** 激光能量模式：手动模式 */
    laser_energy_pattern_manual,
    /** 激光能量模式：自动模式 */
    laser_energy_pattern_automatic,
  };
  /** 激光能量等级 */
  enum laser_energy_level {
    /** 第 1 级 */
    laser_energy_level_1,
    /** 第 2 级 */
    laser_energy_level_2,
    /** 第 3 级 */
    laser_energy_level_3,
    /** 第 4 级 */
    laser_energy_level_4,
    /** 第 5 级 */
    laser_energy_level_5,
    /** 第 6 级 */
    laser_energy_level_6,
    /** 第 7 级 */
    laser_energy_level_7,
    /** 第 8 级 */
    laser_energy_level_8,
  };
  /** 激光能量设置
   * @param pattern 激光能量模式
   * @param level 激光能量等级
   * @return 0 for success, -1 for failure
  */
  int set_laser_energy(enum laser_energy_pattern pattern, enum laser_energy_level level);

  /** 目的源地址设置
   * @param destination_address 目的地址
   * @param destination_port 目的端口
   * @param original_address 源地址
   * @param original_port 源端口
   * @return 0 for success, -1 for failure
  */
  int set_network_info(std::string destination_address, unsigned int destination_port, std::string original_address, unsigned int original_port);

  /** 高压模式 */
  enum high_voltage_pattern {
    /** 温度自动控制 */
    high_voltage_pattern_temperature,
    /** 程序设置 */
    high_voltage_pattern_software,
    /** 高压减 1V */
    high_voltage_pattern_minus,
    /** 高压加 1V */
    high_voltage_pattern_add,
  };
  /** 高压控制设置
   * @param pattern 高压模式
   * @param high_voltage 高压参数，600-2000
   * @return 0 for success, -1 for failure
   */
  int set_high_voltage(enum high_voltage_pattern pattern, unsigned int high_voltage);

  /** 噪声控制设置 */
  int set_noise();

  /** 转速控制
   * @param speed 转速
   * @return 0 for success, -1 for failure
   */
  int set_rotate_speed(unsigned int speed);

  /** 停止转动
   * @return 0 for success, -1 for failure
  */
  int set_rotate_stop();
  /** 仅设置转速
   * @param speed 转速
   * @return 0 for success, -1 for failure
  */
  int set_rotate_speed_raw(unsigned int speed);
  /** 开始转动
   * @return 0 for success, -1 for failure
  */
  int set_rotate_begin();

  /** 上位机发给激光雷达控制电机的命令
   * @return 0 for success, -1 for failure
   */
  int set_laser_machine();
};
} // namespace LaserRadar

#endif // __INCLUDE_RADAR_HPP__
