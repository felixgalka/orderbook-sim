#include "../include/Order.h"
#include "../include/OrderBook.h"
#include <random>
#include <iostream>
#include <chrono>
#include "../include/Tests.h"

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