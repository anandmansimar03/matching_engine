#include "include/order_pool.hpp"

#include <iostream>
#include <stdexcept>

namespace order_book {

OrderPool::OrderPool(const size_t capacity)
{
    if (capacity == 0) [[unlikely]] {
        throw std::runtime_error("pool capacity cannot be 0");
    }

    _slots.resize(capacity);
    for (size_t index = 0 ; index < capacity - 1 ; ++index) {
        _slots[index].next = &(_slots[index + 1]);
    }
    _slots[capacity - 1].next = nullptr;

    // point to the first free slot
    _free_head = &_slots[0];
}

types::Order* OrderPool::find(const types::OrderIdT id)
{
    auto it = _lookup_by_id.find(id);
    return (it == _lookup_by_id.end()) ? nullptr : it->second;
}

types::Order* OrderPool::allocate(const types::Order& order)
{
    // order pool is full
    if (_free_head == nullptr) [[unlikely]] {
        return nullptr;
    }

    // order id already taken
    if (find(order.order_id) != nullptr) [[unlikely]] {
        return nullptr;
    }

    // find the first available slot
    types::Order* order_slot = _free_head;
    *order_slot = order;
    // sanitization
    order_slot->prev = order_slot->next = nullptr;
    _lookup_by_id[order_slot->order_id] = order_slot;

    // move to next available slot
    _free_head = _free_head->next;

    return order_slot;
}

void OrderPool::free(types::Order* order)
{
    // cannot de-allocate nullptr
    if (order == nullptr) [[unlikely]] {
        return;
    }

    // cannot free the order which don't exist, corrupt memory
    if (find(order->order_id) == nullptr) [[unlikely]] {
        return;
    }

    // cleanup, and move the order slot to the start
    _lookup_by_id.erase(order->order_id);
    order->prev = nullptr;
    order->next = _free_head;
    _free_head = order;
}

} // namespace order_book
