#include"mqConsumer.h"


mqConsumer::mqConsumer()
:_channel(Channel::Create())//Create()无参构造默认使用guest:guest@127.0.0.1:5672
{
    //创建一条和mq的连接
}

mqConsumer::~mqConsumer(){

}

AmqpClient::Envelope::ptr_t mqConsumer::doConsume(const string &queuename){
    //指定mq的一些信息
    RabbitMqInfo MqInfo;
    
    //从mq中提取消息
    _channel->BasicConsume(MqInfo.TransQueueName,MqInfo.TransQueueName);
    //创建一个信封用来获取消息
    AmqpClient::Envelope::ptr_t envelope;
    //获取消息最多等待5s
    bool isNotTimeout = _channel->BasicConsumeMessage(envelope,5000);
    if(isNotTimeout == false){
        //超时
        fprintf(stderr,"BasicConsumeMessage timeout\n");
        return nullptr;
    }
    fprintf(stderr,"message = %s\n",envelope->Message()->Body().c_str());//以字符串形式获取消息内容
    return envelope;
}