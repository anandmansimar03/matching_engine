#pragma once

#include "order_book.hpp"
#include "types.hpp"

#include <unordered_map>

namespace order_book {

// Provides an aggregated view of all the books identified via symbols
// Order Id will be unique per symbol
class GlobalOrderBook {
private:
    std::unordered_map<types::SymbolT, OrderBook> _books;
    std::size_t _book_capacity;

public:
    // disable copy
    GlobalOrderBook(const GlobalOrderBook&) = delete;
    GlobalOrderBook& operator=(const GlobalOrderBook&) = delete;

    // book capacity is the order pool size per symbol book
    explicit GlobalOrderBook(const size_t capacity) : _book_capacity(capacity) {}

    types::MatchResult add_order(const types::SymbolT& symbol, types::Order order);
    types::MatchResult modify_order(
        const types::SymbolT& symbol,
        const types::OrderIdT order_id,
        const types::QtyT new_qty);
    types::MatchResult replace_order(
        const types::SymbolT& symbol,
        const types::OrderIdT order_id,
        const types::PriceT new_price,
        const types::QtyT new_qty);
    types::MatchResult cancel_order(const types::SymbolT& symbol, const types::OrderIdT order_id);

    // utils
    size_t symbol_count() const;
    bool has_symbol(const types::SymbolT& symbol) const;
    bool has_bids(const types::SymbolT& symbol) const;
    bool has_asks(const types::SymbolT& symbol) const;
    types::PriceT best_bid(const types::SymbolT& symbol) const;
    types::PriceT best_ask(const types::SymbolT& symbol) const;
    types::QtyT qty_at(
        const types::SymbolT& symbol,
        const types::SideEnum& side,
        const types::PriceT price) const;
};

} // namespace order_book