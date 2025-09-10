# Copyright 2025 Joshua Whitley
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of this
# software and associated documentation files (the "Software"), to deal in the Software
# without restriction, including without limitation the rights to use, copy, modify,
# merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
# PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
# OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

cmake_minimum_required(VERSION 3.14.4)
project(decanstructor2)

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake_auto REQUIRED)
ament_auto_find_build_dependencies()

find_package(TGUI REQUIRED)

set(ROS_DISTRO "$ENV{ROS_DISTRO}")

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/CanEvent.msg"
  DEPENDENCIES builtin_interfaces
  ADD_LINTER_TESTS
)

ament_auto_add_library(${PROJECT_NAME}_node
  src/decanstructor2_node.cpp
)

get_available_rmw_implementations(rmw_implementations2)
foreach(rmw_implementation ${rmw_implementations2})
  find_package("${rmw_implementation}" REQUIRED)
  get_rmw_typesupport(typesupport_impls "${rmw_implementation}" LANGUAGE "cpp")
  foreach(typesupport_impl ${typesupport_impls})
    rosidl_get_typesupport_target(cpp_typesupport_target "${PROJECT_NAME}" "${typesupport_impl}")
    target_link_libraries(${PROJECT_NAME}_node "${cpp_typesupport_target}")
  endforeach()
endforeach()

target_link_libraries(${PROJECT_NAME}_node
  TGUI::TGUI
)

rclcpp_components_register_node(${PROJECT_NAME}_node
  PLUGIN "DeCANstructor2::DC2RosNode"
  EXECUTABLE ${PROJECT_NAME}_node_exe
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

if(ROS_DISTRO STREQUAL "humble" OR ROS_DISTRO STREQUAL "jazzy")
  ament_auto_package(
    INSTALL_TO_SHARE launch
    USE_SCOPED_HEADER_INSTALL_DIR
  )
else()
  ament_auto_package(
    INSTALL_TO_SHARE launch
  )
endif()
