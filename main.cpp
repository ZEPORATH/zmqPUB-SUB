#include <spdlog/spdlog.h>
#include <iostream>
#include "zmq.hpp"
#include "zmq_addon.hpp"
#include <signal.h>
#include <thread>

using namespace std;
#define oneSecSleep         std::this_thread::sleep_for(std::chrono::seconds(1));
#define sleepForNSec(n)     std::this_thread::sleep_for(std::chrono::seconds(n));

#define fname \
    spdlog::info("{}",__PRETTY_FUNCTION__)

bool appExit = false;
void ctrlCHandler(int s)
{
    printf("Caught signal %d\n",s);
    appExit= true;
}

void recvPrint(std::string message)
{

    std::cout << __PRETTY_FUNCTION__ << " " <<  message << " " << message.size() << std::endl;
}

void errPrint(const std::string& message)
{
    spdlog::info("{} {}", __PRETTY_FUNCTION__, message);
}

class StreamServerPrivate {
public:
    StreamServerPrivate(const uint16_t& port, const std::string& ip): m_port(port), m_ip(ip)
    {
        m_socketType = zmq::socket_type::pub;
        m_socket = zmq::socket_t(m_context, m_socketType);
        m_exit = false;
    }

    ~StreamServerPrivate()
    {
        if (m_socket.connected())
        {
            m_socket.send(zmq::message_t("END"), zmq::send_flags::none);
        }
        m_context.shutdown();
    }

    void init()
    {
        fname;
        //        m_socket.bind("tcp://"+ m_ip + ":" + std::to_string(m_port));
        m_socket.bind("tcp://*:" + std::to_string(m_port));
        if (m_socket.connected())
        {
            spdlog::info("{} Server ready to publish frame", __PRETTY_FUNCTION__);
        }
    }

    void disconnect()
    {
        fname;
        m_socket.send(zmq::message_t("END"), zmq::send_flags::none);
        m_exit = false;
        try
        {
            m_context.shutdown();
        }  catch (zmq::error_t e)
        {
            spdlog::info("{} On shutdown: {}", __PRETTY_FUNCTION__, e.what());
        }
    }

    std::optional<size_t> sendMessage(const std::string& topic, const std::string& message)
    {
        fname;
        zmq::message_t topicName(topic.size());
        zmq::message_t messageData(message.size());
        memcpy(topicName.data(), topic.data(), topic.size());
        memcpy(messageData.data(), message.data(), message.size());

        auto rc = m_socket.send(topicName, zmq::send_flags::sndmore);
         rc = m_socket.send(messageData, zmq::send_flags::none);
         return rc;
    }


private:

    zmq::context_t m_context;
    zmq::socket_t m_socket;
    zmq::socket_type m_socketType;

    bool m_exit;

    uint16_t m_port;
    std::string m_ip;
};

class StreamClientPrivate {
public:
    StreamClientPrivate(const uint16_t& port, const std::string& ip): m_port(port), m_ip(ip)
    {
        m_socketType = zmq::socket_type::sub;
        m_socket = zmq::socket_t(m_context, m_socketType);
        m_exit = false;
    }

    ~StreamClientPrivate()
    {
        m_context.shutdown();
    }

    void init(const std::string& topicName)
    {
        fname;
        m_socket.connect("tcp://"+ m_ip + ":" + std::to_string(m_port));
        m_socket.setsockopt(ZMQ_SUBSCRIBE, topicName.c_str(), topicName.size());
        //        m_socket.connect("tcp://*:" + std::to_string(m_port));
        if (m_socket.connected())
        {
            spdlog::info("{} Client ready to sub and listen frame", __PRETTY_FUNCTION__);
        }
    }

    void disconnect()
    {
        fname;
        m_exit = false;
        try {
            m_context.shutdown();
        }  catch (zmq::error_t e)
        {
            spdlog::info("{} On shutdown: {}", __PRETTY_FUNCTION__, e.what());
        }
    }

    std::optional<size_t> recvMessage(const std::string& topic, std::string& message)
    {
        fname;
        if (!m_socket.connected())
        {
            spdlog::error("{} socket uninitialized", __PRETTY_FUNCTION__);
            return std::nullopt;
        }
        zmq::message_t env;
        m_socket.recv(&env);
        std::string env_str = std::string(static_cast<char*>(env.data()), env.size());
        std::cout << "Received envelope '" << env_str << "'" << std::endl;

        zmq::message_t msg;
        m_socket.recv(&msg);
        std::string msg_str = std::string(static_cast<char*>(msg.data()), msg.size());
        std::cout << "Received '" << msg_str << "'" << std::endl;
//        zmq::pollitem_t pollItem[2]  = {{m_socket, 0, ZMQ_POLLIN, 0}};
//        zmq::poll(pollItem, 2, 1);

//        if(pollItem[0].revents & ZMQ_POLLIN)
//        {
//            zmq::message_t recvMessage;
//            auto res = m_socket.recv(recvMessage, zmq::recv_flags::none);
//            message = recvMessage.to_string();
//            printf("<< Message is: %s >>", message.data());
//            return res;
//        }
//        else
//            return std::nullopt;

    }

private:
    zmq::context_t m_context;
    zmq::socket_t m_socket;
    zmq::socket_type m_socketType;
    bool m_exit;
    uint16_t m_port;
    std::string m_ip;
};

int main(int argc, char *argv[])
{
    std::string ip = "192.168.29.20";
    uint16_t port = 3400;
    printf("Given argc %d\n", argc);

    if (argc == 2)
    {
        printf("Given port %s\n", argv[1]);
        port = atoi(argv[1]);
    }

    signal(SIGINT, ctrlCHandler);
    bool subReady = false;
    std::thread camService = std::thread([&](){
        fname;

        while (!subReady) {
            spdlog::info("{} Waiting for sub to join", __PRETTY_FUNCTION__);
            sleep(1);
        }
        StreamServerPrivate server(port, ip);
        server.init();
        oneSecSleep;
        static int counter = 1;


        spdlog::info("Subscriber ready. Broadcasting now");

        while(!appExit)
        {
            server.sendMessage("frame", "Hello from server: "+ ::to_string(counter));
            usleep(100000);
            //            oneSecSleep;
            counter++;
        }
        server.disconnect();
    });

    sleepForNSec(5);

    std::thread streamClient = std::thread([&](){
        fname;
        StreamClientPrivate client(port, ip);
        client.init("frame");
        subReady = true;
        while (!appExit) {
            std::string message;
            client.recvMessage("frame", message);
            recvPrint(message);
            //            oneSecSleep;
        }
        client.disconnect();
    });

//    sleepForNSec(5);
//    appExit = true;

    camService.join();
    streamClient.join();
    return true;
}
