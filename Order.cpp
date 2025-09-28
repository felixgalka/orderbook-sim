#include "Order.h"

Order::Order(OrderID id, Quantity qty, Price price, Side side)
    : _id(id), _quantity(qty), _price(price), _side(side) {
}

OrderID Order::getId() const { return _id; }
Quantity Order::getQuantity() const { return _quantity; }
Price Order::getPrice() const { return _price; }
Side Order::getSide() const { return _side; }

Order* Order::getNext() const { return _next; }
Order* Order::getPrev() const { return _prev; }
void Order::setNext(Order* next) { _next = next; }
void Order::setPrev(Order* prev) { _prev = prev; }

void Order::FillOrder(Quantity quantity) {
    if (quantity <= _quantity) _quantity -= quantity;
}

void Order::CancelOrder() {
    if (_next) _next->_prev = _prev;
    if (_prev) _prev->_next = _next;
    _prev = nullptr;
    _next = nullptr;
}

bool Order::isFilled() const {
    return _quantity == 0;
}