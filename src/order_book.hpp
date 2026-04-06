#pragma once

#include "order.hpp"
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

// Price level contains all orders at a specific price
struct PriceLevel {
  double price;
  std::deque<std::shared_ptr<Order>>
      orders; // deque for FIFO and efficient removal
  uint64_t total_quantity;

  explicit PriceLevel(double p) : price(p), total_quantity(0) {}

  void add_order(std::shared_ptr<Order> order) {
    total_quantity += order->quantity;
    orders.push_back(std::move(order));
  }

  void remove_order(uint64_t order_id) {
    auto it = std::find_if(
        orders.begin(), orders.end(),
        [order_id](const auto &order) { return order->order_id == order_id; });
    if (it != orders.end()) {
      total_quantity -= (*it)->quantity;
      orders.erase(it);
    }
  }

  bool is_empty() const { return orders.empty(); }
};

class OrderBook {
public:
  using TradeCallback = std::function<void(const Trade &)>;

  explicit OrderBook(std::string symbol)
      : symbol_(std::move(symbol)), next_trade_id_(1) {}

  // Add a new order to the book and attempt to match
  std::vector<Trade> add_order(std::shared_ptr<Order> order);

  // Cancel an existing order
  bool cancel_order(uint64_t order_id);

  // Modify an existing order (cancel and replace)
  bool modify_order(uint64_t order_id, double new_price, uint64_t new_quantity);

  // Get the best bid price
  std::optional<double> get_best_bid() const;

  // Get the best ask price
  std::optional<double> get_best_ask() const;

  // Get the spread
  std::optional<double> get_spread() const;

  // Get total volume at price level
  uint64_t get_bid_volume_at_price(double price) const;
  uint64_t get_ask_volume_at_price(double price) const;

  // Set callback for trade notifications
  void set_trade_callback(TradeCallback callback) {
    trade_callback_ = std::move(callback);
  }

  // Print the order book (for debugging)
  void print() const;

private:
  std::string symbol_;
  uint64_t next_trade_id_;

  // Buy side: highest price first (descending)
  std::map<double, PriceLevel, std::greater<double>> bids_;

  // Sell side: lowest price first (ascending)
  std::map<double, PriceLevel, std::less<double>> asks_;

  // Fast lookup for orders by ID
  std::unordered_map<uint64_t, std::shared_ptr<Order>> orders_;

  // Trade callback
  TradeCallback trade_callback_;

  // Internal matching logic
  std::vector<Trade> match_order(std::shared_ptr<Order> order);

  // Helper to add order to appropriate side without matching
  void add_to_book(std::shared_ptr<Order> order);

  // Helper to create a trade
  Trade create_trade(std::shared_ptr<Order> buy_order,
                     std::shared_ptr<Order> sell_order, double price,
                     uint64_t quantity, uint64_t timestamp);

  // Helper to remove filled orders
  void cleanup_filled_orders();
};
