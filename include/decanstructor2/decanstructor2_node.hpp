// Copyright 2025 Joshua Whitley
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef DECANSTRUCTOR2__DECANSTRUCTOR2_NODE_HPP_
#define DECANSTRUCTOR2__DECANSTRUCTOR2_NODE_HPP_

#include <cstdint>
#include <string>
#include <can_msgs/msg/frame.hpp>
#include <rclcpp/rclcpp.hpp>
#include <ros2_socketcan_msgs/msg/fd_frame.hpp>
#include "decanstructor2/frame_model.hpp"

namespace DeCANstructor2
{
// Callbacks and model reads run on the application's single-threaded executor.
class DC2RosNode : public rclcpp::Node
{
public:
  explicit DC2RosNode(const rclcpp::NodeOptions & options);
  const FrameModel & model() const {return model_;}
  uint64_t rejected_frames() const {return rejected_frames_;}

private:
  void receive_classic(const can_msgs::msg::Frame & message);
  void receive_fd(const ros2_socketcan_msgs::msg::FdFrame & message);
  void reject();
  void ingest(Frame frame);

  FrameModel model_;
  uint64_t rejected_frames_ = 0;
  rclcpp::Subscription<can_msgs::msg::Frame>::SharedPtr classic_subscription_;
  rclcpp::Subscription<ros2_socketcan_msgs::msg::FdFrame>::SharedPtr fd_subscription_;
};
}  // namespace DeCANstructor2
#endif  // DECANSTRUCTOR2__DECANSTRUCTOR2_NODE_HPP_
