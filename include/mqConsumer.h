#pragma once

#include<string>
#include<SimpleAmqpClient/SimpleAmqpClient.h>
#include"rabbitmq.h"
using std::string;
using AmqpClient::BasicMessage;
using AmqpClient::Channel;

//消息队列的消费者
class mqConsumer
{
public:
    mqConsumer();
    ~mqConsumer();

    //从队列中以取出消息，返回一个信封
    AmqpClient::Envelope::ptr_t doConsume(const string &queuename);

private:
    //创建一个channel
    Channel::ptr_t _channel;
};