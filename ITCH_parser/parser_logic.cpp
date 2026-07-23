#include <iostream>
#include <sys/mman.h>
#include <string_view>
#include <endian.h>
#include <cstdint>
#include <cstring>
#include <sstream>
#include "parser_logic.h"

uint32_t read_be32(const unsigned char* p) {
    uint32_t v;
    std::memcpy(&v, p, 4);
    return be32toh(v);
}

uint64_t read_be64(const unsigned char* p) {
    uint64_t v;
    std::memcpy(&v, p, 8);
    return be64toh(v);
}

uint16_t read_be16(const unsigned char* p){
    uint16_t v;
    std::memcpy(&v, p, 2);
    return be16toh(v);
}

uint64_t read_be48(const unsigned char* p){
    uint64_t v = 0;
    for(int i = 0; i< 6; ++i){
        v = (v << 8) | p[i];
    }
    return v;
}



void AddOrder::print_struct(){
    std::ostringstream ss;
    ss<< "Add Order Information: "<<std::endl;
    ss<< "stock_locate: "<< this->stock_locate<<std::endl;
    ss<< "timestamp: "<< this->timestamp<<std::endl;
    ss<< "stock: "<< this->stock<<std::endl;
    ss<< "ref: " << this->ref << std::endl;
    ss<< "price: "<< this-> price << std::endl;
    ss << "shares: "<< this-> shares << std::endl;
    ss << "side: " << this-> side << std::endl;
    std::cout<< ss.str()<<std::endl;
}

void AddOrder::validate_info(){
    if(this->timestamp <= 0) std::cerr << "timestamp must be > 0" << std::endl;
    if(this->ref <= 0) std::cerr << "ref must be > 0" << std::endl;
    if(this->price <= 0) std::cerr << "price must be > 0" << std::endl;
    if(this->side != 'B' && this->side != 'S') std::cerr << "side must be B or S" << std::endl;
}

void ExecutedOrder::print_struct(){
    std::ostringstream ss;
    ss<< "Executed Order Information: "<<std::endl;
    ss<< "stock_locate: "<< this->stock_locate<<std::endl;
    ss<< "timestamp: "<< this->timestamp<<std::endl;
    ss<< "ref: " << this->ref << std::endl;
    ss<< "executed_shares: "<< this->executed_shares << std::endl;
    std::cout<< ss.str()<<std::endl;
}

void ExecutedOrder::validate_info(){
    if(this->timestamp <= 0) std::cerr << "timestamp must be > 0" << std::endl;
    if(this->ref <= 0) std::cerr << "ref must be > 0" << std::endl;
    if(this->executed_shares <= 0) std::cerr << "executed_shares must be > 0" << std::endl;
}

void ExecutedWithPriceOrder::print_struct(){
    std::ostringstream ss;
    ss<< "Executed With Price Order Information: "<<std::endl;
    ss<< "stock_locate: "<< this->stock_locate<<std::endl;
    ss<< "timestamp: "<< this->timestamp<<std::endl;
    ss<< "ref: " << this->ref << std::endl;
    ss<< "executed_shares: "<< this->executed_shares << std::endl;
    ss<< "execution_price: "<< this->execution_price << std::endl;
    std::cout<< ss.str()<<std::endl;
}

void ExecutedWithPriceOrder::validate_info(){
    if(this->timestamp <= 0) std::cerr << "timestamp must be > 0" << std::endl;
    if(this->ref <= 0) std::cerr << "ref must be > 0" << std::endl;
    if(this->executed_shares <= 0) std::cerr << "executed_shares must be > 0" << std::endl;
    if(this->execution_price <= 0) std::cerr << "execution_price must be > 0" << std::endl;
}

void CancelOrder::print_struct(){
    std::ostringstream ss;
    ss<< "Cancel Order Information: "<<std::endl;
    ss<< "stock_locate: "<< this->stock_locate<<std::endl;
    ss<< "timestamp: "<< this->timestamp<<std::endl;
    ss<< "ref: " << this->ref << std::endl;
    ss<< "cancelled_shares: "<< this->cancelled_shares << std::endl;
    std::cout<< ss.str()<<std::endl;
}

void CancelOrder::validate_info(){
    if(this->timestamp <= 0) std::cerr << "timestamp must be > 0" << std::endl;
    if(this->ref <= 0) std::cerr << "ref must be > 0" << std::endl;
    if(this->cancelled_shares <= 0) std::cerr << "cancelled_shares must be > 0" << std::endl;
}

void DeleteOrder::print_struct(){
    std::ostringstream ss;
    ss<< "Delete Order Information: "<<std::endl;
    ss<< "stock_locate: "<< this->stock_locate<<std::endl;
    ss<< "timestamp: "<< this->timestamp<<std::endl;
    ss<< "ref: " << this->ref << std::endl;
    std::cout<< ss.str()<<std::endl;
}

void DeleteOrder::validate_info(){
    if(this->timestamp <= 0) std::cerr << "timestamp must be > 0" << std::endl;
    if(this->ref <= 0) std::cerr << "ref must be > 0" << std::endl;
}


void ReplaceOrder::print_struct(){
    std::ostringstream ss;
    ss<< "Replace Order Information: "<<std::endl;
    ss<< "stock_locate: "<< this->stock_locate<<std::endl;
    ss<< "timestamp: "<< this->timestamp<<std::endl;
    ss<< "ref: " << this->ref << std::endl;
    ss<< "new_ref: "<< this->new_ref << std::endl;
    ss<< "shares: "<< this->shares << std::endl;
    ss<< "price: "<< this->price << std::endl;
    std::cout<< ss.str()<<std::endl;
}

void ReplaceOrder::validate_info(){
    if(this->timestamp <= 0) std::cerr << "timestamp must be > 0" << std::endl;
    if(this->ref <= 0) std::cerr << "ref must be > 0" << std::endl;
    if(this->new_ref <= 0) std::cerr << "new_ref must be > 0" << std::endl;
    if(this->shares <= 0) std::cerr << "shares must be > 0" << std::endl;
    if(this->price <= 0) std::cerr << "price must be > 0" << std::endl;
}

AddOrder ParseAddOrderMessage(const unsigned char* message){
    std::string_view sym(reinterpret_cast<const char*>(&message[0] + 24), 8);
    sym = sym.substr(0, sym.find_last_not_of(' ') + 1);
    AddOrder o{
        .stock_locate = read_be16(&message[1]),
        .timestamp = read_be48(&message[5]),
        .stock = std::string(sym),
        .ref = read_be64(&message[11]),
        .price = read_be32(&message[32]),
        .shares = read_be32(&message[20]),
        .side = message[19]
    };
    return o;
}

ExecutedOrder ParseExecutedOrderMessage(const unsigned char* message){
    ExecutedOrder o{
        .stock_locate = read_be16(&message[1]),
        .timestamp = read_be48(&message[5]),
        .ref = read_be64(&message[11]),
        .executed_shares = read_be32(&message[19])
    };
    return o;
}

ExecutedWithPriceOrder ParseExecutedWithPriceOrderMessage(const unsigned char* message){
    ExecutedWithPriceOrder o{
        .stock_locate = read_be16(&message[1]),
        .timestamp = read_be48(&message[5]),
        .ref = read_be64(&message[11]),
        .executed_shares = read_be32(&message[19]),
        .execution_price = read_be32(&message[32])
    };
    return o;
}

CancelOrder ParseCancelOrder(const unsigned char* message){
    CancelOrder c{
        .stock_locate = read_be16(&message[1]),
        .timestamp = read_be48(&message[5]),
        .ref = read_be64(&message[11]),
        .cancelled_shares = read_be32(&message[19])
    };
    return c;
}
DeleteOrder ParseDeleteOrder(const unsigned char* message){
    DeleteOrder d{
        .stock_locate = read_be16(&message[1]),
        .timestamp = read_be48(&message[5]),
        .ref = read_be64(&message[11])
    };
    return d;
}

ReplaceOrder ParseReplaceOrder(const unsigned char* message){
    ReplaceOrder o{
        .stock_locate = read_be16(&message[1]),
        .timestamp = read_be48(&message[5]),
        .ref = read_be64(&message[11]),
        .new_ref = read_be64(&message[19]),
        .shares = read_be32(&message[27]),
        .price = read_be32(&message[31])
    };
    return o;
}
