// Copyright 2025 Joshua Whitley
// SPDX-License-Identifier: MIT
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
