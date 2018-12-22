/*
* binary-cpp-api - Binary C++ API client
*
* Copyright (c) 2018 Elektro Yar. Email: git.electroyar@gmail.com
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/
#ifndef HISTORICALDATAEASY_HPP_INCLUDED
#define HISTORICALDATAEASY_HPP_INCLUDED
//------------------------------------------------------------------------------
#include "ZstdEasy.hpp"
#include "BinaryApiEasy.hpp"
//------------------------------------------------------------------------------
namespace HistoricalDataEasy
{
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
        class HistoricalData
        {
        public:
                enum ErrorType {
                        OK = 0,
                        INVALID_PARAMETER = -6,
                        DATA_NOT_AVAILABLE = -7,
                        NO_TIMESTAMP = - 14,
                };

                enum ContractType {
                        BUY = 1,
                        SELL = -1,
                };

                enum StateType {
                        WIN = 1,
                        NEUTRAL = 0,
                        LOSS = -1,
                };
        private:
                std::string dictionary_file_;
                std::string path_;
                std::string current_file_name = "";
                std::vector<double> prices;
                std::vector<unsigned long long> times;
//------------------------------------------------------------------------------
                int read_file(unsigned long long timestamp)
                {
                        prices.clear();
                        times.clear();
                        int err = 0;

                        std::string file_name = path_ + "//" + BinaryApiEasy::get_file_name_from_date(timestamp);
                        if(dictionary_file_ != "") {
                                file_name += ".zstd";
                                err = ZstdEasy::read_binary_quotes_compress_file(
                                        file_name,
                                        dictionary_file_,
                                        prices,
                                        times);
                        } else {
                                file_name += ".hex";
                                err = BinaryApiEasy::read_binary_quotes_file(
                                        file_name,
                                        prices,
                                        times);
                        }
                        return err;
                }
//------------------------------------------------------------------------------
                /** \brief Добавить данные из файла
                 * \param file_name имя файла
                 * \return состояние ошибки
                 */
                int add_data_from_file(unsigned long long timestamp)
                {
                        std::vector<double> _prices;
                        std::vector<unsigned long long> _times;
                        int err = 0;
                        std::string file_name = path_ + "//" + BinaryApiEasy::get_file_name_from_date(timestamp);
                        if(dictionary_file_ != "") {
                                file_name += ".zstd";
                                err = ZstdEasy::read_binary_quotes_compress_file(
                                        file_name,
                                        dictionary_file_,
                                        _prices,
                                        _times);
                        } else {
                                file_name += ".hex";
                                err = BinaryApiEasy::read_binary_quotes_file(
                                        file_name,
                                        _prices,
                                        _times);
                        }
                        if(err == OK && _times.size() > 0) {
                                if(times.size() == 0) {
                                        times = _times;
                                        prices = _prices;
                                } else {
                                        if(_times.back() < times[0]) {
                                                _times.insert(_times.end(), times.begin(), times.end());
                                                _prices.insert(_prices.end(), prices.begin(), prices.end());
                                                times = _times;
                                                prices = _prices;
                                        } else
                                        if(_times[0] > times.back()) {
                                                times.insert(times.end(), _times.begin(), _times.end());
                                                prices.insert(prices.end(), _prices.begin(), _prices.end());
                                        } else {
                                                return DATA_NOT_AVAILABLE;
                                        }
                                }
                                return OK;
                        }
                        return DATA_NOT_AVAILABLE;
                }
//------------------------------------------------------------------------------
        public:
//------------------------------------------------------------------------------
                HistoricalData(std::string path, std::string dictionary_file = "") :
                dictionary_file_(dictionary_file), path_(path)
                {

                }
//------------------------------------------------------------------------------
                /** \brief Получить данные цены тика или цены закрытия свечи
                 * \param data цена
                 * \param timestamp временная метка
                 * \return состояние огибки, 0 если все в порядке
                 */
                int get_data(double& data, unsigned long long timestamp)
                {
                        if(times.size() > 0 &&
                                timestamp >= times[0] &&
                                timestamp <= times.back()) {
                                // если данные находятся в пределах загруженных данных
                                auto lower = std::lower_bound(times.cbegin(), times.cend(), timestamp);
                                if(lower == times.end())
                                        return DATA_NOT_AVAILABLE;
                                size_t pos = std::distance(times.cbegin(), lower);
                                data = prices[pos];
                                return OK;
                        } else {
                                int err = read_file(timestamp);
                                if(err != ZstdEasy::OK)
                                        return err;

                                auto lower = std::lower_bound(times.cbegin(), times.cend(), timestamp);
                                if(lower == times.end())
                                        return DATA_NOT_AVAILABLE;
                                size_t pos = std::distance(times.cbegin(), lower);
                                data = prices[pos];
                                return OK;
                        }
                }
//------------------------------------------------------------------------------
                /** \brief Получить данные цен тиков или цен закрытия свечей
                 * Внимание! Для цен закрытия свечей указывается временная метка НАЧАЛА свечи
                 * \param data цены тиков или цены закрытия свечей
                 * \param data_size количество данных для записи в data
                 * \param step шаг времени
                 * \param timestamp временная метка
                 * \return состояние огибки, 0 если все в порядке
                 */
                int get_data(std::vector<double>& data, int data_size, int step, unsigned long long timestamp)
                {
                        unsigned long long& t1 = timestamp;
                        unsigned long long t2 = timestamp + data_size * step;

                        while(true) {
                                if(times.size() > 0) {
                                        //std::cout << "cmp " << xtime::get_str_unix_date_time(t1) << " / " << xtime::get_str_unix_date_time(t2) << std::endl;
                                        //std::cout << "dat " << xtime::get_str_unix_date_time(times[0]) << " / " << xtime::get_str_unix_date_time(times.back()) << std::endl;
                                        if(t1 >= times[0] && t2 <= times.back()) {
                                                // если данные находятся в пределах загруженных данных
                                                auto lower = std::lower_bound(times.cbegin(), times.cend(), timestamp);
                                                if(lower == times.end())
                                                        return NO_TIMESTAMP;
                                                size_t pos = std::distance(times.cbegin(), lower);
                                                data.clear();
                                                data.reserve(data_size);
                                                unsigned long long t0 = t1;
                                                while(true) {
                                                        unsigned long long current_time = times[pos];
                                                        //std::cout << xtime::get_str_unix_date_time(current_time) << std::endl;
                                                        if(t0 == current_time) { // временная метка существует
                                                                data.push_back(prices[pos]); // добавляем цену
                                                                if(data.size() == data_size) // если буфер заполнен, выходим
                                                                        return OK;
                                                                t0 += step; // иначе ставим следующую временную метку
                                                                pos++; // увеличиваем позицию в массивах
                                                                if(pos >= times.size())
                                                                        return NO_TIMESTAMP;
                                                        } else
                                                        if(t0 > current_time) {
                                                                pos++; // увеличиваем позицию в массивах
                                                                if(pos >= times.size())
                                                                        return NO_TIMESTAMP;
                                                        } else {
                                                                // временная метка отсутсвует, выходим
                                                                return NO_TIMESTAMP;
                                                        }
                                                }
                                                return DATA_NOT_AVAILABLE;
                                        } else
                                        if(t1 >= times[0] && t2 > times.back()) {
                                                //std::cout << "add_dara_from_file (1) " << xtime::get_str_unix_date_time(times.back() + xtime::SEC_DAY) << std::endl;
                                                int err = add_data_from_file(times.back() + xtime::SEC_DAY);
                                                if(err != OK)
                                                        return err;
                                        } else
                                        if(t1 < times[0] && t2 <= times.back()) {
                                                //std::cout << "add_dara_from_file (2) " << xtime::get_str_unix_date_time(times[0] - xtime::SEC_DAY) << std::endl;
                                                int err = add_data_from_file(times[0] - xtime::SEC_DAY);
                                                if(err != OK)
                                                        return err;
                                        } else
                                        if(t1 < times[0] && t2 > times.back()) {
                                                //std::cout << "add_dara_from_file (3) " << xtime::get_str_unix_date_time(times[0] - xtime::SEC_DAY) << std::endl;
                                                int err = add_data_from_file(times[0] - xtime::SEC_DAY);
                                                if(err != OK)
                                                        return err;
                                                //std::cout << "add_dara_from_file (4) " << xtime::get_str_unix_date_time(times.back() + xtime::SEC_DAY) << std::endl;
                                                err = add_data_from_file(times.back() + xtime::SEC_DAY);
                                                if(err != OK)
                                                        return err;
                                        } // if
                                } else {
                                        int err = read_file((t1 + t2)/2);
                                        if(err != ZstdEasy::OK)
                                                return err;
                                } // if
                        } // while
                }
//------------------------------------------------------------------------------
                /** \brief Проверить бинарный опцион
                 * \param state состояние бинарного опциона (уданая сделка WIN = 1, убыточная LOSS = -1 и нейтральная 0)
                 * \param contract_type тип контракта (см. ContractType, доступно BUY и SELL)
                 * \param duration_sec длительность опциона в секундах
                 * \param timestamp временная метка начала опциона
                 * \return состояние ошибки (0 в случае успеха)
                 */
                int check_binary_option(int& state, int contract_type, int duration_sec, unsigned long long timestamp)
                {
                        if(contract_type != BUY && contract_type != SELL)
                                return INVALID_PARAMETER;
                        unsigned long long& t1 = timestamp;
                        unsigned long long t2 = t1 + duration_sec;
                        double price_start, price_stop;
                        int err = get_data(price_start, t1);
                        if(err != OK)
                                return err;
                        err = get_data(price_stop, t2);
                        if(err != OK)
                                return err;
                        //std::cout << price_start << " " << price_stop << std::endl;
                        state = price_start != price_stop ? (contract_type == BUY ? (price_start < price_stop ? WIN : LOSS) : (price_start > price_stop ? WIN : LOSS)) : NEUTRAL;
                        return OK;
                }
//------------------------------------------------------------------------------
        };
}
//------------------------------------------------------------------------------
#endif // HISTORICALDATAEASY_HPP_INCLUDED
