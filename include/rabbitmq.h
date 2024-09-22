#include<string>

struct RabbitMqInfo{
    std::string RabbitURL = "amqp://guest:guest@127.0.0.1:5672";
    std::string TransExchangeName = "WebDisk_oss_exchange";
    std::string TransQueueName = "WebDisk_oss_queue1";
    std::string TransRoutingKey = "oss";
};