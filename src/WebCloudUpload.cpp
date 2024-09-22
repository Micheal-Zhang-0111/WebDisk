#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <fstream>
#include <alibabacloud/oss/OssClient.h> 
#include <openssl/sha.h>
#include "WebCloudUpload.h"

using namespace AlibabaCloud::OSS;
using std::string;

struct OSSInfo
{
    string Endpoint = "";
    string BucketName = "";
    string AccessKeyID = "";
    string AccessKeySecret = "";
};

bool WebCloudUpload::writeFile(const std::string &name, const std::string &data)
{
    string path = "../tmp/" + name;
    int fd = open(path.c_str(), O_CREAT | O_RDWR, 0666);
    if (fd == -1)
    {
        perror("open");
        return false;
    }
   
    ssize_t ret = write(fd, data.c_str(), data.size());
    if (ret == -1)
    {
        perror("write");
        return false;
    }
    close(fd);
    return true;
}

string WebCloudUpload::makeSha1(const std::string &name)
{
    string path = "../tmp/" + name;
    int fd = open(path.c_str(), O_RDWR, 0666);
    if (fd == -1)
    {
        perror("open");
        return {};
    }
    
    char buff[1024] = {0};
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    while (1)
    {
        int ret = read(fd, buff, 1024);
        if (ret == 0)
            break;

        SHA1_Update(&ctx, buff, ret);
        ::memset(buff, 0, sizeof(buff));
    }

    unsigned char md[20] = {0};
    SHA1_Final(md, &ctx);
    char fragment[3] = {0};
    string result;
    for(int i = 0; i < 20; ++i)
    {
        sprintf(fragment, "%02x", md[i]);
        result += fragment;
    }
    return result;
}

// 阿里云 OSS 简单上传
bool WebCloudUpload::uploadOSS(std::string fileName)
{
    // // 初始化账号信息
    // OSSInfo info;        

    // // 初始化网络等资源
    // InitializeSdk();

    // // 配置账号信息
    // ClientConfiguration conf;
    // OssClient client(info.Endpoint, info.AccessKeyID, info.AccessKeySecret, conf);

    // // 初始化 本地文件路径 和 OSS路径
    // string realName = "../tmp/" + fileName;
    // string ObjectName = "disk/" + fileName;

    // // 读取文件内容
    // std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(realName, std::ios::in | std::ios::binary);

    // // 发起请求
    // PutObjectRequest request(info.BucketName, ObjectName, content);
    // auto outcome = client.PutObject(request);

    // // 异常处理
    // if (!outcome.isSuccess()) {
    //     std::cout << "PutObject fail" <<
    //     ",code:" << outcome.error().Code() <<
    //     ",message:" << outcome.error().Message() <<
    //     ",requestId:" << outcome.error().RequestId() << std::endl;
    //     return false;
    // }

    // // 释放网络等资源
    // ShutdownSdk();

    return true;
}

