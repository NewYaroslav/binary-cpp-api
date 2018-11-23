/*
* binary-cpp-api - Binary C++ API client
*
* Copyright (c) 2018 Yaroslav Barabanov. Email: elektroyar@yandex.ru
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
#ifndef BINARY_API_HPP_INCLUDED
#define BINARY_API_HPP_INCLUDED
//------------------------------------------------------------------------------
#include <client_wss.hpp>
#include <nlohmann/json.hpp>
#include <thread>
#include <string>
#include <vector>
#include <queue>
#include <chrono>
#include <iostream>
//------------------------------------------------------------------------------
class BinaryApi
{
public:
        using json = nlohmann::json;
        using WssClient = SimpleWeb::SocketClient<SimpleWeb::WSS>;

        enum ErrorType {
                OK = 0,
                NO_AUTHORIZATION = -1,
                NO_COMMAND = -2,
                UNKNOWN_ERROR = -3,
                NO_INIT = -4,
                NO_OPEN_CONNECTION = -5,
        };
private:
        WssClient client_; // Класс клиента
        std::shared_ptr<WssClient::Connection> save_connection_; // Соединение
        std::recursive_mutex connection_lock_;
        bool is_open_connection_ = false; // состояние соединения
        std::queue<std::string> send_queue_; // Очередь сообщений
        bool is_add_delay = false;

        std::string token_ = "";
        bool is_error_token_ = false;

        // параметры счета
        double balance_ = 0; // Баланс счета
        std::string currency_ = ""; // Валюта счета
        bool is_authorize_ = false;
        bool is_repeat_authorize_ = false;
        std::recursive_mutex balance_lock_;
        // поток выплат и котировок
        std::vector<std::string> symbols_;
        std::unordered_map<std::string, int> map_symbol_;
        std::vector<double> proposal_buy_;
        std::vector<double> proposal_sell_;
        std::vector<std::vector<double>> close_data_;
        std::vector<std::vector<unsigned long long>> time_data_;
        bool is_stream_quotations = false;
        unsigned long long last_time_ = 0;
        bool is_last_time = false;

        std::recursive_mutex proposal_lock_;
        std::recursive_mutex quotations_lock_;
        std::recursive_mutex time_lock_;

        std::recursive_mutex send_queue_lock_;

        std::recursive_mutex array_candles_lock_;
        json array_candles;
        bool is_array_candles = false;
        bool is_array_candles_error = false;
        bool is_send_array_candles = false;
//------------------------------------------------------------------------------
        int send_json_with_authorize(json &j)
        {
                std::unique_lock<std::recursive_mutex> locker(balance_lock_);
                if(is_authorize_) {
                        std::string message = j.dump();
                        std::unique_lock<std::recursive_mutex> locker(send_queue_lock_);
                        send_queue_.push(message);
                        return OK;
                }
                return NO_AUTHORIZATION;
        }
//------------------------------------------------------------------------------
        int send_json(json &j)
        {
                std::unique_lock<std::recursive_mutex> locker(connection_lock_);
                if(is_open_connection_) {
                        std::string message = j.dump();
                        std::unique_lock<std::recursive_mutex> locker(send_queue_lock_);
                        send_queue_.push(message);
                        return OK;
                }
                return NO_OPEN_CONNECTION;
        }
//------------------------------------------------------------------------------
        bool check_time_message(json &j)
        {
                if(j["msg_type"] == "time") {
                        if(j["error"] != nullptr) {
                                // попробуем еще раз
                                std::string message = j["echo_req"].dump();
                                std::unique_lock<std::recursive_mutex> locker_send(send_queue_lock_);
                                if(j["error"]["code"] == "RateLimit")
                                        is_add_delay = true;
                                send_queue_.push(message);
                        } else {
                                std::unique_lock<std::recursive_mutex> locker(time_lock_);
                                last_time_ = j["time"];
                                is_last_time = true;
                        }
                        return true;
                } else {
                        return false;
                }
        }
//------------------------------------------------------------------------------
        bool check_tick_message(json &j)
        {
                if(j["msg_type"] == "tick") {
                        if(j["error"] != nullptr) {
                                if(j["error"]["code"] != "AlreadySubscribed") {
                                        // попробуем еще раз
                                        std::string message = j["echo_req"].dump();
                                        std::unique_lock<std::recursive_mutex> locker_send(send_queue_lock_);
                                        if(j["error"]["code"] == "RateLimit")
                                                is_add_delay = true;
                                        send_queue_.push(message);
                                }
                        } else {
                                if(j["tick"]["quote"] != nullptr) {
                                        double quote = atof((j["tick"]["quote"].get<std::string>()).c_str());              // котировка
                                        unsigned long long epoch = atoi((j["tick"]["epoch"].get<std::string>()).c_str());  // время
                                        std::string symbol = j["tick"]["symbol"];                                          // символ
                                        unsigned long long lastepoch = (epoch/60)*60;                                               // время послденей закрытой свечи

                                        std::unique_lock<std::recursive_mutex> locker_send(quotations_lock_);
                                        if(map_symbol_.find(symbol) == map_symbol_.end())
                                                return true;

                                        int indx = map_symbol_[symbol];
                                        if(indx >= 0 && indx < (int)close_data_.size()) {
                                                if(epoch % 60 == 0) {
                                                        close_data_[indx].push_back(quote);
                                                        time_data_[indx].push_back(epoch);
                                                } else
                                                if(close_data_[indx].size() > 0) {
                                                        if(lastepoch > time_data_[indx].back()) {
                                                                close_data_[indx].push_back(quote);
                                                                time_data_[indx].push_back(lastepoch);
                                                        } else {
                                                                close_data_[indx][close_data_[indx].size() - 1] = quote;
                                                    }
                                                } else {
                                                        close_data_[indx].push_back(quote);
                                                        time_data_[indx].push_back(lastepoch);
                                                }
                                                std::unique_lock<std::recursive_mutex> locker(time_lock_);
                                                last_time_ = std::max(epoch, last_time_);
                                                is_last_time = true;
                                        } // if
                                }
                        }
                        return true;
                } else {
                        return false;
                }
        }
//------------------------------------------------------------------------------
        bool check_candles_message(json &j)
        {
                if(j["msg_type"] == "candles") {
                        std::unique_lock<std::recursive_mutex> locker(array_candles_lock_);
                        is_array_candles = true;
                        if(j["error"] != nullptr) {
                                if(j["error"]["code"] == "RateLimit") {
                                        // попробуем еще раз
                                        std::string message = j["echo_req"].dump();
                                        std::unique_lock<std::recursive_mutex> locker_send(send_queue_lock_);
                                        send_queue_.push(message);
                                        is_add_delay = true;
                                } else {
                                        is_array_candles_error = true;
                                }
                        } else {
                                auto it_candles = j.find("candles");
                                if(it_candles != j.end()) {
                                        array_candles = *it_candles;
                                }
                                is_array_candles_error = false;
                        }
                        return true;
                } else {
                        return false;
                }
        }
//------------------------------------------------------------------------------
        bool check_authorize_message(json &j)
        {
                // получили сообщение авторизации
                if(j["msg_type"] == "authorize") {
                        if(j["error"] != nullptr) {
                                std::unique_lock<std::recursive_mutex> locker(balance_lock_);
                                is_authorize_ = false;
                                // попробуем еще раз залогиниться
                                std::string message = j["echo_req"].dump();
                                std::unique_lock<std::recursive_mutex> locker_send(send_queue_lock_);
                                if(j["error"]["code"] == "RateLimit")
                                        is_add_delay = true;
                                else
                                if(j["error"]["code"] == "InvalidToken") {
                                        std::unique_lock<std::recursive_mutex> locker(connection_lock_);
                                        is_error_token_ = true;
                                        return true;
                                }
                                send_queue_.push(message);
                        } else {
                                std::unique_lock<std::recursive_mutex> locker(balance_lock_);
                                balance_ = atof((j["authorize"]["balance"].get<std::string>()).c_str());
                                currency_ = j["authorize"]["currency"];
                                is_authorize_ = true;
                        }
                        return true;
                } else {
                        return false;
                }
        }
//------------------------------------------------------------------------------
        bool check_proposal_message(json &j)
        {
                if(j["msg_type"] == "proposal" &&
                        j["echo_req"]["subscribe"] == 1) {
                        std::string _symbol = j["echo_req"]["symbol"];
                        std::unique_lock<std::recursive_mutex> locker(proposal_lock_);
                        if(map_symbol_.find(_symbol) != map_symbol_.end()) {
                                int indx = map_symbol_[_symbol];
                                double temp = 0.0;
                                if(j["error"] == nullptr) {
                                        double ask_price = atof((j["proposal"]["ask_price"].get<std::string>()).c_str());
                                        double payout = atof((j["proposal"]["payout"].get<std::string>()).c_str());
                                        temp = ask_price != 0 ? (payout/ask_price) - 1 : 0.0;
                                } else {
                                        // отправим сообщение о подписки на выплаты
                                        std::string message = j["echo_req"].dump();
                                        std::unique_lock<std::recursive_mutex> locker_send(send_queue_lock_);
                                        if(j["error"]["code"] == "RateLimit")
                                                is_add_delay = true;
                                        send_queue_.push(message);
                                }
                                if(j["echo_req"]["contract_type"] == "CALL") {
                                        proposal_buy_[indx] = temp;
                                } else
                                if(j["echo_req"]["contract_type"] == "PUT") {
                                        proposal_sell_[indx] = temp;
                                } // if
                        } //if map_proposal_symbol
                        return true;
                } else {
                        return false;
                }
        }
//------------------------------------------------------------------------------
        void parse_json(std::string& str)
        {
                try {
                        json j = json::parse(str);
                        // обрабатываем сообщение
                        if(check_tick_message(j))
                                return;
                        if(check_proposal_message(j))
                                return;
                        if(check_authorize_message(j))
                                return;
                        if(check_candles_message(j))
                                return;
                }
                catch(...) {
                    std::cout << "BinaryApi: on_message error! Message: " <<
                        str << std::endl;
                }
        }

public:
//------------------------------------------------------------------------------
        /** \brief Инициализировать класс
         * \param token Токен. Можно указать пустую строку, но тогда не все функции будут доступны
         * \param app_id ID API вашего приложения
         */
        BinaryApi(std::string token = "", std::string app_id = "1089")
                : client_("ws.binaryws.com/websockets/v3?l=en&app_id=" +
                        app_id, false) , token_(token)
        {
                client_.on_open =
                        [&](std::shared_ptr<WssClient::Connection> connection)
                {
                        std::cout << "BinaryApi: Opened connection: " <<
                                connection.get() << std::endl;
                        std::unique_lock<std::recursive_mutex> locker(connection_lock_);
                        /* сохраняем соединение,
                         * чтобы можно было отправлять сообщения
                         */
                        save_connection_ = connection;
                        json j;
                        if(token_ != "") {
                                j["authorize"] = token_;
                        } else {
                                j["ping"] = (int)1;
                        }
                        std::string message = j.dump();
                        std::cout << "BinaryApi: Sending message: \"" <<
                                message << "\"" << std::endl;
                        connection->send(message);

                        is_open_connection_ = true;
                };

                client_.on_message =
                        [&](std::shared_ptr<WssClient::Connection> connection,
                                std::shared_ptr<WssClient::InMessage> message)
                {
                        std::string text = message->string();
                        //std::cout << "message: " << text << std::endl;
                        parse_json(text);
                };

                client_.on_close = [&](std::shared_ptr<WssClient::Connection> /*connection*/,
                        int status,
                        const std::string & /*reason*/)
                {
                        std::unique_lock<std::recursive_mutex> locker(connection_lock_);
                        is_open_connection_ = false;
                        std::cout << "BinaryApi: Closed connection with status code " <<
                                status << std::endl;
                };

                client_.on_error = [&](std::shared_ptr<WssClient::Connection> /*connection*/,
                        const SimpleWeb::error_code &ec)
                {
                        std::unique_lock<std::recursive_mutex> locker(connection_lock_);
                        is_open_connection_ = false;
                        std::cout << "BinaryApi: Error: " <<
                                ec << ", error message: " << ec.message() << std::endl;
                };

                std::thread client_thread([&]() {
                        while(true) {
                                client_.start();
                                std::this_thread::sleep_for(std::chrono::seconds(5));
                        }
                });

                std::thread send_thread([&]() {
                        //std::chrono::duration<double> start;
                        std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::steady_clock::now();
                        std::chrono::time_point<std::chrono::steady_clock> stop = std::chrono::steady_clock::now();
                        while(true) {
                                std::unique_lock<std::recursive_mutex> locker(connection_lock_);
                                if(is_open_connection_) {
                                        std::unique_lock<std::recursive_mutex> locker(send_queue_lock_);
                                        // добавим задержку, если нужно
                                        if(is_add_delay) {
                                                std::this_thread::sleep_for(std::chrono::seconds(1));
                                                is_add_delay = false;
                                        }
                                        // отправим сообщение, если есть что отправлять
                                        if(send_queue_.size() > 0) {
                                                std::string message = send_queue_.front();
                                                send_queue_.pop();
                                                //std::cout << "BinaryApi: send " <<
                                                //        message << std::endl;
                                                save_connection_->send(message);
                                                start = std::chrono::steady_clock::now();
                                        } else {
                                                stop = std::chrono::steady_clock::now();
                                                std::chrono::duration<double> diff = std::chrono::duration_cast<std::chrono::seconds>(stop - start);
                                                const float PING_DELAY = 20.0f;
                                                if(diff.count() > PING_DELAY) {
                                                        json j;
                                                        j["ping"] = 1;
                                                        std::string message = j.dump();
                                                        send_queue_.push(message);
                                                        start = std::chrono::steady_clock::now();
                                                }
                                        }
                                } else {
                                        start = std::chrono::steady_clock::now();
                                }
                                std::this_thread::yield();
                        }
                });

                client_thread.detach();
                send_thread.detach();

                while(true) {
                        if(token_ != "") {
                                std::unique_lock<std::recursive_mutex> locker(balance_lock_);
                                if(is_authorize_)
                                        break;
                                std::unique_lock<std::recursive_mutex> locker2(connection_lock_);
                                if(is_error_token_)
                                        break;
                        } else {
                                std::unique_lock<std::recursive_mutex> locker(connection_lock_);
                                if(is_open_connection_)
                                        break;
                        }
                        std::this_thread::yield();
                }
        }
//------------------------------------------------------------------------------
        /** \brief Получить данные о размере депозита
         * \param balance Депозит
         * \return состояние ошибки (0 в случае успеха, иначе см. ErrorType)
         */
         inline int get_balance(double &balance)
         {
                std::unique_lock<std::recursive_mutex> locker(balance_lock_);
                if(is_authorize_) {
                        balance = balance_;
                        return OK;
                }
                return NO_AUTHORIZATION;
         }
//------------------------------------------------------------------------------
        /** \brief Получить данные о валюте депозита
         * \param currency Валюта депозита
         * \return состояние ошибки (0 в случае успеха, иначе см. ErrorType)
         */
         inline int get_currency(std::string &currency)
         {
                std::unique_lock<std::recursive_mutex> locker(balance_lock_);
                if(is_authorize_) {
                        currency = currency_;
                        return OK;
                }
                return NO_AUTHORIZATION;
         }
//------------------------------------------------------------------------------
        /** \brief Инициализировать поток баланса депозита
         * \return состояние ошибки (0 в случае успеха, иначе см. ErrorType)
         */
        int init_stream_balance()
        {
                json j;
                j["balance"] = 1;
                // отправлять сообщения при изменении баланса
                j["subscribe"] = 1;
                return send_json_with_authorize(j);
        }
//------------------------------------------------------------------------------
        /** \brief Остановить поток баланса депозита
         * \return состояние ошибки (0 в случае успеха, иначе см. ErrorType)
         */
        int stop_stream_balance()
        {
                json j;
                j["forget_all"] = "balance";
                return send_json_with_authorize(j);
        }
//------------------------------------------------------------------------------
        /** \brief Загрузить исторические данные минутных свечей
         * \param symbol Имя валютной пары
         * \param candles_size Количество свечей. Не имеет смысла ставить данный параметр больше 5000
         * \param startepoch Время начала получения тиков. Влияет, если тиков между startepoch и endepoch не больше 5000
         * \param endepoch Конечное время получения тиков. Если нужно получить последние тики, укажите 0
         * \return состояние ошибки (0 в случае успеха, иначе см. ErrorType)
         */
        int download_candles(std::string symbol,
                             int candles_size,
                             unsigned long long startepoch,
                             unsigned long long endepoch)
        {
                json j;
                j["ticks_history"] = symbol;
                if(endepoch == 0) j["end"] = "latest";
                else j["end"] = endepoch;
                if(startepoch == 0) {
                        j["start"] = 1;
                        j["count"] = candles_size;
                } else {
                        j["start"] = startepoch;
                }
                j["style"] = "candles";
                j["granularity"] = 60;
                std::unique_lock<std::recursive_mutex> locker(array_candles_lock_);
                array_candles.clear();
                is_array_candles_error = false;
                is_array_candles = false;
                is_send_array_candles = true;
                return send_json(j);
        }
//------------------------------------------------------------------------------
        /** \brief Получить исторические данные минутных свечей
         * \param close Цены закрытия свечей
         * \param times Временные метки открытия свечей
         * \return состояние ошибки (0 в случае успеха, иначе см. ErrorType)
         */
        int get_candles(std::vector<double> &close,
                        std::vector<unsigned long long> &times)
        {
                if(!is_send_array_candles)
                        return NO_COMMAND;
                is_send_array_candles = false;
                std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::steady_clock::now();
                std::chrono::time_point<std::chrono::steady_clock> stop = std::chrono::steady_clock::now();
                while(true) {
                        std::this_thread::yield();
                        std::unique_lock<std::recursive_mutex> locker(array_candles_lock_);
                        // получили историю
                        if(is_array_candles)
                                break;
                        stop = std::chrono::steady_clock::now();
                        std::chrono::duration<double> diff = std::chrono::duration_cast<std::chrono::seconds>(stop - start);
                        const float MAX_DELAY = 5.0f;
                        // слишком долго ждем историю
                        if(diff.count() > MAX_DELAY) {
                                return UNKNOWN_ERROR;
                        }
                }
                std::unique_lock<std::recursive_mutex> locker(array_candles_lock_);
                // если была ошибка получения котировок
                if(is_array_candles_error) {
                        return UNKNOWN_ERROR;
                } else {
                        // преобразуем данные
                        close.resize(array_candles.size());
                        times.resize(array_candles.size());
                        for(size_t i = 0; i < array_candles.size(); i++) {
                                json j = array_candles[i];
                                close[i] = atof((j["close"].get<std::string>()).c_str());
                                std::string timestr = j["epoch"].dump();
                                times[i] = atoi(timestr.c_str());
                        }
                        return OK;
                }
        }
//------------------------------------------------------------------------------
        /** \brief Получить исторические данные минутных свечей
         * \param symbol Имя валютной пары
         * \param candles_size Количество свечей. Не имеет смысла ставить данный параметр больше 5000
         * \param close Цены закрытия свечей
         * \param times Временные метки открытия свечей
         * \param startepoch Время начала получения тиков. Влияет, если тиков между startepoch и endepoch не больше 5000
         * \param endepoch Конечное время получения тиков. Если нужно получить последние тики, укажите 0
         * \return состояние ошибки (0 в случае успеха, иначе см. ErrorType)
         */
        int get_candles(std::string symbol,
                        int candles_size,
                        std::vector<double> &close,
                        std::vector<unsigned long long> &times,
                        unsigned long long startepoch,
                        unsigned long long endepoch)
        {
                int err_data = download_candles(symbol, candles_size, startepoch, endepoch);
                if(err_data != OK)
                        return err_data;
                return get_candles(close, times);
        }
//------------------------------------------------------------------------------
        /** \brief Инициализировать список валютных пар
         * \param symbols Список валютных пар для получения котировок и процентов выплат
         */
        inline void init_symbols(std::vector<std::string> &symbols)
        {
                std::unique_lock<std::recursive_mutex> locker(proposal_lock_);
                std::unique_lock<std::recursive_mutex> locker2(quotations_lock_);
                symbols_ = symbols;
                map_symbol_.clear();
                close_data_.clear();
                time_data_.clear();
                proposal_buy_.clear();
                proposal_sell_.clear();

                close_data_.resize(symbols.size());
                time_data_.resize(symbols.size());
                proposal_buy_.resize(symbols.size());
                proposal_sell_.resize(symbols.size());
                for(size_t i = 0; i < symbols.size(); ++i) {
                        map_symbol_[symbols[i]] = i;
                }
        }
//------------------------------------------------------------------------------
        /** \brief Инициализировать поток котировок
         * \param init_size Начальный размер массива с котировками (в минутах)
         * \return состояние ошибки (0 в случае успеха, иначе см. ErrorType)
         */
        int init_stream_quotations(int init_size)
        {
                if(symbols_.size() == 0)
                        return NO_INIT;
                for(size_t i = 0; i < symbols_.size(); ++i) {
                        std::unique_lock<std::recursive_mutex> locker(quotations_lock_);
                        int err_data = get_candles(symbols_[i], init_size, close_data_[i], time_data_[i], 0, 0);
                        if(err_data != OK)
                                return err_data;
                }
                json j;
                json j_array = json::array();
                for(size_t i = 0; i < symbols_.size(); ++i) {
                        j_array[i] = symbols_[i];
                }
                j["ticks"] = j_array;
                j["subscribe"] = 1;
                int err_data = send_json(j);
                if(err_data == OK)
                        is_stream_quotations = true;
                return err_data;
        }
//------------------------------------------------------------------------------
        /** \brief Остановить поток котировок
         * \return состояние ошибки (0 в случае успеха, иначе см. ErrorType)
         */
        int stop_stream_quotations()
        {
                json j;
                j["forget_all"] = "ticks";
                is_stream_quotations = false;
                return send_json(j);
        }
//------------------------------------------------------------------------------
        /** \brief Получить данные потока котировок
         * \param close_data массив цен закрытия минутных свечей
         * \param time_data массив временных меток цен открытия минутных свечей
         * \return состояние ошибки (0 в случае успеха, иначе см. ErrorType)
         */
        inline int get_stream_quotations(std::vector<std::vector<double>> &close_data,
                                  std::vector<std::vector<unsigned long long>> &time_data)
        {
                if(symbols_.size() == 0 || !is_stream_quotations)
                        return NO_INIT;
                std::unique_lock<std::recursive_mutex> locker_send(quotations_lock_);
                close_data = close_data_;
                time_data = time_data_;
                return OK;
        }
//------------------------------------------------------------------------------
        /** \brief Запрос на получение времери сервера
         * \return состояние ошибки (0 в случае успеха, иначе см. ErrorType)
         */
        int request_servertime()
        {
                json j;
                j["time"] = 1;
                return send_json(j);
        }
//------------------------------------------------------------------------------
        /** \brief Получить время сервера
         * Данная функция вернет время сервера после вызова функции request_servertime(), а также
         * если инициализирован поток котировок
         * \param timestamp Время сервера
         * \return состояние ошибки (0 в случае успеха, иначе см. ErrorType)
         */
        inline int get_servertime(unsigned long long &timestamp)
        {
                std::unique_lock<std::recursive_mutex> locker(time_lock_);

                if(is_last_time || is_stream_quotations) {
                        timestamp = last_time_;
                        is_last_time = false;
                        return OK;
                } else {
                        return NO_INIT;
                }
        }
};

#endif // BINARY_API_HPP_INCLUDED
