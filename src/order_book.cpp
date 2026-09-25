#include "include/order_book.hpp"

#include <algorithm>
#include <chrono>
#include <limits>
#include <utility>

namespace order_book {

types::SeqNumT OrderBook::next_seq_num()
{
    return ++_seq_num;
}

types::TimestampT OrderBook::now_ns()
{
    return static_cast<types::TimestampT>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
}

bool OrderBook::has_bids() const
{
    return !_bids.empty();
}

bool OrderBook::has_asks() const
{
    return !_asks.empty();
}

types::PriceT OrderBook::best_bid() const
{
    return has_bids() ? _bids.begin()->first : std::numeric_limits<types::PriceT>::min();
}

types::PriceT OrderBook::best_ask() const
{
    return has_asks() ? _asks.begin()->first : std::numeric_limits<types::PriceT>::max();
}

types::QtyT OrderBook::qty_at(const types::SideEnum side, const types::PriceT price) const
{
    switch (side) {
        case types::SideEnum::Bid: {
            auto it = _bids.find(price);
            return (it == _bids.end()) ? 0 : it->second.total_qty;
        }
        case types::SideEnum::Ask: {
            auto it = _asks.find(price);
            return (it == _asks.end()) ? 0 : it->second.total_qty;
        }
    }

    std::unreachable();
}

types::MatchResult OrderBook::add_order(types::Order order)
{
    const types::TimestampT timestamp_ns = now_ns();
    types::MatchResult result;

    // override the original quantity
    order.original_qty = order.qty;
    
    // check for a valid quantity
    if (order.qty <= 0) [[unlikely]] {
        result.cancels.emplace_back(types::Cancel{
            .seq_num = next_seq_num(),
            .timestamp_ns = timestamp_ns,
            .order_id = order.order_id,
            .qty = order.qty,
            .original_qty = order.original_qty,
            .cancel_reason = types::CancelReasonEnum::InvalidQuantity,
        });

        return result;
    }

    // check if order already exists
    if (_order_pool.find(order.order_id) != nullptr) [[unlikely]] {
        result.cancels.emplace_back(types::Cancel{
            .seq_num = next_seq_num(),
            .timestamp_ns = timestamp_ns,
            .order_id = order.order_id,
            .qty = order.qty,
            .original_qty = order.original_qty,
            .cancel_reason = types::CancelReasonEnum::DuplicateOrder,
        });

        return result;
    }

    switch (order.side) {
        case types::SideEnum::Bid: {
            while (order.qty > 0 && has_asks()) {
                auto ask_it = _asks.begin();
                types::PriceT ask_price = ask_it->first;
                PriceLevel& price_level = ask_it->second;
                
                // stop going up for limit order
                if (order.order_type == types::OrderTypeEnum::Limit && ask_price > order.price) {
                    break;
                }

                fill_against_level(price_level, ask_price, order, true, timestamp_ns, result.trades);
                // check of level is empty
                if (price_level.head == nullptr) {
                    _asks.erase(ask_it);
                }
            }
            break;
        }
        case types::SideEnum::Ask: {
            while (order.qty > 0 && has_bids()) {
                auto bid_it = _bids.begin();
                types::PriceT bid_price = bid_it->first;
                PriceLevel& price_level = bid_it->second;
                
                // stop going down for limit order
                if (order.order_type == types::OrderTypeEnum::Limit && bid_price < order.price) {
                    break;
                }

                fill_against_level(price_level, bid_price, order, false, timestamp_ns, result.trades);
                // check of level is empty
                if (price_level.head == nullptr) {
                    _bids.erase(bid_it);
                }
            }
            break;
        }
    }

    // exit if the order is executed
    if (order.qty == 0) {
        return result;
    }

    switch (order.order_type) {
        case types::OrderTypeEnum::Limit: {
            if (!fill_limit_order(order)) [[unlikely]] {
                result.cancels.emplace_back(types::Cancel{
                    .seq_num = next_seq_num(),
                    .timestamp_ns = timestamp_ns,
                    .order_id = order.order_id,
                    .qty = order.qty,
                    .original_qty = order.original_qty,
                    .cancel_reason = types::CancelReasonEnum::OrderPoolFull,
                });
            }
            break;
        }
        case types::OrderTypeEnum::Market: {
            result.cancels.emplace_back(types::Cancel{
                .seq_num = next_seq_num(),
                .timestamp_ns = timestamp_ns,
                .order_id = order.order_id,
                .qty = order.qty,
                .original_qty = order.original_qty,
                .cancel_reason = types::CancelReasonEnum::NoLiquidity,
            });
            break;
        }
    }

    return result;
}

bool OrderBook::remove_order(types::Order* order)
{
    auto remove_from_map = [&] <typename MapT> (MapT& curr_map) -> bool {
        auto it = curr_map.find(order->price);
        if (it == curr_map.end()) [[unlikely]] {
            return false;
        }

        PriceLevel& price_level = it->second;

        // Update aggregate quantity.
        price_level.total_qty -= order->qty;

        // Remove from doubly-linked FIFO queue.
        types::Order* prev_order = order->prev;
        types::Order* next_order = order->next;

        if (prev_order) {
            prev_order->next = next_order;
        }
        else {
            price_level.head = next_order;
        }

        if (next_order) {
            next_order->prev = prev_order;
        }
        else {
            price_level.tail = prev_order;
        }

        // Remove empty price level.
        if (price_level.head == nullptr) [[unlikely]] {
            curr_map.erase(it);
        }

        return true;
    };

    bool removed = false;
    switch (order->side) {
        case types::SideEnum::Ask:
            removed = remove_from_map(_asks);
            break;

        case types::SideEnum::Bid:
            removed = remove_from_map(_bids);
            break;
    }

    if (!removed) [[unlikely]] {
        return false;
    }

    _order_pool.free(order);

    return true;
}

void OrderBook::fill_against_level(
    PriceLevel& price_level,
    const types::PriceT trade_price,
    types::Order& incoming_order,
    const bool is_incoming_order_buy,
    const types::TimestampT timestamp_ns,
    std::vector<types::Trade>& trades)
{
    while (incoming_order.qty > 0 && price_level.head != nullptr) {
        types::Order* resting_order = price_level.head;
        
        // cross book events
        types::QtyT fill_qty = std::min(resting_order->qty, incoming_order.qty);
        incoming_order.qty -= fill_qty;
        price_level.total_qty -= fill_qty;
        resting_order->qty -= fill_qty;

        // generate trade events
        trades.emplace_back(types::Trade{
            .seq_num = next_seq_num(),
            .timestamp_ns = timestamp_ns,
            .price = trade_price,
            .qty = fill_qty,
            .buyer_id = (is_incoming_order_buy) ? incoming_order.order_id : resting_order->order_id,
            .seller_id = (is_incoming_order_buy) ? resting_order->order_id : incoming_order.order_id,
        });

        // clear order at price level
        if (resting_order->qty == 0) {
            price_level.head = resting_order->next;
            if (price_level.head == nullptr) [[unlikely]] {
                price_level.tail = nullptr;
            }
            else {
                price_level.head->prev = nullptr;
            }
            _order_pool.free(resting_order);
        }
    }
}

bool OrderBook::fill_limit_order(types::Order& order)
{
    types::Order* incoming_order = _order_pool.allocate(order);
    if (incoming_order == nullptr) [[unlikely]] {
        return false;
    }

    auto fill_price_level = [&] <typename MapT> (MapT& curr_map) -> void {
        auto it = curr_map.find(order.price);
        if (it == curr_map.end()) {
            PriceLevel price_level {
                .total_qty = incoming_order->qty,
                .head = incoming_order,
                .tail = incoming_order,
            };
            curr_map.emplace(incoming_order->price, price_level);
        }
        else {
            PriceLevel& price_level = it->second;
            price_level.total_qty += incoming_order->qty;
            price_level.tail->next = incoming_order;
            incoming_order->prev = price_level.tail;
            price_level.tail = price_level.tail->next;
        }
    };

    switch (order.side) {
        case types::SideEnum::Bid: {
            fill_price_level(_bids);
            break;
        }
        case types::SideEnum::Ask: {
            fill_price_level(_asks);
            break;
        }
    }

    return true;
}

types::MatchResult OrderBook::modify_order(const types::OrderIdT order_id, const types::QtyT new_qty)
{
    types::MatchResult result;

    const types::TimestampT timestamp_ns = now_ns();

    types::Order* order = _order_pool.find(order_id);
    // Order does not exist.
    if (order == nullptr) [[unlikely]] {
        result.cancels.emplace_back(types::Cancel{
            .seq_num = next_seq_num(),
            .timestamp_ns = timestamp_ns,
            .order_id = order_id,
            .qty = 0,
            .original_qty = 0,
            .cancel_reason = types::CancelReasonEnum::UnknownOrder,
        });

        return result;
    }

    // Quantity must be positive.
    if (new_qty <= 0) [[unlikely]] {
        result.cancels.emplace_back(types::Cancel{
            .seq_num = next_seq_num(),
            .timestamp_ns = timestamp_ns,
            .order_id = order_id,
            .qty = order->qty,
            .original_qty = order->original_qty,
            .cancel_reason = types::CancelReasonEnum::InvalidQuantity,
        });

        return result;
    }

    // We only allow quantity reduction through MODIFY.
    if (new_qty > order->qty) [[unlikely]] {
        result.cancels.emplace_back(types::Cancel{
            .seq_num = next_seq_num(),
            .timestamp_ns = timestamp_ns,
            .order_id = order_id,
            .qty = order->qty,
            .original_qty = order->original_qty,
            .cancel_reason = types::CancelReasonEnum::InvalidQuantity,
        });

        return result;
    }

    // Nothing actually changed.
    if (new_qty == order->qty) {
        return result;
    }

    // Reduce the order quantity while preserving:
    // - order id
    // - side
    // - price
    // - order type
    // - queue position
    const types::QtyT quantity_reduced = order->qty - new_qty;

    order->qty = new_qty;

    // Update the aggregate quantity at this price level.
    switch (order->side) {
        case types::SideEnum::Bid: {
            auto it = _bids.find(order->price);
            if (it == _bids.end()) [[unlikely]] {
                // Internal book corruption.
                result.cancels.emplace_back(types::Cancel{
                    .seq_num = next_seq_num(),
                    .timestamp_ns = timestamp_ns,
                    .order_id = order_id,
                    .qty = order->qty,
                    .original_qty = order->original_qty,
                    .cancel_reason = types::CancelReasonEnum::UnknownOrder,
                });

                return result;
            }

            it->second.total_qty -= quantity_reduced;
            break;
        }

        case types::SideEnum::Ask: {
            auto it = _asks.find(order->price);
            if (it == _asks.end()) [[unlikely]] {
                // Internal book corruption.
                result.cancels.emplace_back(types::Cancel{
                    .seq_num = next_seq_num(),
                    .timestamp_ns = timestamp_ns,
                    .order_id = order_id,
                    .qty = order->qty,
                    .original_qty = order->original_qty,
                    .cancel_reason = types::CancelReasonEnum::UnknownOrder,
                });

                return result;
            }

            it->second.total_qty -= quantity_reduced;
            break;
        }
    }

    return result;
}

types::MatchResult OrderBook::replace_order(
    const types::OrderIdT order_id,
    const types::PriceT new_price,
    const types::QtyT new_qty)
{
    types::MatchResult result;

    const types::TimestampT timestamp_ns = now_ns();

    types::Order* existing_order = _order_pool.find(order_id);
    if (existing_order == nullptr) [[unlikely]] {
        result.cancels.emplace_back(types::Cancel{
            .seq_num = next_seq_num(),
            .timestamp_ns = timestamp_ns,
            .order_id = order_id,
            .qty = 0,
            .original_qty = 0,
            .cancel_reason = types::CancelReasonEnum::UnknownOrder,
        });

        return result;
    }

    if (new_qty <= 0) [[unlikely]] {
        result.cancels.emplace_back(types::Cancel{
            .seq_num = next_seq_num(),
            .timestamp_ns = timestamp_ns,
            .order_id = order_id,
            .qty = existing_order->qty,
            .original_qty = existing_order->original_qty,
            .cancel_reason = types::CancelReasonEnum::InvalidQuantity,
        });

        return result;
    }

    // -------------------------------------------------------------------------
    // Capture attributes of the old order before removing it.
    //
    // REPLACE preserves:
    // - order id
    // - side
    // - order type
    //
    // REPLACE changes:
    // - price
    // - quantity
    //
    // The resulting order gets a new queue position.
    // -------------------------------------------------------------------------

    const types::SideEnum side = existing_order->side;
    const types::OrderTypeEnum order_type = existing_order->order_type;

    if (!remove_order(existing_order)) [[unlikely]] {
        result.cancels.emplace_back(types::Cancel{
            .seq_num = next_seq_num(),
            .timestamp_ns = timestamp_ns,
            .order_id = order_id,
            .qty = 0,
            .original_qty = 0,
            .cancel_reason = types::CancelReasonEnum::UnknownOrder,
        });

        return result;
    }

    // -------------------------------------------------------------------------
    // Submit replacement as a brand-new order.
    //
    // add_order() will:
    // - set original_qty = new_qty
    // - attempt matching
    // - generate trades
    // - place remaining quantity at the tail of the price level
    // -------------------------------------------------------------------------

    const types::Order replacement_order{
        .order_id = order_id,
        .side = side,
        .order_type = order_type,
        .price = new_price,
        .qty = new_qty,
    };

    return add_order(replacement_order);
}

types::MatchResult OrderBook::cancel_order(
    const types::OrderIdT order_id)
{
    types::MatchResult result;

    const types::TimestampT timestamp_ns = now_ns();

    types::Order* order = _order_pool.find(order_id);

    if (order == nullptr) [[unlikely]] {
        result.cancels.emplace_back(types::Cancel{
            .seq_num = next_seq_num(),
            .timestamp_ns = timestamp_ns,
            .order_id = order_id,
            .qty = 0,
            .original_qty = 0,
            .cancel_reason = types::CancelReasonEnum::UnknownOrder,
        });

        return result;
    }

    const types::QtyT remaining_qty = order->qty;
    const types::QtyT original_qty = order->original_qty;

    if (!remove_order(order)) [[unlikely]] {
        result.cancels.emplace_back(types::Cancel{
            .seq_num = next_seq_num(),
            .timestamp_ns = timestamp_ns,
            .order_id = order_id,
            .qty = remaining_qty,
            .original_qty = original_qty,
            .cancel_reason = types::CancelReasonEnum::UnknownOrder,
        });

        return result;
    }

    result.cancels.emplace_back(types::Cancel{
        .seq_num = next_seq_num(),
        .timestamp_ns = timestamp_ns,
        .order_id = order_id,
        .qty = remaining_qty,
        .original_qty = original_qty,
        .cancel_reason = types::CancelReasonEnum::UserRequested,
    });

    return result;
}

} // namespace order_book
