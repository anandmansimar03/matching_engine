#include "include/global_order_book.hpp"

#include <limits>

namespace order_book {

types::MatchResult GlobalOrderBook::add_order(const types::SymbolT& symbol, types::Order order)
{
    auto [it, inserted] = _books.try_emplace(symbol, _book_capacity);
    return it->second.add_order(order);
}

types::MatchResult GlobalOrderBook::cancel_order(const types::SymbolT& symbol, const types::OrderIdT order_id)
{
    auto it = _books.find(symbol);
    if (it == _books.end()) [[unlikely]] {
        types::MatchResult result;
        result.cancels.emplace_back(types::Cancel{
            .seq_num = 0,
            .timestamp_ns = 0,
            .order_id = order_id,
            .qty = 0,
            .original_qty = 0,
            .cancel_reason = types::CancelReasonEnum::UnknownOrder,
        });
    }

    return it->second.cancel_order(order_id);
}

size_t GlobalOrderBook::symbol_count() const
{
    return _books.size();
}

bool GlobalOrderBook::has_symbol(const types::SymbolT& symbol) const
{
    return _books.count(symbol) != 0;
}

bool GlobalOrderBook::has_bids(const types::SymbolT& symbol) const
{
    auto it = _books.find(symbol);
    return (it == _books.end()) ? false : it->second.has_bids();
}

bool GlobalOrderBook::has_asks(const types::SymbolT& symbol) const
{
    auto it = _books.find(symbol);
    return (it == _books.end()) ? false : it->second.has_asks();
}

types::PriceT GlobalOrderBook::best_bid(const types::SymbolT& symbol) const
{
    auto it = _books.find(symbol);
    return (it == _books.end()) ? std::numeric_limits<types::PriceT>::min() : it->second.best_bid();
}

types::PriceT GlobalOrderBook::best_ask(const types::SymbolT& symbol) const
{
    auto it = _books.find(symbol);
    return (it == _books.end()) ? std::numeric_limits<types::PriceT>::max() : it->second.best_ask();
}

types::QtyT GlobalOrderBook::qty_at(const types::SymbolT& symbol, const types::SideEnum& side, const types::PriceT price) const
{
    auto it = _books.find(symbol);
    return (it == _books.end()) ? 0 : it->second.qty_at(side, price);
}

} // namespace order_book