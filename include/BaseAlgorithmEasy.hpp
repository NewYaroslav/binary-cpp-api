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
        enum ErrorType {
                OK = 0,                 ///< Ошибок нет
                INVALID_PARAMETER = -6, ///< Неверно задан параметр
                DATA_NOT_AVAILABLE = -7,///< Данные не доступны
        };
//------------------------------------------------------------------------------
        enum ForecastType {
                UP = 1,                 ///< Прогноз на ставку вверх (BUY)
                DN = -1,                ///< Прогноз на ставку вниз  (SELL)
                NO_FORECAST = 0,        ///< Нет прогноза
        };
//------------------------------------------------------------------------------
        /** \brief Класс для построения алгоритмов торговли
         */
        class BigFatherRobots {
        public:

                /** \brief Сбросить состояния индикаторов
                 * Данный метод очищает внутренние состояния индикаторов
                 */
                virtual void reset_indicators() {};

                /** \brief Протестировать индикаторы робота на цене
                 * \param price цена
                 * \param timestamp временная метка
                 * \param indx индекс котировок
                 * \param forecast состояние прогнозирования робота
                 * \return вернет 0 в случае успеха
                 */
                virtual int test_indicators(double price, unsigned long long timestamp, int indx, int &forecast) {return DATA_NOT_AVAILABLE;};

                /** \brief Обновить внутренние состояния индикаторов робота
                 * \param price цена
                 * \param timestamp временная метка
                 * \param indx индекс котировок
                 * \param forecast состояние прогнозирования робота
                 * \return вернет 0 в случае успеха
                 */
                virtual int update_indicators(double price, unsigned long long timestamp, int indx, int &forecast) {return DATA_NOT_AVAILABLE;};

                /** \brief Обновить внутренние состояния индикаторов робота
                 * \param price цены
                 * \param timestamp временная метка
                 * \param forecast состояние прогнозов робота
                 * \return вернет 0 в случае успеха
                 */
                virtual int update_indicators(std::vector<double> &price, unsigned long long timestamp, std::vector<int> &forecast, std::vector<int> &bots) {return DATA_NOT_AVAILABLE;};

                /** \brief Получить время экспирации опциона (в секундах)
                 * \return время экспирации опциона (значение меньше 0 означает ошибку)
                 */
                virtual int get_duration() {return DATA_NOT_AVAILABLE;};

                /** \brief Установить длительность бинарного опциона
                 * \param duration длительность бинарного опциона в секундах
                 */
                virtual void set_duration(int duration) {};

                /** \brief Получить задержку перед началом работы робота
                 * \return задержка в секундах
                 */
                virtual int get_delay_before_work() {
                        return DATA_NOT_AVAILABLE;
                }
        };
//------------------------------------------------------------------------------
        /** \brief Класс робота для торговли по индикатору Боллинджер
         */
        class SimpleBbRobot : public BigFatherRobots {
                private:
                std::vector<IndicatorsEasy::BollingerBands<double>> iBB;
                int duration_ = xtime::SEC_MINUTE * 3;
                int delay_before_work_ = 0;
                const int bot_id_ = 0;
                public:
                SimpleBbRobot() {};

                /** \brief Инициализировать робота
                 * \param period периоды Боллинджера
                 * \param std_factor множители стандартного отклонения Боллинджера
                 */
                SimpleBbRobot(std::vector<int> periods, std::vector<double> std_factors) {
                        if(periods.size() != std_factors.size() || periods.size() == 0)
                                return;
                        iBB.resize(periods.size());
                        for(size_t i = 0; i < periods.size(); ++i) {
                                iBB[i] = IndicatorsEasy::BollingerBands<double>(periods[i], std_factors[i]);
                                delay_before_work_ = std::max(delay_before_work_, periods[i]);
                        }
                        delay_before_work_ *= xtime::SEC_MINUTE;
                };

                /** \brief Сбросить состояния индикаторов
                 * Данный метод очищает внутренние состояния индикаторов
                 */
                virtual void reset_indicators()
                {
                        for(size_t i = 0; i < iBB.size(); ++i) {
                                iBB[i].clear();
                        }
                }

                /** \brief Протестировать робота на цене
                 * \param price цена
                 * \param timestamp временная метка
                 * \param indx индекс котировок
                 * \param forecast состояние прогнозирования робота
                 * \return вернет 0 в случае успеха
                 */
                virtual int test_indicators(double price, unsigned long long timestamp, int indx, int &forecast)
                {
                        forecast = NO_FORECAST;
                        if(indx >= iBB.size())
                                return INVALID_PARAMETER;
                        double ml, tl, bl;
                        int err = iBB[indx].test(price, tl, ml, bl);
                        if(err != OK) return err;
                        if(price > tl) {
                                forecast = DN;
                        } else
                        if(price < bl) {
                                forecast = UP;
                        }
                        return OK;
                }

                /** \brief Обновить внутренние состояния робота
                 * \param price цена
                 * \param timestamp временная метка
                 * \param indx индекс котировок
                 * \param forecast состояние прогнозирования робота
                 * \return вернет 0 в случае успеха
                 */
                virtual int update_indicators(double price, unsigned long long timestamp, int indx, int &forecast)
                {
                        forecast = NO_FORECAST;
                        if(indx >= iBB.size())
                                return INVALID_PARAMETER;
                        double ml, tl, bl;
                        int err = iBB[indx].update(price, tl, ml, bl);
                        if(err != OK) return err;
                        if(price > tl) {
                                forecast = DN;
                        } else
                        if(price < bl) {
                                forecast = UP;
                        }
                        return OK;
                }

                /** \brief Обновить внутренние состояния робота
                 * \param price цены всех валютных пар
                 * \param timestamp временная метка
                 * \param forecast состояния прогнозирования робота
                 * \return вернет 0 в случае успеха
                 */
                virtual int update_indicators(std::vector<double> &price, unsigned long long timestamp, std::vector<int> &forecast, std::vector<int> &bots)
                {
                        if(price.size() != iBB.size())
                                return INVALID_PARAMETER;
                        forecast.resize(iBB.size());
                        bots.resize(iBB.size());
                        for(size_t n = 0; n < iBB.size(); ++n) {
                                update_indicators(price[n], timestamp, n, forecast[n]);
                                bots[n] = bot_id_;
                        }
                        return OK;
                }

                /** \brief Установить длительность бинарного опциона
                 * \param duration длительность бинарного опциона в секундах
                 */
                virtual void set_duration(int duration) {
                        duration_ = duration;
                };

                /** \brief Получить время экспирации опциона (в секундах)
                 * \return время экспирации опциона (значение меньше 0 означает ошибку)
                 */
                virtual int get_duration() {return duration_;};

                /** \brief Получить задержку перед началом работы робота
                 * \return задержка в секундах
                 */
                virtual int get_delay_before_work() {return delay_before_work_;}
        };
//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------
#endif // BASEALGORITHMEASY_HPP_INCLUDED
