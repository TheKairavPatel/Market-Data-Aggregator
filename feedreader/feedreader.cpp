#include "feedreader.hpp"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <chrono>

FeedReader::FeedReader(std::string serverIP, int serverPort, bool& running, SPSCQueue<TradeEvent, 4096>& ingressQueue) : 
    serverIP_(serverIP), 
    serverPort_(serverPort), 
    running_(running), 
    ingressQueue_(ingressQueue) 
    {}

bool FeedReader::connectToServer() 
{
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        perror("socket failed");
        return false;
    }

    sockaddr_in serv_addr {};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(serverPort_);
    if (inet_pton(AF_INET, serverIP_.c_str(), &serv_addr.sin_addr) <= 0) {
        perror("inet_pton failed");
        return false;
    }
    if (connect(server_fd_, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect failed");
        return false;
    }

    return true;
}

uint16_t FeedReader::readRawMsg(char* buffer, size_t bufferCap) 
{
    // Ensure we have at least 2 bytes to read the msgLength
    int bytesRead = 0;
    char tempBuf[2];
    while (bytesRead < 2) 
    {
        int readRes = read(server_fd_, tempBuf + bytesRead, 2 - bytesRead);
        if (readRes <= 0) 
        {
            perror("read failed or connection closed");
            return 0; // return 0 on error
        }
        bytesRead += readRes;
    }

    uint16_t msgLength;
    std::memcpy(&msgLength, tempBuf, 2);
    
}



