
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <unordered_map>

using OrderID = std::uint64_t;
using Quantity = std::uint32_t;
using Price = std::uint32_t;
using TimeStamp = std::uint64_t;

enum Side {
	Buy,
	Sell,
};

class Order {

private:
	OrderID _id{};
	Quantity _quantity{};
	Price _price{};
	Side _side{};
	TimeStamp _stamp{ __rdtsc() };
	Order* _next{};
	Order* _prev{};

public:
	Order(OrderID id, Quantity qty, Price price, Side side)
		: _id(id),
		_quantity(qty),
		_price(price),
		_side(side),
		_stamp(__rdtsc()),
		_next(nullptr),
		_prev(nullptr)
	{
	}
	const OrderID getId() { return _id; }
	const Quantity getQuantity() { return _quantity; }
	const Price getPrice() { return _price; }
	const Side getSide() { return _side; }
	const TimeStamp getTime() { return _stamp; }
	Order* getNext() const { return _next; }
	Order* getPrev() const { return _prev; }
	void setNext(Order* next) { _next = next; }
	void setPrev(Order* prev) { _prev = prev; }

	void FillOrder(Quantity quantity) {
		if (quantity > getQuantity()) {
			return;
		}
		_quantity -= quantity;
	}

	void CancelOrder() {
		if (_next) {
			_next->_prev = _prev;
		}
		if (_prev) {
			_prev->_next = _next;
		}
		_prev = nullptr;
		_next = nullptr;
	}

	bool isFilled() const {
		return _quantity == 0;
	}
};

using BidMap = std::map<Price, Order*>;
using AskMap = std::map<Price, Order*, std::greater<Price>>; // descending key order
using OrderMap = std::unordered_map<OrderID, Order*>;

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

class OrderBook {

	

private:
	BidMap _bids{};
	AskMap _asks{};
	OrderMap _orders{};

public:

	OrderBook(BidMap bids, AskMap asks, OrderMap orders)
		: _bids(bids),
		_asks(asks),
		_orders(orders)
	{
	}

	bool canMatch(Side side, Price price) const {
		if (side == Side::Buy) {
			if (_asks.empty()) {
				return false;
			}

			auto bestPrice = _asks.begin()->first;
			return (bestPrice <= price);
		}
		if (side == Side::Sell) {
			if (_bids.empty()) {
				return false;
			}
			auto bestPrice = _bids.begin()->first;
			return (bestPrice >= price);
		}	
	}
	Trades matchOrders() {
		Trades trades{};
		while (true) {

			if (_asks.empty() || _bids.empty()) { break; }
			auto bidPrice = _bids.begin()->first;
			Order* bids_head = _bids.begin()->second;
			auto askPrice = _asks.begin()->first;
			Order* asks_head = _asks.begin()->second;

			if (askPrice > bidPrice) {
				break;
			}

			while (asks_head && bids_head) {
		
				Quantity quantity = std::min(asks_head->getQuantity(), bids_head->getQuantity());
				bids_head->FillOrder(quantity);
				asks_head->FillOrder(quantity);

				OrderID lastSold = asks_head->getId();
				OrderID lastBought = bids_head->getId();

				if (bids_head->isFilled()) {
					_bids.at(bidPrice) = (bids_head->getNext());
					bids_head = bids_head->getNext();
					bids_head->CancelOrder();
					_orders.erase(bids_head->getId());
				}
				if (asks_head->isFilled()) {
					_asks.at(askPrice) = (asks_head->getNext());
					asks_head = asks_head->getNext();
					asks_head->CancelOrder();
					_orders.erase(asks_head->getId());
				}
				if (!asks_head) {
					_asks.erase(askPrice);
				}
				if (!bids_head) {
					_bids.erase(bidPrice);
				}

				TradeInfo sellInfo{ lastSold, askPrice, quantity };
				TradeInfo buyInfo{ lastBought, askPrice, quantity };
				Trade newTrade{ buyInfo, sellInfo };
				trades.push_back(newTrade);
			}
		
		}
		return trades;
	}

	bool CancelOrder(OrderID orderID) { // true if successful, false if not
		if (!_orders.contains(orderID)) {
			return false;
		}
		Order* order = _orders[orderID];
		if (order->getSide() == Side::Buy) {
			_orders.erase(orderID);
			if (!order->getPrev()) {
				if (!order->getNext()) {
					_bids.erase(order->getPrice());
				}
				else {
					_bids[order->getPrice()] = order->getNext();
				}
			}
			order->CancelOrder();
			delete(order);
			return true;
		}
		else {
			_orders.erase(orderID);
			if (!order->getPrev()) {
				if (!order->getNext()) {
					_asks.erase(order->getPrice());
				}
				else {
					_asks[order->getPrice()] = order->getNext();
				}
			}
			order->CancelOrder();
			delete(order);
			return true;
		}
	}
	void addOrder(Order* order) {
		if (order->getSide() == Side::Buy) {
			if (!_bids[order->getPrice()]) { // first insertion at this price
				_bids[order->getPrice()] = order;
			}
			else {  // exists order for given order price -> traverse till end of LL
				Order* curr = _bids[order->getPrice()];
				while (curr->getNext()) {
					curr = curr->getNext();
				}
				curr->setNext(order);
				order->setNext(nullptr);
				order->setPrev(curr);
			}
			_orders[order->getId()] = order;
		}
		else {
			Price price = order->getPrice();
			if (!_asks[price]) { // first insertion at this price
				_asks[price] = order;
			}
			else {
				Order* curr = _asks[price];
				while (curr->getNext()) {
					curr = curr->getNext();
				}
				curr->setNext(order);
				order->setNext(nullptr);
				order->setPrev(curr);
			}
			_orders[order->getId()] = order;
		}
	}
};

int main()
{

	Order ord1{ 1, 100, 50, Side::Buy };
	Order ord2{ 2, 90, 49, Side::Sell };
	Order ord3{ 3, 10, 60, Side::Buy };

	OrderBook book{ {}, {}, {} };
	book.addOrder(&ord1);
	book.addOrder(&ord2);
	bool res = book.canMatch(Side::Buy, 48);
	if (res) {
		std::cout << "Can match" << "\n";
	}
	else {
		std::cout << "Can not match" << "\n";
	}
}
