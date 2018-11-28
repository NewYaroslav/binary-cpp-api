#include <iostream>
#include "BinaryAPI.hpp"

using namespace std;

int main() {
        BinaryApi iBinaryApi;
        std::vector<std::string> symbols;
        symbols.push_back("frxEURUSD");
        symbols.push_back("frxEURGBP");
        symbols.push_back("frxEURJPY");
        std::cout << "init_symbols " << std::endl;
        iBinaryApi.init_symbols(symbols);
        std::cout << "init_stream_quotations " << iBinaryApi.init_stream_quotations(60) << std::endl;
        std::cout << "init_stream_proposal " << iBinaryApi.init_stream_proposal(10, 3, iBinaryApi.MINUTES, "USD") << std::endl;
        std::cout << "..." << std::endl;
        unsigned long long servertime_last = 0;
        while(true) {
                // для всех валютных пар
                std::vector<std::vector<double>> close_data; // цены закрытия
                std::vector<std::vector<unsigned long long>> time_data; // время открытия свечей
                std::vector<double> buy_data; // проценты выплат
                std::vector<double> sell_data;
                unsigned long long servertime = 0;

                //std::cout << "get_servertime" << std::endl;
                if(iBinaryApi.get_servertime(servertime) != iBinaryApi.OK) {
                        //std::cout << "request_servertime" << std::endl;
                        iBinaryApi.request_servertime();
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        continue;
                } else {
                        if(servertime > servertime_last) {
                                servertime_last = servertime;
                        } else {
                                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                                //std::cout << "continue " << servertime << std::endl;
                                continue;
                        }
                }
                //std::cout << "get_stream_quotations" << std::endl;
                if(iBinaryApi.get_stream_quotations(close_data, time_data) == iBinaryApi.OK &&
                   iBinaryApi.get_stream_proposal(buy_data, sell_data) == iBinaryApi.OK) {
                        // выводим последние значения котировок валютных пар
                        for(size_t i = 0; i < symbols.size(); ++i) {
                                std::cout << symbols[i] << " " << buy_data[i] << "/" << sell_data[i] << std::endl;
                                for(size_t j = time_data[i].size() - 3; j < time_data[i].size(); ++j) {
                                        std::cout << close_data[i][j] << " " << time_data[i][j] << std::endl;
                                }
                                std::cout << "size: " << time_data[i].size() << std::endl;
                        }
                }
                // время сервера в GMT\UTC
                std::cout << servertime << std::endl;
                std::cout << std::endl;
                std::this_thread::yield();
        }
}
