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

#include "line_center_reconstruction/line_center_reconstruction.hpp"

#include <deque>
#include <exception>
#include <memory>
#include <utility>
#include <vector>

#include "opencv2/opencv.hpp"

namespace line_center_reconstruction
{

using shared_interfaces::msg::LineCenter;
using sensor_msgs::msg::PointCloud2;
using sensor_msgs::msg::PointField;

class LineCenterReconstruction::_Impl
{
public:
  explicit _Impl(LineCenterReconstruction * ptr)
  : _node(ptr)
  {
    _InitializeParameters();
    _UpdateParameters();

    _thread = std::thread(&LineCenterReconstruction::_Impl::_Worker, this);
  }

  ~_Impl()
  {
    _con.notify_all();
    _thread.join();
  }

  void PushBack(LineCenter::UniquePtr & ptr)
  {
    std::unique_lock<std::mutex> lk(_mutex);
    _deq.emplace_back(std::move(ptr));
    lk.unlock();
    _con.notify_all();
  }

private:
  void _InitializeParameters()
  {
    std::vector<double> temp;
    _node->declare_parameter("camera_matrix", temp);
    _node->declare_parameter("distort_coeffs", temp);
    _node->declare_parameter("homography_matrix", temp);
  }

  void _UpdateParameters()
  {
    std::vector<double> c, d, h;
    _node->get_parameter("camera_matrix", c);
    _node->get_parameter("distort_coeffs", d);
    _node->get_parameter("homography_matrix", h);

    _coef = cv::Mat(3, 3, CV_64F, c.data()).clone();
    _dist = cv::Mat(1, 5, CV_64F, d.data()).clone();
    _H = cv::Mat(3, 3, CV_64F, h.data()).clone();
  }

  void _Worker()
  {
    cv::Mat dx;

    while (rclcpp::ok()) {
      std::unique_lock<std::mutex> lk(_mutex);
      if (_deq.empty() == false) {
        auto ptr = std::move(_deq.front());
        _deq.pop_front();
        lk.unlock();
        _Execute(ptr);
      } else {
        _con.wait(lk);
      }
    }
  }

  void _Execute(LineCenter::UniquePtr & ptr)
  {
    if (ptr->header.frame_id == "-1") {
      auto pnts = std::make_unique<PointCloud2>();
      pnts->header = ptr->header;
      _node->Publish(pnts);
    } else {
      std::vector<cv::Point2f> line, temp;
      line.reserve(ptr->center.size());
      temp.reserve(ptr->center.size());
      for (size_t i = 0; i < ptr->center.size(); ++i) {
        if (ptr->center[i] != -1) {
          line.emplace_back(ptr->center[i], i);
        }
      }

      if (line.empty()) {
        return;
      }

      cv::undistortPoints(line, temp, _coef, _dist, cv::noArray(), _coef);
      cv::perspectiveTransform(temp, line, _H);

      std::vector<float> xyz;
      xyz.reserve(line.size() * 3);
      for (const auto & p : line) {
        xyz.push_back(0);
        xyz.push_back(p.x);
        xyz.push_back(p.y);
      }

      auto pnts = _ConstructPointCloud2(line.size(), xyz.data());

      _node->Publish(pnts);
    }
  }

  PointCloud2::UniquePtr _ConstructPointCloud2(size_t num, const void * src)
  {
    auto pnts = std::make_unique<PointCloud2>();

    pnts->height = 1;
    pnts->width = num;

    pnts->fields.resize(3);
    pnts->fields[0].name = "x";
    pnts->fields[0].offset = 0;
    pnts->fields[0].datatype = 7;
    pnts->fields[0].count = 1;

    pnts->fields[1].name = "y";
    pnts->fields[1].offset = 4;
    pnts->fields[1].datatype = 7;
    pnts->fields[1].count = 1;

    pnts->fields[2].name = "z";
    pnts->fields[2].offset = 8;
    pnts->fields[2].datatype = 7;
    pnts->fields[2].count = 1;

    pnts->is_bigendian = false;
    pnts->point_step = 4 * 3;
    pnts->row_step = 12 * num;

    pnts->data.resize(12 * num);

    pnts->is_dense = true;

    memcpy(pnts->data.data(), src, 12 * num);

    return pnts;
  }

private:
  LineCenterReconstruction * _node;
  cv::Mat _coef, _dist, _H;
  std::mutex _mutex;              ///< Mutex to protect shared storage
  std::condition_variable _con;   ///< Conditional variable rely on mutex
  std::deque<LineCenter::UniquePtr> _deq;
  std::thread _thread;
};

LineCenterReconstruction::LineCenterReconstruction(const rclcpp::NodeOptions & options)
: Node("line_center_reconstruction_node", options)
{
  _pub = this->create_publisher<PointCloud2>(_pubName, rclcpp::SensorDataQoS());

  _impl = std::make_unique<_Impl>(this);

  _sub = this->create_subscription<LineCenter>(
    _subName,
    rclcpp::SensorDataQoS(),
    [this](LineCenter::UniquePtr ptr)
    {
      _impl->PushBack(ptr);
    }
  );

  RCLCPP_INFO(this->get_logger(), "Ininitialized successfully");
}

LineCenterReconstruction::~LineCenterReconstruction()
{
  _sub.reset();
  _impl.reset();
  _pub.reset();

  RCLCPP_INFO(this->get_logger(), "Destroyed successfully");
}

}  // namespace line_center_reconstruction

#include "rclcpp_components/register_node_macro.hpp"

// Register the component with class_loader.
// This acts as a sort of entry point, allowing the component to be discoverable when its library
// is being loaded into a running process.
RCLCPP_COMPONENTS_REGISTER_NODE(line_center_reconstruction::LineCenterReconstruction)