#include "OrderBook.h"
#include <iostream>

int main() {
    OrderBook book{ {}, {}, {} };

    Order* ord1 = new Order(1, 100, 50, Side::Sell);
    Order* ord2 = new Order(2, 500, 60, Side::Buy);

    book.addOrder(ord1);
    book.addOrder(ord2);

    Trades trades = book.matchOrders();
    for (auto& trade : trades) {
        std::cout << "Trade: "
            << trade._buyTradeInfo._orderID << " <-> "
            << trade._sellTradeInfo._orderID << "\n";
    }
}