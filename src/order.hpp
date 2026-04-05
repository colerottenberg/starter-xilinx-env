#pragma once

#include <cstdint>
#include <string>

enum class Side { BUY, SELL };

enum class OrderType { LIMIT, MARKET };

struct Order {
  uint64_t order_id;
  std::string symbol;
  Side side;
  OrderType type;
  double price;       // Price per unit (0 for market orders)
  uint64_t quantity;  // Remaining quantity
  uint64_t original_quantity;
  uint64_t timestamp; // Nanoseconds since epoch for time priority

  Order(uint64_t id, std::string sym, Side s, OrderType t, double p,
        uint64_t qty, uint64_t ts)
      : order_id(id), symbol(std::move(sym)), side(s), type(t), price(p),
        quantity(qty), original_quantity(qty), timestamp(ts) {}

  bool is_filled() const { return quantity == 0; }
};

struct Trade {
  uint64_t trade_id;
  uint64_t buy_order_id;
  uint64_t sell_order_id;
  double price;
  uint64_t quantity;
  uint64_t timestamp;

  Trade(uint64_t id, uint64_t buy_id, uint64_t sell_id, double p, uint64_t qty,
        uint64_t ts)
      : trade_id(id), buy_order_id(buy_id), sell_order_id(sell_id), price(p),
        quantity(qty), timestamp(ts) {}
};

