#pragma once
#include <string>
#include <cstdint>
#include <unordered_map>
#include "../queue/queues.hpp"
#include "../feedserver/feedserver.hpp" // OrderMsg, StockDirectoryMsg

// TradeEvent is the pipeline-internal representation FeedReader produces.
// Dispatcher/Aggregator consume this, not the raw OrderMsg wire struct.
struct TradeEvent
{
    uint16_t symbolID;    // mapped from OrderMsg::tickerID via tickerToSymbolId_
    uint32_t price;       // cents, same as OrderMsg::price
    uint16_t qty;
    char     msgType;     // 'A' / 'U' / 'E'
    uint64_t timestampNs; // stamped on receipt
};

class FeedReader
{
public:
    FeedReader(std::string serverIP, int serverPort, bool& running,
               SPSCQueue<TradeEvent, 4096>& ingressQueue);

    void run(); // connect, consume initial StockDirectoryMsgs, then loop reading OrderMsgs

private:
    // connection info
    int server_fd_;
    std::string serverIP_;
    int serverPort_;

    // control
    bool& running_;

    // where finished events go
    SPSCQueue<TradeEvent, 4096>& ingressQueue_;

    // tickerID -> symbol_id mapping, populated from the initial
    // StockDirectoryMsg burst FeedServer sends on connect
    std::unordered_map<uint16_t, uint16_t> tickerToSymbolId_;

    // --- TCP stream reassembly ---
    // read() can return partial messages, multiple messages, or a split
    // in the middle of a struct. recvBuf_ accumulates raw bytes; we only
    // "consume" a message once bufLen_ has at least msgLength bytes for it.
    static constexpr size_t kRecvBufSize = 4096;
    char   recvBuf_[kRecvBufSize];
    size_t bufLen_ = 0; // valid bytes currently sitting in recvBuf_

    bool connectToServer();          // opens server_fd_, connects to serverIP_:serverPort_
    void readStockDirectory();       // blocks reading the initial 'R' message burst, fills tickerToSymbolId_
    void processBuffer();            // walks recvBuf_, extracts complete OrderMsgs, dispatches them
    void handleOrderMsg(const OrderMsg& msg); // converts to TradeEvent, pushes to ingressQueue_
};