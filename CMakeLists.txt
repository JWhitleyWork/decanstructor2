cmake_minimum_required(VERSION 3.14.4)
project(decanstructor2)

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake_auto REQUIRED)
ament_auto_find_build_dependencies()

find_package(SFGUI REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/CanEvent.msg"
  DEPENDENCIES builtin_interfaces
  ADD_LINTER_TESTS
)

ament_auto_add_executable(${PROJECT_NAME}_node
  src/decanstructor2_gui.cpp
  src/decanstructor2_node.cpp
  src/main.cpp
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
  SFGUI::SFGUI
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_auto_package()
