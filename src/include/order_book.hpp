#pragma once

#include "order_pool.hpp"
#include "price_level.hpp"

#include <functional>
#include <map>
#include <vector>

namespace order_book {

class OrderBook {
private:
    OrderPool _order_pool;
    std::map<types::PriceT, PriceLevel, std::greater<types::PriceT>> _bids;
    std::map<types::PriceT, PriceLevel, std::less<types::PriceT>> _asks;

    // localized sequencing
    types::SeqNumT _seq_num = 0;

    types::SeqNumT next_seq_num();
    static types::TimestampT now_ns();

    bool fill_limit_order(types::Order& order);
    void fill_against_level(
        PriceLevel& price_level,
        const types::PriceT trade_price,
        types::Order& incoming_order,
        const bool is_incoming_order_buy,
        const types::TimestampT timestamp_ns,
        std::vector<types::Trade>& trades);

public:
    // disable copy
    OrderBook(const OrderBook& order_book) = delete;
    OrderBook& operator=(const OrderBook& order_book) = delete;

    explicit OrderBook(const size_t capacity) : _order_pool(capacity) {}

    types::MatchResult add_order(types::Order order);
    types::MatchResult cancel_order(const types::OrderIdT order_id);

    // utils
    bool has_bids() const;
    bool has_asks() const;
    types::PriceT best_bid() const;
    types::PriceT best_ask() const;
    types::QtyT qty_at(const types::SideEnum side, const types::PriceT price) const;
};

} // namespace order_book
