#pragma once
#include <string>
#include <vector>
#include <cstdint>

// The focus of this project is not on the server, so the implentation is kept overly simple. 

struct StockDirectoryMsg
{
    uint16_t msgLength; // how many bytes in this message
    uint16_t tickerID; // unique identifier for the stock
    char symbol[7]; // stock symbol, null-terminated
    char msgType; // R -> stock directory message, A -> Add order, E -> order executed
};

struct OrderMsg
{
    uint16_t msgLength; // how many bytes in this message
    uint16_t tickerID; // unique identifier for the stock
    uint32_t price; // Order Price
    uint16_t qty; // How many shares in order
    char msgType; // R -> stock directory message, A -> Add order, E -> order executed
};

struct Symbol
{
    char symbol[7]; // stock symbol, null-terminated
    uint16_t tickerID; // unique identifier for the stock
    uint32_t price; // price of the stock in cents
};

class FeedServer
{
    public:
    FeedServer(int port, std::string IP);
    void run();

    private:
    int port_; // port number to listen on
    std::string IP_; // IP address to listen on
    int server_fd_; // file descriptor for the server socket
    int client_fd_; // file descriptor for the client socket
    Symbol activeSymbols[10]; // array of active symbols
    void sendStockDirectoryMsgs(); // sends stock directory messages to the client
    OrderMsg GenerateEvent(); // generate a random event (add order, order executed, etc.) SIMULATION ONLY
    void initSymbols();
};