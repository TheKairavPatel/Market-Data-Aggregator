#include "feedserver.hpp"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <random>
#include <cmath>
#include <cstring>

FeedServer::FeedServer(int port, std::string IP)
{
    // Saving server IP and Port
    port_ = port;
    IP_ = IP;
    
    // Initialize the active symbols
    initSymbols();

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
    static std::normal_distribution<double> pctMovePick(0.0, 0.001); // mean 0, stddev 0.1% of price

    int outcome = eventPick(gen);
    char type = (outcome == 0) ? 'A' : (outcome == 1) ? 'U' : 'E';

    int symIdx = symbolPick(gen);
    Symbol& sym = activeSymbols[symIdx];

    double pctMove = pctMovePick(gen);
    int priceMove = (int)(std::round(sym.price * pctMove));
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

void FeedServer::initSymbols()
{
    activeSymbols[0] = {"AAPL", 1, 15000}; // $150.00
    activeSymbols[1] = {"GOOGL", 2, 280000}; // $2800.00
    activeSymbols[2] = {"MSFT", 3, 30000}; // $300.00
    activeSymbols[3] = {"AMZN", 4, 350000}; // $3500.00
    activeSymbols[4] = {"TSLA", 5, 70000}; // $700.00
    activeSymbols[5] = {"FB", 6, 35000}; // $350.00
    activeSymbols[6] = {"NFLX", 7, 55000}; // $550.00
    activeSymbols[7] = {"NVDA", 8, 22000}; // $220.00
    activeSymbols[8] = {"BABA", 9, 20000}; // $200.00
    activeSymbols[9] = {"INTC", 10, 5000}; // $50.00
}

void FeedServer::sendStockDirectoryMsgs()
{
    for (int i = 0; i < 10; i++)
    {
        StockDirectoryMsg msg{};
        msg.tickerID = activeSymbols[i].tickerID;
        std::strncpy(msg.symbol, activeSymbols[i].symbol, sizeof(msg.symbol));
        msg.msgType = 'R';
        msg.msgLength = sizeof(uint16_t) + sizeof(uint16_t) + sizeof(msg.symbol) + sizeof(char); // 12
        write(client_fd_, &msg, sizeof(msg));
    }
}

void FeedServer::run()
{









}