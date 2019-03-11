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
#ifndef BASEALGORITHMEASY_HPP_INCLUDED
#define BASEALGORITHMEASY_HPP_INCLUDED
//------------------------------------------------------------------------------
#include "HistoricalDataEasy.hpp"
#include "BasePayoutModelEasy.hpp"
//------------------------------------------------------------------------------
namespace BaseAlgorithmEasy
{
//------------------------------------------------------------------------------
        /** \brief Класс для построения алгоритмов торговли
         */
        class BigFather {
        protected:
                unsigned long long timestamp;   /**< Временная метка */
        public:
//------------------------------------------------------------------------------
                enum ErrorType {
                        OK = 0,
                        INVALID_PARAMETER = -6,
                        DATA_NOT_AVAILABLE = -7,
                };
//------------------------------------------------------------------------------
                /** \brief
                 */
                virtual int optimization()
                {

                }
//------------------------------------------------------------------------------
                /** \brief Торговать
                 * \param price_data данные цен по всем валютным парам
                 * \param time_data временные метки по всем валютным парам
                 * \param buy_data проценты выплат для ставок BUY
                 * \param sell_data проценты выплат для ставок SELL
                 * \return
                 */
                virtual int trade(
                                std::vector<std::vector<double>> &price_data,
                                std::vector<std::vector<unsigned long long>> &time_data,
                                std::vector<double> &buy_data,
                                std::vector<double> &sell_data)
                {

                        return 0;
                }
//------------------------------------------------------------------------------
                /** \brief Протестировать алгоритм
                 * \param beg_timestamp временная метка начала тестирования
                 * \param end_timestamp временная метка конца тестирования
                 * \param payout_paths массив директорий моделей процентов выплат
                 * \param history_paths путь к папке с историческими данными
                 * \param dictionary_file библиотека для zstd, если не используется, указать пустую строку (по умолчанию не используется)
                 * \return верент 0 в случае успеха
                 */
                virtual int test(
                                unsigned long long beg_timestamp,
                                unsigned long long end_timestamp,
                                unsigned long long timestamp_step,
                                std::vector<std::string> payout_paths,
                                std::vector<std::string> history_paths,
                                std::string dictionary_file = "")
                {
                        BasePayoutModelEasy iPayout(payout_paths);                                      // имитация процентов выплат
                        HistoricalDataMultipleCurrencies iHistory(history_paths, dictionary_file);      // загрузка исторических данных
                        unsigned long long _beg_timestamp = 0, _end_timestamp = 0;                      // начало и конец исторических данных
                        iHistory.get_beg_end_timestamp(_beg_timestamp, _end_timestamp);
                        if(_beg_timestamp > beg_timestamp || end_timestamp > _end_timestamp)
                                return DATA_NOT_AVAILABLE;
                        for(unsigned long long t = beg_timestamp; t <= end_timestamp; t+= timestamp_step)
                        {
                                // получаем данные цены для всех валютных пар
                                std::vector<double> price;
                                iHistory.get_data(price, t);
                                // получаем проценты выплат
                                iPayout.
                                std::vector<double> buy_data, sell_data;

                                // торгуем

                        } // for
                }
//------------------------------------------------------------------------------
        };
//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------
#endif // BASEALGORITHMEASY_HPP_INCLUDED
