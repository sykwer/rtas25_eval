#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/int32.hpp>
#include <rclcpp_components/register_node_macro.hpp>
#include <vector>
#include <string>
#include <thread>  // hardware_concurrency の取得
#include <unistd.h>  // getpid() のために追加
#include "static_callback_isolated_executor.hpp"

class SubscriberNode : public rclcpp::Node
{
public:
    explicit SubscriberNode(const rclcpp::NodeOptions & options)
        : Node("subscriber_node", options)
    {
        // パラメータを宣言
        this->declare_parameter("callback_group_count", 1);
        this->declare_parameter("executor_type", "single");

        this->get_parameter("callback_group_count", callback_group_count_);
        this->get_parameter("executor_type", executor_type_);

        pid_ = getpid();  // プロセスIDを取得

        for (int i = 0; i < callback_group_count_; ++i)
        {
            auto callback_group = this->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
            callback_groups_.push_back(callback_group);

            std::string topic_name = "topic" + std::to_string(i);
            rclcpp::SubscriptionOptions sub_options;
            sub_options.callback_group = callback_group;

            auto subscription = this->create_subscription<std_msgs::msg::Int32>(
                topic_name, 10,
                [this, i](const std_msgs::msg::Int32::SharedPtr msg) { this->subscription_callback(i, msg); },
                sub_options);
            subscriptions_.push_back(subscription);
        }

        RCLCPP_INFO(this->get_logger(), "SubscriberNode started with PID: %d", pid_);
        RCLCPP_INFO(this->get_logger(), "Executor type: %s", executor_type_.c_str());
    }

private:
    void subscription_callback(int index, const std_msgs::msg::Int32::SharedPtr msg)
    {
        RCLCPP_INFO(this->get_logger(), "[PID: %d] Received on %s: %d",
                    pid_, subscriptions_[index]->get_topic_name(), msg->data);
    }

    std::vector<rclcpp::CallbackGroup::SharedPtr> callback_groups_;
    std::vector<rclcpp::Subscription<std_msgs::msg::Int32>::SharedPtr> subscriptions_;
    int callback_group_count_, pid_;
    std::string executor_type_;
};

// Composable Nodeとして登録
RCLCPP_COMPONENTS_REGISTER_NODE(SubscriberNode)

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);

    auto node = std::make_shared<SubscriberNode>(rclcpp::NodeOptions());

    int callback_group_count;
    node->get_parameter("callback_group_count", callback_group_count);

    int thread_num = std::min(callback_group_count, static_cast<int>(std::thread::hardware_concurrency()));

    std::string executor_type;
    node->get_parameter("executor_type", executor_type);

    if (executor_type == "multi")
    {
        auto executor = std::make_shared<rclcpp::executors::MultiThreadedExecutor>(
            rclcpp::ExecutorOptions(), thread_num);
        RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "Using MultiThreadedExecutor with %d threads", thread_num);

        executor->add_node(node);
        executor->spin();
    }
    else if (executor_type == "isolated") {
        auto executor = std::make_shared<StaticCallbackIsolatedExecutor>();
        RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "Using StaticCallbackIsolatedExecutor");

        executor->add_node(node);
        executor->spin();
    }
    else
    {
        auto executor = std::make_shared<rclcpp::executors::SingleThreadedExecutor>();
        RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "Using SingleThreadedExecutor");

        executor->add_node(node);
        executor->spin();
    }

    rclcpp::shutdown();
    return 0;
}
