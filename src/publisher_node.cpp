#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/int32.hpp>
#include <rclcpp_components/register_node_macro.hpp>
#include <vector>
#include <string>
#include <thread>  // hardware_concurrency の取得
#include <unistd.h>  // getpid() のために追加
#include "static_callback_isolated_executor.hpp"

class PublisherNode : public rclcpp::Node
{
public:
    explicit PublisherNode(const rclcpp::NodeOptions & options)
        : Node("publisher_node", options)
    {
        // パラメータを宣言
        this->declare_parameter("callback_group_count", 1);
        this->declare_parameter("timer_period", 1000);
        this->declare_parameter("executor_type", "single");

        this->get_parameter("callback_group_count", callback_group_count_);
        this->get_parameter("timer_period", timer_period_);
        this->get_parameter("executor_type", executor_type_);

        pid_ = getpid();  // プロセスIDを取得

        for (int i = 0; i < callback_group_count_; ++i)
        {
            auto callback_group = this->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
            callback_groups_.push_back(callback_group);

            std::string topic_name = "topic" + std::to_string(i);
            auto publisher = this->create_publisher<std_msgs::msg::Int32>(topic_name, 10);
            publishers_.push_back(publisher);

            auto timer = this->create_wall_timer(
                std::chrono::milliseconds(timer_period_),
                [this, i]() { this->timer_callback(i); },
                callback_group);
            timers_.push_back(timer);
        }

        RCLCPP_INFO(this->get_logger(), "PublisherNode started with PID: %d", pid_);
        RCLCPP_INFO(this->get_logger(), "Executor type: %s", executor_type_.c_str());
    }

private:
    void timer_callback(int index)
    {
        auto message = std_msgs::msg::Int32();
        message.data = index;
        publishers_[index]->publish(message);
        RCLCPP_INFO(this->get_logger(), "[PID: %d] Published on %s: %d",
                    pid_, publishers_[index]->get_topic_name(), message.data);
    }

    std::vector<rclcpp::CallbackGroup::SharedPtr> callback_groups_;
    std::vector<rclcpp::Publisher<std_msgs::msg::Int32>::SharedPtr> publishers_;
    std::vector<rclcpp::TimerBase::SharedPtr> timers_;
    int callback_group_count_, timer_period_, pid_;
    std::string executor_type_;
};

// Composable Nodeとして登録
RCLCPP_COMPONENTS_REGISTER_NODE(PublisherNode)

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);

    // ノードを作成
    auto node = std::make_shared<PublisherNode>(rclcpp::NodeOptions());

    // callback_group_count を取得
    int callback_group_count;
    node->get_parameter("callback_group_count", callback_group_count);

    // hardware concurrency を考慮してスレッド数を決定
    int thread_num = std::min(callback_group_count, static_cast<int>(std::thread::hardware_concurrency()));

    // エグゼキュータを選択
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
