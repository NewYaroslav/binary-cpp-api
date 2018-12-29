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
#ifndef INDICATORSEASY_HPP_INCLUDED
#define INDICATORSEASY_HPP_INCLUDED
//------------------------------------------------------------------------------
#include <vector>
#include <numeric>
//------------------------------------------------------------------------------
namespace IndicatorsEasy
{
//------------------------------------------------------------------------------
        enum ErrorType {
                OK = 0,
                NO_INIT = -4,
                INVALID_PARAMETER = -6,
                INDICATOR_NOT_READY_TO_WORK = -16,
        };
//------------------------------------------------------------------------------
        /** \brief Посчитать простую скользящую среднюю (SMA)
         * Данная функция для расчетов использует последние N = period значений
         * \param input массив значений
         * \param period период SMA
         * \param output значение SMA
         * \return вернет 0 в случае успеха
         */
        template <typename T1, typename T2>
        int calculate_sma(std::vector<T1> &input, T2 &output, size_t period)
        {
                if(input.size() < period)
                        return INVALID_PARAMETER;
                T2 sum = std::accumulate(input.end() - period, input.end(), T1(0));
                output = sum / (T2)period;
                return OK;
        }
//------------------------------------------------------------------------------
        /** \brief Простая скользящая средняя
         */
        template <typename T>
        class SMA
        {
        private:
                std::vector<T> data_;
                T last_data_;
                int period_ = 0;
        public:
                SMA() {};
                /** \brief Инициализировать простую скользящую среднюю
                 * \param period период
                 */
                SMA(int period) : period_(period)
                {
                        if(period_ < 0)
                                period_ = -period_;
                        data_.reserve(period_);
                }

                /** \brief Получить новые данные индикатора
                 * \param in сигнал на входе
                 * \param out сигнал на выходе
                 * \return вернет 0 в случае успеха, иначе см. ErrorType
                 */
                int update(T in, T &out)
                {
                        if(period_ == 0)
                                return NO_INIT;
                        if(data_.size() < (size_t)period_) {
                                data_.push_back(in);
                                if(data_.size() == (size_t)period_) {
                                        T sum = std::accumulate(data_.begin(), data_.end(), T(0));
                                        out = last_data_ = sum / (T)period_;
                                        return OK;
                                }
                        } else {
                                last_data_ = last_data_ - data_[0] + in;
                                data_.push_back(in);
                                data_.erase(data_.begin());
                                out = last_data_;
                                return OK;
                        }
                        return INDICATOR_NOT_READY_TO_WORK;
                }

                /** \brief Протестировать индикатор
                 * Данная функция отличается от update тем, что не влияет на внутреннее состояние индикатора
                 * \param in сигнал на входе
                 * \param out сигнал на выходе
                 * \return вернет 0 в случае успеха, иначе см. ErrorType
                 */
                int test(T in, T &out)
                {
                        if(period_ == 0)
                                return NO_INIT;
                        std::vector<T> _data = data_;
                        if(_data.size() < (size_t)period_) {
                                _data.push_back(in);
                                if(_data.size() == (size_t)period_) {
                                        T sum = std::accumulate(_data.begin(), _data.end(), T(0));
                                        out = sum / (T)period_;
                                        return OK;
                                }
                        } else {
                                out = last_data_ - data_[0] + in;
                                return OK;
                        }
                        return INDICATOR_NOT_READY_TO_WORK;
                }

                /** \brief Очистить данные индикатора
                 */
                void clear()
                {
                        data_.clear();
                }
        };
//------------------------------------------------------------------------------
        /** \brief Взвешенное скользящее среднее
         */
        template <typename T>
        class WMA
        {
        private:
                std::vector<T> data_;
                int period_ = 0;
        public:
                WMA() {};
                /** \brief Инициализировать взвешенное скользящее среднее
                 * \param period период
                 */
                WMA(int period) : period_(period)
                {
                        if(period_ < 0)
                                period_ = -period_;
                        data_.reserve(period_);
                }

                /** \brief Получить новые данные индикатора
                 * \param in сигнал на входе
                 * \param out сигнал на выходе
                 * \return вернет 0 в случае успеха, иначе см. ErrorType
                 */
                int update(T in, T &out)
                {
                        if(period_ == 0)
                                return NO_INIT;
                        if(data_.size() < (size_t)period_) {
                                data_.push_back(in);
                                if(data_.size() == (size_t)period_) {
                                        T sum = 0;
                                        for(size_t i = data_.size(); i > 0; i--) {
                                                sum += data_[i - 1] * (T)i;
                                        }
                                        out = (sum * 2.0d) / ((T)period_ * ((T)period_ + 1.0d));
                                        return OK;
                                }
                        } else {
                                data_.push_back(in);
                                data_.erase(data_.begin());
                                T sum = 0;
                                for(size_t i = data_.size(); i > 0; i--) {
                                        sum += data_[i - 1] * (T)i;
                                }
                                out = (sum * 2.0d) / ((T)period_ * ((T)period_ + 1.0d));
                                return OK;
                        }
                        return INDICATOR_NOT_READY_TO_WORK;
                }

                /** \brief Протестировать индикатор
                 * Данная функция отличается от update тем, что не влияет на внутреннее состояние индикатора
                 * \param in сигнал на входе
                 * \param out сигнал на выходе
                 * \return вернет 0 в случае успеха, иначе см. ErrorType
                 */
                int test(T in, T &out)
                {
                        if(period_ == 0)
                                return NO_INIT;
                        std::vector<T> _data = data_;
                        if(_data.size() < (size_t)period_) {
                                _data.push_back(in);
                                if(_data.size() == (size_t)period_) {
                                        T sum = 0;
                                        for(size_t i = _data.size(); i > 0; i--) {
                                                sum += _data[i - 1] * (T)i;
                                        }
                                        out = (sum * 2.0d) / ((T)period_ * ((T)period_ + 1.0d));
                                        return OK;
                                }
                        } else {
                                _data.push_back(in);
                                _data.erase(_data.begin());
                                T sum = 0;
                                for(size_t i = data_.size(); i > 0; i--) {
                                        sum += data_[i - 1] * (T)i;
                                }
                                out = (sum * 2.0d) / ((T)period_ * ((T)period_ + 1.0d));
                                return OK;
                        }
                        return INDICATOR_NOT_READY_TO_WORK;
                }

                /** \brief Очистить данные индикатора
                 */
                void clear()
                {
                        data_.clear();
                }
        };
//------------------------------------------------------------------------------
        /** \brief Экспоненциально взвешенное скользящее среднее
         */
        template <typename T>
        class EMA
        {
        private:
                std::vector<T> data_;
                T last_data_;
                T a;
                int period_ = 0;
        public:
                EMA() {};

                /** \brief Инициализировать экспоненциально взвешенное скользящее среднее
                 * \param period период
                 */
                EMA(int period) : period_(period)
                {
                        if(period_ < 0)
                                period_ = -period_;
                        data_.reserve(period_);
                        a = 2.0/(T)(period_ + 1.0d);
                }

                /** \brief Получить новые данные индикатора
                 * \param in сигнал на входе
                 * \param out сигнал на выходе
                 * \return вернет 0 в случае успеха, иначе см. ErrorType
                 */
                int update(T in, T &out)
                {
                        if(period_ == 0)
                                return NO_INIT;
                        if(data_.size() < (size_t)period_) {
                                data_.push_back(in);
                                if(data_.size() == (size_t)period_) {
                                        T sum = std::accumulate(data_.begin(), data_.end(), T(0));
                                        last_data_ = sum / (T)period_;
                                }
                        } else {
                                last_data_ = a * in + (1.0 - a) * last_data_;
                                out = last_data_;
                                return OK;
                        }
                        return INDICATOR_NOT_READY_TO_WORK;
                }

                /** \brief Протестировать индикатор
                 * Данная функция отличается от update тем, что не влияет на внутреннее состояние индикатора
                 * \param in сигнал на входе
                 * \param out сигнал на выходе
                 * \return вернет 0 в случае успеха, иначе см. ErrorType
                 */
                int test(T in, T &out)
                {
                        if(period_ == 0)
                                return NO_INIT;
                        if(data_.size() == period_) {
                                out = a * in + (1.0 - a) * last_data_;
                                return OK;
                        }
                        return INDICATOR_NOT_READY_TO_WORK;
                }

                /** \brief Очистить данные индикатора
                 */
                void clear()
                {
                        data_.clear();
                }
        };
//------------------------------------------------------------------------------
        /** \brief Модифицированное скользящее среднее
         */
        template <typename T>
        class MMA : public EMA<T>
        {
        public:
                /** \brief Инициализировать модифицированное скользящее среднее
                 * \param period период
                 */
                MMA(int period) : EMA<T>::period_(period)
                {
                        if(EMA<T>::period_ < 0)
                                EMA<T>::period_ = -EMA<T>::period_;
                        EMA<T>::data_.reserve(EMA<T>::period_);
                        EMA<T>::a = 1.0/(T)EMA<T>::period_;
                }
        };
//------------------------------------------------------------------------------
        /** \brief Скользящее окно
         */
        template <typename T>
        class MW
        {
        private:
                std::vector<T> data_;
                T last_data_;
                int period_ = 0;
        public:
                MW() {};
                /** \brief Инициализировать скользящее окно
                 * \param period период
                 */
                MW(int period) : period_(period)
                {
                        if(period_ < 0)
                                period_ = -period_;
                        data_.reserve(period_);
                }

                /** \brief Получить новые данные индикатора
                 * \param in сигнал на входе
                 * \param out массив на выходе
                 * \return вернет 0 в случае успеха, иначе см. ErrorType
                 */
                int update(T in, std::vector<T> &out)
                {
                        if(period_ == 0)
                                return NO_INIT;
                        if(data_.size() < (size_t)period_) {
                                data_.push_back(in);
                                if(data_.size() == (size_t)period_) {
                                        out = data_;
                                        return OK;
                                }
                        } else {
                                data_.push_back(in);
                                data_.erase(data_.begin());
                                out = data_;
                                return OK;
                        }
                        return INDICATOR_NOT_READY_TO_WORK;
                }

                /** \brief Протестировать индикатор
                 * Данная функция отличается от update тем, что не влияет на внутреннее состояние индикатора
                 * \param in сигнал на входе
                 * \param out сигнал на выходе
                 * \return вернет 0 в случае успеха, иначе см. ErrorType
                 */
                int test(T in, std::vector<T> &out)
                {
                        if(period_ == 0)
                                return NO_INIT;
                        std::vector<T> _data = data_;
                        if(_data.size() < (size_t)period_) {
                                _data.push_back(in);
                                if(_data.size() == (size_t)period_) {
                                        out = _data;
                                        return OK;
                                }
                        } else {
                                _data.push_back(in);
                                _data.erase(_data.begin());
                                out = _data;
                                return OK;
                        }
                        return INDICATOR_NOT_READY_TO_WORK;
                }

                /** \brief Очистить данные индикатора
                 */
                void clear()
                {
                        data_.clear();
                }
        };


//------------------------------------------------------------------------------
}

#endif // INDICATORSEASY_HPP_INCLUDED
