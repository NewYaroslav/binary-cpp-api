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
#ifndef BINARYAPIEASY_HPP_INCLUDED
#define BINARYAPIEASY_HPP_INCLUDED

#include "BinaryApi.hpp"
#include "banana_filesystem.hpp"
//------------------------------------------------------------------------------
namespace BinaryApiEasy
{

        enum ErrorType {
                OK = 0,
                NOT_ALL_DATA_DOWNLOADED = -8,
        };

        enum QuotesType {
                QUOTES_TICKS = 0,
                QUOTES_BARS = 1,
        };
//------------------------------------------------------------------------------
        /** \brief Загрузить данные за последние пару дней
         * Данная функция загружает полные данные за день за последние N дней
         * Важно: При этом текущий день не учавствует в загрузке! Это значит,
         * например, что загружая данные 5 числа в 12 часов дня по GMT за последние 3 дня,
         * данная функция загрузит 4, 3, 2 дни месяца. Котировок из 5-го числа присутствовать в данных не будет!
         * \param api Ссылка на класс BinaryAPI
         * \param symbol валютная пара для загрузки
         * \param timestamp день, с которого начнется загрузка
         * \param prices цены
         * \param times временные метки
         * \param num_days количество дней, минимум 1
         * \param is_skip_day_off пропускать в загрузке выходные дни
         * \param type тип загружаемых данных, QUOTES_TICKS - загрузить тики, QUOTES_BARS - загрузить бары\свечи
         * \return состояние ошибки (OK = 0 в случае успеха, иначе см. ErrorType)
         */
        int download_last_few_days( BinaryAPI &api,
                                    std::string symbol,
                                    unsigned long long timestamp,
                                    std::vector<std::vector<double>> &prices,
                                    std::vector<std::vector<unsigned long long>> &times,
                                    unsigned int num_days = 1,
                                    bool is_skip_day_off = true,
                                    int type = QUOTES_BARS)
        {
                xtime::DateTime iTime(timestamp);
                iTime.hour = iTime.seconds = iTime.minutes = 0;
                unsigned long long stop_time = iTime.get_timestamp() - xtime::SEC_DAY;
                if(is_skip_day_off) {
                        while(xtime::is_day_off(stop_time)) {
                                stop_time -= xtime::SEC_DAY;
                        }
                }
                prices.resize(num_days);
                times.resize(num_days);
                int err = BinaryAPI::OK;
                int num_download = 0;
                for(unsigned int i = 0; i < num_days; ++i) {
                        std::vector<double> _prices;
                        std::vector<unsigned long long> _times;
                        if(type == QUOTES_BARS) {
                                err = api.get_candles_without_limits(
                                        symbol,
                                        _prices,
                                        _times,
                                        stop_time,
                                        stop_time + xtime::SEC_DAY - 1);
                        } else
                        if(type == QUOTES_TICKS) {
                                err = api.get_ticks_without_limits(
                                        symbol,
                                        _prices,
                                        _times,
                                        stop_time,
                                        stop_time + xtime::SEC_DAY - 1);
                        }
                        if(is_skip_day_off) {
                                stop_time -= xtime::SEC_DAY;
                                while(xtime::is_day_off(stop_time)) {
                                        stop_time -= xtime::SEC_DAY;
                                }
                        } else {
                                stop_time -= xtime::SEC_DAY;
                        }
                        if(err == BinaryAPI::OK) {
                                size_t indx = num_days - 1 - i;
                                prices[indx] = _prices;
                                times[indx] = _times;
                                num_download++;
                        }
                }
                if(num_download != num_days) {
                        if(err != BinaryAPI::OK)
                                return err;
                        return NOT_ALL_DATA_DOWNLOADED;
                }
                return BinaryAPI::OK;
        }
//------------------------------------------------------------------------------
        /** \brief Получить имя файла из даты
         * Выпрана последовательность ГОД МЕСЯЦ ДЕНЬ чтобы файлы были
         * в алфавитном порядке
         * \param timestamp временная метка
         * \return имя файла
         */
        std::string get_file_name_from_date(unsigned long long timestamp)
        {
                xtime::DateTime iTime(timestamp);
                std::string file_name =
                        std::to_string(iTime.year) + "_" +
                        std::to_string(iTime.month) + "_" +
                        std::to_string(iTime.day);
                return file_name;
        }
//------------------------------------------------------------------------------
        /** \brief Записать бинарный файл для котировок
         * \param file_name имя файла
         * \param prices котировки
         * \param times временные метки
         */
        void write_binary_quotes_file(std::string file_name,
                                      std::vector<double> &prices,
                                      std::vector<unsigned long long> &times)
        {
                // сохраняем
                std::ofstream file(file_name, std::ios_base::binary);
                unsigned long data_size = times.size();
                file.write(reinterpret_cast<char *>(&data_size),sizeof (data_size));
                for(int i = 0; i < times.size(); i++) {
                        file.write(reinterpret_cast<char *>(&prices[i]),sizeof (double));
                        file.write(reinterpret_cast<char *>(&times[i]),sizeof (unsigned long long));
                }
                file.close();
        }
//------------------------------------------------------------------------------
        /** \brief Читать бинарный файл для котировок
         * \param file_name имя файла
         * \param prices котировки
         * \param times временные метки
         */
        int read_binary_quotes_file(std::string file_name,
                                    std::vector<double> &prices,
                                    std::vector<unsigned long long> &times)
        {
                // сохраняем
                std::ifstream file(file_name, std::ios_base::binary);
                if(!file)
                        return BinaryAPI::UNKNOWN_ERROR;
                unsigned long data_size = 0;
                file.read(reinterpret_cast<char *>(&data_size),sizeof (data_size));
                prices.resize(data_size);
                times.resize(data_size);
                for(int i = 0; i < times.size(); i++) {
                        file.read(reinterpret_cast<char *>(&prices[i]),sizeof (double));
                        file.read(reinterpret_cast<char *>(&times[i]),sizeof (unsigned long long));
                }
                file.close();
                return BinaryAPI::OK;
        }
//------------------------------------------------------------------------------
        void standart_user_function(std::string str, std::vector<double> &prices, std::vector<unsigned long long> &times, unsigned long long timestamp)
        {
                std::cout << "write binary quotes file: " << str << " " << xtime::get_str_unix_date_time(times.back()) << std::endl;
        }
//------------------------------------------------------------------------------
        /** \brief Скачать и сохранить все доступыне данные по котировкам
         *
         * \param
         * \param
         * \return
         *
         */
        int download_and_save_all_data(BinaryAPI &api,
                                       std::string symbol,
                                       std::string path,
                                       unsigned long long timestamp,
                                       bool is_skip_day_off = true,
                                       int type = QUOTES_BARS,
                                       void (*user_function)(std::string,
                                        std::vector<double> &,
                                        std::vector<unsigned long long> &,
                                        unsigned long long) = NULL)
        {
                bf::create_directory(path);
                xtime::DateTime iTime(timestamp);
                iTime.hour = iTime.seconds = iTime.minutes = 0;
                unsigned long long stop_time = iTime.get_timestamp() - xtime::SEC_DAY;
                if(is_skip_day_off) {
                        while(xtime::is_day_off(stop_time)) {
                                stop_time -= xtime::SEC_DAY;
                        }
                }
                int err = BinaryAPI::OK;
                int num_download = 0;
                int num_errors = 0;
                while(true) {
                        // сначала выполняем проверку
                        std::string file_name = path + "//" +
                                get_file_name_from_date(stop_time) + ".hex";

                        if(bf::check_file(file_name)) {
                                if(is_skip_day_off) {
                                        stop_time -= xtime::SEC_DAY;
                                        while(xtime::is_day_off(stop_time)) {
                                                stop_time -= xtime::SEC_DAY;
                                        }
                                } else {
                                        stop_time -= xtime::SEC_DAY;
                                }
                                continue;
                        }

                        //std::cout << xtime::get_str_unix_date_time(stop_time) << std::endl;
                        // загружаем файл
                        std::vector<double> _prices;
                        std::vector<unsigned long long> _times;
                        if(type == QUOTES_BARS) {
                                err = api.get_candles_without_limits(
                                        symbol,
                                        _prices,
                                        _times,
                                        stop_time,
                                        stop_time + xtime::SEC_DAY - 1);
                        } else
                        if(type == QUOTES_TICKS) {
                                err = api.get_ticks_without_limits(
                                        symbol,
                                        _prices,
                                        _times,
                                        stop_time,
                                        stop_time + xtime::SEC_DAY - 1);
                        }
                        if(is_skip_day_off) {
                                stop_time -= xtime::SEC_DAY;
                                while(xtime::is_day_off(stop_time)) {
                                        stop_time -= xtime::SEC_DAY;
                                }
                        } else {
                                stop_time -= xtime::SEC_DAY;
                        }
                        if(err == BinaryAPI::OK && _times.size() > 0) { // данные получены
                                //std::cout << "write_binary_quotes_file: " << file_name << " " << xtime::get_str_unix_date_time(_times.back()) << std::endl;
                                if(user_function != NULL)
                                        user_function(file_name, _prices, _times, stop_time);
                                write_binary_quotes_file(file_name, _prices, _times);
                                num_download++;
                                num_errors = 0;
                        } else {
                                num_errors++;
                                //std::cout << "num_errors: " << num_errors << std::endl;
                        }
                        if(num_errors > 30) {
                                break;
                        }
                }
                if(num_download == 0) {
                        if(err != BinaryAPI::OK)
                                return err;
                        return NOT_ALL_DATA_DOWNLOADED;
                }
                return BinaryAPI::OK;
        }
//------------------------------------------------------------------------------

}

#endif // BINARYAPIEASY_HPP_INCLUDED
