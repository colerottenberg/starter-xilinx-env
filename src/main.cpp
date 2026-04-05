#include "order.hpp"
#include "order_book.hpp"
#include <chrono>
#include <iostream>
#include <memory>

int main() {
  std::cout << "=== Central Limit Order Book Demo ===\n\n";

  // Create an order book for AAPL
  OrderBook book("AAPL");

  // Set up trade callback to print trades
  book.set_trade_callback([](const Trade &trade) {
    std::cout << "TRADE: ID=" << trade.trade_id
              << " Buy Order=" << trade.buy_order_id
              << " Sell Order=" << trade.sell_order_id << " Price=" << trade.price
              << " Quantity=" << trade.quantity << "\n";
  });

  auto now = std::chrono::system_clock::now().time_since_epoch().count();

  // Add some buy orders (bids)
  std::cout << "Adding buy orders...\n";
  auto order1 =
      std::make_shared<Order>(1, "AAPL", Side::BUY, OrderType::LIMIT, 150.00, 100, now++);
  auto order2 =
      std::make_shared<Order>(2, "AAPL", Side::BUY, OrderType::LIMIT, 149.50, 200, now++);
  auto order3 =
      std::make_shared<Order>(3, "AAPL", Side::BUY, OrderType::LIMIT, 149.00, 150, now++);

  book.add_order(order1);
  book.add_order(order2);
  book.add_order(order3);

  // Add some sell orders (asks)
  std::cout << "Adding sell orders...\n";
  auto order4 =
      std::make_shared<Order>(4, "AAPL", Side::SELL, OrderType::LIMIT, 151.00, 100, now++);
  auto order5 =
      std::make_shared<Order>(5, "AAPL", Side::SELL, OrderType::LIMIT, 151.50, 200, now++);
  auto order6 =
      std::make_shared<Order>(6, "AAPL", Side::SELL, OrderType::LIMIT, 152.00, 150, now++);

  book.add_order(order4);
  book.add_order(order5);
  book.add_order(order6);

  // Print the order book
  book.print();

  // Get best bid and ask
  auto best_bid = book.get_best_bid();
  auto best_ask = book.get_best_ask();
  auto spread = book.get_spread();

  std::cout << "Best Bid: " << (best_bid ? std::to_string(*best_bid) : "N/A") << "\n";
  std::cout << "Best Ask: " << (best_ask ? std::to_string(*best_ask) : "N/A") << "\n";
  std::cout << "Spread: " << (spread ? std::to_string(*spread) : "N/A") << "\n\n";

  // Add a buy order that matches
  std::cout << "Adding aggressive buy order at 151.50 for 250 shares...\n";
  auto order7 =
      std::make_shared<Order>(7, "AAPL", Side::BUY, OrderType::LIMIT, 151.50, 250, now++);
  book.add_order(order7);

  std::cout << "\nAfter matching:\n";
  book.print();

  // Cancel an order
  std::cout << "Canceling order 2...\n";
  book.cancel_order(2);

  book.print();

  // Modify an order
  std::cout << "Modifying order 3 to price 149.75 and quantity 300...\n";
  book.modify_order(3, 149.75, 300);

  book.print();

  // Add a market order
  std::cout << "Adding market sell order for 50 shares...\n";
  auto order8 =
      std::make_shared<Order>(8, "AAPL", Side::SELL, OrderType::MARKET, 0.0, 50, now++);
  book.add_order(order8);

  std::cout << "\nFinal order book:\n";
  book.print();

  return 0;
}
