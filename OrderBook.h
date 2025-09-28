#pragma once
#include <map>
#include <unordered_map>
#include <vector>
#include "Order.h"

struct TradeInfo {
    OrderID _orderID{};
    Price _price{};
    Quantity _quantity{};
};

struct Trade {
    TradeInfo _buyTradeInfo{};
    TradeInfo _sellTradeInfo{};
};

using Trades = std::vector<Trade>;

struct PriceLevel {
    Order* head{ nullptr };
    Order* tail{ nullptr };
};

using BidMap = std::map<Price, PriceLevel, std::greater<Price>>;
using AskMap = std::map<Price, PriceLevel>;
using OrderMap = std::unordered_map<OrderID, Order*>;

class OrderBook {
private:
    BidMap _bids{};
    AskMap _asks{};
    OrderMap _orders{};

public:
    OrderBook(BidMap bids, AskMap asks, OrderMap orders);
    bool canMatch(Side side, Price price) const;
    Trades matchOrders();
    bool CancelOrder(OrderID orderID);
    void addOrder(Order* order);
};