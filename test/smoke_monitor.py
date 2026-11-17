#!/usr/bin/env python3
# Copyright 2026 Joshua Whitley
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

"""Installed-app smoke with synthetic ROS frames; run under xvfb-run."""
import os
from pathlib import Path
import signal
import struct
import subprocess
import sys
import time

from can_msgs.msg import Frame
import rclpy
from rclpy.qos import qos_profile_sensor_data
from ros2_socketcan_msgs.msg import FdFrame


def wait_until(predicate, timeout=8):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if predicate():
            return
        time.sleep(0.02)
    raise AssertionError('Timed out waiting for application/window/subscriptions')


def capture(window, screenshot, size):
    subprocess.run(['import', '-window', window, str(screenshot)], check=True)
    with screenshot.open('rb') as image:
        header = image.read(24)
    assert header[:8] == b'\x89PNG\r\n\x1a\n'
    assert struct.unpack('>II', header[16:24]) == size, (
        f'screenshot must contain the entire {size[0]}x{size[1]} client')


def main():
    output = Path(sys.argv[1])
    output.mkdir(parents=True, exist_ok=True)
    rclpy.init()
    node = rclpy.create_node('dc2_smoke_producer')
    classic_pub = node.create_publisher(Frame, 'can_in', qos_profile_sensor_data)
    fd_pub = node.create_publisher(FdFrame, 'can_fd_in', 10)
    try:
        for close_mode in ('escape', 'signal'):
            with (output / f'app-{close_mode}.log').open('w') as log:
                process = subprocess.Popen(
                    ['ros2', 'run', 'decanstructor2', 'decanstructor2_node_exe'],
                    stdout=log, stderr=subprocess.STDOUT, start_new_session=True,
                )
                try:
                    wait_until(lambda: classic_pub.get_subscription_count() > 0
                               and fd_pub.get_subscription_count() > 0)
                    window = None

                    def find_window():
                        nonlocal window
                        result = subprocess.run(
                            ['xdotool', 'search', '--name', '^DeCANstructor2'],
                            capture_output=True, text=True, check=False,
                        )
                        if result.returncode == 0 and result.stdout.strip():
                            window = result.stdout.splitlines()[0]
                            return True
                        return False

                    wait_until(find_window)
                    assert window is not None
                    classic = Frame(id=0x123, dlc=8, data=[0xA5, 0x5A, 1, 2, 3, 4, 5, 6])
                    fd_a = FdFrame(id=0x18DAF110, is_extended=True, len=64,
                                   data=list(range(64)))
                    fd_b = FdFrame(id=0x18DAF111, is_extended=True, len=64,
                                   data=list(range(0x80, 0xC0)))
                    for i in range(30):
                        fd_a.data[63] = 0xEE if i % 2 else 0xFF
                        fd_b.data[56] = 0xAA if i % 2 else 0xBB
                        classic_pub.publish(classic)
                        fd_pub.publish(fd_a)
                        fd_pub.publish(fd_b)
                        time.sleep(0.03)
                    wait_until(lambda: '3 message IDs' in subprocess.check_output(
                        ['xdotool', 'getwindowname', window], text=True))
                    if close_mode == 'escape':
                        # With no window manager, keep the client fully on-screen and
                        # click physical screen coordinates inside the sidebar.
                        for width, height, suffix in (
                            (1280, 800, ''), (1000, 600, '-resized')
                        ):
                            subprocess.run(
                                ['xdotool', 'windowsize', window, str(width), str(height)],
                                check=True)
                            subprocess.run(['xdotool', 'windowmove', window, '0', '0'],
                                           check=True)
                            time.sleep(0.2)
                            sidebar_x = width - 258

                            def click(x, y):
                                subprocess.run(
                                    ['xdotool', 'mousemove', '--sync', str(x), str(y),
                                     'click', '1'], check=True)
                                time.sleep(0.2)

                            click(30, 92)  # Select the classic row for metadata inspection
                            capture(window, output / f'monitor{suffix}.png', (width, height))
                            click(sidebar_x + 124 + 54, 55 + 36 + 15)  # Hide all
                            capture(window, output / f'monitor{suffix}-hidden.png',
                                    (width, height))
                            click(sidebar_x + 8 + 54, 55 + 36 + 15)  # Show all
                            capture(window, output / f'monitor{suffix}-restored.png',
                                    (width, height))
                            click(sidebar_x + 13, 55 + 88)  # First checkbox
                            capture(window, output / f'monitor{suffix}-first-hidden.png',
                                    (width, height))
                            click(sidebar_x + 13, 55 + 88)
                            capture(window, output / f'monitor{suffix}-first-restored.png',
                                    (width, height))
                        # Targeted XSendEvent does not change physical key state; the
                        # window may close on keydown, so do not send it a keyup.
                        subprocess.run(
                            ['xdotool', 'keydown', '--window', window, 'Escape'], check=True)
                    else:
                        os.killpg(process.pid, signal.SIGINT)
                    result = process.wait(timeout=5)
                    assert result == 0, f'{close_mode} shutdown returned {result}'
                    print('PASS: installed GUI received three message IDs of both message types '
                          f'and exited via {close_mode}')
                finally:
                    if process.poll() is None:
                        os.killpg(process.pid, signal.SIGKILL)
                        process.wait()
            wait_until(lambda: classic_pub.get_subscription_count() == 0
                       and fd_pub.get_subscription_count() == 0)
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
