#pragma once

#include <string>

class WebCloudUpload
{
public:
    static bool writeFile(const std::string &name, const std::string &data);

    static std::string makeSha1(const std::string &name);

    // 阿里云 OSS 简单上传
    static bool uploadOSS(std::string fileName);
};