#include <iostream>
#include "order_book_fast.h"
#include <map>
#include <cassert>
#include <algorithm>
#include <functional>
#include <array>
#include <string_view>

void error_with_log(std::string_view message){
    std::cerr << "Error: "<< message << std::endl;
    exit(1);
}

OrderBook::OrderBook(uint16_t max_stock_locate)
   :bid(max_stock_locate + 1),
    ask(max_stock_locate + 1),
    base(max_stock_locate + 1)
{}

int OrderBook::get_index(Order &ord){
    return 10'000 + ((int64_t)ord.limit_price - this->base[ord.stock_locate]) / 100;
}

bool OrderBook::is_hot_storage(int idx){
    return idx >=0 && idx < 20'000 ? true : false;
}


void OrderBook::add_order(AddOrder &o){
    Order ord {
        .ref = o.ref,
        .sym = o.stock,
        .stock_locate = o.stock_locate,
        .add_timestamp = o.timestamp,
        .limit_price = o.price,
        .side = o.side,
        .add_shares = o.shares
    };
    this -> order_map[ord.ref] = ord;

    int idx;
    if (this->base[ord.stock_locate] == 0){
        this->base[ord.stock_locate] = ord.limit_price;
        idx = 10'000;
    } else {
        idx = get_index(ord);
    }

    if (is_hot_storage(idx)){
        auto& book = (ord.side == 'B') ? bid : ask;
        book[ord.stock_locate][idx] += ord.add_shares;
    } else {
        auto& cold_book = (ord.side == 'B') ? cold_bid : cold_ask;
        cold_book[ord.stock_locate][ord.limit_price] += ord.add_shares;
    }
}

void OrderBook::execute_order(ExecutedOrder &e){
    auto &order_node = this->order_map.at(e.ref);
    assert(order_node.status != OrderStatus::EXECUTED && "Got execute order for already executed order");

    order_node.execute_shares += e.executed_shares;
    order_node.execute_timestamp = e.timestamp;

    if (order_node.execute_shares == (order_node.add_shares - order_node.cancel_shares)) {
        order_node.price_executed = order_node.limit_price;
        order_node.status = OrderStatus::EXECUTED;
    } else{
        order_node.status = OrderStatus::PARTIAL_EXECUTED;
    }

    int idx = get_index(order_node);

    if (is_hot_storage(idx)){
        auto &book = (order_node.side == 'B'? bid : ask);
        auto &shares = book[order_node.stock_locate][idx];
        
        if (shares < e.executed_shares){
            error_with_log("book desynced");
        }

        shares -= e.executed_shares;
    } else {
        auto &cold_book = (order_node.side == 'B' ? cold_bid : cold_ask);
        auto &shares = cold_book[order_node.stock_locate][order_node.limit_price];

        if (shares < e.executed_shares){
            error_with_log("Book desynced");
        }
        shares -=e.executed_shares;
    }
}

void OrderBook::execute_order_with_price(ExecutedWithPriceOrder &e){
    auto &order_node = this->order_map.at(e.ref);
    assert(order_node.status != OrderStatus::EXECUTED && "Got execute order for already executed order");

    order_node.execute_shares += e.executed_shares;
    order_node.execute_timestamp = e.timestamp;


    if (order_node.execute_shares == (order_node.add_shares - order_node.cancel_shares)) {
        order_node.price_executed = e.execution_price;
        order_node.status = OrderStatus::EXECUTED;
    } else {
        order_node.status = OrderStatus::PARTIAL_EXECUTED;
    }
    int idx = get_index(order_node);

    if (is_hot_storage(idx)){
        auto &book = (order_node.side == 'B'? bid : ask);
        auto &shares = book[order_node.stock_locate][idx];
        
        if (shares < e.executed_shares){
            error_with_log("book desynced");
        }

        shares -= e.executed_shares;
    } else {
        auto &cold_book = (order_node.side == 'B' ? cold_bid : cold_ask);
        auto &shares = cold_book[order_node.stock_locate][order_node.limit_price];

        if (shares < e.executed_shares){
            error_with_log("Book desynced");
        }
        shares -=e.executed_shares;
    }
}

void OrderBook::cancel_order(CancelOrder &c){

    auto &order_node = this->order_map.at(c.ref);

    if (order_node.status == OrderStatus::PARTIAL_EXECUTED) {
        if (c.cancelled_shares > order_node.available_shares()) {
            error_with_log("cancelling more then available shares");
        }
        order_node.cancel_shares += c.cancelled_shares;
    } else if (order_node.status == OrderStatus::EXECUTED){
        error_with_log("Cancel request for the order which is already completely executed");
    } else if (order_node.status == OrderStatus::CANCELLED){
        error_with_log("Cancel request for the order which is already completely cancelled");
    } else {
        order_node.cancel_shares += c.cancelled_shares;
        if (order_node.add_shares == order_node.cancel_shares){
            order_node.status = OrderStatus::CANCELLED;
        }
    }
    int idx = get_index(order_node);

    if (is_hot_storage(idx)){
        auto &book = (order_node.side == 'B'? bid : ask);
        auto &shares = book[order_node.stock_locate][idx];

        shares -= c.cancelled_shares;
    } else {
        auto &cold_book = (order_node.side == 'B' ? cold_bid : cold_ask);
        auto &shares = cold_book[order_node.stock_locate][order_node.limit_price];

        shares -= c.cancelled_shares;
    }
}

void OrderBook::delete_order(DeleteOrder &d){
    auto &order_node = this->order_map.at(d.ref);

    order_node.status = OrderStatus::CANCELLED;
    int shares_to_cancel = order_node.available_shares();
    order_node.cancel_shares += (shares_to_cancel);

    int idx = get_index(order_node);

    if (is_hot_storage(idx)){
        auto &book = (order_node.side == 'B'? bid : ask);
        auto &shares = book[order_node.stock_locate][idx];

        shares -= shares_to_cancel;   // remove only this order's remaining shares
    } else {
        auto &cold_book = (order_node.side == 'B' ? cold_bid : cold_ask);
        auto &shares = cold_book[order_node.stock_locate][order_node.limit_price];

        shares -= shares_to_cancel;   // remove only this order's remaining shares
    }
    this->order_map.erase(d.ref);
}

void OrderBook::replace_order(ReplaceOrder &r){
    auto &order_node = this->order_map.at(r.ref);

    DeleteOrder d {
        .stock_locate = r.stock_locate,
        .timestamp = r.timestamp,
        .ref = r.ref
    };
    std::string sym = order_node.sym;
    char side = order_node.side;
    this->delete_order(d);

    AddOrder o {
        .stock_locate = r.stock_locate,
        .timestamp = r.timestamp,
        .stock = sym, 
        .ref = r.new_ref, 
        .price = r.price,
        .shares = r.shares,
        .side = side
    };
    this->add_order(o);
}

static bool cmp(const std::pair<uint32_t, uint32_t> &a,const std::pair<uint32_t,uint32_t> &b){
    return a.first > b.first;
}

book_map OrderBook::get_book(uint16_t stock_locate, int levels){
    // std::cout<<"came to get book"<<std::endl;
    book_map bm {};

    // ASK
    int count =0;
    std::vector<std::pair<uint32_t, uint32_t>> ask_array; 
    for(auto [price, quantity] : this->cold_ask[stock_locate]){
        ask_array.push_back({price, quantity});
    }
    // std::cout<<"pushed map aarray"<< std::endl;
    for(int i = 0; i < 20'000; i++){
        if (this->ask[stock_locate][i] != 0){
            uint32_t price = this->base[stock_locate] + (i - 10'000) * 100;
            ask_array.push_back({price, this->ask[stock_locate][i]});
        }
    }
    // std::cout<<"pushed other one"<<std::endl;
    std::sort(ask_array.begin(), ask_array.end());
    
    for(int i =0;i < ask_array.size(); i++){
        if (i >= levels) break;
        bm.ask.push_back(ask_array[i]);
    }
    
    // BID
    count = 0;
    std::vector<std::pair<uint32_t, uint32_t>> bid_array; 
    for(auto [price, quantity] : this->cold_bid[stock_locate]){
        bid_array.push_back({price, quantity});
    }
    for(int i = 0; i < 20'000; i++){
        if (this->bid[stock_locate][i] != 0){
            int price = this->base[stock_locate] + (i - 10'000) * 100;
            bid_array.push_back({price, this->bid[stock_locate][i]});
        }
    }
    std::sort(bid_array.begin(), bid_array.end(), cmp);
    
    for(int i =0;i < bid_array.size(); i++){
        if (i >= levels) break;
        bm.bid.push_back(bid_array[i]);
    }
    
    return bm;
}