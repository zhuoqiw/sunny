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

#ifndef ROTATE_IMAGE__ROTATE_IMAGE_HPP_
#define ROTATE_IMAGE__ROTATE_IMAGE_HPP_

#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"

namespace rotate_image
{

class RotateImage : public rclcpp::Node
{
public:
  explicit RotateImage(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());
  virtual ~RotateImage();

  void Publish(sensor_msgs::msg::Image::UniquePtr & msg)
  {
    _pub->publish(std::move(msg));
  }

private:
  void _InitializeParameters();
  void _UpdateParameters();

private:
  const char * _pubName = "~/image_rotated";
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr _pub;

  class _Impl;
  std::unique_ptr<_Impl> _impl;

  const char * _subName = "~/image";
  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr _sub;
};

}  // namespace rotate_image

#endif  // ROTATE_IMAGE__ROTATE_IMAGE_HPP_