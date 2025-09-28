
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <unordered_map>
#include <random>

// 200,000 orders in 1000ms with head only LL
// 350,000 orders in 1000ms with O(1) adding (tail and head LL)

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


struct PriceLevel {
	Order* head{nullptr};
	Order* tail{nullptr};
};

using AskMap = std::map<Price, PriceLevel>;
using BidMap = std::map<Price, PriceLevel, std::greater<Price>>; // descending key order
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
			Price bestPrice = _asks.begin()->first;
			return (bestPrice <= price);
		}
		if (side == Side::Sell) {
			if (_bids.empty()) {
				return false;
			}
			Price bestPrice = _bids.begin()->first;
			return (bestPrice >= price);
		}	
	}
	Trades matchOrders() {
		Trades trades{};
		while (true) {
			if (_asks.empty() || _bids.empty()) { break; }
			Price bidPrice = _bids.begin()->first;
			Order* bids_head = (_bids.begin()->second).head;
			Price askPrice = _asks.begin()->first;
			Order* asks_head = (_asks.begin()->second).head;

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
						_bids.at(bidPrice).head = (next);
						next->setPrev(nullptr);
					}
					else {
						_bids.erase(bidPrice);
					}
					toDelete->CancelOrder();
					_orders.erase(toDelete->getId());
					bids_head = next;
					//delete toDelete;
				}
				if (asks_head->isFilled()) {
					Order* toDelete = asks_head;
					Order* next = asks_head->getNext();
					if (next != nullptr) {
						_asks.at(askPrice).head = (next);
						next->setPrev(nullptr);
					}
					else {
						_asks.erase(askPrice);
					}
					toDelete->CancelOrder();
					_orders.erase(toDelete->getId());
					asks_head = next;
					//delete toDelete;
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
		Price price = order->getPrice();
		if (order->getSide() == Side::Buy) {
			Order* head = _bids.at(price).head;
			Order* tail = _bids.at(price).tail;
			if (head == nullptr) {
				_bids.erase(price);
			}
			else {
				if (order->getNext() == nullptr) {
					_bids.at(price).tail = order->getPrev();
				}
				if (order->getPrev() == nullptr) {
					_bids.at(price).head = order->getNext();
				}
			}

			_orders.erase(orderID);
			order->CancelOrder();

			return true;
		}
		else {
			Order* head = _asks.at(price).head;
			Order* tail = _asks.at(price).tail;
			if (head == nullptr) {
				_asks.erase(price);
			}
			else {
				if (order->getNext() == nullptr) {
					_asks.at(price).tail = order->getPrev();
				}
				if (order->getPrev() == nullptr) {
					_asks.at(price).head = order->getNext();
				}
			}

			_orders.erase(orderID);
			order->CancelOrder();

			return true;
		}
	}

	void addOrder(Order* order) {
		if (order->getSide() == Side::Buy) {
			Price price = order->getPrice();
			if (!(_bids[price].head)) { // first insertion at this price
				_bids[price].head = order;
				_bids[price].tail = order;
			}
			else {  // exists order for given order price -> traverse till end of LL
				Order* end = _bids[price].tail;
				end->setNext(order);
				_bids[price].tail = order;
			}
			_orders[order->getId()] = order;
		}
		else {
			Price price = order->getPrice();
			if (!(_asks[price].head)) { // first insertion at this price
				_asks[price].head = order;
				_asks[price].tail = order;
			}
			else {  // exists order for given order price -> traverse till end of LL
				Order* end = _asks[price].tail;
				end->setNext(order);
				_asks[price].tail = order;
			}
			_orders[order->getId()] = order;
		}
	}
};

void testOrders() {

	Order ord1{ 1, 200, 50, Side::Sell };
	Order ord2{ 2, 100, 49, Side::Sell };
	Order ord3{ 3, 250, 51, Side::Buy };
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
		for (auto& trade : trades) {
			std::cout << "Trade info: " << "Buy ID: " << trade._buyTradeInfo._orderID << ", Sell ID: " << trade._sellTradeInfo._orderID << ", Quantity: " << trade._buyTradeInfo._quantity << '\n';
		}
	}
	else {
		std::cout << "No trades" << "\n";
	}
}
Order* generateRandomOrder(OrderID id, Price midPrice, TimeStamp ts) {
	static std::mt19937 rng(42);
	static std::uniform_int_distribution<int> qtyDist(1, 1000);
	static std::normal_distribution<double> priceDist(midPrice, 5.0);
	static std::bernoulli_distribution sideDist(0.5);

	Quantity qty = qtyDist(rng);
	Price price = static_cast<Price>(std::max<int>(1, (int)std::round(priceDist(rng))));
	Side side = sideDist(rng) ? Side::Buy : Side::Sell;

	return new Order(id, qty, price, side);
}
void testSpeed() {
	OrderBook book{ {}, {}, {} };

	std::mt19937 rng(123);
	std::exponential_distribution<double> interArrival(1e-6);
	TimeStamp simTime = 0;

	const OrderID N = 350000; //
	auto t0 = std::chrono::high_resolution_clock::now();

	for (OrderID id = 1; id <= N; ++id) {
		simTime += static_cast<TimeStamp>(interArrival(rng) + 0.5);
		Order* o = generateRandomOrder(id, 100, simTime);

		book.addOrder(o);      
		Trades trades = book.matchOrders();

	}

	auto t1 = std::chrono::high_resolution_clock::now();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
	std::cout << "Processed " << N << " orders in " << ms << " ms\n";

}
int main() {
	testOrders();
}