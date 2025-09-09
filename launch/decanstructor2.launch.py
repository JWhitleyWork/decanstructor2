from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        Node(
            package='decanstructor2',
            executable='decanstructor2_node_exe',
            name='decanstructor2'
        )
    ])