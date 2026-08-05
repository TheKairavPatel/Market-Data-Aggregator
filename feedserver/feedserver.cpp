#include "feedserver.hpp"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>

FeedServer::FeedServer(int port, std::string IP)
{
    // Saving server IP and Port
    port_ = port;
    IP_ = IP;
    
    // Socket
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);

    // Sockaddr_in 
    sockaddr_in server_addr{};
    server_addr.sin_port = htons(port_);
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, IP_.data(), &server_addr.sin_addr);

    // Bind socket to sockaddr_in & listen for connection (only 1 for this, no epoll needed)
    bind(server_fd_, (sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_fd_, 5);
}

OrderMsg FeedServer::GenerateEvent()
{
    // This function randomly generates order executed, received, and cancel messages for our Feed Reader to consume
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::discrete_distribution<int> eventPick({45, 40, 25});
    static std::uniform_int_distribution<int> symbolPick(0, 9);
    static std::uniform_int_distribution<int> qtyPick(1, 500);
    static std::normal_distribution<double> priceMovePick(0.0, 10.0); // mean 0, stddev 10 cents

    int outcome = eventPick(gen);
    char type = (outcome == 0) ? 'A' : (outcome == 1) ? 'U' : 'E';

    int symIdx = symbolPick(gen);
    Symbol& sym = activeSymbols[symIdx];

    int priceMove = (int)(std::round(priceMovePick(gen)));
    int newPrice = (int)(sym.price) + priceMove;
    if (newPrice < 100) newPrice = 100; // floor at $1.00, in cents
    sym.price = (uint32_t)(newPrice);

    int qty = qtyPick(gen);

    OrderMsg msg{};
    msg.tickerID = sym.tickerID;
    msg.price    = sym.price;
    msg.qty      = (uint16_t)(qty);
    msg.msgType  = type;
    msg.msgLength = sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(char); // 11

    return msg;
}