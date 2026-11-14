// Copyright 2026 Joshua Whitley
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

#include <gtest/gtest.h>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <can_msgs/msg/frame.hpp>
#include <ros2_socketcan_msgs/msg/fd_frame.hpp>
#include "decanstructor2/decanstructor2_node.hpp"

using namespace std::chrono_literals;

class MonitorNodeTest : public ::testing::Test
{
protected:
  void SetUp() override {rclcpp::init(0, nullptr);}
  void TearDown() override {rclcpp::shutdown();}

  template<typename Predicate>
  bool wait(rclcpp::Executor & executor, Predicate predicate)
  {
    const auto deadline = std::chrono::steady_clock::now() + 5s;
    do {
      executor.spin_some();
      if (predicate()) {return true;}
      std::this_thread::sleep_for(10ms);
    } while (std::chrono::steady_clock::now() < deadline);
    return false;
  }
};

TEST_F(MonitorNodeTest, ConstructionReturnsWithoutDisplayAndDestructionKeepsContextAlive)
{
  {
    auto node = std::make_shared<DeCANstructor2::DC2RosNode>(rclcpp::NodeOptions());
    EXPECT_EQ(node->get_name(), std::string("decanstructor2_ros_node"));
  }
  EXPECT_TRUE(rclcpp::ok());
}

TEST_F(MonitorNodeTest, EmptyTopicsDisableOnlyTheirSubscriptions)
{
  for (const auto & topics : {
    std::pair<std::string, std::string>{"", "/dc2_test/enabled_fd"},
    {"/dc2_test/enabled_classic", ""}, {"", ""}})
  {
    rclcpp::NodeOptions options;
    options.parameter_overrides({{"classic_topic", topics.first}, {"fd_topic", topics.second}});
    auto monitor = std::make_shared<DeCANstructor2::DC2RosNode>(options);
    const auto classic_count = monitor->count_subscribers("/dc2_test/enabled_classic");
    EXPECT_EQ(classic_count, topics.first.empty() ? 0U : 1U);
    EXPECT_EQ(monitor->count_subscribers("/dc2_test/enabled_fd"), topics.second.empty() ? 0U : 1U);
    EXPECT_EQ(monitor->count_subscribers("/can_in"), 0U);
    EXPECT_EQ(monitor->count_subscribers("/can_fd_in"), 0U);
    EXPECT_TRUE(monitor->model().rows().empty());
  }
  EXPECT_TRUE(rclcpp::ok());
}

TEST_F(MonitorNodeTest, ReceivesClassicAnd64ByteFdWithoutIdentityCollisions)
{
  rclcpp::NodeOptions options;
  const std::vector<rclcpp::Parameter> parameters = {
    {"classic_topic", "/dc2_test/classic"}, {"fd_topic", "/dc2_test/fd"}};
  options.parameter_overrides(parameters);
  auto monitor = std::make_shared<DeCANstructor2::DC2RosNode>(options);
  auto producer = std::make_shared<rclcpp::Node>("dc2_test_producer");
  auto classic_pub = producer->create_publisher<can_msgs::msg::Frame>(
    "/dc2_test/classic", rclcpp::QoS(10).best_effort());
  auto fd_pub = producer->create_publisher<ros2_socketcan_msgs::msg::FdFrame>(
    "/dc2_test/fd", rclcpp::QoS(10));
  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(monitor);
  executor.add_node(producer);
  const auto subscribed = [&]() {
      return classic_pub->get_subscription_count() == 1 && fd_pub->get_subscription_count() == 1;
    };
  ASSERT_TRUE(wait(executor, subscribed));
  can_msgs::msg::Frame classic;
  classic.id = 0x123;
  classic.dlc = 2;
  classic.data[0] = 0xA5;
  classic.data[1] = 0x5A;
  ros2_socketcan_msgs::msg::FdFrame fd;
  fd.id = classic.id;
  fd.len = 64;
  fd.data.resize(64);
  for (size_t i = 0; i < fd.data.size(); ++i) {
    fd.data[i] = i;
  }
  classic_pub->publish(classic);
  fd_pub->publish(fd);
  ASSERT_TRUE(wait(executor, [&]() {return monitor->model().rows().size() == 2;}));
  for (const auto & entry : monitor->model().rows()) {
    const auto & state = entry.second;
    EXPECT_EQ(state.count, 1U);
    if (entry.first.format == DeCANstructor2::FrameFormat::FD) {
      EXPECT_EQ(state.frame.data.size(), 64U);
      EXPECT_EQ(state.frame.data[63], 63U);
    } else {
      EXPECT_EQ(state.frame.data, (std::vector<uint8_t>{0xA5, 0x5A}));
    }
  }
  fd.data[63] = 0xFF;
  fd_pub->publish(fd);
  const auto fd_received_twice = [&]() {
      for (const auto & entry : monitor->model().rows()) {
        if (entry.first.format == DeCANstructor2::FrameFormat::FD) {
          return entry.second.count == 2;
        }
      }
      return false;
    };
  ASSERT_TRUE(wait(executor, fd_received_twice));
  for (const auto & entry : monitor->model().rows()) {
    if (entry.first.format == DeCANstructor2::FrameFormat::FD) {
      EXPECT_EQ(entry.second.frame.data[63], 0xFF);
      EXPECT_TRUE(entry.second.changed_at_ns[63].has_value());
      EXPECT_FALSE(entry.second.changed_at_ns[0].has_value());
      EXPECT_TRUE(entry.second.average_interval_ms.has_value());
    }
  }
  fd.header.frame_id = "another_bus";
  fd_pub->publish(fd);
  ASSERT_TRUE(wait(executor, [&]() {return monitor->model().rows().size() == 3;}));
}

TEST_F(MonitorNodeTest, RejectsMalformedPayloadAndKeepsClassicRemoteFramesPayloadFree)
{
  auto monitor = std::make_shared<DeCANstructor2::DC2RosNode>(rclcpp::NodeOptions());
  auto producer = std::make_shared<rclcpp::Node>("dc2_invalid_producer");
  auto fd_pub = producer->create_publisher<ros2_socketcan_msgs::msg::FdFrame>("can_fd_in", 10);
  auto classic_pub = producer->create_publisher<can_msgs::msg::Frame>("can_in", 10);
  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(monitor);
  executor.add_node(producer);
  const auto subscribed = [&]() {
      return classic_pub->get_subscription_count() == 1 && fd_pub->get_subscription_count() == 1;
    };
  ASSERT_TRUE(wait(executor, subscribed));
  ros2_socketcan_msgs::msg::FdFrame fd;
  fd.id = 0x123;
  fd.len = 64;
  fd.data.resize(2);
  fd_pub->publish(fd);
  ASSERT_TRUE(wait(executor, [&]() {return monitor->rejected_frames() == 1;}));
  EXPECT_TRUE(monitor->model().rows().empty());
  can_msgs::msg::Frame classic;
  classic.id = 0x123;
  classic.is_rtr = true;
  classic.dlc = 8;
  classic_pub->publish(classic);
  ASSERT_TRUE(wait(executor, [&]() {return monitor->model().rows().size() == 1;}));
  const auto & frame = monitor->model().rows().begin()->second.frame;
  EXPECT_TRUE(frame.key.remote);
  EXPECT_EQ(frame.length, 8);
  EXPECT_TRUE(frame.data.empty());
}

TEST_F(MonitorNodeTest, RejectsInvalidClassicAndFdFramesWithoutChangingExistingData)
{
  auto monitor = std::make_shared<DeCANstructor2::DC2RosNode>(rclcpp::NodeOptions());
  auto producer = std::make_shared<rclcpp::Node>("dc2_invalid_fields_producer");
  auto classic_pub = producer->create_publisher<can_msgs::msg::Frame>("can_in", 10);
  auto fd_pub = producer->create_publisher<ros2_socketcan_msgs::msg::FdFrame>("can_fd_in", 10);
  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(monitor);
  executor.add_node(producer);
  const auto subscribed = [&]() {
      return classic_pub->get_subscription_count() == 1 && fd_pub->get_subscription_count() == 1;
    };
  ASSERT_TRUE(wait(executor, subscribed));

  can_msgs::msg::Frame classic;
  classic.id = 0x123;
  classic.dlc = 1;
  classic.data[0] = 0xA5;
  classic_pub->publish(classic);
  ros2_socketcan_msgs::msg::FdFrame fd;
  fd.id = 0x123;
  fd.len = 1;
  fd.data.resize(1);
  fd.data[0] = 0x5A;
  fd_pub->publish(fd);
  ASSERT_TRUE(wait(executor, [&]() {return monitor->model().rows().size() == 2;}));

  auto verify_unchanged = [&]() {
      ASSERT_EQ(monitor->model().rows().size(), 2U);
      for (const auto & entry : monitor->model().rows()) {
        const auto & state = entry.second;
        EXPECT_EQ(state.count, 1U);
        EXPECT_EQ(state.frame.key.id, 0x123U);
        const uint8_t expected = entry.first.format ==
          DeCANstructor2::FrameFormat::FD ? 0x5A : 0xA5;
        EXPECT_EQ(state.frame.data, (std::vector<uint8_t>{expected}));
      }
    };

  classic.dlc = 9;
  classic_pub->publish(classic);
  ASSERT_TRUE(wait(executor, [&]() {return monitor->rejected_frames() == 1;}));
  verify_unchanged();
  classic.dlc = 1;
  classic.id = 0x800;
  classic_pub->publish(classic);
  ASSERT_TRUE(wait(executor, [&]() {return monitor->rejected_frames() == 2;}));
  verify_unchanged();

  fd.id = 0x800;
  fd_pub->publish(fd);
  ASSERT_TRUE(wait(executor, [&]() {return monitor->rejected_frames() == 3;}));
  verify_unchanged();
  fd.id = 0x123;
  fd.len = 65;
  fd.data.resize(64);
  fd_pub->publish(fd);
  ASSERT_TRUE(wait(executor, [&]() {return monitor->rejected_frames() == 4;}));
  verify_unchanged();
}
