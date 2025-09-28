
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <unordered_map>


/*
consider struct pricelevel {
 Order* head
 Order* tail

 map<Price, PriceLevel>

 */
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

using AskMap = std::map<Price, Order*>;
using BidMap = std::map<Price, Order*, std::greater<Price>>; // descending key order
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

			while ((asks_head != nullptr) && (bids_head != nullptr)) {
				Quantity quantity = std::min(asks_head->getQuantity(), bids_head->getQuantity());
				bids_head->FillOrder(quantity);
				asks_head->FillOrder(quantity);

				OrderID lastSold = asks_head->getId();
				OrderID lastBought = bids_head->getId();

				if (bids_head->isFilled()) {
					Order* toDelete = bids_head;
					Order* next = bids_head->getNext();
					if (next != nullptr) {
						_bids.at(bidPrice) = (next);
						next->setPrev(nullptr);
					}
					else {
						_bids.erase(bidPrice);
					}
					toDelete->CancelOrder();
					_orders.erase(toDelete->getId());
					bids_head = next;
				}
				if (asks_head->isFilled()) {
					Order* toDelete = asks_head;
					Order* next = asks_head->getNext();
					if (next != nullptr) {
						_asks.at(bidPrice) = (next);
						next->setPrev(nullptr);
					}
					else {
						_asks.erase(askPrice);
					}
					toDelete->CancelOrder();
					_orders.erase(toDelete->getId());
					asks_head = next;
				}
			

				TradeInfo sellInfo{ lastSold, askPrice, quantity };
				TradeInfo buyInfo{ lastBought, askPrice, quantity };
				Trade newTrade{ buyInfo, sellInfo };
				trades.push_back(newTrade);

				if (_asks.size() == 0 || _bids.size() == 0) {
					break;
				}
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
			return true;
		}
	}

	void addOrder(Order* order) {
		if (order->getSide() == Side::Buy) {
			Price price = order->getPrice();
			if (!_bids[price]) { // first insertion at this price
				_bids[price] = order;
			}
			else {  // exists order for given order price -> traverse till end of LL
				Order* curr = _bids[price];
				while (curr->getNext() != nullptr) {
					curr = curr->getNext();
				}
				curr->setNext(order);
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
				order->setPrev(curr);
			}
			_orders[order->getId()] = order;
		}
	}
};

int main()
{
	Order ord1{ 1, 201, 50, Side::Sell };
	Order ord2{ 2, 100, 56, Side::Buy };
	Order ord3{ 3, 100, 58, Side::Buy };
	Order ord4{ 4, 49, 57, Side::Buy };

	OrderBook book{ {}, {}, {} };

	book.addOrder(&ord1);
	book.addOrder(&ord2);
	book.addOrder(&ord3);
	book.addOrder(&ord4);
	/*
	book.addOrder(&ord5);
	book.addOrder(&ord6);
	book.addOrder(&ord7);
	*/
	Trades trades = book.matchOrders();

	if (trades.size() > 0) {
		for (auto &trade : trades) {
			std::cout << "Trade info: "<< "Buy ID: " << trade._buyTradeInfo._orderID << ", Sell ID: " << trade._sellTradeInfo._orderID << ", Quantity: " << trade._buyTradeInfo._quantity << '\n';
		}
	}
	else {
		std::cout << "No trades" << "\n";
	}
	
}