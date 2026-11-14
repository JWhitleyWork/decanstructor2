// Copyright 2025 Joshua Whitley
// SPDX-License-Identifier: MIT
#include "decanstructor2/decanstructor2_node.hpp"
#include <chrono>
#include <utility>

namespace DeCANstructor2
{
namespace
{
std::string source_name(const std::string & topic, const std::string & frame_id)
{
  return frame_id.empty() ? topic : topic + " [" + frame_id + "]";
}
}  // namespace

DC2RosNode::DC2RosNode(const rclcpp::NodeOptions & options)
: rclcpp::Node("decanstructor2_ros_node", options)
{
  const auto classic_topic = declare_parameter<std::string>("classic_topic", "can_in");
  const auto fd_topic = declare_parameter<std::string>("fd_topic", "can_fd_in");
  // Best effort subscribers match both best-effort and reliable CAN publishers.
  const auto qos = rclcpp::QoS(rclcpp::KeepLast(1000)).best_effort();
  if (!classic_topic.empty()) {
    classic_subscription_ = create_subscription<can_msgs::msg::Frame>(
      classic_topic, qos,
      [this](const can_msgs::msg::Frame & message) {receive_classic(message);});
  }
  if (!fd_topic.empty()) {
    fd_subscription_ = create_subscription<ros2_socketcan_msgs::msg::FdFrame>(
      fd_topic, qos,
      [this](const ros2_socketcan_msgs::msg::FdFrame & message) {receive_fd(message);});
  }
}

void DC2RosNode::reject()
{
  ++rejected_frames_;
  RCLCPP_WARN_THROTTLE(
    get_logger(), *get_clock(), 5000, "Rejected malformed CAN frame (total: %llu)",
    static_cast<unsigned long long>(rejected_frames_));
}

void DC2RosNode::ingest(Frame frame)
{
  const auto received_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
    std::chrono::steady_clock::now().time_since_epoch()).count();
  if (!model_.ingest(frame, received_ns)) {reject();}
}

void DC2RosNode::receive_classic(const can_msgs::msg::Frame & message)
{
  if (message.dlc > message.data.size()) {reject(); return;}
  Frame frame;
  frame.key = {source_name(classic_subscription_->get_topic_name(), message.header.frame_id),
    message.id, message.is_extended, message.is_error, message.is_rtr, FrameFormat::Classic};
  frame.length = message.dlc;
  if (!message.is_rtr) {
    frame.data.assign(message.data.begin(), message.data.begin() + message.dlc);
  }
  ingest(std::move(frame));
}

void DC2RosNode::receive_fd(const ros2_socketcan_msgs::msg::FdFrame & message)
{
  if (message.len > 64 || message.len > message.data.size()) {reject(); return;}
  Frame frame;
  frame.key = {source_name(fd_subscription_->get_topic_name(), message.header.frame_id),
    message.id, message.is_extended, message.is_error, false, FrameFormat::FD};
  frame.length = message.len;
  frame.data.assign(message.data.begin(), message.data.begin() + message.len);
  ingest(std::move(frame));
}
}  // namespace DeCANstructor2

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(DeCANstructor2::DC2RosNode)
