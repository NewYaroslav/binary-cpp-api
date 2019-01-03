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
                if(input.size() <= start_pos + period)
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
                        if(period_ == 0) {
                                out = 0;
                                return NO_INIT;
                        }
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
                                        T sum = std::accumulate(_data.begin(), _data.end(), T(0));
                                        out = sum / (T)period_;
                                        return OK;
                                }
                        } else {
                                out = last_data_ - data_[0] + in;
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

                /** \brief Получить новые данные индикатора
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
                 */
                void get_max_data(T &out)
                {
                        if(is_test_)
                                out = *std::max_element(data_test_.begin(), data_test_.end());
                        else
                                out = *std::max_element(data_.begin(), data_.end());
                }

                /** \brief Получить минимальное значение буфера
                 * \param out минимальное значение
                 */
                void get_min_data(T &out)
                {
                        if(is_test_)
                                out = *std::min_element(data_test_.begin(), data_test_.end());
                        else
                                out = *std::min_element(data_.begin(), data_.end());
                }

                /** \brief Получить среднее значение буфера
                 * \param out среднее значение
                 */
                void get_average_data(T &out)
                {
                        if(is_test_) {
                                T sum = std::accumulate(data_test_.begin(), data_test_.end(), T(0));
                                out = sum / (T)data_test_.size();
                        } else {
                                T sum = std::accumulate(data_.begin(), data_.end(), T(0));
                                out = sum / (T)data_.size();
                        }
                }

                /** \brief Получить стандартное отклонение буфера
                 * \param out стандартное отклонение
                 */
                void get_std_data(T &out)
                {
                        if(is_test_) {
                                T ml = std::accumulate(data_test_.begin(), data_test_.end(), T(0));
                                ml /= (T)data_test_.size();
                                T sum = 0;
                                for (size_t i = 0; i < data_test_.size(); i++) {
                                        T diff = (data_test_[i] - ml);
                                        sum +=  diff * diff;
                                }
                                out = std::sqrt(sum / (T)(data_test_.size() - 1));
                        } else {
                                T ml = std::accumulate(data_.begin(), data_.end(), T(0));
                                ml /= (T)data_.size();
                                T sum = 0;
                                for (size_t i = 0; i < data_.size(); i++) {
                                        T diff = (data_[i] - ml);
                                        sum +=  diff * diff;
                                }
                                out = std::sqrt(sum / (T)(data_.size() - 1));
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
        class LowPassFilter
        {
        private:
                T alfa_;
                T beta_;
                T prev_;
                T tranTime;
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
                }

                /** \brief Получить новые данные индикатора
                 * \param in сигнал на входе
                 * \param out массив на выходе
                 * \return вернет 0 в случае успеха, иначе см. ErrorType
                 */
                int update(T in, T &out) {
                        if (!is_init_) {
                                prev_ = in;
                                is_init_ = true;
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
                        is_init_ = false;
                }
        };
//------------------------------------------------------------------------------
        /** \brief Индекс относительной силы
         */
        template <typename T, class INDICATOR_TYPE>
        class RSI
        {
        private:
                INDICATOR_TYPE iU;
                INDICATOR_TYPE iD;
                bool is_init_ = false;
                T prev_;
        public:

                /** \brief Инициализировать индикатор индекса относительной силы
                 * \param period период индикатора
                 */
                RSI(int period) : iU(period), iD(period)
                {

                }

                /** \brief Получить новые данные индикатора
                 * \param in сигнал на входе
                 * \param out массив на выходе
                 * \return вернет 0 в случае успеха, иначе см. ErrorType
                 */
                int update(T in, T &out)
                {
                        if(!is_init_) {
                                prev_ = in;
                                is_init_ = true;
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
                        erru = iU.update(u, u);
                        errd = iD.update(d, d);
                        prev_ = in;
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

                /** \brief Протестировать индикатор
                 * Данная функция отличается от update тем, что не влияет на внутреннее состояние индикатора
                 * \param in сигнал на входе
                 * \param out сигнал на выходе
                 * \return вернет 0 в случае успеха, иначе см. ErrorType
                 */
                int test(T in, T &out)
                {
                        if(!is_init_) {
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
                        is_init_ = false;
                }
        };
//------------------------------------------------------------------------------
        /** \brief Линии Боллинджера
         */
        template <typename T>
        class BollingerBands
        {
        private:
                std::vector<T> data_;
                int period_ = 0;
                T d_;
        public:

                /** \brief Инициализация линий Боллинджера
                 * \param period период  индикатора
                 * \param d множитель стандартного отклонения
                 */
                BollingerBands(int period, T d)
                {
                        period_ = period;
                        d_ = d;
                }

                /** \brief Получить новые данные индикатора
                 * \param in сигнал на входе
                 * \param out массив на выходе
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

                /** \brief Протестировать индикатор
                 * Данная функция отличается от update тем, что не влияет на внутреннее состояние индикатора
                 * \param in сигнал на входе
                 * \param out сигнал на выходе
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
