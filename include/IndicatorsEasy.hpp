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
#include <algorithm>
#include <numeric>
#include <cmath>
#include <CorrelationEasy.hpp>
#include <NormalizationEasy.hpp>
#include <AlgorithmsEasy.hpp>

#define INDICATORSEASY_DEF_RING_BUFFER_SIZE 64
//------------------------------------------------------------------------------
namespace IndicatorsEasy
{
//------------------------------------------------------------------------------
        /// Набор возможных состояний ошибки
        enum ErrorType {
                OK = 0,                             ///< Ошибок нет, все в порядке
                NO_INIT = -4,                       ///< Нет инициализации
                INVALID_PARAMETER = -6,             ///< Неверный параметр
                INDICATOR_NOT_READY_TO_WORK = -16,
        };
//------------------------------------------------------------------------------
        /** \brief Посчитать простую скользящую среднюю (SMA)
         * Данная функция для расчетов использует последние N = period значений
         * \param input массив значений
         * \param output значение SMA
         * \param period период SMA
         * \param start_pos начальная позиция в массиве
         * \return вернет 0 в случае успеха
         */
        template <typename T1, typename T2>
        int calculate_sma(std::vector<T1> &input, T2 &output, size_t period, size_t start_pos = 0)
        {
                if(input.size() <= start_pos + period)
                        return INVALID_PARAMETER;
                T2 sum = std::accumulate(input.begin() + start_pos, input.begin() + start_pos + period, T1(0));
                output = sum / (T2)period;
                return OK;
        }

        /** @brief Расчитать стандартное отклонение
         * \param input входные данные индикатора
         * \param output стандартное отклонение
         * \param period период STD
         * \param start_pos начальная позиция в массиве
         * \return вернет 0 в случае успеха
         */
        template<typename T1, typename T2>
        int calculate_standard_deviation(std::vector<T1> &input, T2 &output, size_t period, size_t start_pos = 0)
        {
                if(input.size() < start_pos + period)
                        return INVALID_PARAMETER;
                T1 mean = std::accumulate(input.begin() + start_pos, input.begin() + start_pos + period, T1(0));
                mean /= (T1)period;
                double _std_dev = 0;
                for (int i = 0; i < (int)input.size(); i++) {
                        double diff = (input[i] - mean);
                        _std_dev +=  diff * diff;
                }
                output = std::sqrt(_std_dev / (T2)(period - 1));
                return OK;
        }
//------------------------------------------------------------------------------
        template <typename T, int SIZE = INDICATORSEASY_DEF_RING_BUFFER_SIZE>
        class RingBuffer {
        public:
                //std::vector<T> data;
                T data[SIZE];
        private:
                int pos = 0;
                int data_size = 0;
                int read_count = 0;
        public:
                RingBuffer() {};

                RingBuffer(size_t size) {
                        //data.resize(size);
                        data_size = size;
                }

                void resize(size_t size) {
                        //data.resize(size);
                        data_size = size;
                }

                inline int size() {
                        return data_size;
                }

                inline int count() {
                        return read_count;
                }

                void push(T value) {
                        data[pos] = value;
                        pos =(pos + 1) % data_size;
                        if(read_count < data_size) read_count++;
                }

                bool empty() {
                        if(read_count > 0) return false;
                        return true;
                }

                void clear() {
                        pos = 0;
                        read_count = 0;
                }

                inline T& operator[] (int i) {
                        return data[(pos + i) % data_size];
                }

                inline const T operator[] (int i)const {
                        return data[(pos + i) % data_size];
                }

                std::vector<T> get_data() {
                        std::vector<T> temp(data_size);
                        for(int i = 0; i < data_size; ++i) {
                                temp[i] = (data[(pos + i) % data_size]);
                        }
                        return temp;
                }
#if(0)
                inline std::vector<T> get_raw_data() {
                        return data;
                }
#endif
                inline double get_sum() {
                        double sum = 0;
                        for(int i = 0; i < data_size; ++i) {
                                sum += (data[(pos + i) % data_size]);
                        }
                        return sum;
                }
        };
//------------------------------------------------------------------------------
        /** \brief Базовый класс индикатора
         */
        template <typename T>
        class BaseIndicator {
                public:
                virtual int update(T in) {return INVALID_PARAMETER;};
                virtual int test(T in) {return INVALID_PARAMETER;};
                virtual int update(T in, T &out) {return INVALID_PARAMETER;};
                virtual int test(T in, T &out) {return INVALID_PARAMETER;};
                virtual int update(T in, std::vector<T> &out) {return INVALID_PARAMETER;};
                virtual int test(T in, std::vector<T> &out) {return INVALID_PARAMETER;};
                virtual void clear() {};
        };
//------------------------------------------------------------------------------
        /** \brief Простая скользящая средняя
         */
        template <typename T, int SIZE = INDICATORSEASY_DEF_RING_BUFFER_SIZE>
        class SMA : BaseIndicator<T>
        {
        private:
                RingBuffer<T, SIZE> data_;
                T last_data_;
                int period_ = 0;
                int pos_ = 0;
        public:
                SMA() {};

                /** \brief Инициализировать простую скользящую среднюю
                 * \param period период
                 */
                SMA(int period) : period_(period)
                {
                        if(period_ < 0)
                                period_ = -period_;
                        data_.resize(period_);
                }

                /** \brief Обновить состояние индикатора
                 * \param in сигнал на входе
                 * \param out сигнал на выходе
                 * \return вернет 0 в случае успеха, иначе см. ErrorType
                 */
                int update(T in, T &out)
                {
                        if(period_ == 0) {
                                out = 0;
                                return NO_INIT;
                        }
                        if(data_.count() < period_) {
                                data_.push(in);
                                if(data_.count() == period_) {
                                        //T sum = std::accumulate(data_.data.begin(), data_.data.end(), T(0));
                                        T sum = data_.get_sum();
                                        last_data_ = sum ;
                                        out = sum / (T)period_;
                                        pos_ = 0;
                                        return OK;
                                }
                        } else {
                                last_data_ = last_data_ + (in - data_[0]);
                                data_.push(in);
                                out = last_data_/(T)period_;
                                return OK;
                        }
                        out = 0;
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
                        if(period_ == 0) {
                                out = 0;
                                return NO_INIT;
                        }
                        RingBuffer<T, SIZE> _data = data_;
                        if(_data.count() < (size_t)period_) {
                                _data.push(in);
                                if(_data.count() == (size_t)period_) {
                                        //T sum = std::accumulate(_data.data.begin(), _data.data.end(), T(0));
                                        T sum = _data.get_sum();
                                        out = sum / (T)period_;
                                        return OK;
                                }
                        } else {
                                out = (last_data_ - data_[0] + in)/(T)period_;
                                return OK;
                        }
                        out = 0;
                        return INDICATOR_NOT_READY_TO_WORK;
                }

                /** \brief Очистить данные индикатора
                 */
                void clear()
                {
                        data_.clear();
                        pos_ = 0;
                }
        };
//------------------------------------------------------------------------------
        /** \brief Взвешенное скользящее среднее
         */
        template <typename T>
        class WMA : BaseIndicator<T>
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

                /** \brief Обновить состояние индикатора
                 * \param in сигнал на входе
                 * \param out сигнал на выходе
                 * \return вернет 0 в случае успеха, иначе см. ErrorType
                 */
                int update(T in, T &out)
                {
                        if(period_ == 0) {
                                out = 0;
                                return NO_INIT;
                        }
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
                        out = 0;
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
                        if(period_ == 0) {
                                out = 0;
                                return NO_INIT;
                        }
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
                        out = 0;
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
        class EMA : BaseIndicator<T>
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

                /** \brief Обновить состояние индикатора
                 * \param in сигнал на входе
                 * \param out сигнал на выходе
                 * \return вернет 0 в случае успеха, иначе см. ErrorType
                 */
                int update(T in, T &out)
                {
                        if(period_ == 0) {
                                out = 0;
                                return NO_INIT;
                        }
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
                        out = 0;
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
                        if(period_ == 0) {
                                out = 0;
                                return NO_INIT;
                        }
                        if(data_.size() == period_) {
                                out = a * in + (1.0 - a) * last_data_;
                                return OK;
                        }
                        out = 0;
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
                MMA() {};

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
        class MW : BaseIndicator<T>
        {
        private:
                std::vector<T> data_;
                std::vector<T> data_test_;
                int period_ = 0;
                bool is_test_ = false;
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

                /** \brief Обновить состояние индикатора
                 * \param in сигнал на входе
                 * \param out массив на выходе
                 * \return вернет 0 в случае успеха, иначе см. ErrorType
                 */
                int update(T in, std::vector<T> &out)
                {
                        is_test_ = false;
                        if(period_ == 0) {
                                return NO_INIT;
                        }
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

                /** \brief Обновить состояние индикатора
                 * \param in сигнал на входе
                 * \return вернет 0 в случае успеха, иначе см. ErrorType
                 */
                int update(T in)
                {
                        is_test_ = false;
                        if(period_ == 0) {
                                return NO_INIT;
                        }
                        if(data_.size() < (size_t)period_) {
                                data_.push_back(in);
                                if(data_.size() == (size_t)period_) {
                                        return OK;
                                }
                        } else {
                                data_.push_back(in);
                                data_.erase(data_.begin());
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
                        is_test_ = true;
                        if(period_ == 0) {
                                return NO_INIT;
                        }
                        data_test_ = data_;
                        if(data_test_.size() < (size_t)period_) {
                                data_test_.push_back(in);
                                if(data_test_.size() == (size_t)period_) {
                                        out = data_test_;
                                        return OK;
                                }
                        } else {
                                data_test_.push_back(in);
                                data_test_.erase(data_test_.begin());
                                out = data_test_;
                                return OK;
                        }
                        return INDICATOR_NOT_READY_TO_WORK;
                }

                /** \brief Протестировать индикатор
                 * Данная функция отличается от update тем, что не влияет на внутреннее состояние индикатора
                 * \param in сигнал на входе
                 * \return вернет 0 в случае успеха, иначе см. ErrorType
                 */
                int test(T in)
                {
                        is_test_ = true;
                        if(period_ == 0) {
                                return NO_INIT;
                        }
                        data_test_ = data_;
                        if(data_test_.size() < (size_t)period_) {
                                data_test_.push_back(in);
                                if(data_test_.size() == (size_t)period_) {
                                        return OK;
                                }
                        } else {
                                data_test_.push_back(in);
                                data_test_.erase(data_test_.begin());
                                return OK;
                        }
                        return INDICATOR_NOT_READY_TO_WORK;
                }

                /** \brief Получить данные внутреннего буфера индикатора
                 * \param out буфер
                 */
                void get_data(std::vector<T> &out)
                {
                        if(is_test_)
                                out = data_test_;
                        else
                                out = data_;
                }

                /** \brief Получить максимальное значение буфера
                 * \param out максимальное значение
                 * \param offset смещение в массиве
                 */
                void get_max_data(T &out, const size_t offset = 0)
                {
                        if(is_test_)
                                out = *std::max_element(data_test_.begin() + offset, data_test_.end());
                        else
                                out = *std::max_element(data_.begin() + offset, data_.end());
                }

                /** \brief Получить минимальное значение буфера
                 * \param out минимальное значение
                 * \param offset смещение в массиве
                 */
                void get_min_data(T &out, const size_t offset = 0)
                {
                        if(is_test_)
                                out = *std::min_element(data_test_.begin() + offset, data_test_.end());
                        else
                                out = *std::min_element(data_.begin() + offset, data_.end());
                }

                /** \brief Получить среднее значение буфера
                 * \param out среднее значение
                 * \param offset смещение в массиве
                 */
                void get_average_data(T &out, const size_t offset = 0)
                {
                        if(is_test_) {
                                T sum = std::accumulate(data_test_.begin() + offset, data_test_.end(), T(0));
                                out = sum / (T)(data_test_.size() - offset);
                        } else {
                                T sum = std::accumulate(data_.begin() + offset, data_.end(), T(0));
                                out = sum / (T)(data_.size() - offset);
                        }
                }

                /** \brief Получить набор средних значений и стандартного отклонения буфера
                 * Минимальный период равен 2
                 * \param average_data массив средних значений
                 * \param std_data массив стандартного отклонения
                 * \param min_period минимальный период
                 * \param min_period максимальный период
                 * \param step_period шаг периода
                 */
                void get_average_and_std_data(std::vector<T> &average_data, std::vector<T> &std_data, size_t min_period, size_t max_period, size_t step_period)
                {
                        min_period--;
                        max_period--;
                        average_data.clear();
                        std_data.clear();
                        if(is_test_) {
                                T sum = 0;
                                T sum_std = 0;
                                int num_element = 0;
                                for(int i = (int)data_test_.size() - 1; i >= 0; --i) { // начинаем список с конца
                                        sum += data_test_[i]; // находим сумму элементов
                                        if(num_element > max_period) break;
                                        if(num_element >= min_period) {
                                                num_element++; // находим число элементов
                                                T ml = (T)(sum/(T)num_element); // находим среднее
                                                average_data.push_back(ml); // добавляем среднее
                                                T sum_std = 0;
                                                int max_len = (int)data_test_.size() - num_element;
                                                for(int j = (int)data_test_.size() - 1; j >= max_len; --j) {
                                                        T diff = (data_test_[j] - ml);
                                                        sum_std += diff * diff;
                                                }
                                                std_data.push_back((T)std::sqrt(sum_std / (T)(num_element - 1)));
                                                min_period += step_period;
                                        } else {
                                                num_element++;
                                        }
                                } // for i
                        } else {
                                T sum = 0;
                                T sum_std = 0;
                                int num_element = 0;
                                for(int i = (int)data_.size() - 1; i >= 0; --i) { // начинаем список с конца
                                        sum += data_[i]; // находим сумму элементов
                                        if(num_element > max_period) break;
                                        if(num_element >= min_period) {
                                                num_element++; // находим число элементов
                                                T ml = (T)(sum/(T)num_element); // находим среднее
                                                average_data.push_back(ml); // добавляем среднее
                                                T sum_std = 0;
                                                int max_len = (int)data_.size() - num_element;
                                                for(int j = (int)data_.size() - 1; j >= max_len; j--) {
                                                        T diff = (data_[j] - ml);
                                                        sum_std += diff * diff;
                                                }
                                                std_data.push_back((T)std::sqrt(sum_std / (T)(num_element - 1)));
                                                min_period += step_period;
                                        } else {
                                                num_element++;
                                        }
                                } // for i
                        }
                }

                /** \brief Получить массив значений RSI
                 * \param rsi_data массив значений RSI
                 * \param min_period минимальный период
                 * \param min_period максимальный период
                 * \param step_period шаг периода
                 */
                void get_rsi_data(std::vector<T> &rsi_data, size_t min_period, size_t max_period, size_t step_period) {
                        min_period--;
                        max_period--;
                        rsi_data.clear();
                        if(is_test_) {
                                T sum_u = 0, sum_d = 0;
                                int num_element = 0;
                                for(int i = (int)data_test_.size() - 1; i >= 1; --i) { // начинаем список с конца
                                        T u = 0, d = 0;
                                        const T prev_ = data_test_[i - 1];
                                        const T in_ = data_test_[i];
                                        if(prev_ < in_) u = in_ - prev_;
                                        else if(prev_ > in_) d = prev_ - in_;
                                        sum_u += u;
                                        sum_d += d;
                                        if(num_element > max_period) break;
                                        if(num_element >= min_period) {
                                                num_element++;
                                                u = sum_u /(T)num_element;
                                                d = sum_d /(T)num_element;
                                                if(d == 0) rsi_data.push_back(100.0);
                                                else rsi_data.push_back(((T)100.0 - ((T)100.0 / ((T)1.0 + (u / d)))));
                                                min_period += step_period;
                                        } else {
                                                num_element++;
                                        }
                                } // for i
                        } else {
                                T sum_u = 0;
                                T sum_d = 0;
                                int num_element = 0;
                                for(int i = (int)data_.size() - 1; i >= 1; --i) { // начинаем список с конца
                                        if(data_[i - 1] < data_[i]) sum_u += data_[i] - data_[i - 1];
                                        else if(data_[i - 1] > data_[i]) sum_d += data_[i - 1] - data_[i];
                                        if(num_element > max_period) break;
                                        if(num_element >= min_period) {
                                                num_element++;
                                                T u = sum_u /(T)num_element;
                                                T d = sum_d /(T)num_element;
                                                if(d == 0) rsi_data.push_back(100.0);
                                                else rsi_data.push_back(((T)100.0 - ((T)100.0 / ((T)1.0 + (u / d)))));
                                                min_period += step_period;
                                        } else {
                                                num_element++;
                                        }
                                } // for i
                        }
                }

                /** \brief Получить стандартное отклонение буфера
                 * \param out стандартное отклонение
                 * \param offset смещение в массиве
                 */
                void get_std_data(T &out, size_t offset = 0)
                {
                        if(is_test_) {
                                T ml = std::accumulate(data_test_.begin() + offset, data_test_.end(), T(0));
                                ml /= (T)(data_test_.size() - offset);
                                T sum = 0;
                                for (size_t i = offset; i < data_test_.size(); i++) {
                                        T diff = (data_test_[i] - ml);
                                        sum +=  diff * diff;
                                }
                                out = std::sqrt(sum / (T)(data_test_.size() - offset - 1));
                        } else {
                                T ml = std::accumulate(data_.begin() + offset, data_.end(), T(0));
                                ml /= (T)(data_.size() - offset);
                                T sum = 0;
                                for (size_t i = offset; i < data_.size(); i++) {
                                        T diff = (data_[i] - ml);
                                        sum +=  diff * diff;
                                }
                                out = std::sqrt(sum / (T)(data_.size() - offset - 1));
                        }
                }

                /** \brief Очистить данные индикатора
                 */
                void clear()
                {
                        data_.clear();
                        data_test_.clear();
                }
        };
//------------------------------------------------------------------------------
        /** \brief Класс фильтра низкой частоты
         */
        template <typename T>
        class LowPassFilter : BaseIndicator<T>
        {
        private:
                T alfa_;
                T beta_;
                T prev_;
                T tranTime;
                bool is_update_ = false;
                bool is_init_ = false;
        public:

                /** \brief Инициализация фильтра низкой частоты
                 * \param dt время переходного процесса
                 * \param period период дискретизации
                 * \param error_signal ошибка сигнала
                 */
                LowPassFilter(T dt, T period = 1.0, T error_signal = 0.03)
                {
                        T N = dt / period;
                        T Ntay = std::log(1.0 / error_signal);
                        alfa_ = std::exp(-Ntay / N);
                        beta_ = 1.0 - alfa_;
                        is_init_ = true;
                }

                /** \brief Получить новые данные индикатора
                 * \param in сигнал на входе
                 * \param out массив на выходе
                 * \return вернет 0 в случае успеха, иначе см. ErrorType
                 */
                int update(T in, T &out) {
                        if(!is_init_) {
                                return NO_INIT;
                        }
                        if (!is_update_) {
                                prev_ = in;
                                is_update_ = true;
                                out = 0;
                                return INDICATOR_NOT_READY_TO_WORK;
                        }
                        out = alfa_ * prev_ + beta_ * in;
                        prev_ = out;
                        return OK;
                }

                /** \brief Протестировать индикатор
                 * Данная функция отличается от update тем, что не влияет на внутреннее состояние индикатора
                 * \param in сигнал на входе
                 * \param out сигнал на выходе
                 * \return вернет 0 в случае успеха, иначе см. ErrorType
                 */
                int test(T in, T &out) {
                        if(!is_init_) {
                                return NO_INIT;
                        }
                        if (!is_init_) {
                                out = 0;
                                return INDICATOR_NOT_READY_TO_WORK;
                        }
                        out = alfa_ * prev_ + beta_ * in;
                        return OK;
                }

                /** \brief Очистить данные индикатора
                 */
                void clear()
                {
                        is_update_ = false;
                }
        };
//------------------------------------------------------------------------------
        /** \brief Индекс относительной силы
         */
        template <typename T, class INDICATOR_TYPE>
        class RSI : BaseIndicator<T>
        {
        private:
                INDICATOR_TYPE iU;
                INDICATOR_TYPE iD;
                bool is_init_ = false;
                bool is_update_ = false;
                T prev_;
        public:
                RSI() {}

                /** \brief Инициализировать индикатор индекса относительной силы
                 * \param period период индикатора
                 */
                RSI(int period) : iU(period), iD(period)
                {
                        is_init_ = true;
                }

                /** \brief Инициализировать индикатор индекса относительной силы
                 * \param period период индикатора
                 */
                void init(int period) {
                        is_init_ = true;
                        is_update_ = false;
                        iU = INDICATOR_TYPE(period);
                        iD = INDICATOR_TYPE(period);
                }

                /** \brief Обновить состояние индикатора
                 * \param in сигнал на входе
                 * \param out сигнал на выходе
                 * \return вернет 0 в случае успеха, иначе см. ErrorType
                 */
                int update(T in, T &out)
                {
                        if(!is_init_) {
                                return NO_INIT;
                        }
                        if(!is_update_) {
                                prev_ = in;
                                is_update_ = true;
                                out = 50.0;
                                return INDICATOR_NOT_READY_TO_WORK;
                        }
                        T u = 0;
                        T d = 0;
                        if(prev_ < in) {
                                u = in - prev_;
                        } else
                        if(prev_ > in) {
                                d = prev_ - in;
                        }
                        int erru, errd = 0;
                        T mu = 0;
                        T md = 0;
                        erru = iU.update(u, mu);
                        errd = iD.update(d, md);
                        prev_ = in;
                        if(erru != OK || errd != OK) {
                                out = 50.0;
                                return INDICATOR_NOT_READY_TO_WORK;
                        }
                        if(md == 0) {
                                out = 100.0;
                                return OK;
                        }
                        T rs = mu / md;
                        out = 100.0 - (100.0 / (1.0 + rs));
                        return OK;
                }

                /** \brief Обновить состояние индикатора
                 * \param in сигнал на входе
                 * \return вернет 0 в случае успеха, иначе см. ErrorType
                 */
                int update(T in)
                {
                        if(!is_init_) {
                                return NO_INIT;
                        }
                        if(!is_update_) {
                                prev_ = in;
                                is_update_ = true;
                                return INDICATOR_NOT_READY_TO_WORK;
                        }
                        T u = 0, d = 0;
                        if(prev_ < in) {
                                u = in - prev_;
                        } else
                        if(prev_ > in) {
                                d = prev_ - in;
                        }
                        int erru, errd = 0;
                        erru = iU.update(u, u);
                        errd = iD.update(d, d);
                        prev_ = in;
                        if(erru != OK || errd != OK) {
                                return INDICATOR_NOT_READY_TO_WORK;
                        }
                        return OK;
                }

                /** \brief Протестировать индикатор
                 * Данная функция отличается от update тем, что не влияет на внутреннее состояние индикатора
                 * \param in сигнал на входе
                 * \param out сигнал на выходе
                 * \return вернет 0 в случае успеха, иначе см. ErrorType
                 */
                int test(T in, T &out)
                {
                        if(!is_init_) {
                                return NO_INIT;
                        }
                        if(!is_update_) {
                                out = 50.0;
                                return INDICATOR_NOT_READY_TO_WORK;
                        }
                        T u = 0, d = 0;
                        if(prev_ < in) {
                                u = in - prev_;
                        } else
                        if(prev_ > in) {
                                d = prev_ - in;
                        }
                        int erru, errd = 0;
                        erru = iU.test(u, u);
                        errd = iD.test(d, d);
                        if(erru != OK || errd != OK) {
                                out = 50.0;
                                return INDICATOR_NOT_READY_TO_WORK;
                        }
                        if(d == 0) {
                                out = 100.0;
                                return OK;
                        }
                        T rs = u / d;
                        out = 100.0 - (100.0 / (1.0 + rs));
                        return OK;
                }

                /** \brief Очистить данные индикатора
                 */
                void clear()
                {
                        is_update_ = false;
                }
        };
//------------------------------------------------------------------------------
        /** \brief Линии Боллинджера
         */
        template <typename T>
        class BollingerBands : BaseIndicator<T>
        {
        private:
                std::vector<T> data_;
                int period_ = 0;
                T d_;
        public:
                BollingerBands() {};

                /** \brief Инициализация линий Боллинджера
                 * \param period период  индикатора
                 * \param factor множитель стандартного отклонения
                 */
                BollingerBands(int period, T factor)
                {
                        period_ = period;
                        d_ = factor;
                }

                /** \brief Инициализация линий Боллинджера
                 * \param period период  индикатора
                 * \param factor множитель стандартного отклонения
                 */
                void init(int period, T factor)
                {
                        period_ = period;
                        d_ = factor;
                        data_.clear();
                }

                /** \brief Обновить состояние индикатора
                 * \param in сигнал на входе
                 * \param tl верхняя полоса боллинджера
                 * \param ml среняя полоса боллинджера
                 * \param bl нижняя полоса боллинджера
                 * \return вернет 0 в случае успеха, иначе см. ErrorType
                 */
                int update(T in, T &tl, T &ml, T &bl)
                {
                        if(period_ == 0) {
                                tl = 0;
                                ml = 0;
                                bl = 0;
                                return NO_INIT;
                        }
                        if(data_.size() < (size_t)period_) {
                                data_.push_back(in);
                                if(data_.size() != (size_t)period_) {
                                        tl = 0;
                                        ml = 0;
                                        bl = 0;
                                        return INDICATOR_NOT_READY_TO_WORK;
                                }
                        } else {
                                data_.push_back(in);
                                data_.erase(data_.begin());
                        }
                        ml = std::accumulate(data_.begin(), data_.end(), T(0));
                        ml /= (T)period_;
                        T sum = 0;
                        for (int i = 0; i < period_; i++) {
                                T diff = (data_[i] - ml);
                                sum +=  diff * diff;
                        }
                        T std_dev = std::sqrt(sum / (T)(period_ - 1));
                        tl = std_dev * d_ + ml;
                        bl = ml - std_dev * d_;
                        return OK;
                }

                /** \brief Обновить состояние индикатора
                 * \param in сигнал на входе
                 * \param ml среняя полоса боллинджера
                 * \param std_dev стандартное отклонение
                 * \return вернет 0 в случае успеха, иначе см. ErrorType
                 */
                int update(T in, T &ml, T &std_dev)
                {
                        if(period_ == 0) {
                                ml = 0;
                                std_dev = 0;
                                return NO_INIT;
                        }
                        if(data_.size() < (size_t)period_) {
                                data_.push_back(in);
                                if(data_.size() != (size_t)period_) {
                                        ml = 0;
                                        std_dev = 0;
                                        return INDICATOR_NOT_READY_TO_WORK;
                                }
                        } else {
                                data_.push_back(in);
                                data_.erase(data_.begin());
                        }
                        ml = std::accumulate(data_.begin(), data_.end(), T(0));
                        ml /= (T)period_;
                        T sum = 0;
                        for (int i = 0; i < period_; i++) {
                                T diff = (data_[i] - ml);
                                sum +=  diff * diff;
                        }
                        std_dev = std::sqrt(sum / (T)(period_ - 1));
                        return OK;
                }

                /** \brief Обновить состояние индикатора
                 * \param in сигнал на входе
                 * \return вернет 0 в случае успеха, иначе см. ErrorType
                 */
                int update(T in)
                {
                        if(period_ == 0) {
                                return NO_INIT;
                        }
                        if(data_.size() < (size_t)period_) {
                                data_.push_back(in);
                                if(data_.size() != (size_t)period_) {
                                        return INDICATOR_NOT_READY_TO_WORK;
                                }
                        } else {
                                data_.push_back(in);
                                data_.erase(data_.begin());
                        }
                        return OK;
                }

                /** \brief Протестировать индикатор
                 * Данная функция отличается от update тем, что не влияет на внутреннее состояние индикатора
                 * \param in сигнал на входе
                 * \param tl верхняя полоса боллинджера
                 * \param ml среняя полоса боллинджера
                 * \param bl нижняя полоса боллинджера
                 * \return вернет 0 в случае успеха, иначе см. ErrorType
                 */
                int test(T in, T &tl, T &ml, T &bl)
                {
                        if(period_ == 0) {
                                tl = 0;
                                ml = 0;
                                bl = 0;
                                return NO_INIT;
                        }
                        std::vector<T> data_test = data_;
                        if(data_test.size() < (size_t)period_) {
                                data_test.push_back(in);
                                if(data_test.size() != (size_t)period_) {
                                        tl = 0;
                                        ml = 0;
                                        bl = 0;
                                        return INDICATOR_NOT_READY_TO_WORK;
                                }
                        } else {
                                data_test.push_back(in);
                                data_test.erase(data_test.begin());
                        }
                        ml = std::accumulate(data_test.begin(), data_test.end(), T(0));
                        ml /= (T)period_;
                        T sum = 0;
                        for (int i = 0; i < period_; i++) {
                                T diff = (data_test[i] - ml);
                                sum +=  diff * diff;
                        }
                        T std_dev = std::sqrt(sum / (T)(period_ - 1));
                        tl = std_dev * d_ + ml;
                        bl = ml - std_dev * d_;
                        return OK;
                }

                /** \brief Протестировать индикатор
                 * Данная функция отличается от update тем, что не влияет на внутреннее состояние индикатора
                 * \param in сигнал на входе
                 * \param ml среняя полоса боллинджера
                 * \param std_dev стандартное отклонение
                 * \return вернет 0 в случае успеха, иначе см. ErrorType
                 */
                int test(T in, T &ml, T &std_dev)
                {
                        if(period_ == 0) {
                                ml = 0;
                                std_dev = 0;
                                return NO_INIT;
                        }
                        std::vector<T> data_test = data_;
                        if(data_test.size() < (size_t)period_) {
                                data_test.push_back(in);
                                if(data_test.size() != (size_t)period_) {
                                        ml = 0;
                                        std_dev = 0;
                                        return INDICATOR_NOT_READY_TO_WORK;
                                }
                        } else {
                                data_test.push_back(in);
                                data_test.erase(data_test.begin());
                        }
                        ml = std::accumulate(data_test.begin(), data_test.end(), T(0));
                        ml /= (T)period_;
                        T sum = 0;
                        for (int i = 0; i < period_; i++) {
                                T diff = (data_test[i] - ml);
                                sum +=  diff * diff;
                        }
                        std_dev = std::sqrt(sum / (T)(period_ - 1));
                        return OK;
                }

                /** \brief Очистить данные индикатора
                 */
                void clear()
                {
                        data_.clear();
                }
        };
//------------------------------------------------------------------------------
        /** \brief Средняя скорость
         */
        template <typename T>
        class AverageSpeed : BaseIndicator<T>
        {
        private:
                MW<T> iMW;
                bool is_init_ = false;
        public:
                AverageSpeed() {};

                /** \brief Инициализировать класс индикатора
                 * \param period период индикатора
                 */
                AverageSpeed(int period) : iMW(period + 1)
                {
                        is_init_ = true;
                }

                /** \brief Обновить состояние индикатора
                 * \param in Цена
                 * \param out Значение индикатора
                 * \return вернет 0 в случае успеха
                 */
                int update(T in, T &out)
                {
                        if(!is_init_) {
                                return NO_INIT;
                        }
                        std::vector<T> mw_out;
                        int err = iMW.update(in, mw_out);
                        if(err == OK) {
                                std::vector<T> mw_diff;
                                NormalizationEasy::calculate_difference(mw_out, mw_diff);
                                T sum = std::accumulate(mw_diff.begin(), mw_diff.end(), T(0));
                                out = sum /(T)mw_diff.size();
                                return OK;
                        }
                        return err;
                }

                /** \brief Протестировать индикатор
                 * \param in Цена
                 * \param out Значение индикатора
                 * \return вернет 0 в случае успеха
                 */
                int test(T in, T &out)
                {
                        if(!is_init_) {
                                return NO_INIT;
                        }
                        std::vector<T> mw_out;
                        int err = iMW.test(in, mw_out);
                        if(err == OK) {
                                std::vector<T> mw_diff;
                                NormalizationEasy::calculate_difference(mw_out, mw_diff);
                                T sum = std::accumulate(mw_diff.begin(), mw_diff.end(), T(0));
                                out = sum /(T)mw_diff.size();
                                return OK;
                        }
                        return err;
                }

                /** \brief Очистить состояние индикатора
                 */
                void clear()
                {
                        iMW.clear();
                }
        };
//------------------------------------------------------------------------------
        template <typename T>
        class DetectorWaveform
        {
        private:
                MW<T> iMW;
                const int MIN_WAVEFORM_LEN = 3;
                T coeff_exp = 3.141592;
                std::vector<std::vector<T>> exp_data_up_;
                std::vector<std::vector<T>> exp_data_dn_;

                void init_exp_data_up(std::vector<T> &data)
                {
                        T dt = 1.0/(T)data.size();
                        for(size_t i = 0; i < data.size(); ++i) {
                                data[i] = exp(coeff_exp*(T)i*dt);
                        }
                        NormalizationEasy::calculate_min_max(data, data, NormalizationEasy::MINMAX_0_1);
                }

                void init_exp_data_dn(std::vector<T> &data)
                {
                        T dt = 1.0/(T)data.size();
                        for(size_t i = 0; i < data.size(); ++i) {
                                data[i] = -exp(coeff_exp*(T)i*dt);
                        }
                        NormalizationEasy::calculate_min_max(data, data, NormalizationEasy::MINMAX_0_1);
                }
        public:
                /** \brief Инициализировать класс
                 * \param max_len максимальная длина файла
                 */
                DetectorWaveform(int max_len) : iMW(max_len)
                {
                        if(max_len < MIN_WAVEFORM_LEN)
                                return;
                        size_t max_num_exp_data = max_len - MIN_WAVEFORM_LEN + 1;
                        exp_data_up_.resize(max_num_exp_data);
                        exp_data_dn_.resize(max_num_exp_data);
                        for(int l = MIN_WAVEFORM_LEN; l <= max_len; ++l) {
                                exp_data_up_[l-MIN_WAVEFORM_LEN].resize(l);
                                exp_data_dn_[l-MIN_WAVEFORM_LEN].resize(l);
                                init_exp_data_up(exp_data_up_[l-MIN_WAVEFORM_LEN]);
                                init_exp_data_dn(exp_data_dn_[l-MIN_WAVEFORM_LEN]);
                        }
                }

                int update(T in, T &out, int len_waveform)
                {
                        std::vector<T> mw_out;
                        int err = iMW.update(in, mw_out);
                        if(err == OK) {
                                if((int)mw_out.size() >= MIN_WAVEFORM_LEN && len_waveform <= (int)mw_out.size()) {
                                        std::vector<T> fragment_data;
                                        fragment_data.insert(fragment_data.begin(), mw_out.begin() + mw_out.size() - len_waveform, mw_out.end());
                                        int err_n = NormalizationEasy::calculate_min_max(fragment_data, fragment_data, NormalizationEasy::MINMAX_0_1);
                                        if(err_n != OK) return err_n;
                                        T coeff_up = 0, coeff_dn = 0;
                                        int err_up = CorrelationEasy::calculate_spearman_rank_correlation_coefficient(fragment_data, exp_data_up_[len_waveform-MIN_WAVEFORM_LEN], coeff_up);
                                        //AlgorithmsEasy::calc_levenstein_distance(fragment_data, exp_data_up_[len_waveform-MIN_WAVEFORM_LEN])
                                        int err_dn = CorrelationEasy::calculate_spearman_rank_correlation_coefficient(fragment_data, exp_data_dn_[len_waveform-MIN_WAVEFORM_LEN], coeff_dn);
                                        if(err_up != OK) return err_up;
                                        if(err_dn != OK) return err_dn;
                                        if(abs(coeff_up) > abs(coeff_dn)) {
                                                out = coeff_up;
                                        } else {
                                                out = coeff_dn;
                                        }
                                        return OK;
                                }
                        }
                        return err;
                }

                void clear()
                {
                     iMW.clear();
                }
        };
//------------------------------------------------------------------------------
        /** \brief Класс для подсчета коррлеяции между валютными парами
         */
        template <typename T>
        class CurrencyCorrelation
        {
        private:
                std::vector<std::vector<T>> data_;
                std::vector<std::vector<T>> data_test_;
                int period_ = 0;
                bool is_test_ = false;
        public:
                enum CorrelationType {
                        SPEARMAN_RANK = 0,
                        PEARSON = 1,
                };
                /** \brief Инициализировать индикатор
                 * \param period период индикатора
                 * \param num_symbols колючество валютных пар
                 */
                CurrencyCorrelation(int period, int num_symbols)
                {
                        data_.resize(num_symbols);
                        data_test_.resize(num_symbols);
                        period_ = period;
                }

                /** \brief Обновить состояние индикатора
                 * \param in сигнал на входе
                 * \param num_symbol номер валютной пары
                 * \return вернет 0 в случае успеха, иначе см. ErrorType
                 */
                int update(T in, int num_symbol)
                {
                        is_test_ = false;
                        if(period_ == 0) {
                                return NO_INIT;
                        }
                        if(data_[num_symbol].size() < (size_t)period_) {
                                data_[num_symbol].push_back(in);
                                if(data_[num_symbol].size() == (size_t)period_) {
                                        return OK;
                                }
                        } else {
                                data_[num_symbol].push_back(in);
                                data_[num_symbol].erase(data_[num_symbol].begin());
                                return OK;
                        }
                        return INDICATOR_NOT_READY_TO_WORK;
                }

                /** \brief Обновить состояние индикатора
                 * \param in сигнал на входе
                 * \param num_symbol номер валютной пары
                 * \return вернет 0 в случае успеха, иначе см. ErrorType
                 */
                int test(T in, int num_symbol)
                {
                        is_test_ = true;
                        if(period_ == 0) {
                                return NO_INIT;
                        }
                        data_test_ = data_;
                        if(data_test_[num_symbol].size() < (size_t)period_) {
                                data_test_[num_symbol].push_back(in);
                                if(data_test_[num_symbol].size() == (size_t)period_) {
                                        return OK;
                                }
                        } else {
                                data_test_[num_symbol].push_back(in);
                                data_test_[num_symbol].erase(data_test_[num_symbol].begin());
                                return OK;
                        }
                        return INDICATOR_NOT_READY_TO_WORK;
                }

                /** \brief Посчитать корреляцию между двумя валютными парами
                 * \param out значение корреляции
                 * \param num_symbol_1 номер первой валютной пары
                 * \param num_symbol_2 номер второй валютной пары
                 * \param correlation_type тип корреляции (SPEARMAN_RANK, PEARSON)
                 * \return состояние ошибки, 0 в случае успеха
                 */
                int calculate_correlation(T &out, int num_symbol_1, int num_symbol_2, int correlation_type = SPEARMAN_RANK)
                {
                        std::vector<T> norm_vec_1, norm_vec_2;
                        if(is_test_) {
                                if(data_test_[num_symbol_1].size() == (size_t)period_ &&
                                        data_test_[num_symbol_2].size() == (size_t)period_) {
                                        if(correlation_type == SPEARMAN_RANK) {
                                                NormalizationEasy::calculate_min_max(data_test_[num_symbol_1], norm_vec_1, NormalizationEasy::MINMAX_1_1);
                                                NormalizationEasy::calculate_min_max(data_test_[num_symbol_2], norm_vec_2, NormalizationEasy::MINMAX_1_1);
                                                return CorrelationEasy::calculate_spearman_rank_correlation_coefficient(norm_vec_1, norm_vec_2, out);
                                        } else
                                        if(correlation_type == PEARSON) {
                                                NormalizationEasy::calculate_min_max(data_test_[num_symbol_1], norm_vec_1, NormalizationEasy::MINMAX_1_1);
                                                NormalizationEasy::calculate_min_max(data_test_[num_symbol_2], norm_vec_2, NormalizationEasy::MINMAX_1_1);
                                                return CorrelationEasy::calculate_pearson_correlation_coefficient(norm_vec_1, norm_vec_2, out);
                                        } else {
                                                return INVALID_PARAMETER;
                                        }
                                }
                        } else {
                                if(data_[num_symbol_1].size() == (size_t)period_ &&
                                        data_[num_symbol_2].size() == (size_t)period_) {
                                        if(correlation_type == SPEARMAN_RANK) {
                                                NormalizationEasy::calculate_min_max(data_[num_symbol_1], norm_vec_1, NormalizationEasy::MINMAX_1_1);
                                                NormalizationEasy::calculate_min_max(data_[num_symbol_2], norm_vec_2, NormalizationEasy::MINMAX_1_1);
                                                return CorrelationEasy::calculate_spearman_rank_correlation_coefficient(norm_vec_1, norm_vec_2, out);
                                        } else
                                        if(correlation_type == PEARSON) {
                                                NormalizationEasy::calculate_min_max(data_[num_symbol_1], norm_vec_1, NormalizationEasy::MINMAX_1_1);
                                                NormalizationEasy::calculate_min_max(data_[num_symbol_2], norm_vec_2, NormalizationEasy::MINMAX_1_1);
                                                return CorrelationEasy::calculate_pearson_correlation_coefficient(norm_vec_1, norm_vec_2, out);
                                        } else {
                                                return INVALID_PARAMETER;
                                        }
                                }
                        }
                        return INDICATOR_NOT_READY_TO_WORK;
                }

                /** \brief Найти коррелирующие валютные пары
                 * \param symbol_1 список первой валютной пары в коррелирующей паре
                 * \param symbol_2 список второй валютной пары в коррелирующей паре
                 * \param threshold_coefficient порог срабатывания для коэффициента корреляции
                 * \param correlation_type тип корреляции (SPEARMAN_RANK, PEARSON)
                 */
                void find_correlated_pairs(std::vector<int> &symbol_1, std::vector<int> &symbol_2, std::vector<T> &coefficient, T threshold_coefficient, int correlation_type = SPEARMAN_RANK)
                {
                        symbol_1.clear();
                        symbol_2.clear();
                        coefficient.clear();
                        if(is_test_) {
                                for(size_t i = 0; i < data_test_.size() - 1; ++i) {
                                        for(size_t j = i + 1; j < data_test_.size(); ++j) {
                                                T coeff;
                                                if(calculate_correlation(coeff, i, j, correlation_type) == OK) {
                                                        if(std::abs(coeff) > threshold_coefficient) {
                                                                symbol_1.push_back(i);
                                                                symbol_2.push_back(j);
                                                                coefficient.push_back(coeff);
                                                        }
                                                }
                                        }
                                }
                        }
                }

                /** \brief Очистить данные индикатора
                 */
                void clear()
                {
                        data_.clear();
                        data_test_.clear();
                }
        };
//------------------------------------------------------------------------------
        /** \brief Мера склонности к чередовнию знаков (z-счет)
         * Z - число СКО, на которое количество серий в выборке отклоняется от своего математчиеского ожидания
         * Если z > 3, то с вероятностью 0,9973 знаки имеют склонность к чередованию
         * Если z <-3, то с аналогичной вероятнсотью проявляется склонность к сохранению знака
         * \param n общее число элементов в последовательности
         * \param r общее число серий положительных и отрицательных приращений
         * \param w общее число положительных приращений
         * \param l общее число отрицательных приращений
         * \return вернет Z
         */
        double calc_z_score(int n, int r, int w, int l)
        {
                double P = 2.0d * w * l;
                return (n * ((double)r - 0.5d) -  P) / sqrt((P * (P - (double)n))/((double)n - 1.0d));
        }
//------------------------------------------------------------------------------
        /** \brief Рассчитать долю капитала для стратегии на основе меры склонности к чередовнию знаков
         * \param p вероятность правильного прогноза (от 0.0 до 1.0)
         * \param winperc процент выплаты брокера (от 0.0)
         * \return оптимальная доля капитала
         */
        double calc_z_scor_capital_share(double p, double winperc)
        {
                return p - (1.0 - p) * (1.0/winperc);
        }
//------------------------------------------------------------------------------
}

#endif // INDICATORSEASY_HPP_INCLUDED
