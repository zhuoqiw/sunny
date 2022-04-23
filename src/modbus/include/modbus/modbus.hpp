// Copyright 2019 Zhushi Tech, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef MODBUS__MODBUS_HPP_
#define MODBUS__MODBUS_HPP_

#include <map>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/point_cloud2.hpp"

/**
 * @brief Forward declaration for inner implementation.
 *
 */
typedef struct _modbus modbus_t;

/**
 * @brief Forward declaration for inner implementation.
 *
 */
typedef struct _modbus_mapping_t modbus_mapping_t;

namespace modbus
{

/**
 * @brief Modbus protocal wrapped from libmodbus-dev.
 *
 */
class Modbus : public rclcpp::Node
{
public:
  /**
   * @brief Construct a new Modbus object.
   *
   * Create an inner implementation.
   * Initialize subscription.
   * Initialize parameter client for camera.
   * Initialize parameter client for gpio.
   * Print success if all done.
   * @param options Encapsulation of options for node initialization.
   */
  explicit Modbus(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());

  /**
   * @brief Destroy the Modbus:: Modbus object.
   *
   * Release parameter client for camera.
   * Release parameter client for gpio.
   * Release subscription.
   * Release inner implementation.
   * Print success if all done.
   * Throw no exception.
   */
  ~Modbus();

private:
  /**
   * @brief Control laser on of off.
   *
   */
  void _gpio_laser(bool);

  /**
   * @brief Control camera capture or not.
   *
   */
  void _camera_power(bool);

  /**
   * @brief Control camera capture or not.
   *
   */
  void _modbus(int);

  /**
   * @brief Update coordinates in mapping block.
   *
   * @param valid true if found.
   * @param u Coordinates in u.
   * @param v Coordinates in v.
   */
  void _update(bool valid, float u = 0., float v = 0.);

private:
  /**
   * @brief Subscription name.
   *
   */
  const char * _sub_name = "~/seam";

  /**
   * @brief Shared pointer to subscription.
   *
   */
  rclcpp::Subscription<sensor_msgs::msg::PointCloud2>::SharedPtr _sub;

  /**
   * @brief Parameter client for camera.
   *
   */
  std::shared_ptr<rclcpp::AsyncParametersClient> _param_camera;

  /**
   * @brief Parameter client for gpio.
   *
   */
  std::shared_ptr<rclcpp::AsyncParametersClient> _param_gpio;

  /**
   * @brief Modbus context.
   *
   */
  modbus_t * _ctx = NULL;

  /**
   * @brief Modbus mapping zone.
   *
   */
  modbus_mapping_t * _mb_mapping = NULL;

  /**
   * @brief Socket file descriptor
   *
   */
  int _sock = -1;

  /**
   * @brief Mutex to protect from data race.
   *
   */
  std::mutex _mutex;

  /**
   * @brief Thread for communication through modbus tcp.
   *
   */
  std::thread _thread;
};

}  // namespace modbus

#endif  // MODBUS__MODBUS_HPP_
