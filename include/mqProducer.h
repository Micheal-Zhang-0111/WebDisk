#pragma once

#include<string>
#include<SimpleAmqpClient/SimpleAmqpClient.h>
#include"rabbitmq.h"
using std::string;
using AmqpClient::BasicMessage;
using AmqpClient::Channel;

// 消息队列产品使用rabbitmq，部署在docker中，基于生产者-消费者模型
// C++从代码层面使用rabbitmq所需要安装的第三方库，前置库为rabbitmq-c（C使用）

//消息队列的生产者
class mqProducer
{
public:
    mqProducer();
    ~mqProducer();

    void doPublish(const string &exchange, const string &routingkey, const string &msg);

private:
    //创建一个channel
    Channel::ptr_t _channel;
};