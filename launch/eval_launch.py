import launch
import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, LogInfo, OpaqueFunction
from launch.conditions import IfCondition, UnlessCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import ComposableNodeContainer, Node
from launch_ros.descriptions import ComposableNode

def get_limited_thread_num(callback_group_count):
    """callback_group_count * 2 にしつつ、最大スレッド数 (hardware concurrency) を超えないよう制限"""
    max_threads = os.cpu_count()  # ハードウェアの最大スレッド数
    return min(callback_group_count * 2, max_threads)  # 制限を適用

def create_composable_nodes():
    """PublisherNode と SubscriberNode の ComposableNode を作成"""
    return [
        ComposableNode(
            package='rtas25_eval',
            plugin='PublisherNode',
            name='publisher_node',
            parameters=[
                {'timer_period': LaunchConfiguration('timer_period')},
                {'callback_group_count': LaunchConfiguration('callback_group_count')},
                {'executor_type': LaunchConfiguration('executor')}
            ]
        ),
        ComposableNode(
            package='rtas25_eval',
            plugin='SubscriberNode',
            name='subscriber_node',
            parameters=[
                {'callback_group_count': LaunchConfiguration('callback_group_count')},
                {'executor_type': LaunchConfiguration('executor')}
            ]
        ),
    ]

def create_container(context, *args, **kwargs):
    """ComposableNodeContainer を作成"""
    executor = LaunchConfiguration('executor').perform(context)
    callback_group_count = int(LaunchConfiguration('callback_group_count').perform(context))

    # スレッド数の制限を適用
    thread_num = get_limited_thread_num(callback_group_count)

    if executor == 'multi':
        executable = 'component_container_mt'
        container = ComposableNodeContainer(
            name='container',
            namespace='',
            package='rclcpp_components',
            executable=executable,
            parameters=[{'thread_num': thread_num}],  # 制限適用後のスレッド数を設定
            composable_node_descriptions=create_composable_nodes(),
            output='screen',
            condition=UnlessCondition(LaunchConfiguration('separate_process'))
        )
    elif executor == 'isolated':
        executable = 'component_container_callback_isolated'
        container = ComposableNodeContainer(
            name='container',
            namespace='',
            package='rclcpp_component_container_callback_isolated',
            executable=executable,
            composable_node_descriptions=create_composable_nodes(),
            output='screen',
            condition=UnlessCondition(LaunchConfiguration('separate_process'))
        )
    else:
        executable = 'component_container'
        container = ComposableNodeContainer(
            name='container',
            namespace='',
            package='rclcpp_components',
            executable=executable,
            composable_node_descriptions=create_composable_nodes(),
            output='screen',
            condition=UnlessCondition(LaunchConfiguration('separate_process'))
        )
    return [container]

def create_separate_nodes():
    """別プロセスで PublisherNode と SubscriberNode を実行する Node を作成"""
    return [
        Node(
            package='rtas25_eval',
            executable='publisher_node_exec',
            name='publisher_node',
            output='screen',
            parameters=[
                {'timer_period': LaunchConfiguration('timer_period')},
                {'callback_group_count': LaunchConfiguration('callback_group_count')},
                {'executor_type': LaunchConfiguration('executor')}
            ],
            condition=IfCondition(LaunchConfiguration('separate_process'))
        ),
        Node(
            package='rtas25_eval',
            executable='subscriber_node_exec',
            name='subscriber_node',
            output='screen',
            parameters=[
                {'callback_group_count': LaunchConfiguration('callback_group_count')},
                {'executor_type': LaunchConfiguration('executor')}
            ],
            condition=IfCondition(LaunchConfiguration('separate_process'))
        ),
    ]

def generate_launch_description():
    """Launch ファイルのエントリポイント"""
    return LaunchDescription([
        DeclareLaunchArgument(
            'callback_group_count',
            default_value='1',
            description='Number of callback groups for Publisher and Subscriber'
        ),
        DeclareLaunchArgument(
            'separate_process',
            default_value='false',
            description='Whether to run PublisherNode and SubscriberNode in separate processes'
        ),
        DeclareLaunchArgument(
            'timer_period',
            default_value='1000',
            description='Timer period in milliseconds'
        ),
        DeclareLaunchArgument(
            'executor',
            default_value='single',
            description='Executor type: single or multi'
        ),
        LogInfo(msg=['Executor mode: ', LaunchConfiguration('executor')]),
        OpaqueFunction(function=create_container),
        *create_separate_nodes()  # 別プロセスノードも追加
    ])
