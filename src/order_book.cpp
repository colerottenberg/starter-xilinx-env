#include "order_book.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>

std::vector<Trade> OrderBook::add_order(std::shared_ptr<Order> order) {
  if (order->symbol != symbol_) {
    return {}; // Order for different symbol
  }

  orders_[order->order_id] = order;

  // Try to match the order
  auto trades = match_order(order);

  // If order not fully filled, add remainder to book
  if (!order->is_filled()) {
    add_to_book(order);
  }

  // Notify trades via callback
  if (trade_callback_) {
    for (const auto &trade : trades) {
      trade_callback_(trade);
    }
  }

  return trades;
}

bool OrderBook::cancel_order(uint64_t order_id) {
  auto it = orders_.find(order_id);
  if (it == orders_.end()) {
    return false;
  }

  auto order = it->second;
  double price = order->price;

  // Remove from appropriate side
  if (order->side == Side::BUY) {
    auto price_it = bids_.find(price);
    if (price_it != bids_.end()) {
      price_it->second.remove_order(order_id);
      if (price_it->second.is_empty()) {
        bids_.erase(price_it);
      }
    }
  } else {
    auto price_it = asks_.find(price);
    if (price_it != asks_.end()) {
      price_it->second.remove_order(order_id);
      if (price_it->second.is_empty()) {
        asks_.erase(price_it);
      }
    }
  }

  orders_.erase(it);
  return true;
}

bool OrderBook::modify_order(uint64_t order_id, double new_price,
                             uint64_t new_quantity) {
  auto it = orders_.find(order_id);
  if (it == orders_.end()) {
    return false;
  }

  auto old_order = it->second;

  // Cancel the old order
  if (!cancel_order(order_id)) {
    return false;
  }

  // Create new order with same ID but new parameters
  auto new_order = std::make_shared<Order>(
      order_id, old_order->symbol, old_order->side, old_order->type, new_price,
      new_quantity,
      static_cast<uint64_t>(
          std::chrono::system_clock::now().time_since_epoch().count()));

  // Add the new order (this will attempt matching)
  add_order(new_order);

  return true;
}

std::optional<double> OrderBook::get_best_bid() const {
  if (bids_.empty()) {
    return std::nullopt;
  }
  return bids_.begin()->first;
}

std::optional<double> OrderBook::get_best_ask() const {
  if (asks_.empty()) {
    return std::nullopt;
  }
  return asks_.begin()->first;
}

std::optional<double> OrderBook::get_spread() const {
  auto bid = get_best_bid();
  auto ask = get_best_ask();

  if (bid && ask) {
    return *ask - *bid;
  }
  return std::nullopt;
}

uint64_t OrderBook::get_bid_volume_at_price(double price) const {
  auto it = bids_.find(price);
  if (it != bids_.end()) {
    return it->second.total_quantity;
  }
  return 0;
}

uint64_t OrderBook::get_ask_volume_at_price(double price) const {
  auto it = asks_.find(price);
  if (it != asks_.end()) {
    return it->second.total_quantity;
  }
  return 0;
}

std::vector<Trade> OrderBook::match_order(std::shared_ptr<Order> order) {
  std::vector<Trade> trades;

  if (order->side == Side::BUY) {
    // Match against asks (sell orders)
    while (!order->is_filled() && !asks_.empty()) {
      auto &[ask_price, price_level] = *asks_.begin();

      // For limit orders, check if price is acceptable
      if (order->type == OrderType::LIMIT && ask_price > order->price) {
        break; // No more matches possible
      }

      while (!order->is_filled() && !price_level.is_empty()) {
        auto &sell_order = price_level.orders.front();

        // Determine trade quantity
        uint64_t trade_qty = std::min(order->quantity, sell_order->quantity);

        // Create trade at the resting order's price (price-time priority)
        auto trade = create_trade(
            order, sell_order, ask_price, trade_qty,
            static_cast<uint64_t>(
                std::chrono::system_clock::now().time_since_epoch().count()));
        trades.push_back(trade);

        // Update quantities
        order->quantity -= trade_qty;
        sell_order->quantity -= trade_qty;
        price_level.total_quantity -= trade_qty;

        // Remove filled sell order
        if (sell_order->is_filled()) {
          price_level.orders.pop_front();
          orders_.erase(sell_order->order_id);
        }
      }

      // Remove empty price level
      if (price_level.is_empty()) {
        asks_.erase(asks_.begin());
      }
    }
  } else { // SELL order
    // Match against bids (buy orders)
    while (!order->is_filled() && !bids_.empty()) {
      auto &[bid_price, price_level] = *bids_.begin();

      // For limit orders, check if price is acceptable
      if (order->type == OrderType::LIMIT && bid_price < order->price) {
        break; // No more matches possible
      }

      while (!order->is_filled() && !price_level.is_empty()) {
        auto &buy_order = price_level.orders.front();

        // Determine trade quantity
        uint64_t trade_qty = std::min(order->quantity, buy_order->quantity);

        // Create trade at the resting order's price (price-time priority)
        auto trade = create_trade(
            buy_order, order, bid_price, trade_qty,
            static_cast<uint64_t>(
                std::chrono::system_clock::now().time_since_epoch().count()));
        trades.push_back(trade);

        // Update quantities
        order->quantity -= trade_qty;
        buy_order->quantity -= trade_qty;
        price_level.total_quantity -= trade_qty;

        // Remove filled buy order
        if (buy_order->is_filled()) {
          price_level.orders.pop_front();
          orders_.erase(buy_order->order_id);
        }
      }

      // Remove empty price level
      if (price_level.is_empty()) {
        bids_.erase(bids_.begin());
      }
    }
  }

  // Remove the incoming order from orders map if filled
  if (order->is_filled()) {
    orders_.erase(order->order_id);
  }

  return trades;
}

void OrderBook::add_to_book(std::shared_ptr<Order> order) {
  if (order->type == OrderType::MARKET) {
    // Market orders should be fully matched or rejected
    return;
  }

  if (order->side == Side::BUY) {
    auto [it, inserted] = bids_.try_emplace(order->price, order->price);
    it->second.add_order(order);
  } else {
    auto [it, inserted] = asks_.try_emplace(order->price, order->price);
    it->second.add_order(order);
  }
}

Trade OrderBook::create_trade(std::shared_ptr<Order> buy_order,
                              std::shared_ptr<Order> sell_order, double price,
                              uint64_t quantity, uint64_t timestamp) {
  return Trade(next_trade_id_++, buy_order->order_id, sell_order->order_id,
               price, quantity, timestamp);
}

void OrderBook::print() const {
  std::cout << "\n=== Order Book for " << symbol_ << " ===\n\n";

  std::cout << "ASKS (Sell Orders):\n";
  std::cout << "Price\t\tQuantity\tOrders\n";
  std::cout << "-----\t\t--------\t------\n";

  // Print asks in reverse order (highest to lowest)
  for (auto it = asks_.rbegin(); it != asks_.rend(); ++it) {
    const auto &[price, level] = *it;
    std::cout << price << "\t\t" << level.total_quantity << "\t\t"
              << level.orders.size() << "\n";
  }

  std::cout << "\n--- SPREAD: ";
  auto spread = get_spread();
  if (spread) {
    std::cout << *spread;
  } else {
    std::cout << "N/A";
  }
  std::cout << " ---\n\n";

  std::cout << "BIDS (Buy Orders):\n";
  std::cout << "Price\t\tQuantity\tOrders\n";
  std::cout << "-----\t\t--------\t------\n";

  for (const auto &[price, level] : bids_) {
    std::cout << price << "\t\t" << level.total_quantity << "\t\t"
              << level.orders.size() << "\n";
  }

  std::cout << "\n";
}
