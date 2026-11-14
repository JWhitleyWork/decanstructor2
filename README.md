# DeCANstructor2

DeCANstructor2 is a receive-only ROS 2 monitor for classic CAN and CAN-FD frames.
The supported distro targets are Humble, Jazzy, Kilted, Lyrical, and Rolling.

## Monitor

![DeCANstructor2 graphite interface showing a classic CAN row and two complete CAN FD byte matrices](docs/screenshots/monitor.png)

*Actual application capture using synthetic ROS messages, not a design mockup.*

The table is designed for spotting payload changes without opening an inspector:

- **Classic CAN:** compact rows with up to eight bytes.
- **CAN FD:** an always-expanded 8×8 byte matrix in every row, including
  unselected frames. Unused offsets show `--`, not invented data.
- **Change highlighting:** individual bytes turn amber for one second after a
  change. Small hexadecimal row offsets identify positions within an FD payload.
- **Frame metadata:** select a row to see its source, ID, format and flags in the
  sidebar; the payload stays in the table.

Count and mean receipt interval remain alongside each payload. The interval is
measured with a steady clock and is not a bus timestamp. Scroll through longer
stream lists without collapsing FD payloads. Press **Escape**, close the window,
or use **Ctrl+C** in the launching terminal to exit.

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

For tests, install `xvfb` and `xauth` before building (tests are enabled by default). Then run:

```bash
colcon test
colcon test-result --verbose
```

The CI container additionally runs an installed-app graphical smoke test under
Xvfb using `tools/run-tests.sh`; its image includes `xdotool` and ImageMagick.
It publishes a classic stream and two distinct 64-byte FD streams, and captures
the default and resized windows for visual inspection. The stream-count check
does not verify the appearance or contents of those screenshots.

The manifest currently pins the small SFML 2/3 compatibility fix on the vendor
fork because write access to the upstream vendor is not available. TGUI remains
at v1.10.0.

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
count and mean interval between receptions measured with a steady clock.
The graphite-themed table keeps classic rows compact and shows every FD payload
in its row as an 8-by-8 byte matrix, with placeholders for unused bytes. Bytes
changed within the last second have an amber highlight. The details panel shows
metadata only; selecting a frame is not needed to see its payload. The monitor
holds at most 4096 streams: once full, new streams are rejected while existing
streams continue.

## Development checks

See [CONTRIBUTING.md](CONTRIBUTING.md#quick-checks) to install the pre-commit
hook and run fast checks without ROS or Docker. These include repository hygiene,
YAML/XML and Bash syntax checks, and standalone frame-model unit tests. CI runs
the same quick checks before the five-distribution ROS build, test and GUI smoke
matrix; the quick checks do not replace that integration coverage.

## Scope and limitations

This milestone only receives ROS messages; it does not transmit frames, decode
DBC files, or provide a CAN driver. A separate driver or publisher is needed.
The `FdFrame` message does not expose CAN-FD BRS or ESI flags, so they cannot be
shown or distinguished by this monitor.
