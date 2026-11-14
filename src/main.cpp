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

#include <chrono>
#include <exception>
#include <iostream>
#include <memory>
#include "decanstructor2/decanstructor2_node.hpp"
#include "decanstructor2/monitor_window.hpp"

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  int result = 0;
  try {
    auto node = std::make_shared<DeCANstructor2::DC2RosNode>(rclcpp::NodeOptions());
    rclcpp::executors::SingleThreadedExecutor executor;
    executor.add_node(node);
    DeCANstructor2::MonitorWindow window;
    while (rclcpp::ok() && window.process_events()) {
      executor.spin_some(std::chrono::milliseconds(5));
      if (!rclcpp::ok()) {break;}
      const auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
      window.update(node->model(), node->rejected_frames(), now_ns);
      window.draw();
    }
  } catch (const std::exception & error) {
    if (rclcpp::ok()) {
      std::cerr << "DeCANstructor2: " << error.what() << '\n';
      result = 1;
    }
  }
  if (rclcpp::ok()) {rclcpp::shutdown();}
  return result;
}
