// Copyright 2025 Joshua Whitley
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify,
// merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "decanstructor2/decanstructor2_node.hpp"

namespace DeCANstructor2
{

DC2RosNode::DC2RosNode(const rclcpp::NodeOptions & options)
: rclcpp::Node("decanstructor2_ros_node", options),
  window_(sf::VideoMode(800, 600), "DeCANstructor2"),
  gui_(window_)
{
  build_main_window();
  main_window_loop();
  rclcpp::shutdown();
}

void DC2RosNode::build_main_window()
{
  can_msg_list_ = tgui::ListView::create();
  can_msg_list_->setPosition(10, 10);
  can_msg_list_->setSize("58%", "parent.bottom - 20");
  can_msg_list_->setItemHeight(20);
  can_msg_list_->addColumn("ID", 100.0f);

  for (uint8_t i = 0; i < 8; i++) {
    can_msg_list_->addColumn(std::to_string(i), 25.0f, tgui::HorizontalAlignment::Center);
  }

  can_msg_list_->addColumn("Avg Rate (ms)", 100.0f, tgui::HorizontalAlignment::Right);

  for (uint8_t i = 0; i < 50; i++) {
    can_msg_list_->addItem({"0x12345678", "FF", "24", "C9", "3B", "D2", "56", "97", "DD", "5.0"});
  }

  gui_.add(can_msg_list_);
}

void DC2RosNode::main_window_loop()
{
  while (window_.isOpen()) {
    sf::Event event;
    while (window_.pollEvent(event)) {
      gui_.handleEvent(event);

      if (event.type == sf::Event::Closed) {
        window_.close();
      }
    }

    window_.clear();
    gui_.draw();
    window_.display();

    rclcpp::spin_some(this->get_node_base_interface());
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
}

}  // namespace DeCANstructor2

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(DeCANstructor2::DC2RosNode)
