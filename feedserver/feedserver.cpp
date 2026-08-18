#include "feedserver.hpp"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <random>
#include <cmath>
#include <cstring>

FeedServer::FeedServer(int port, std::string IP, bool &running) : port_(port), IP_(IP), running_(running)
{
    // Initialize the active symbols
    initSymbols();

    // Socket
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);

    // Sockaddr_in 
    sockaddr_in server_addr{};
    server_addr.sin_port = htons(port_);
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, IP_.data(), &server_addr.sin_addr);

    // Avoid OS protecting port in TIME_WAIT
    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

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
    static std::normal_distribution<double> pctMovePick(0.0, 0.001); // mean 0, stddev 0.1% of price
    static std::discrete_distribution<int> qtyBucketPick({70, 20, 8, 2}); // bucket weights
    static std::uniform_int_distribution<int> qtySmall(1, 10);
    static std::uniform_int_distribution<int> qtyMed(11, 50);
    static std::uniform_int_distribution<int> qtyLarge(51, 200);
    static std::uniform_int_distribution<int> qtyBlock(201, 500);

    int outcome = eventPick(gen);
    char type = (outcome == 0) ? 'A' : (outcome == 1) ? 'U' : 'E';

    int symIdx = symbolPick(gen);
    Symbol& sym = activeSymbols[symIdx];

    double pctMove = pctMovePick(gen);
    int priceMove = (int)(std::round(sym.price * pctMove));
    int newPrice = (int)(sym.price) + priceMove;
    if (newPrice < 100) newPrice = 100; // floor at $1.00, in cents
    sym.price = (uint32_t)(newPrice);

    int bucket = qtyBucketPick(gen);
    int qty = (bucket == 0) ? qtySmall(gen)
            : (bucket == 1) ? qtyMed(gen)
            : (bucket == 2) ? qtyLarge(gen)
            : qtyBlock(gen);

    OrderMsg msg{};
    msg.tickerID = sym.tickerID;
    msg.price    = sym.price;
    msg.qty      = (uint16_t)(qty);
    msg.msgType  = type;
    msg.msgLength = sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(char); // 11

    return msg;
}

void FeedServer::initSymbols()
{
    activeSymbols[0] = {"AAPL", 1, 15000}; // $150.00
    activeSymbols[1] = {"GOOGL", 2, 280000}; // $2800.00
    activeSymbols[2] = {"MSFT", 3, 30000}; // $300.00
    activeSymbols[3] = {"AMZN", 4, 350000}; // $3500.00
    activeSymbols[4] = {"TSLA", 5, 70000}; // $700.00
}

void FeedServer::sendStockDirectoryMsgs()
{
    for (int i = 0; i < 5; i++)
    {
        StockDirectoryMsg msg{};
        msg.tickerID = activeSymbols[i].tickerID;
        std::strncpy(msg.symbol, activeSymbols[i].symbol, sizeof(msg.symbol));
        msg.msgType = 'R';
        msg.msgLength = sizeof(uint16_t) + sizeof(uint16_t) + sizeof(msg.symbol) + sizeof(char); // 12
        write(client_fd_, &msg, msg.msgLength);
    }
}

void FeedServer::run()
{
    // Setup and accept client connection
    sockaddr_in client_addr{};
    socklen_t client_addr_len = sizeof(client_addr);
    client_fd_ = accept(server_fd_, (sockaddr*)&client_addr, &client_addr_len);
    
    // Let client know ID to symbol mapping
    sendStockDirectoryMsgs();

    while (running_)
    {
        OrderMsg msg = GenerateEvent();
        ssize_t sent = write(client_fd_, &msg, msg.msgLength);
        if (sent <= 0)
        {
            break;
        }
        usleep(10000);
    }
    close(client_fd_);
    close(server_fd_);
}