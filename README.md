# DeCANstructor2

DeCANstructor2 is a receive-only ROS 2 monitor for classic CAN and CAN-FD frames.
The supported distro targets are Humble, Jazzy, Kilted, Lyrical, and Rolling.

## Build

Install ROS 2 for your distro, `git`, `python3-vcstool`, `python3-rosdep`,
`python3-colcon-common-extensions`, and the development tools for your platform.
Initialize rosdep if necessary (`sudo rosdep init`, then `rosdep update`). In a
new workspace, import the vendor dependency from the repository manifest:

```bash
export ROS_DISTRO=jazzy  # or humble, kilted, lyrical, rolling
source /opt/ros/$ROS_DISTRO/setup.bash
mkdir -p ~/dc2_ws/src
cd ~/dc2_ws
git clone https://github.com/JWhitleyWork/decanstructor2.git src/decanstructor2
vcs import . < src/decanstructor2/external.repos
rosdep install --from-paths src --ignore-src -r -y
colcon build --cmake-args -DCMAKE_BUILD_TYPE=Debug
source install/setup.bash
```

For tests, install `xvfb` and `xauth` as well. Then run:

```bash
colcon test
colcon test-result --verbose
```

The CI container additionally runs a graphical smoke test under Xvfb using
`tools/run-tests.sh`; its image includes `xdotool` and ImageMagick.

## Run

```bash
ros2 run decanstructor2 decanstructor2_node_exe
```

The `classic_topic` parameter defaults to `can_in` (`can_msgs/msg/Frame`), and
`fd_topic` defaults to `can_fd_in` (`ros2_socketcan_msgs/msg/FdFrame`). Set either
parameter to an empty string to disable that subscription. For example:

```bash
ros2 run decanstructor2 decanstructor2_node_exe --ros-args -p classic_topic:=can_in -p "fd_topic:=''"
```

Both subscriptions use best-effort QoS. Classic frames support up to 8 data
bytes and CAN-FD frames up to 64. A row identifies a stream by topic,
`header.frame_id`, CAN ID, flags and classic/FD format. It displays the receive
count and mean interval between receptions measured with a steady clock;
changed data bytes are highlighted yellow. The monitor holds at most 4096
streams: once full, new streams are rejected while existing streams continue.

## Scope and limitations

This milestone only receives ROS messages; it does not transmit frames, decode
DBC files, or provide a CAN driver. A separate driver or publisher is needed.
The `FdFrame` message does not expose CAN-FD BRS or ESI flags, so they cannot be
shown or distinguished by this monitor.
