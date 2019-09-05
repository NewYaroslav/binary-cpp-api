#include <iostream>
#include <xtime.hpp>
#include "BinaryAPI.hpp"
#include "BinaryApiEasy.hpp"
#include "xquotes_history.hpp"

using namespace std;

int download_symbol(
        BinaryAPI &api,
        const std::string symbol,
        const std::string path,
        const bool is_day_off_filter = true);

int main(int argc, char *argv[]) {
    std::string path;
    if(argc == 2) {
        path = std::string(argv[1]);
    }
    while(true) {
        try {
            BinaryAPI apiBinary;
            apiBinary.set_use_log(true);
            std::vector<std::string> symbols = BinaryApiEasy::get_list_symbol();

            for(size_t i = 0; i < symbols.size(); ++i) {
                bool is_day_off_filter = true;
                if(symbols[i] == "R_100" ||
                    symbols[i] == "R_50" ||
                    symbols[i] == "R_25" ||
                    symbols[i] == "R_10") is_day_off_filter = false;
                download_symbol(apiBinary, symbols[i], path, is_day_off_filter);
            }
            break;
        }
        catch(...) {
            std::cout << "error, restart..." << std::endl;
        }
    }
    return 0;
}

int download_symbol(
        BinaryAPI &api,
        const std::string symbol,
        const std::string path,
        const bool is_day_off_filter) {
    // самая ранняя метка времени
    if(path != "") bf::create_directory(path);
    // конечная метка времени
    const xtime::timestamp_t stop_time = xtime::get_first_timestamp_day(xtime::get_timestamp()) - xtime::SECONDS_IN_DAY;
    const xtime::timestamp_t start_time = stop_time - 2*xtime::SECONDS_IN_YEAR;
    // создаем или сохраняем в уже созданное хранилище
    std::string file_name = path + "//" + symbol + ".qhs4";
    xquotes_history::QuotesHistory<> iQuotesHistory(
            file_name,
            xquotes_history::PRICE_OHLC,
            xquotes_history::USE_COMPRESSION);

    int err = BinaryAPI::OK;
    int num_download = 0;
    int num_errors = 0;
    xtime::timestamp_t start_day_timestamp = start_time;
    while(start_day_timestamp <= stop_time) {
        // проверяем, существует файл или если сработал фильтр выходного дня
        if((iQuotesHistory.check_timestamp(start_day_timestamp)) ||
            (is_day_off_filter && xtime::is_day_off(start_day_timestamp))) {
            start_day_timestamp += xtime::SECONDS_IN_DAY;
            continue;
        }
        std::vector<xquotes_common::Candle> _candles;
        err = api.get_candles_without_limits(
            symbol,
            _candles,
            start_day_timestamp,
            start_day_timestamp + xtime::SECONDS_IN_DAY - 1);

        if(err == BinaryAPI::OK && _candles.size() > 0) { // данные получены
            std::array<xquotes_common::Candle, xtime::MINUTES_IN_DAY> candles;
            for(size_t i = 0; i < _candles.size(); ++i) {
                candles[xtime::get_minute_day(_candles[i].timestamp)] = _candles[i];
            }
            iQuotesHistory.write_candles(candles, start_day_timestamp);
            start_day_timestamp += xtime::SECONDS_IN_DAY;
            num_download++;
            num_errors = 0;
            std::cout << "write " << symbol << " " << xtime::get_str_date(start_day_timestamp) << std::endl;
        } else {
            num_errors++;
            std::cout << "error: " << num_errors << " " << xtime::get_str_date(start_day_timestamp) << " code " << err << std::endl;
            const int MAX_ERRORS = 3;
            if(num_errors > MAX_ERRORS) {
                start_day_timestamp += xtime::SECONDS_IN_DAY;
            }
        }
        const int MAX_ERRORS = 30;
        if(num_errors > MAX_ERRORS) {
            break;
        }
    }
    std::cout << "total downloads: " << num_download << std::endl;
    if(num_download == 0) {
            if(err != BinaryAPI::OK)
                    return err;
            return -1;
    }
    return BinaryAPI::OK;
}
