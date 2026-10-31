#include <iostream>
#include <vector>
#include <benchmark/benchmark.h>
#include <chrono>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unordered_map>
#include <cstring>
#include <sstream>
#include <cstdint>
#include <cstdio>
#include "parser_logic.h"
#include "order_book.h"
#include <string_view>
#include <algorithm>
#define SAMPLE_SIZE 100

// Usage: itch_parser [path/to/itch50.bin] [stock_locate_to_snapshot]
// Defaults keep the original behaviour: sample_itch50.bin in the working directory, locate 393.
int get_sample_bin(const char* path){
    int fd = open(path, O_RDONLY);
    if (fd < 0){
        std::cout<<"File Reading Failed: "<<path<<std::endl;
        exit(1);
    }
    return fd;
}

const unsigned char* get_bin_to_char_ds(int fd, size_t size){
    const unsigned char* data = static_cast<const unsigned char*>(
        mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0));
    if (data == MAP_FAILED){
        std::cout<<"data conversion failed"<<std::endl;
        exit(1);
    }
    return data;
}

int parse_message(const unsigned char* data, size_t size, int max_stock_locate, uint16_t snapshot_locate){
    size_t i = 0;
    int count = 0;
    OrderBook order_book {max_stock_locate};
    // Each frame is a 2-byte big-endian length followed by the message. Stop at a truncated tail.
    while(i + 2 <= size){
        size_t length = read_be16(&data[i]);
        if (length == 0 || i + 2 + length > size) break;
        count++;
        i+=2;
        switch(data[i]){
            case 'A':
            case 'F': {
                AddOrder o = ParseAddOrderMessage(&data[i]);
                benchmark::DoNotOptimize(o);
                order_book.add_order(o);
                // o.print_struct();
                // o.validate_info();
                break;
            }
            case 'E': {
                ExecutedOrder e = ParseExecutedOrderMessage(&data[i]);
                benchmark::DoNotOptimize(e);
                order_book.execute_order(e);
                // e.print_struct();
                // e.validate_info();
                break;
            }
            case 'C': {
                ExecutedWithPriceOrder e = ParseExecutedWithPriceOrderMessage(&data[i]);
                benchmark::DoNotOptimize(e);
                order_book.execute_order_with_price(e);
                // e.print_struct();
                // e.validate_info();
                break;
            }
            case 'X': {
                CancelOrder c = ParseCancelOrder(&data[i]);
                benchmark::DoNotOptimize(c);
                order_book.cancel_order(c);
                // c.print_struct();
                // c.validate_info();
                break;
            }
            case 'D': {
                DeleteOrder d = ParseDeleteOrder(&data[i]);
                benchmark::DoNotOptimize(d);
                order_book.delete_order(d);
                // d.print_struct();
                // d.validate_info();
                break;
            }
            case 'U': {
                ReplaceOrder r = ParseReplaceOrder(&data[i]);
                benchmark::DoNotOptimize(r);
                order_book.replace_order(r);
                // r.print_struct();
                // r.validate_info();
                break;
            }
        }

        if (count % 100 == 0){
            book_map b = order_book.get_book(snapshot_locate, 10);
            b.validate_book();
        }
        i+=length;
    }
    return count;
}


int main(int argc, char** argv){
    const char* path = argc > 1 ? argv[1] : "sample_itch50.bin";
    uint16_t snapshot_locate = argc > 2 ? static_cast<uint16_t>(std::stoi(argv[2])) : 393;
    int fd = get_sample_bin(path);

    struct stat st;
    fstat(fd, &st);
    size_t size = st.st_size;
    
    const unsigned char* data = get_bin_to_char_ds(fd, size);


    std::vector<double> result;
    int max_stock_locate = FindMaxStockLocateCode(data, static_cast<size_t>(size));

    int sam_size = SAMPLE_SIZE;
    int current_count = 1;
    while(sam_size--){
        std::cout<<"Sample Count: "<< current_count<<std::endl;
        current_count++;
        auto t0 = std::chrono::steady_clock::now();
        int message_count = parse_message(data, size, max_stock_locate, snapshot_locate);
        auto t1 = std::chrono::steady_clock::now();
    
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        result.push_back(static_cast<double>(duration) / message_count);
    }

    std::sort(result.begin(), result.end());

    std::ostringstream ss;
    std::unordered_map<std::string, double> m{
        {"50", 0.50 * SAMPLE_SIZE},
        {"99", 0.99 * SAMPLE_SIZE},
        {"99.9", 0.999 * SAMPLE_SIZE}
    };

    ss<< "messages per replay: "<< parse_message(data, size, max_stock_locate, snapshot_locate) <<std::endl;
    ss<< "50th percentile: "<< result[(int)m["50"]]<<std::endl;
    ss << "99th percentile: "<< result[(int)m["99"]] << std::endl;
    ss<< "99.9 percentile: "<< result[(int)m["99.9"]] << std::endl;
    std::cout<< ss.str()<<std::endl;

    return 0;
}