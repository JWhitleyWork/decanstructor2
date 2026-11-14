#!/usr/bin/env bash
# Run inside tools/Dockerfile's image, with both source packages under /ws/src.
set -eo pipefail
source "/opt/ros/${ROS_DISTRO}/setup.bash"
cd /ws
colcon build --executor sequential --cmake-args -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
source /ws/install/setup.bash
# Preserve detailed results even when a test fails.
status=0
colcon test --return-code-on-test-failure || status=$?
colcon test-result --verbose || status=$?
if [ "$status" -ne 0 ]; then
  exit "$status"
fi
xvfb-run -a python3 /ws/src/decanstructor2/test/smoke_monitor.py /ws/smoke
