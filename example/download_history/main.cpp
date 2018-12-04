#include <iostream>
#include <xtime.hpp>
#include "BinaryAPI.hpp"

using namespace std;

int main() {
        BinaryAPI apiBinary;
        apiBinary.set_use_log(true);
        std::vector<std::string> symbols;
        std::vector<double> candles_close;
        std::vector<unsigned long long> candles_times;
        std::vector<double> prices;
        std::vector<unsigned long long> times;

        unsigned long long t1 = xtime::get_unix_timestamp(5,11,2018,0,0,0);
        unsigned long long t2 = t1 + xtime::SEC_DAY;


        std::cout << "get_candles " << apiBinary.get_candles_without_limits("frxEURUSD", candles_close, candles_times, t1 + 30, t2 + xtime::SEC_DAY * 30) << std::endl;
        if(candles_times.size() > 0) {
                std::cout << "t1: " << t1 << "/" << candles_times[0] << std::endl;
                std::cout << "t2: " << t2 << "/" << candles_times.back() << std::endl;
                std::cout << "candles_times.size() " << candles_times.size() << std::endl;
        } else {
                std::cout << "candles_times.size() " << candles_times.size() << std::endl;
        }

        std::cout << "get_ticks " << apiBinary.get_ticks_without_limits("frxEURUSD", prices, times, t1, t2) << std::endl;
        if(times.size() > 0) {
                std::cout << "t1: " << t1 << "/" << times[0] << std::endl;
                std::cout << "t2: " << t2 << "/" << times.back() << std::endl;
                std::cout << "times.size() " << times.size() << std::endl;
        } else {
                std::cout << "times.size() " << times.size() << std::endl;
        }
        return 0;
}
