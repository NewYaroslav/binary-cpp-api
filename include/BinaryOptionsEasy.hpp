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
#ifndef BINARYOPTIONSEASY_HPP_INCLUDED
#define BINARYOPTIONSEASY_HPP_INCLUDED
//------------------------------------------------------------------------------
#include <queue>
#include <vector>
#include <cmath>
//------------------------------------------------------------------------------
namespace BinaryOptionsEasy
{
//------------------------------------------------------------------------------
        // варианты ошибок API
        enum ErrorType {
                OK = 0,
                NO_AUTHORIZATION = -1,
                NO_COMMAND = -2,
                UNKNOWN_ERROR = -3,
                NO_INIT = -4,
                INVALID_PARAMETER = -6,
                DATA_NOT_AVAILABLE = -7,
        };
//------------------------------------------------------------------------------
        // типы контрактов
        enum ContractType {
                BUY = 1,
                SELL = -1,
                UP = 1,
                DN = -1,
        };
//------------------------------------------------------------------------------
        // состояния опционов
        enum OptionStatus {
                WIN = 1,
                LOSS = -1,
        };
//------------------------------------------------------------------------------
        /** \brief Посчитать математическое ожидание прибыли
         * \param eff эффективность стратегии (от 0 до 1.0)
         * \param profit выплата брокера в случае успеха (обычно от 0 до 1.0, но можно больше 1.0)
         * \param loss потери в случае поражения (обычно всегда 1.0)
         * \return математическое ожидание прибыли
         */
        double calc_math_expectation_profit(double eff, double profit, double loss = 1.0)
        {
                if(eff > 1.0 || eff < 0.0) return 0.0;
                return (eff * profit) - ((1.0 - eff) * loss);
        }
//------------------------------------------------------------------------------
        /** \brief Посчитать минимальную эффективность стратегии при заданном уровне выплат
         * \param profit выплата брокера в случае успеха (обычно от 0 до 1.0, но можно больше 1.0)
         * \param loss потери в случае поражения (обычно всегда 1.0)
         * \return минимальная эффективность стратегии
         */
        double calc_min_strategy_eff(double profit, double loss = 1.0)
        {
                // profit * eff - (1.0 - eff) *  loss
                return loss/(profit + loss);
        }
//------------------------------------------------------------------------------
        /** \brief Посчитать оптимальный процент ставки по критерию Келли
         * Если прибыль невозможна из-за низкого математического ожидания, вернет 0
         * \param eff эффективность стратегии (от 0 до 1.0)
         * \param profit выплата брокера в случае успеха (обычно от 0 до 1.0, но можно больше 1.0)
         * \param attenuation коэффициент ослабления (рекомендуемое значение по умолчанию 0.4)
         * \return процент ставки (от 0 до 1.0)
         */
        double calc_deposit_rate_kelly_criterion(double eff, double profit, double attenuation = 0.4)
        {
                if(eff <= calc_min_strategy_eff(profit))
                        return 0.0;
                return attenuation * (((profit + 1.0) * eff - 1.0) / profit);
        }
//------------------------------------------------------------------------------
        /** \brief Посчитать стабильность прибыли
         * Данный параметр тем лучше, чем кривая депозита ближе к экспоненте (для стратегии ставок по критерию Келли)
         * Первый элемент массива должен быть начальным уровнем депозита (депозит до первой сделки)
         * \param array_depo массив депозита
         * \return значение консистенции
         */
        template<typename T>
        double calc_profit_stability(std::vector<T> &array_depo)
        {
                if(array_depo.size() == 0)
                        return  0.0;
                double start_depo = std::log(array_depo.front());
                double stop_depo = std::log(array_depo.back());
                double delta = (double)(stop_depo - start_depo) / (double)(array_depo.size() - 1);
                double sum = 0;
                for(size_t i = 1; i < array_depo.size(); ++i) {
                        double y = start_depo + delta * (double)i;
                        double diff = std::log(array_depo[i]) - y;
                        sum += diff * diff;
                }
                sum /= (double)(array_depo.size() - 1);
                return sum;
        }
//------------------------------------------------------------------------------
        /** \brief Класс для хранения данных одного бинарного опциона
         */
        class OptionData
        {
//------------------------------------------------------------------------------
        public:
                double profit = 0;                      /**< Процент выплаты для удачной сделки*/
                double start_price = 0;                 /**< Начальная цена */
                int direction = 0;                      /**< Направление сделки */
                int duration = 0;                       /**< Длительность опциона */
                int status = 0;                         /**< Состояние сделки */
                unsigned long long timestamp = 0;       /**< Временная метка начала сделки */
//------------------------------------------------------------------------------
                OptionData() {}
//------------------------------------------------------------------------------
                /** \brief Инициализировать класс
                 * \param _price цена входа в сделку
                 * \param _profit процент выплаты в слуучае успеха
                 * \param _timestamp временная метка начала опциона
                 * \param _duration длительность опциона (секунды)
                 * \param _direction направление опциона (BUY или SELL)
                 */
                OptionData(double _price, double _profit, unsigned long long _timestamp, int _duration, int _direction)
                {
                        start_price = _price;
                        profit = _profit;
                        timestamp = _timestamp;
                        direction = _direction;
                        duration = _duration;
                }
//------------------------------------------------------------------------------
                /** \brief Инициализировать класс
                 * \param _price цена входа в сделку
                 * \param _profit процент выплаты в слуучае успеха
                 * \param _timestamp временная метка начала опциона
                 * \param _duration длительность опциона (секунды)
                 * \param _direction направление опциона (BUY или SELL)
                 * \param _status состояние сделки(WIN или LOSS)
                 */
                OptionData(double _price, double _profit, unsigned long long _timestamp, int _duration, int _direction, int _status)
                {
                        start_price = _price;
                        profit = _profit;
                        timestamp = _timestamp;
                        direction = _direction;
                        duration = _duration;
                        status = _status;
                }
//------------------------------------------------------------------------------
                /** \brief Инициализировать класс для теста без цены
                 * \param _profit процент выплаты в слуучае успеха
                 * \param _timestamp временная метка начала опциона
                 * \param _direction направление опциона (BUY или SELL)
                 * \param _status состояние сделки(WIN или LOSS)
                 */
                OptionData(double _profit, unsigned long long _timestamp, int _status)
                {
                        start_price = 0;
                        profit = _profit;
                        timestamp = _timestamp;
                        direction = 0;
                        duration = 1;
                        status = _status;
                }
//------------------------------------------------------------------------------
                /** \brief Получить временную метку конца сделки
                 * \return временная метка
                 */
                unsigned long long get_end_timestamp()
                {
                        return timestamp + duration;
                }
        };
//------------------------------------------------------------------------------
        /** \brief Класс для хранения параметров тестирования
         */
        class TestDataIn
        {
        public:
                double start_depo = 1000.0;                     /**< Начальный депозит */
                double min_amount = 0.0;                        /**< Минимальный размер ставки */
                double deposit_accuracy = 100.0;                /**< Данный параметр определяет число знаков после запятой для депозита */
                double average_strategy_eff = 0.5;              /**< Средняя эффективность стратегии */
                double kelly_criterion_attenuation = 0.2;       /**< Коэффициент ослабления для критерия Келии */
                double martingale_coeff = 2.5;                  /**< Коэффициент мартингейла */
                double martingale_stake = 0.01;                 /**< Начальная ставка по стратегии Мартингейла */
                int martingale_max_step = 7;                    /**< Максимальное число ступеней Мартингейла */
                unsigned long long timestamp_beg = 0;           /**< Временная метка начала теста */
                unsigned long long timestamp_end = 0;           /**< Временная метка конца теста */

                TestDataIn() {};

                /** \brief Инициализировать класс для хранения параметров тестирования
                 * \param _start_depo Начальный депозит
                 * \param _min_amount Минимальный размер ставки
                 * \param _average_strategy_eff Средняя эффективность стратегии
                 * \param _kelly_criterion_attenuation Коэффициент ослабления для критерия Келии (по умолчанию 0.2)
                 * \param _timestamp_beg Временная метка начала теста (0 если не задействована)
                 * \param _timestamp_end Временная метка конца теста (0 если не задействована)
                 */
                TestDataIn(
                        double _start_depo,
                        double _min_amount,
                        double _average_strategy_eff,
                        double _kelly_criterion_attenuation = 0.2,
                        unsigned long long _timestamp_beg = 0,
                        unsigned long long _timestamp_end = 0)
                {
                        start_depo = _start_depo;
                        min_amount = _min_amount;
                        average_strategy_eff = _average_strategy_eff;
                        kelly_criterion_attenuation = _kelly_criterion_attenuation;
                        timestamp_beg = _timestamp_beg;
                        timestamp_end = _timestamp_end;
                }

                /** \brief Инициализировать мартингейл
                 * \param _martingale_coeff коэффициент мартингейла
                 * \param _martingale_stake начальная ставка мартингейла
                 * \param _martingale_max_step максимальное число ступеней мартингейла
                 */
                void init_martingale(double _martingale_coeff, double _martingale_stake, int _martingale_max_step)
                {
                        martingale_coeff = _martingale_coeff;
                        martingale_stake = _martingale_stake;
                        martingale_max_step = _martingale_max_step;
                }
        };
//------------------------------------------------------------------------------
        class TestDataOut
        {
        public:
                double depo = 0.0;                      /**< Депозит */
                double eff = 0.0;                       /**< Эффективность стратгеии на тесте */
                double gain = 0.0;                      /**< Усиление */
                int num_wins = 0;                       /**< Количество удачрых сделок */
                int num_losses = 0;                     /**< Количество неудачных сделок */
                int num_deals = 0;                      /**< Количество сделок */
                std::vector<float> array_depo;         /**< Массив изменения депозита */
                TestDataOut() {};

                /** \brief Очистить данные
                 */
                void clear()
                {
                        array_depo.clear();
                        depo = 0.0;
                        eff = 0.0;
                        gain = 0.0;
                        num_wins = 0;
                        num_losses = 0;
                        num_deals = 0;
                }
        };
//------------------------------------------------------------------------------
        /** \brief Класс для проверки торговли бинарными опционами
         */
        class TradingSimulator
        {
        private:
                std::vector<OptionData> deals;          /**< Массив сделок */
                bool is_use_same_price = false;         /**< Использовать совпадение цен как услвоие удачной сделки */
        public:
//------------------------------------------------------------------------------
                TradingSimulator()
                {

                }
//------------------------------------------------------------------------------
                /** \brief Обновить состояние симулятора торговли
                 * \param price цена
                 * \param timestamp временная метка
                 */
                void update(double price, unsigned long long timestamp)
                {
                        // делаем поиск в массивах задом наперед
                        for(size_t i = deals.size(); i >= 0; --i) {
                                // если временная метка конца сделки совпадает и сделка не была обработана
                                if(deals[i].status == 0 && deals[i].get_end_timestamp() == timestamp) {
                                        if(deals[i].direction == BUY) {
                                                if(price > deals[i].start_price) {
                                                        deals[i].status = WIN;
                                                } else
                                                if(is_use_same_price && price == deals[i].start_price) {
                                                        deals[i].status = WIN;
                                                } else {
                                                        deals[i].status = LOSS;
                                                }
                                        } else
                                        if(deals[i].direction == SELL) {
                                                if(price < deals[i].start_price) {
                                                        deals[i].status = WIN;
                                                } else
                                                if(is_use_same_price && price == deals[i].start_price) {
                                                        deals[i].status = WIN;
                                                } else {
                                                        deals[i].status = LOSS;
                                                }
                                        } // if
                                } // if
                        } // for i
                }
//------------------------------------------------------------------------------
                /** \brief Открыть сделку
                 * Важно соблюдать последовательность открытия сделок, чтобы временная метка монотонно возрастала.
                 * \param price цена входа в сделку
                 * \param profit процент выплаты в случае успеха (обычно от 0 до 1 в случе 100%)
                 * \param timestamp временная метка начала опциона
                 * \param duration длительность опциона (секунды)
                 * \param direction направление опциона (BUY или SELL)
                 * \return вернет 0 в случае успеха
                 */
                int open_deal(double price, double profit, unsigned long long timestamp, int duration, int direction)
                {
                        if(deals.size() > 0) {
                                if(deals[deals.size() - 1].timestamp > timestamp) {
                                        return INVALID_PARAMETER;
                                }
                        }
                        deals.push_back(OptionData(price, profit, timestamp, duration, direction));
                        return OK;
                }
//------------------------------------------------------------------------------
                int open_virtual_deal(double profit, unsigned long long index, int status)
                {
                        if(deals.size() > 0) {
                                if(deals[deals.size() - 1].timestamp > index) {
                                        return INVALID_PARAMETER;
                                }
                        }

                        deals.push_back(OptionData(profit, index, status));
                        return OK;
                }
//------------------------------------------------------------------------------
                /** \brief Посчитать эффективность торговли
                 * \param eff эффективность протестированной стратегии (соотношение удачных сделок к общему числу сделок)
                 * \param timestamp_beg временная метка начала теста, указать 0 если не используется
                 * \param timestamp_end временная метка конца теста, указать 0 если не используется
                 * \return вернет 0 в случае успеха
                 */
                int calc_eff(double &eff, unsigned long long timestamp_beg = 0, unsigned long long timestamp_end = 0)
                {
                        int sum = 0;
                        int num_sum = 0;
                        for(size_t i = 0; i < deals.size(); ++i) {
                                if(timestamp_beg > 0 && deals[i].timestamp < timestamp_beg)
                                        continue;
                                if(timestamp_end > 0 && deals[i].timestamp > timestamp_end)
                                        continue;
                                if(deals[i].status == WIN) {
                                        sum += 1;
                                        num_sum++;
                                } else
                                if(deals[i].status == LOSS) {
                                        num_sum++;
                                }
                        }
                        if(num_sum == 0) {
                                eff = 0;
                                return DATA_NOT_AVAILABLE;
                        }
                        eff = (double)sum /(double)num_sum;
                        return OK;
                }
//------------------------------------------------------------------------------
                /** \brief Протестировать с использованием критерия Келли
                 * Данный вид теста всегда ставит оптимальный процент от депозита
                 * \param in параметры тестирования
                 * \param out результат тестирования
                 * \return вернет 0 в случае успеха
                 */
                int test_kelly_criterion(TestDataIn in, TestDataOut &out)
                {
                        out.clear();
                        out.array_depo.reserve(deals.size() + 1);
                        out.array_depo.push_back(in.start_depo);
                        out.depo = in.start_depo;
                        std::vector<double> payout_win;
                        std::vector<unsigned long long> timestamp_win;
                        std::vector<unsigned long long> timestamp_loss;
                        for(size_t i = 0; i < deals.size(); ++i) {
                                // проверяем наличие выплаты
                                bool is_update_depo = false;
                                for(size_t j = 0; j < timestamp_win.size(); ++j) {
                                        // проверем, что сделка завершилась
                                        if(deals[i].timestamp >= timestamp_win[j]) {
                                                out.depo += payout_win[j];
                                                out.num_wins++;
                                                payout_win.erase(payout_win.begin() + j);
                                                timestamp_win.erase(timestamp_win.begin() + j);
                                                is_update_depo = true;
                                        }
                                }
                                for(size_t j = 0; j < timestamp_loss.size(); ++j) {
                                        // проверем, что сделка завершилась
                                        if(deals[i].timestamp >= timestamp_loss[j]) {
                                                out.num_losses++;
                                                timestamp_loss.erase(timestamp_loss.begin() + j);
                                                is_update_depo = true;
                                        }
                                }
                                if(is_update_depo) {
                                        out.array_depo.push_back(out.depo);
                                }
                                if(out.depo <= in.min_amount) {
                                        break;
                                }
                                if(in.timestamp_beg > 0 && deals[i].timestamp < in.timestamp_beg)
                                        continue;
                                if(in.timestamp_end > 0 && deals[i].timestamp > in.timestamp_end)
                                        continue;
                                // посчитаем размер ставки
                                double amount = std::floor(out.depo *
                                        calc_deposit_rate_kelly_criterion(
                                                in.average_strategy_eff,
                                                deals[i].profit,
                                                in.kelly_criterion_attenuation) *
                                        in.deposit_accuracy) / in.deposit_accuracy;

                                // проверяем, вдруг процент выплат был слишком низкий
                                if(amount > 0) {
                                        if(amount < in.min_amount) {
                                                // если ставка меньше минимальной, приравняем ее минимальной
                                                amount = in.min_amount;
                                        } // if
                                        if(amount > out.depo) {
                                                amount = out.depo;
                                        }
                                        if(deals[i].status == WIN) {
                                                out.depo -= amount;
                                                // округляем выплату до 2 знаков после
                                                double payout = std::floor((amount * (1.0 + deals[i].profit)) * in.deposit_accuracy) / in.deposit_accuracy;
                                                // запоминаем выплату и момент времени, когда она произойдет
                                                payout_win.push_back(payout);
                                                timestamp_win.push_back(deals[i].get_end_timestamp());
                                        } else
                                        if(deals[i].status == LOSS) {
                                                out.depo -= amount;
                                                timestamp_loss.push_back(deals[i].get_end_timestamp());
                                        } // if
                                } // if
                        } // for
                        // если остались необработанные сделки, обработаем их
                        if((timestamp_win.size() > 0 || timestamp_loss.size() > 0) && out.depo >= in.min_amount) {
                                for(size_t j = 0; j < timestamp_win.size(); ++j) {
                                        out.depo += payout_win[j];
                                        out.num_wins++;
                                        payout_win.erase(payout_win.begin() + j);
                                        timestamp_win.erase(timestamp_win.begin() + j);
                                }
                                for(size_t j = 0; j < timestamp_loss.size(); ++j) {
                                        out.num_losses++;
                                        timestamp_loss.erase(timestamp_loss.begin() + j);
                                }
                                out.array_depo.push_back(out.depo);
                        } // if
                        out.num_deals = out.num_wins + out.num_losses;
                        if(out.num_deals == 0) {
                                out.eff = 0.0;
                                return DATA_NOT_AVAILABLE;
                        }
                        out.num_deals = out.num_wins + out.num_losses;
                        out.eff = (double)out.num_wins/(double)out.num_deals;
                        out.gain = out.array_depo.back()/out.array_depo.front();

                        return OK;
                }
//------------------------------------------------------------------------------
                /** \brief Протестировать с использованием мартингейла
                 * Данный тестр стратегий не учитывает закономерности между сделками, такие как длина экспирации опциона
                 * \param in параметры тестирования
                 * \param out результат тестирования
                 * \return вернет 0 в случае успеха
                 */
                int test_martingale(TestDataIn in, TestDataOut &out)
                {
                        out.clear();
                        out.array_depo.reserve(deals.size() + 1);
                        out.array_depo.push_back(in.start_depo);
                        out.depo = in.start_depo;
                        int martingale_step = 1;
                        double amount = std::floor(
                                out.depo *
                                in.martingale_stake *
                                in.deposit_accuracy) / in.deposit_accuracy;
                        double last_amount = amount;
                        for(size_t i = 0; i < deals.size(); ++i) {
                                if(out.depo < in.min_amount) {
                                        break;
                                }
                                if(deals[i].status == WIN) {
                                        double payout = std::floor(amount * deals[i].profit * in.deposit_accuracy) / in.deposit_accuracy;
                                        out.depo += payout;
                                        out.num_wins++;
                                        // посчитаем размер ставки
                                        amount = std::floor(
                                                out.depo *
                                                in.martingale_stake *
                                                in.deposit_accuracy) / in.deposit_accuracy;
                                        out.array_depo.push_back(out.depo);
                                        last_amount = amount;
                                } else
                                if(deals[i].status == LOSS) {
                                        out.depo -= amount;
                                        amount = std::floor(
                                                std::pow(in.martingale_coeff, martingale_step) *
                                                last_amount *
                                                in.deposit_accuracy) / in.deposit_accuracy;
                                        out.array_depo.push_back(out.depo);
                                        martingale_step++;
                                        out.num_losses++;
                                        if(martingale_step == in.martingale_max_step) {
                                                martingale_step = 1;
                                                amount = std::floor(
                                                        out.depo *
                                                        in.martingale_stake *
                                                        in.deposit_accuracy) / in.deposit_accuracy;
                                        }
                                } // if
                        } // for
                        out.num_deals = out.num_wins + out.num_losses;
                        if(out.num_deals == 0) {
                                out.eff = 0.0;
                                return DATA_NOT_AVAILABLE;
                        }
                        out.num_deals = out.num_wins + out.num_losses;
                        out.eff = (double)out.num_wins/(double)out.num_deals;
                        out.gain = out.array_depo.back()/out.array_depo.front();

                        return OK;
                }
//------------------------------------------------------------------------------
                /** \brief Протестировать с использованием системы Миллера
                 * \param in параметры тестирования
                 * \param out результат тестирования
                 * \return вернет 0 в случае успеха
                 */
                int test_miller_system(TestDataIn in, TestDataOut &out)
                {
                        out.clear();
                        out.array_depo.reserve(deals.size() + 1);
                        out.array_depo.push_back(in.start_depo);
                        out.depo = in.start_depo;

                        const double DEPOSIT_RATE = 0.03;
                        const double DEPOSIT_GAIN = 1.25;
                        double last_depo = out.depo;

                        double amount = std::floor(
                                out.depo *
                                DEPOSIT_RATE *
                                in.deposit_accuracy) / in.deposit_accuracy;

                        for(size_t i = 0; i < deals.size(); ++i) {
                                if(out.depo < in.min_amount) {
                                        break;
                                }
                                if(deals[i].status == WIN) {
                                        double payout = std::floor(amount * deals[i].profit * in.deposit_accuracy) / in.deposit_accuracy;
                                        out.depo += payout;
                                        out.num_wins++;
                                        if(out.depo >= last_depo * DEPOSIT_GAIN) {
                                                amount = std::floor(
                                                        out.depo *
                                                        DEPOSIT_RATE *
                                                        in.deposit_accuracy) / in.deposit_accuracy;
                                                last_depo = out.depo;
                                        }
                                        out.array_depo.push_back(out.depo);
                                } else
                                if(deals[i].status == LOSS) {
                                        out.depo -= amount;
                                        out.num_losses++;
                                        out.array_depo.push_back(out.depo);
                                } // if
                        } // for
                        out.num_deals = out.num_wins + out.num_losses;
                        if(out.num_deals == 0) {
                                out.eff = 0.0;
                                return DATA_NOT_AVAILABLE;
                        }
                        out.num_deals = out.num_wins + out.num_losses;
                        out.eff = (double)out.num_wins/(double)out.num_deals;
                        out.gain = out.array_depo.back()/out.array_depo.front();

                        return OK;
                }
//------------------------------------------------------------------------------
                /** \brief Очистить от сделок
                 */
                void clear()
                {
                        deals.clear();
                }
//------------------------------------------------------------------------------
        };
}
//------------------------------------------------------------------------------
#endif // BINARYOPTIONSEASY_HPP_INCLUDED
