#include "../include/OrderBook.h"

OrderBook::OrderBook(BidMap bids, AskMap asks, OrderMap orders)
	: _bids(bids), _asks(asks), _orders(orders) {
}

bool OrderBook::canMatch(Side side, Price price) const {
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
Trades OrderBook::matchOrders() {
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

bool OrderBook::CancelOrder(OrderID orderID) { // true if successful, false if not
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

void OrderBook::addOrder(Order* order) {
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
