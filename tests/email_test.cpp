#include "smtp.h"
#include "iomanager.h"

void send_email() {
    auto email = sylar::EMail::Create("164510644@qq.com", "arkicmeuplxsbhcd",
                            "Hello LY", "hello", {"1263753461@qq.com"}
                            , {}, {});
    auto entity = sylar::EMailEntity::CreateAttach("/home/wwt/email.txt");
    if(entity) {
        email->addEntity(entity);
    }

    // 默认端口是465(SSL加密连接)，普通socket不使用SSL的端口是25
    auto client = sylar::SmtpClient::Create("smtp.qq.com", 465, true);
    if(!client) {
        std::cout << "connect smtp.163.com:25 fail" << std::endl;
        return;
    }

    auto result = client->send(email, 1000, true);
    std::cout << "result=" << result->result << " msg=" << result->msg << std::endl;
    std::cout << client->getDebugInfo() << std::endl;
}

int main() {
    sylar::IOManager iom(1);
    iom.scheduler(send_email);
    //iom.stop();
    return 0;
}