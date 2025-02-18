cmake_minimum_required(VERSION 3.8)
project(rtas25_eval)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(std_msgs REQUIRED)
find_package(rclcpp_component_container_callback_isolated REQUIRED)

add_library(publisher_node SHARED src/publisher_node.cpp)
ament_target_dependencies(publisher_node rclcpp rclcpp_components std_msgs rclcpp_component_container_callback_isolated)
target_include_directories(publisher_node PRIVATE ${rclcpp_component_container_callback_isolated_INCLUDE_DIRS})
rclcpp_components_register_nodes(publisher_node "PublisherNode")

add_library(subscriber_node SHARED src/subscriber_node.cpp)
ament_target_dependencies(subscriber_node rclcpp rclcpp_components std_msgs rclcpp_component_container_callback_isolated)
target_include_directories(subscriber_node PRIVATE ${rclcpp_component_container_callback_isolated_INCLUDE_DIRS})
rclcpp_components_register_nodes(subscriber_node "SubscriberNode")

add_executable(publisher_node_exec src/publisher_node.cpp)
ament_target_dependencies(publisher_node_exec rclcpp rclcpp_components std_msgs rclcpp_component_container_callback_isolated)
target_include_directories(publisher_node_exec PRIVATE ${rclcpp_component_container_callback_isolated_INCLUDE_DIRS})
target_link_libraries(publisher_node_exec publisher_node)  # publisher_node ライブラリをリンク

add_executable(subscriber_node_exec src/subscriber_node.cpp)
ament_target_dependencies(subscriber_node_exec rclcpp rclcpp_components std_msgs rclcpp_component_container_callback_isolated)
target_include_directories(subscriber_node_exec PRIVATE ${rclcpp_component_container_callback_isolated_INCLUDE_DIRS})
target_link_libraries(subscriber_node_exec subscriber_node)  # subscriber_node ライブラリをリンク

install(TARGETS
  publisher_node
  subscriber_node
  publisher_node_exec
  subscriber_node_exec
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY
  launch
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
