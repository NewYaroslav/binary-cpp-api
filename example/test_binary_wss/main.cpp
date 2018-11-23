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
        std::cout << "..." << std::endl;
        unsigned long long servertime_last = 0;
        while(true) {
                std::vector<std::vector<double>> close_data;
                std::vector<std::vector<unsigned long long>> time_data;
                unsigned long long servertime = 0;
                if(iBinaryApi.get_servertime(servertime) != iBinaryApi.OK) {
                        iBinaryApi.request_servertime();
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        continue;
                } else {
                        if(servertime > servertime_last) {
                                servertime_last = servertime;
                        } else {
                              continue;
                        }
                }
                iBinaryApi.get_stream_quotations(close_data, time_data);
                // выводим последние значения котировок валютных пар
                for(size_t i = 0; i < symbols.size(); ++i) {
                        for(size_t j = time_data[i].size() - 3; j < time_data[i].size(); ++j) {
                                std::cout << symbols[i] << " " << close_data[i][j] << " " << time_data[i][j] << std::endl;
                        }
                        std::cout << symbols[i] << " size: " << time_data[i].size() << std::endl;
                }
                // время сервера в GMT\UTC
                std::cout << servertime << std::endl;
                std::cout << std::endl;
        }
}
