#pragma once
#include <cstdint>

using OrderID = std::uint64_t;
using Quantity = std::uint32_t;
using Price = std::uint32_t;
using TimeStamp = std::uint64_t;

enum Side { Buy, Sell };

class Order {
private:
    OrderID _id{};
    Quantity _quantity{};
    Price _price{};
    Side _side{};
    Order* _next{};
    Order* _prev{};

public:
    Order(OrderID id, Quantity qty, Price price, Side side);

    OrderID getId() const;
    Quantity getQuantity() const;
    Price getPrice() const;
    Side getSide() const;
    Order* getNext() const;
    Order* getPrev() const;
    void setNext(Order* next);
    void setPrev(Order* prev);

    void FillOrder(Quantity quantity);
    void CancelOrder();
    bool isFilled() const;
};