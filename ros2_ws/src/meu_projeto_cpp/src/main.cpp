#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);

    // Criar Node
    auto node = rclcpp::Node::make_shared("publisher_node");

    // Criar Publisher no Topic /ola
    auto publisher = node->create_publisher<std_msgs::msg::String>(
        "/ola", 10
    );

    // Criar mensagem
    std_msgs::msg::String msg;
    msg.data = "Ola";

   
    // Publicar mensagem
    publisher->publish(msg);

    RCLCPP_INFO(node->get_logger(), "Mensagem publicada: Ola");
    
    rclcpp::shutdown();
    return 0;
}