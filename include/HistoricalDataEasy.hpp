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
        /** \brief Класс для удобного использования исторических данных
         */
        class HistoricalData
        {
        public:
//------------------------------------------------------------------------------
                /// Набор возможных состояний ошибки
                enum ErrorType {
                        OK = 0,                         ///< Ошибки нет, все в порядке
                        NO_INIT = -4,                   ///< Не было инициализации перед использованием метода
                        INVALID_PARAMETER = -6,         ///< Параметр имеет недопустимое значение
                        DATA_NOT_AVAILABLE = -7,        ///< Данные не доступны (их нет)
                        NO_TIMESTAMP = - 14,            ///< Нет временной метки (данные не содержат цен с требуемой временной меткой)
                };
//------------------------------------------------------------------------------
                /// Типы контрактов
                enum ContractType {
                        BUY = 1,                        ///< Ставка на повышение
                        SELL = -1,                      ///< Ставка на понижение
                };
//------------------------------------------------------------------------------
                /// Типы состояний
                enum StateType {
                        WIN = 1,                        ///< Опцион завершился удачно
                        NEUTRAL = 0,                    ///< Опцион завершился в ничью
                        LOSS = -1,                      ///< Опцион был не верно спрогнозирован
                        END_OF_DATA = 2,                ///< Конец данных
                        SKIPPING_DATA = -2,             ///< Пропуск данных (нет цен за указанный период)
                        NORMAL_DATA = 0,                ///< Нормальный доступ к данным
                };
//------------------------------------------------------------------------------
        private:
                std::string dictionary_file_;
                std::string path_;
                std::string current_file_name = "";
                std::vector<double> prices;
                std::vector<unsigned long long> times;
                size_t pos = 0;
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
                 * \param timestamp временная метка в имени файла
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
                                // добавляем проверку адекватности времени
                                if(_times.back() - _times[0] > xtime::SEC_DAY) {
                                        std::cout << "file error: " << file_name << std::endl;
                                        std::cout << "_times size " << _times.size() << std::endl;
                                        std::cout << "_times[0] " << _times[0] << std::endl;
                                        std::cout << "_times.back() " << _times.back() << std::endl;
                                        if(times.size() > 0) {
                                                std::cout << "times[0] " << times[0] << std::endl;
                                                std::cout << "times.back() " << times.back() << std::endl;
                                        }
                                        int temp;
                                        std::cin >> temp;
                                        std::cout << temp << std::endl;
                                        return DATA_NOT_AVAILABLE;
                                }
                                if(times.size() == 0) {
                                        // если загруженных данных нет
                                        times = _times;
                                        prices = _prices;
                                } else {
                                        if(_times.back() < times[0]) {
                                                // если новые данные находятся по дате раньше ранее загруженных данных
                                                _times.insert(_times.end(), times.begin(), times.end());
                                                _prices.insert(_prices.end(), prices.begin(), prices.end());
                                                times = _times;
                                                prices = _prices;
                                        } else
                                        if(_times[0] > times.back()) {
                                                // если новые данные хаодятся по дате после ранее загруженных данных
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
                /** \brief Инициализировать класс
                 * \param path директория с файлами исторических данных
                 * \param dictionary_file файл словаря (если указано "", то считываются несжатые файлы)
                 */
                HistoricalData(std::string path, std::string dictionary_file = "") :
                dictionary_file_(dictionary_file), path_(path)
                {

                }
//------------------------------------------------------------------------------
                /** \brief Получить данные массива цен
                 * \return Массив цен
                 */
                std::vector<double> get_array_prices()
                {
                        return prices;
                }
//------------------------------------------------------------------------------
                /** \brief Получить данные массива временных меток
                 * \return Массив временных меток
                 */
                std::vector<unsigned long long> get_array_times()
                {
                        return times;
                }
//------------------------------------------------------------------------------
                /** \brief Получить данные цены тика или цены закрытия свечи
                 * \param data цена на указанной временной метке
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
                                                                if(data.size() == (size_t)data_size) // если буфер заполнен, выходим
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
                /** \brief Прочитать все данные из директории
                 * \param beg_timestamp временная метка начала чтения исторических данных
                 * \param end_timestamp временная метка конца чтения исторических данных
                 * \return вернет 0 в случае успеха
                 */
                int read_all_data(unsigned long long beg_timestamp, unsigned long long end_timestamp)
                {
                        xtime::DateTime t1(beg_timestamp), t2(end_timestamp);
                        t1.hour = 0; t1.seconds = 0; t1.minutes = 0;
                        t2.hour = 0; t2.seconds = 0; t2.minutes = 0;
                        beg_timestamp = t1.get_timestamp();
                        end_timestamp = t2.get_timestamp();
                        if(end_timestamp < beg_timestamp)
                                return INVALID_PARAMETER;
                        int err = 0;
                        bool is_error = true;
                        for(unsigned long long t = beg_timestamp; t <= end_timestamp; t += xtime::SEC_DAY) {
                                err = add_data_from_file(t);
                                if(err == OK) {
                                        is_error = false;
                                }
                        }
                        if(is_error)
                                return err;
                        pos = 0;
                        return OK;
                }
//------------------------------------------------------------------------------
                /** \brief Прочитать все данные из директории
                 * Данная функция загружает все доступные данные
                 * \return вернет 0 в случае успеха
                 */
                int read_all_data()
                {
                        unsigned long long t1, t2;
                        int err = BinaryApiEasy::get_beg_end_timestamp_for_path(path_, ".", t1, t2);
                        if(err != OK)
                                return err;
                        return read_all_data(t1, t2);
                }
//------------------------------------------------------------------------------
                /** \brief Получить новую цену
                 * Данную функцию можно вызывать после read_all_data чтобы последовательно считывать данные из массива.
                 * \param price цена
                 * \param timestamp временная метка
                 * \param status состояние данных (END_OF_DATA - конец данных, SKIPPING_DATA - пропущен бар или тик, NORMAL_DATA - данные считаны нормально)
                 * \param period_data период данных (для минутных свечей 60, для тиков на сайте binary - 1 секунда)
                 * \return вернет 0 в случае успеха
                 */
                int get_price(double &price, unsigned long long &timestamp, int& status, int period_data = 60)
                {
                        if(pos > prices.size()) {
                                status = END_OF_DATA;
                                return OK;
                        } else if(prices.size() > 0) {
                                price = prices[pos];
                                timestamp = times[pos];
                                if(pos > 0 && timestamp != times[pos - 1] + period_data)
                                        status = SKIPPING_DATA;
                                else
                                        status = NORMAL_DATA;
                                pos++;
                                return OK;
                        }
                        status = END_OF_DATA;
                        return DATA_NOT_AVAILABLE;
                }
//------------------------------------------------------------------------------
        };
//------------------------------------------------------------------------------
        /** \brief Класс для удобного использования исторических данных нескольких валютных пар
         */
        class HistoricalDataMultipleCurrencies
        {
        private:
//------------------------------------------------------------------------------
                std::vector<HistoricalData> datas;              /**< Вектор с историческими данными цен */
                unsigned long long beg_timestamp = 0;           /**< Временная метка начала исторических данных по всем валютным парам */
                unsigned long long end_timestamp = 0;           /**< Временная метка конца исторических данных по всем валютным парам */
                bool is_init = false;
//------------------------------------------------------------------------------
        public:
                /// Набор возможных состояний ошибки
                enum ErrorType {
                        OK = 0,                                 ///< Ошибки нет, все в порядке
                        NO_INIT = -4,                           ///< Не было инициализации перед использованием метода
                        INVALID_PARAMETER = -6,                 ///< Параметр имеет недопустимое значение
                        DATA_NOT_AVAILABLE = -7,                ///< Данные не доступны (их нет)
                        NO_TIMESTAMP = - 14,                    ///< Нет временной метки (данные не содержат цен с требуемой временной меткой)
                };
//------------------------------------------------------------------------------
                /** \brief Инициализировать класс
                 * \param paths директории с файлами исторических данных
                 * \param dictionary_file файл словаря (если указано "", то считываются несжатые файлы)
                 */
                HistoricalDataMultipleCurrencies(std::vector<std::string> paths, std::string dictionary_file = "")
                {
                        for(size_t i = 0; i < paths.size(); ++i) {
                                datas.push_back(HistoricalData(paths[i], dictionary_file));
                        }
                        if(BinaryApiEasy::get_beg_end_timestamp_for_paths(paths, ".", beg_timestamp, end_timestamp) == OK) {
                                if(beg_timestamp != end_timestamp) {
                                        is_init = true;
                                }
                        }
                }
//------------------------------------------------------------------------------
                /** \brief Найти первую и последнюю дату файлов
                 * \param beg_timestamp первая дата, встречающееся среди файлов
                 * \param end_timestamp последняя дата, встречающееся среди файлов
                 * \return вернет 0 в случае успеха
                 */
                inline int get_beg_end_timestamp(unsigned long long &_beg_timestamp, unsigned long long &_end_timestamp)
                {
                        _beg_timestamp = beg_timestamp;
                        _end_timestamp = end_timestamp;
                        if(!is_init) {
                                return NO_INIT;
                        }
                        return OK;
                }
//------------------------------------------------------------------------------
                /** \brief Получить данные цен тиков или цен закрытия свечей
                 * Внимание! Для цен закрытия свечей указывается временная метка НАЧАЛА свечи
                 * \param array_prices массив массивов цен тиков или цен закрытия свечей
                 * \param data_size количество данных для записи в массивы массива array_prices
                 * \param step шаг времени
                 * \param timestamp временная метка
                 * \return состояние огибки, 0 если все в порядке
                 */
                int get_data(std::vector<std::vector<double>>& array_prices, int data_size, int step, unsigned long long timestamp)
                {
                        if(!is_init) {
                                return NO_INIT;
                        }
                        array_prices.resize(datas.size());
                        for(size_t i = 0; i < datas.size(); ++i) {
                                int err = datas[i].get_data(array_prices[i], data_size, step, timestamp);
                                if(err != OK) {
                                        return err;
                                }
                        }
                        return OK;
                }
//------------------------------------------------------------------------------
                /** \brief Получить массив цен со всех валютных пар
                 * \param prices массив цен со всех валютных пар за данную веремнную метку
                 * \param timestamp временная метка
                 * \return вернет 0 в случае успеха
                 */
                int get_data(std::vector<double> &prices, unsigned long long timestamp)
                {
                        if(!is_init) {
                                return NO_INIT;
                        }
                        prices.resize(datas.size());
                        for(size_t i = 0; i < datas.size(); ++i) {
                                int err = datas[i].get_data(prices[i], timestamp);
                                if(err != OK) {
                                        return err;
                                }
                        }
                        return OK;
                }
//------------------------------------------------------------------------------
                /** \brief Проверить бинарный опцион
                 * \param state состояние бинарного опциона (уданая сделка WIN = 1, убыточная LOSS = -1 и нейтральная 0)
                 * \param contract_type тип контракта (см. ContractType, доступно BUY и SELL)
                 * \param duration_sec длительность опциона в секундах
                 * \param timestamp временная метка начала опциона
                 * \param indx позиция валютной пары в массиве валютных пар (должно совпадать с позицией в массиве paths)
                 * \return состояние ошибки (0 в случае успеха)
                 */
                int check_binary_option(int& state, int contract_type, int duration_sec, unsigned long long timestamp, int indx) {
                        if(!is_init) {
                                return NO_INIT;
                        }
                        if(indx >= (int)datas.size()) {
                                return INVALID_PARAMETER;
                        }
                        return datas[indx].check_binary_option(state, contract_type, duration_sec, timestamp);
                }
//------------------------------------------------------------------------------
        };
//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------
#endif // HISTORICALDATAEASY_HPP_INCLUDED
