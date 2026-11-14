# Development/test environment; source stays in the caller's workspace.
ARG ROS_DISTRO=jazzy
FROM ros:${ROS_DISTRO}-ros-base
ARG ROS_DISTRO
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake git libsfml-dev xvfb xauth xdotool imagemagick \
    python3-colcon-common-extensions python3-pytest \
    ros-${ROS_DISTRO}-ament-cmake-auto \
    ros-${ROS_DISTRO}-ament-cmake-vendor-package \
    ros-${ROS_DISTRO}-ament-cmake-gtest \
    ros-${ROS_DISTRO}-ament-lint-auto ros-${ROS_DISTRO}-ament-lint-common \
    ros-${ROS_DISTRO}-can-msgs ros-${ROS_DISTRO}-ros2-socketcan-msgs \
    && rm -rf /var/lib/apt/lists/*
