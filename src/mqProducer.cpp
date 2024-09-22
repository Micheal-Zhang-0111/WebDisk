#include"mqProducer.h"


mqProducer::mqProducer()
:_channel(Channel::Create())//Create()无参构造默认使用guest:guest@127.0.0.1:5672
{
    //创建一条和mq的连接
}

mqProducer::~mqProducer(){

}

void mqProducer::doPublish(const string &exchange, const string &routingkey, const string &msg){
    //指定mq的一些信息
    RabbitMqInfo MqInfo;
    
    // 创建了一个消息
    BasicMessage::ptr_t message = BasicMessage::Create(msg);
    // 把消息发布给交换器
    _channel->BasicPublish(MqInfo.TransExchangeName, MqInfo.TransRoutingKey, message);
}