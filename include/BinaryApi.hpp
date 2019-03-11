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
#ifndef BINARY_API_HPP_INCLUDED
#define BINARY_API_HPP_INCLUDED
//------------------------------------------------------------------------------
#include <client_wss.hpp>
#include <nlohmann/json.hpp>
#include <xtime.hpp>
#include <thread>
#include <string>
#include <vector>
#include <queue>
#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
//------------------------------------------------------------------------------
#define BINARY_API_USE_TICKS_HISTORY_SUBSCRIBE 1
#define BINARY_API_LOG_FILE_NAME "binary_api_log_errors.txt"
//------------------------------------------------------------------------------
class BinaryAPI
{
//------------------------------------------------------------------------------
public:
        using json = nlohmann::json;
        using WssClient = SimpleWeb::SocketClient<SimpleWeb::WSS>;
//------------------------------------------------------------------------------
        /// Набор возможных состояний ошибки
        enum ErrorType {
                OK = 0,                         ///< Ошибок нет, все в порядке
                NO_AUTHORIZATION = -1,          ///< Нет авторизации, поэтому метод класса не может быть использован
                NO_COMMAND = -2,                ///< Не было команды, поэтому метод класса не может быть использован
                UNKNOWN_ERROR = -3,             ///< Неизвестная ошибка
                NO_INIT = -4,                   ///< Не было инициализации, поэтому метод класса не может быть использован
                NO_OPEN_CONNECTION = -5,        ///< Соединение не установлено
                INVALID_PARAMETER = -6,         ///< Какой-то параметр в методе имеет недопустмое значение
                DATA_NOT_AVAILABLE = -7,        ///< Данные не доступны
        };
//------------------------------------------------------------------------------
        /// Типы контрактов
        enum ContractType {
                BUY = 1,                        ///< Ставка на повышение
                SELL = -1,                      ///< Ставка на понижение
        };
//------------------------------------------------------------------------------
        /// Длительности контракта
        enum DurationType {
                TICKS = 0,                      ///< Тики (интервал графика 1 секунда для валютных пар и 2 секунды для индексов волатильности)
                SECONDS = 1,                    ///< Секунды
                MINUTES = 2,                    ///< Минуты
                HOURS = 3,                      ///< Часы
                DAYS = 4,                       ///< Дни
        };
//------------------------------------------------------------------------------
private:
        WssClient client_; // Класс клиента
        std::shared_ptr<WssClient::Connection> save_connection_; // Соединение
        std::atomic<bool> is_open_connection_; // состояние соединения
        std::string token_;
        std::mutex token_mutex_;
        std::atomic<bool> is_error_token_;
        std::mutex connection_mutex_;

        std::queue<std::string> send_queue_; // Очередь сообщений
        std::mutex send_queue_mutex_;

        // параметры счета
        std::atomic<double> balance_; // Баланс счета
        std::string currency_ = ""; // Валюта счета
        std::atomic<bool> is_authorize_;
        std::mutex authorize_mutex_;


        // поток выплат и котировок
        std::vector<std::string> symbols_;
        std::unordered_map<std::string, int> map_symbol_;
        std::mutex map_symbol_mutex_;

        std::vector<double> proposal_buy_;
        std::vector<double> proposal_sell_;
        std::mutex proposal_mutex_;

        std::vector<std::vector<double>> close_data_;
        std::vector<std::vector<unsigned long long>> time_data_;
        std::mutex quotations_mutex_;

        std::atomic<bool> is_stream_quotations_;
        std::atomic<bool> is_stream_quotations_error_;
        std::atomic<bool> is_stream_proposal_;
        // время сервера
        std::atomic<unsigned long long> last_time_;
        std::atomic<bool> is_last_time_;
        // массив баров для истории
        std::mutex array_candles_mutex_;
        json array_candles_;
        std::atomic<bool> is_array_candles_;
        std::atomic<bool> is_array_candles_error_;
        std::atomic<bool> is_send_array_candles_;
        // массив тиков для истории
        std::mutex array_ticks_mutex_;
        json array_ticks_;
        std::atomic<bool> is_array_ticks_;
        std::atomic<bool> is_array_ticks_error_;
        std::atomic<bool> is_send_array_ticks_;

        std::atomic<bool> is_use_log;
        std::mutex file_log_mutex_;
//------------------------------------------------------------------------------
        std::string format(const char *fmt, ...)
        {
                va_list args;
                va_start(args, fmt);
                std::vector<char> v(1024);
                while (true)
                {
                        va_list args2;
                        va_copy(args2, args);
                        int res = vsnprintf(v.data(), v.size(), fmt, args2);
                        if ((res >= 0) && (res < static_cast<int>(v.size())))
                        {
                            va_end(args);
                            va_end(args2);
                            return std::string(v.data());
                        }
                        size_t size;
                        if (res < 0)
                            size = v.size() * 2;
                        else
                            size = static_cast<size_t>(res) + 1;
                        v.clear();
                        v.resize(size);
                        va_end(args2);
                }
        }
//------------------------------------------------------------------------------
        inline void send_message_thread_delay(std::string &message, const int delay)
        {
                std::thread([&, message, delay]{
                        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                        send_queue_mutex_.lock();
                        send_queue_.push(message);
                        send_queue_mutex_.unlock();
                }).detach();
        }
//------------------------------------------------------------------------------
        inline void send_message(std::string &message)
        {
                send_queue_mutex_.lock();
                send_queue_.push(message);
                send_queue_mutex_.unlock();
        }
//------------------------------------------------------------------------------
        int send_json_with_authorize(json &j)
        {
                if(is_authorize_) {
                        std::string message = j.dump();
                        send_queue_mutex_.lock();
                        send_queue_.push(message);
                        send_queue_mutex_.unlock();
                        return OK;
                }
                return NO_AUTHORIZATION;
        }
//------------------------------------------------------------------------------
        int send_json(json &j)
        {
                if(is_open_connection_) {
                        std::string message = j.dump();
                        send_queue_mutex_.lock();
                        send_queue_.push(message);
                        send_queue_mutex_.unlock();
                        return OK;
                }
                return NO_OPEN_CONNECTION;
        }
//------------------------------------------------------------------------------
        void write_log_file(std::string file_name, std::string message)
        {
                if(is_use_log) {
                        std::thread([&, file_name, message]{
                                file_log_mutex_.lock();
                                try {
                                        std::ofstream file(file_name, std::ios::app);
                                        if(file) {
                                                file << message << std::endl;
                                        }
                                        file.close();
                                }
                                catch(...) {

                                }
                                file_log_mutex_.unlock();
                        }).detach();
                }
        }
//------------------------------------------------------------------------------
        void write_log_file(std::string message)
        {
                if(!is_use_log)
                        return;
                std::string str_line = "//------------------------------------------------------------------------------\n";
                std::string error_message = str_line +
                        "start of error message (" +
                        xtime::get_str_unix_date_time() + "):\n" +
                        message + "\nend of error message\n" + str_line;
                write_log_file(BINARY_API_LOG_FILE_NAME, error_message);
        }
//------------------------------------------------------------------------------
        bool check_time_message(json &j,
                                json::iterator &it_msg_type,
                                json::iterator &it_error)
        {
                if(*it_msg_type == "time") {
                        if(it_error != j.end()) {
                                // попробуем еще раз
                                std::string message = j["echo_req"].dump();
                                if((*it_error)["code"] == "RateLimit") {
                                        // отправим сообщение повторно с задержкой
                                        send_message_thread_delay(message, 1000);
                                } else {
                                        send_message(message);
                                }
                        } else {
                                last_time_ = j["time"];
                                is_last_time_ = true;
                        }
                        return true;
                } else {
                        return false;
                }
        }
//------------------------------------------------------------------------------
        bool check_tick_message(json &j,
                                json::iterator &it_msg_type,
                                json::iterator &it_error)
        {
                if(*it_msg_type == "tick") {
                        if(it_error != j.end()) {
                                if((*it_error)["code"] != "AlreadySubscribed") {
                                        // попробуем еще раз
                                        std::string message = j["echo_req"].dump();
                                        if((*it_error)["code"] == "RateLimit" || (*it_error)["code"] ==  "MarketIsClosed") {
                                                send_message_thread_delay(message, 1000);
                                        } else {
                                                send_message(message);
                                        }
                                }
                        } else {
                                auto it_tick = j.find("tick");
                                const double quote = atof(((*it_tick)["quote"].get<std::string>()).c_str());              // котировка
                                const unsigned long long epoch = atoi(((*it_tick)["epoch"].get<std::string>()).c_str());  // время
                                const std::string symbol = (*it_tick)["symbol"];                                          // символ
                                const unsigned long long lastepoch = (epoch/60)*60;                                               // время послденей закрытой свечи

                                map_symbol_mutex_.lock();
                                auto it_symbol = map_symbol_.find(symbol);
                                if(it_symbol == map_symbol_.end()) {
                                        map_symbol_mutex_.unlock();
                                        return true;
                                }
                                const int indx = it_symbol->second;
                                map_symbol_mutex_.unlock();

                                quotations_mutex_.lock();
                                const size_t data_size = close_data_.size();
                                if(indx < (int)data_size) {
                                        if(epoch % 60 == 0) {
                                                close_data_.at(indx).push_back(quote);
                                                time_data_.at(indx).push_back(epoch);
                                        } else
                                        if(close_data_[indx].size() > 0) {
                                                if(lastepoch > time_data_.at(indx).back()) {
                                                        close_data_.at(indx).push_back(quote);
                                                        time_data_.at(indx).push_back(lastepoch);
                                                } else {
                                                        close_data_.at(indx).at(close_data_.at(indx).size() - 1) = quote;
                                                }
                                        } else {
                                                close_data_.at(indx).push_back(quote);
                                                time_data_.at(indx).push_back(lastepoch);
                                        }
                                        unsigned long long _last_time_ = last_time_;
                                        last_time_ = std::max(epoch, _last_time_);
                                        is_last_time_ = true;
                                } // if
                                quotations_mutex_.unlock();
                        }
                        return true;
                } else {
                        return false;
                }
        }
//------------------------------------------------------------------------------
        bool check_ohlc_message(json &j,
                                json::iterator &it_msg_type,
                                json::iterator &it_error)
        {
                if(*it_msg_type == "ohlc") {
                        if(it_error != j.end()) {
                                if((*it_error)["code"] != "AlreadySubscribed") {
                                        if((*it_error)["code"] == "RateLimit" || (*it_error)["code"] ==  "MarketIsClosed") {
                                                // попробуем еще раз
                                                std::string message = j["echo_req"].dump();
                                                send_message_thread_delay(message, 1000);
                                        } else {
                                                is_stream_quotations_error_ = true;
                                        }
                                }
                                return true;
                        } else {
                                auto it_ohlc = j.find("ohlc");
                                const unsigned long long open_time = (*it_ohlc)["open_time"];
                                const unsigned long long epoch = (*it_ohlc)["epoch"];
                                const double _close = atof(((*it_ohlc)["close"].get<std::string>()).c_str());
                                const std::string symbol = (*it_ohlc)["symbol"];
                                // находим номер валютной пары
                                map_symbol_mutex_.lock();
                                auto it_symbol = map_symbol_.find(symbol);
                                if(it_symbol == map_symbol_.end()) {
                                        map_symbol_mutex_.unlock();
                                        return true;
                                }
                                const int indx = it_symbol->second;
                                map_symbol_mutex_.unlock();

                                quotations_mutex_.lock();
                                const unsigned long long last_open_time = time_data_[indx].back();
                                const int data_size = close_data_[indx].size();
                                if(indx < data_size) {
                                        if(close_data_[indx].size() > 0) {
                                                if(last_open_time == open_time) {
                                                        close_data_[indx][data_size - 1] = _close;
                                                } else
                                                if(last_open_time < open_time) {
                                                        close_data_[indx].push_back(_close);
                                                        time_data_[indx].push_back(open_time);
                                                }
                                        } else {
                                                close_data_[indx].push_back(_close);
                                                time_data_[indx].push_back(open_time);
                                        }
                                }
                                quotations_mutex_.unlock();

                                const unsigned long long _last_time_ = last_time_;
                                last_time_ = std::max(epoch, _last_time_);
                                is_last_time_ = true;
                        }
                        return true;
                } else {
                        return false;
                }
        }
//------------------------------------------------------------------------------
        bool check_candles_message(json &j,
                                   json::iterator &it_msg_type,
                                   json::iterator &it_error)
        {
                if(*it_msg_type == "candles") {
                        if(it_error != j.end()) {
                                if((*it_error)["code"] != "AlreadySubscribed") {
                                        if((*it_error)["code"] == "RateLimit" || (*it_error)["code"] ==  "MarketIsClosed") {
                                                // попробуем еще раз
                                                std::string message = j["echo_req"].dump();
                                                send_message_thread_delay(message, 1000);
                                        } else {
                                                if(j["echo_req"]["subscribe"] != 1) {
                                                        is_array_candles_ = true;
                                                        is_array_candles_error_ = true;
                                                } else {
                                                        is_stream_quotations_error_ = true;
                                                }
                                        }
                                }
                                return true;
                        } else {
                                auto j_echo_req = j.find("echo_req");
                                if((*j_echo_req)["subscribe"] == 1) {
                                        std::string symbol = (*j_echo_req)["ticks_history"];                                          // символ
                                        // находим номер валютной пары
                                        map_symbol_mutex_.lock();
                                        auto it_symbol = map_symbol_.find(symbol);
                                        if(it_symbol == map_symbol_.end()) {
                                                map_symbol_mutex_.unlock();
                                                return true;
                                        }
                                        const int indx = it_symbol->second;
                                        map_symbol_mutex_.unlock();

                                        // инициализируем массив свечей
                                        auto it_candles = j.find("candles");
                                        json &j_candles = *it_candles;

                                        const size_t candles_num = j_candles.size();
                                        quotations_mutex_.lock();
                                        close_data_[indx].resize(candles_num);
                                        time_data_[indx].resize(candles_num);
                                        for(size_t i = 0; i < candles_num; i++) {
                                                json &_j = j_candles[i];
                                                close_data_[indx][i] = atof((_j["close"].get<std::string>()).c_str());
                                                std::string timestr = _j["epoch"].dump();
                                                time_data_[indx][i] = atoi(timestr.c_str());
                                        }
                                        quotations_mutex_.unlock();
                                        return true;
                                } else {
                                        auto it_candles = j.find("candles");

                                        array_candles_mutex_.lock();
                                        if(it_candles != j.end())
                                                array_candles_ = *it_candles;
                                        array_candles_mutex_.unlock();
                                        is_array_candles_ = true;
                                        is_array_candles_error_ = false;
                                }
                        }
                        return true;
                } else {
                        return false;
                }
        }
//------------------------------------------------------------------------------
        bool check_history_message(json &j,
                                   json::iterator &it_msg_type,
                                   json::iterator &it_error)
        {
                if(*it_msg_type == "history") {
                        if(it_error != j.end()) {
                                if((*it_error)["code"] != "AlreadySubscribed") {
                                        if((*it_error)["code"] == "RateLimit" || (*it_error)["code"] ==  "MarketIsClosed") {
                                                // попробуем еще раз
                                                std::string message = j["echo_req"].dump();
                                                send_message_thread_delay(message, 1000);
                                        } else {
                                                if(j["echo_req"]["subscribe"] != 1) {
                                                        is_array_ticks_ = true;
                                                        is_array_ticks_error_ = true;
                                                } else {
                                                        is_stream_quotations_error_ = true;
                                                }
                                        }
                                }
                                return true;
                        } else {
                                auto j_echo_req = j.find("echo_req");
                                if((*j_echo_req)["subscribe"] == 1) {
                                        // тиковый поток
                                        return true;
                                } else {
                                        auto it_history = j.find("history");

                                        array_ticks_mutex_.lock();
                                        if(it_history != j.end())
                                                array_ticks_ = *it_history;
                                        array_ticks_mutex_.unlock();
                                        is_array_ticks_ = true;
                                        is_array_ticks_error_ = false;
                                }
                        }
                        return true;
                } else {
                        return false;
                }
        }
//------------------------------------------------------------------------------
        bool check_authorize_message(json &j,
                                     json::iterator &it_msg_type,
                                     json::iterator &it_error)
        {
                // получили сообщение авторизации
                if(*it_msg_type == "authorize") {
                        if(it_error != j.end()) {
                                // попробуем еще раз залогиниться
                                std::string message = j["echo_req"].dump();
                                if((*it_error)["code"] == "RateLimit") {
                                        send_message_thread_delay(message, 1000);
                                } else {
                                        if((*it_error)["code"] == "InvalidToken") {
                                                is_error_token_ = true;
                                                return true;
                                        }
                                        send_message(message);
                                }
                                is_authorize_ = false;
                        } else {
                                balance_ = atof((j["authorize"]["balance"].get<std::string>()).c_str());
                                authorize_mutex_.lock();
                                currency_ = j["authorize"]["currency"];
                                authorize_mutex_.unlock();
                                is_authorize_ = true;
                                //a_mutex_.unlock();
                        }
                        return true;
                } else {
                        return false;
                }
        }
//------------------------------------------------------------------------------
        bool check_proposal_message(json &j,
                                    json::iterator &it_msg_type,
                                    json::iterator &it_error)
        {
                auto it_echo_req = j.find("echo_req");
                if(*it_msg_type == "proposal" &&
                        (*it_echo_req)["subscribe"] == 1) {
                        std::string _symbol = (*it_echo_req)["symbol"];
                        map_symbol_mutex_.lock();
                        auto it_symbol = map_symbol_.find(_symbol);
                        if(it_symbol != map_symbol_.end()) {
                                const int indx = it_symbol->second;
                                map_symbol_mutex_.unlock();
                                double temp = 0.0;
                                if(it_error == j.end()) {
                                        auto it_proposal = j.find("proposal");
                                        const double ask_price = atof(((*it_proposal)["ask_price"].get<std::string>()).c_str());
                                        const double payout = atof(((*it_proposal)["payout"].get<std::string>()).c_str());
                                        temp = ask_price != 0 ? (payout/ask_price) - 1 : 0.0;
                                } else {
                                        if((*it_error)["code"] == "AlreadySubscribed") {
                                                return true;
                                        }
                                        // отправим сообщение о подписки на выплаты
                                        std::string message = j["echo_req"].dump();
                                        if((*it_error)["code"] == "RateLimit" ||
                                          (*it_error)["code"] == "ContractBuyValidationError") {
                                                // отправляем сообщение с задержкой
                                                send_message_thread_delay(message, 2500);
                                        } else {
                                                // отправляем сообщение мгновенно
                                                send_message(message);
                                        }
                                }
                                //std::cout << "6" << std::endl;
                                auto it_contract_type = (*it_echo_req).find("contract_type");
                                if(*it_contract_type == "CALL") {
                                        proposal_mutex_.lock();
                                        proposal_buy_.at(indx) = temp;
                                        proposal_mutex_.unlock();
                                } else
                                if(*it_contract_type == "PUT") {
                                        proposal_mutex_.lock();
                                        proposal_sell_.at(indx) = temp;
                                        proposal_mutex_.unlock();
                                } // if
                        } else {
                                map_symbol_mutex_.unlock();
                        }
                        return true;
                } else {
                        return false;
                }
        }
//------------------------------------------------------------------------------
        void parse_json(std::string &str)
        {
                try {
                        json j = json::parse(str);
                        /* для ускорения заранее находим сообщения
                         * msg_type и error
                         */
                        json::iterator it_msg_type = j.find("msg_type");
                        json::iterator it_error = j.find("error");
                        if(it_error != j.end()) {
                                try {
                                        write_log_file(j.dump());
                                }
                                catch (...) {
                                        write_log_file("check_time_message->j.dump()");
                                }
                        }
                        // обрабатываем сообщение
                        if(check_ohlc_message(j, it_msg_type, it_error))
                                return;
                        if(check_tick_message(j, it_msg_type, it_error))
                                return;
                        if(check_proposal_message(j, it_msg_type, it_error))
                                return;
                        if(check_candles_message(j, it_msg_type, it_error))
                                return;
                        if(check_history_message(j, it_msg_type, it_error))
                                return;
                        if(check_time_message(j, it_msg_type, it_error))
                                return;
                        if(check_authorize_message(j, it_msg_type, it_error))
                                return;
                }
                catch(...) {
                        std::cout << "BinaryApi: on_message error! Message: " <<
                        str << std::endl;
                        write_log_file(str);
                }
        }

public:
//------------------------------------------------------------------------------
        /** \brief Инициализировать класс
         * \param token Токен. Можно указать пустую строку, но тогда не все функции будут доступны
         * \param app_id ID API вашего приложения
         */
        BinaryAPI(std::string token = "", std::string app_id = "1089")
                : client_("ws.binaryws.com/websockets/v3?l=en&app_id=" +
                        app_id, false) ,
                        is_open_connection_(false),
                        token_(token),
                        is_error_token_(false),
                        balance_(0),
                        is_authorize_(false),
                        is_stream_quotations_(false),
                        is_stream_quotations_error_(false),
                        is_stream_proposal_(false),
                        last_time_(0),
                        is_last_time_(false),
                        is_array_candles_(false),
                        is_array_candles_error_(false),
                        is_send_array_candles_(false),
                        is_array_ticks_(false),
                        is_array_ticks_error_(false),
                        is_send_array_ticks_(false),
                        is_use_log(false)
        {
                client_.on_open =
                        [&](std::shared_ptr<WssClient::Connection> connection)
                {
                        std::cout << "BinaryApi: Opened connection: " <<
                                connection.get() << std::endl;
                        /* сохраняем соединение,
                         * чтобы можно было отправлять сообщения
                         */
                        json j;
                        std::lock(connection_mutex_, token_mutex_);
                        save_connection_ = connection;
                        if(token_ != "") {
                                j["authorize"] = token_;
                        } else {
                                j["ping"] = (int)1;
                        }
                        token_mutex_.unlock();
                        std::string message = j.dump();
                        //std::cout << "BinaryApi: Sending message: \"" <<
                         //       message << "\"" << std::endl;

                        connection->send(message);
                        connection_mutex_.unlock();
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
                        is_open_connection_ = false;
                        is_authorize_ = false;
                        std::cout << "BinaryApi: Closed connection with status code " <<
                                status << std::endl;
                        write_log_file("BinaryApi: Closed connection with status code " + std::to_string(status));
                };

                client_.on_error = [&](std::shared_ptr<WssClient::Connection> /*connection*/,
                        const SimpleWeb::error_code &ec)
                {
                        is_open_connection_ = false;
                        is_authorize_ = false;
                        std::cout << "BinaryApi: Error: " <<
                                ec << ", error message: " << ec.message() << std::endl;
                        write_log_file("BinaryApi: Error, error message: " + ec.message());
                };

                std::thread client_thread([&]() {
                        while(true) {
                                std::cout << "BinaryApi: start" << std::endl;
                                try {
                                    client_.start();
                                }
                                catch(std::exception e) {
                                    write_log_file("BinaryApi: Error, error message: " + std::string(e.what()));
                                }
                                catch(...) {
                                    write_log_file("BinaryApi: Error, error");
                                }
                                std::cout << "BinaryApi: restart" << std::endl;
                                is_stream_quotations_ = false;
                                is_stream_proposal_ = false;
                                is_open_connection_ = false;
                                is_authorize_ = false;
                                is_last_time_ = false;
                                std::this_thread::sleep_for(std::chrono::seconds(5));
                        }
                });

                std::thread send_thread([&]() {
                        //std::chrono::duration<double> start;
                        std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::steady_clock::now();
                        std::chrono::time_point<std::chrono::steady_clock> stop = std::chrono::steady_clock::now();
                        std::chrono::time_point<std::chrono::steady_clock> start_min = std::chrono::steady_clock::now();
                        std::chrono::time_point<std::chrono::steady_clock> stop_min = std::chrono::steady_clock::now();
                        const float SECONDS_MINUTE = 60.0f;
                        const int max_num_requestes = 180; // максимальное число запросов в минуту
                        int num_requestes = 0; // число запросов в минуту
                        while(true) {
                                if(is_open_connection_) {
                                        // отправим сообщение, если есть что отправлять
                                        send_queue_mutex_.lock();
                                        if(send_queue_.size() > 0) {
                                                std::string message = send_queue_.front();
                                                send_queue_.pop();
                                                send_queue_mutex_.unlock();
                                                //std::cout << "BinaryApi: send " <<
                                                //        message << std::endl;
                                                while(!is_open_connection_) {
                                                    std::this_thread::yield();
                                                }
                                                connection_mutex_.lock();
                                                save_connection_->send(message);
                                                connection_mutex_.unlock();
                                                /* проверим ограничение запросов в минуту
                                                 */
                                                num_requestes++; // увеличим число запросов в минуту
                                                stop_min = std::chrono::steady_clock::now();
                                                //std::chrono::duration<double> diff;
                                                auto diff = std::chrono::duration_cast<std::chrono::seconds>(stop_min - start_min);
                                                // проверим количество запросов в минуту
                                                if(diff.count() < SECONDS_MINUTE) {
                                                        // запросов стало слишком много?
                                                        if(num_requestes >= max_num_requestes) {
                                                                // подождем, пока не пройдет минута
                                                                while(true) {
                                                                        stop_min = std::chrono::steady_clock::now();
                                                                        std::chrono::duration<double> diff;
                                                                        diff = std::chrono::duration_cast<std::chrono::seconds>(stop_min - start_min);
                                                                        if(diff.count() >= SECONDS_MINUTE) {
                                                                                // минута прошла, уходим
                                                                                start_min = std::chrono::steady_clock::now();
                                                                                stop_min = std::chrono::steady_clock::now();
                                                                                num_requestes = 0;
                                                                                break;
                                                                        }
                                                                        std::this_thread::yield();
                                                                } // while
                                                        } // if
                                                } else {
                                                        start_min = std::chrono::steady_clock::now();
                                                        stop_min = std::chrono::steady_clock::now();
                                                        num_requestes = 0;
                                                }
                                                /*
                                                 */
                                                start = std::chrono::steady_clock::now();
                                        } else {
                                                send_queue_mutex_.unlock();
                                                stop = std::chrono::steady_clock::now();
                                                auto diff = std::chrono::duration_cast<std::chrono::seconds>(stop - start);
                                                const float PING_DELAY = 20.0f;
                                                if(diff.count() > PING_DELAY) {
                                                        json j;
                                                        j["ping"] = 1;
                                                        std::string message = j.dump();
                                                        send_queue_mutex_.lock();
                                                        send_queue_.push(message);
                                                        send_queue_mutex_.unlock();
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
                        token_mutex_.lock();
                        if(token_ != "") {
                                token_mutex_.unlock();
                                if(is_error_token_) {
                                        break;
                                }
                                if(is_authorize_) {
                                        break;
                                }
                        } else {
                                token_mutex_.unlock();
                                if(is_open_connection_) {
                                        break;
                                }
                        }
                        std::this_thread::yield();
                }
        }
 //------------------------------------------------------------------------------
        ~BinaryAPI()
        {
                if(is_open_connection_) {
                        client_.stop();
                }
        }
//------------------------------------------------------------------------------
        /** \brief Запустить или остановить запись логов
         * \param is_use Если true, то идет запись логов
         */
        inline void set_use_log(bool is_use)
        {
                is_use_log = is_use;
        }
//------------------------------------------------------------------------------
        /** \brief Получить данные о размере депозита
         * \param balance Депозит
         * \return состояние ошибки (0 в случае успеха, иначе см. ErrorType)
         */
         inline int get_balance(double &balance)
         {
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
                if(is_authorize_) {
                        authorize_mutex_.lock();
                        currency = currency_;
                        authorize_mutex_.unlock();
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
        /** \brief Загрузить исторические данные тиков
         * \param symbol Имя валютной пары
         * \param startepoch Время начала получения тиков. Влияет, если тиков между startepoch и endepoch не больше 5000
         * \param endepoch Конечное время получения тиков. Если нужно получить последние тики, укажите 0
         * \param count_ticks Количество тиков. Не имеет смысла ставить данный параметр больше 5000
         * \return состояние ошибки (0 в случае успеха, иначе см. ErrorType)
         */
        int download_ticks(std::string symbol,
                           unsigned long long startepoch,
                           unsigned long long endepoch,
                           int count_ticks = 5000)
        {
                if(startepoch > 0 && endepoch > startepoch) {
                        unsigned long long diff = endepoch - startepoch;
                        if((symbol.find("R_") == std::string::npos && diff > 5000)
                                || (symbol.find("R_") != std::string::npos && diff > 10000))
                                return INVALID_PARAMETER;
                }
                if(count_ticks > 5000)
                        return INVALID_PARAMETER;
                json j;
                j["ticks_history"] = symbol;
                if(endepoch == 0) j["end"] = "latest";
                else j["end"] = endepoch;
                if(startepoch == 0) {
                        j["start"] = 1;
                        j["count"] = count_ticks;
                } else {
                        j["start"] = startepoch;
                }
                j["style"] = "ticks";
                array_ticks_mutex_.lock();
                array_ticks_.clear();
                array_ticks_mutex_.unlock();
                is_array_ticks_error_ = false;
                is_array_ticks_ = false;
                is_send_array_ticks_ = true;
                return send_json(j);
        }
//------------------------------------------------------------------------------
        /** \brief Получить исторические данные тиков
         * \param prices Цены тиков
         * \param times Время тиков
         * \return состояние ошибки (0 в случае успеха, иначе см. ErrorType)
         */
        int get_ticks(std::vector<double> &prices,
                      std::vector<unsigned long long> &times)
        {
                if(!is_send_array_ticks_)
                        return NO_COMMAND;
                is_send_array_ticks_ = false;
                //std::chrono::time_point<std::chrono::steady_clock>
                auto start = std::chrono::steady_clock::now();
                auto stop = std::chrono::steady_clock::now();
                while(true) {
                        std::this_thread::yield();

                        // получили историю
                        if(is_array_ticks_) {
                                break;
                        }

                        stop = std::chrono::steady_clock::now();
                        //std::chrono::duration<double>
                        auto diff = std::chrono::duration_cast<std::chrono::seconds>(stop - start);
                        const float MAX_DELAY = 60.0f;
                        // слишком долго ждем историю
                        if(diff.count() > MAX_DELAY) {
                                return UNKNOWN_ERROR;
                        }
                }
                // если была ошибка получения котировок
                if(is_array_ticks_error_) {
                        return UNKNOWN_ERROR;
                } else {
                        // преобразуем данные
                        array_ticks_mutex_.lock();
                        json j_prices = array_ticks_["prices"];
                        json j_times = array_ticks_["times"];
                        prices.resize(j_prices.size());
                        times.resize(j_times.size());
                        for(size_t i = 0; i < prices.size(); i++) {
                                prices[i] = atof((j_prices[i].get<std::string>()).c_str());
                                times[i] = atoi((j_times[i].get<std::string>()).c_str()); //atoi(timestr.c_str());
                        }
                        array_ticks_mutex_.unlock();
                        if(times.size() == 0) {
                                return DATA_NOT_AVAILABLE;
                        }
                }
                return OK;
        }
//------------------------------------------------------------------------------
        /** \brief Получить исторические данные тиков
         * \param symbol Имя валютной пары
         * \param prices Цены тиков
         * \param times Время тиков
         * \param startepoch Время начала получения тиков. Влияет, если тиков между startepoch и endepoch не больше 5000
         * \param endepoch Конечное время получения тиков. Если нужно получить последние тики, укажите 0
         * \param count_ticks Количество тиков. Не имеет смысла ставить данный параметр больше 5000
         * \return состояние ошибки (0 в случае успеха, иначе см. ErrorType)
         */
        int get_ticks(std::string symbol,
                        std::vector<double> &prices,
                        std::vector<unsigned long long> &times,
                        unsigned long long startepoch,
                        unsigned long long endepoch,
                        int count_ticks = 5000)
        {
                int err_data = download_ticks(symbol, startepoch, endepoch, count_ticks);
                if(err_data != OK)
                        return err_data;
                return get_ticks(prices, times);
        }
//------------------------------------------------------------------------------
        /** \brief Получить исторические данные тиков
         * \param symbol Имя валютной пары
         * \param prices Цены тиков
         * \param times Время тиков
         * \param startepoch Время начала получения тиков. Влияет, если тиков между startepoch и endepoch не больше 5000
         * \param endepoch Конечное время получения тиков. Если нужно получить последние тики, укажите 0
         * \return состояние ошибки (0 в случае успеха, иначе см. ErrorType)
         */
        int get_ticks_without_limits(std::string symbol,
                                     std::vector<double> &prices,
                                     std::vector<unsigned long long> &times,
                                     unsigned long long startepoch,
                                     unsigned long long endepoch)
        {
                if(startepoch == 0 || endepoch == 0 || endepoch < startepoch)
                        return INVALID_PARAMETER;
                unsigned long long offset_time = 1;
                if(symbol.find("R_") != std::string::npos)
                        offset_time = 2;

                unsigned long long epoch = startepoch;
                const int COUNT_TICKS_LIMIT = 5000;
                while(true) {
                        std::vector<double> _prices;
                        std::vector<unsigned long long> _times;
                        unsigned long long _endepoch = epoch + COUNT_TICKS_LIMIT;
                        if(_endepoch > endepoch) {
                                _endepoch = endepoch;
                        }
                        int err_data = get_ticks(symbol, _prices, _times, epoch, _endepoch);
                        // при возникновении ограничений по получению данных, дата последних доступных данных не меняется
                        // сервер присылает последние доступные данные, поэтому так и можно проверить
                        if(_times.size() > 0 && !(_times[0] >= epoch && _times.back() <= _endepoch))
                                return DATA_NOT_AVAILABLE;

                        if(err_data != OK && err_data != DATA_NOT_AVAILABLE)
                                return err_data;

                        const auto prices_size = prices.size();
                        const auto times_size = times.size();

                        prices.reserve(prices_size + _prices.size());
                        times.reserve(times_size + _times.size());
                        try {
                                prices.insert(prices.end(), _prices.begin(), _prices.end());
                                times.insert(times.end(), _times.begin(), _times.end());
                        }
                        catch(...) {
                                prices.erase(prices.begin() + prices_size, prices.end());
                                times.erase(times.begin() + times_size, times.end());
                                return UNKNOWN_ERROR;
                        }

                        if(_endepoch == endepoch) {
                                break;
                        }
                        // чтобы один и тот же момент времени не встречался дважды
                        epoch = _endepoch + offset_time;
                } // while
                return OK;
        }
//------------------------------------------------------------------------------
        /** \brief Загрузить исторические данные минутных свечей
         * \param symbol Имя валютной пары
         * \param startepoch Время начала получения тиков. Влияет, если тиков между startepoch и endepoch не больше 5000
         * \param endepoch Конечное время получения тиков. Если нужно получить последние тики, укажите 0
         * \param count_candles Количество свечей. Не имеет смысла ставить данный параметр больше 5000
         * \return состояние ошибки (0 в случае успеха, иначе см. ErrorType)
         */
        int download_candles(std::string symbol,
                             unsigned long long startepoch,
                             unsigned long long endepoch,
                             int count_candles = 5000)
        {
                json j;
                j["ticks_history"] = symbol;
                if(endepoch == 0) j["end"] = "latest";
                else j["end"] = endepoch;
                if(startepoch == 0) {
                        j["start"] = 1;
                        j["count"] = count_candles;
                } else {
                        j["start"] = startepoch;
                }
                j["style"] = "candles";
                j["granularity"] = 60;
                array_candles_mutex_.lock();
                array_candles_.clear();
                array_candles_mutex_.unlock();
                is_array_candles_error_ = false;
                is_array_candles_ = false;
                is_send_array_candles_ = true;
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
                if(!is_send_array_candles_)
                        return NO_COMMAND;
                is_send_array_candles_ = false;
                //std::chrono::time_point<std::chrono::steady_clock>
                auto start = std::chrono::steady_clock::now();
                auto stop = std::chrono::steady_clock::now();
                while(true) {
                        std::this_thread::yield();

                        // получили историю
                        if(is_array_candles_) {
                                break;
                        }

                        stop = std::chrono::steady_clock::now();
                        auto diff = std::chrono::duration_cast<std::chrono::seconds>(stop - start);
                        const float MAX_DELAY = 60.0f;
                        // слишком долго ждем историю
                        if(diff.count() > MAX_DELAY) {
                                return UNKNOWN_ERROR;
                        }
                }
                // если была ошибка получения котировок
                if(is_array_candles_error_) {
                        return UNKNOWN_ERROR;
                } else {
                        // преобразуем данные
                        array_candles_mutex_.lock();
                        close.resize(array_candles_.size());
                        times.resize(array_candles_.size());
                        for(size_t i = 0; i < array_candles_.size(); i++) {
                                json j = array_candles_[i];
                                close[i] = atof((j["close"].get<std::string>()).c_str());
                                std::string timestr = j["epoch"].dump();
                                times[i] = atoi(timestr.c_str());
                        }
                        array_candles_mutex_.unlock();
                        if(times.size() == 0)
                                return DATA_NOT_AVAILABLE;
                }
                return OK;
        }
//------------------------------------------------------------------------------
        /** \brief Получить исторические данные минутных свечей
         * \param symbol Имя валютной пары
         * \param close Цены закрытия свечей
         * \param times Временные метки открытия свечей
         * \param startepoch Время начала получения тиков. Влияет, если тиков между startepoch и endepoch не больше 5000
         * \param endepoch Конечное время получения тиков. Если нужно получить последние тики, укажите 0
         * \param count_candles Количество свечей. Не имеет смысла ставить данный параметр больше 5000
         * \return состояние ошибки (0 в случае успеха, иначе см. ErrorType)
         */
        int get_candles(std::string symbol,
                        std::vector<double> &close,
                        std::vector<unsigned long long> &times,
                        unsigned long long startepoch,
                        unsigned long long endepoch,
                        int count_candles = 5000)
        {
                int err_data = download_candles(symbol, startepoch, endepoch, count_candles);
                if(err_data != OK)
                        return err_data;
                return get_candles(close, times);
        }
//------------------------------------------------------------------------------
        int get_candles_without_limits(std::string symbol,
                                       std::vector<double> &close,
                                       std::vector<unsigned long long> &times,
                                       unsigned long long startepoch,
                                       unsigned long long endepoch)
        {
                if(startepoch == 0 || endepoch == 0 || endepoch < startepoch)
                        return INVALID_PARAMETER;

                unsigned long long epoch = startepoch;
                const int COUNT_TICKS_LIMIT = 60*5000;
                //std::cout << "startepoch " << startepoch << " endepoch " << endepoch << std::endl;
                while(true) {
                        std::vector<double> _close;
                        std::vector<unsigned long long> _times;
                        unsigned long long _endepoch = epoch + COUNT_TICKS_LIMIT;
                        if(_endepoch > endepoch) {
                                _endepoch = endepoch;
                        }
                        //std::cout << "epoch " << epoch << " _endepoch " << _endepoch << std::endl;
                        int err_data = get_candles(symbol, _close, _times, epoch, _endepoch);
                        if(_times.size() > 0 && !(_times[0] >= epoch && _times.back() <= _endepoch))
                                return DATA_NOT_AVAILABLE;

                        if(err_data != OK && err_data != DATA_NOT_AVAILABLE)
                                return err_data;

                        const auto close_size = close.size();
                        const auto times_size = times.size();

                        close.reserve(close_size + _close.size());
                        times.reserve(times_size + _times.size());
                        try {
                                close.insert(close.end(), _close.begin(), _close.end());
                                times.insert(times.end(), _times.begin(), _times.end());
                        }
                        catch(...) {
                                close.erase(close.begin() + close_size, close.end());
                                times.erase(times.begin() + times_size, times.end());
                                return UNKNOWN_ERROR;
                        }

                        if(_endepoch == endepoch) {
                                break;
                        }
                        // чтобы один и тот же момент времени не встречался дважды
                        epoch = _endepoch + xtime::SEC_MINUTE;
                } // while
                return OK;
        }
//------------------------------------------------------------------------------
        /** \brief Инициализировать список валютных пар
         * \param symbols Список валютных пар для получения котировок и процентов выплат
         */
        inline void init_symbols(std::vector<std::string> &symbols)
        {
                symbols_ = symbols;

                proposal_mutex_.lock();
                proposal_buy_.clear();
                proposal_buy_.resize(symbols.size());
                proposal_sell_.clear();
                proposal_sell_.resize(symbols.size());
                proposal_mutex_.unlock();

                quotations_mutex_.lock();
                close_data_.clear();
                time_data_.clear();
                close_data_.resize(symbols.size());
                time_data_.resize(symbols.size());
                quotations_mutex_.unlock();

                map_symbol_mutex_.lock();
                map_symbol_.clear();
                for(size_t i = 0; i < symbols_.size(); ++i) {
                        map_symbol_[symbols_[i]] = i;
                }
                map_symbol_mutex_.unlock();
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
                is_stream_quotations_error_ = false;
#               if BINARY_API_USE_TICKS_HISTORY_SUBSCRIBE == 0
                for(size_t i = 0; i < symbols_.size(); ++i) {
                        quotations_mutex_.lock();
                        int err_data = get_candles(symbols_[i], init_size, close_data_.at(i), time_data_.at(i), 0, 0);
                        quotations_mutex_.unlock();
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
                if(err_data == OK) {
                        is_stream_quotations_ = true;
                }
                return err_data;
#               else
                int err_data;
                for(size_t i = 0; i < symbols_.size(); ++i) {
                        json j;
                        j["ticks_history"] = symbols_[i];
                        j["subscribe"] = 1;
                        j["end"] = "latest";
                        j["style"] = "candles";
                        j["granularity"] = 60;
                        j["adjust_start_time"] = 1;
                        j["count"] = init_size;
                        err_data = send_json(j);
                        if(err_data != OK) {
                                break;
                        }
                }
                if(err_data == OK) {
                        is_stream_quotations_ = true;
                } else {
                        stop_stream_quotations();
                }
                return err_data;
#               endif
        }
//------------------------------------------------------------------------------
        /** \brief Остановить поток котировок
         * \return состояние ошибки (0 в случае успеха, иначе см. ErrorType)
         */
        int stop_stream_quotations()
        {
                json j;
                j["forget_all"] = "ticks";
                is_stream_quotations_ = false;
                is_stream_quotations_error_ = false;
                return send_json(j);
        }
//------------------------------------------------------------------------------
        /** \brief Получить данные потока котировок
         * Порядок следования валютных пар зависит от порядка, указанного в массие функции init_symbols
         * \param close_data массив цен закрытия минутных свечей
         * \param time_data массив временных меток цен открытия минутных свечей
         * \return состояние ошибки (0 в случае успеха, иначе см. ErrorType)
         */
        inline int get_stream_quotations(std::vector<std::vector<double>> &close_data,
                                         std::vector<std::vector<unsigned long long>> &time_data)
        {
                if(symbols_.size() == 0 || !is_stream_quotations_) {
                        return NO_INIT;
                }

                quotations_mutex_.lock();
                close_data = std::vector<std::vector<double>>(close_data_.cbegin(), close_data_.cend());
                time_data = std::vector<std::vector<unsigned long long>>(time_data_.cbegin(), time_data_.cend());
                quotations_mutex_.unlock();
                if(is_stream_quotations_error_)
                        return UNKNOWN_ERROR;
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
                if(is_last_time_ || is_stream_quotations_) {
                        timestamp = last_time_;
                        is_last_time_ = false;
                        return OK;
                } else {
                        return NO_INIT;
                }
        }
//------------------------------------------------------------------------------
        /** \brief Инициализировать поток процентов выплат по ставке по одной валютной паре и одному направлению
         * \param symbol имя валютной пары
         * \param amount размер ставки
         * \param contract_type тип контракта (см. ContractType)
         * \param duration длительность контракта
         * \param duration_unit единица измерения длительности контракта (см. DurationType)
         * \param currency валюта счета
         * \return состояние ошибки (0 в случае успеха, иначе см. ErrorType)
         */
        int init_stream_proposal(std::string symbol,
                                 double amount,
                                 int contract_type,
                                 int duration,
                                 int duration_unit,
                                 std::string currency = "USD")
        {
                json j;
                j["proposal"] = 1;
                j["subscribe"] = 1;
                j["amount"] = std::to_string(amount);
                j["basis"] = "stake"; // у нас ставка
                if(contract_type == BUY)
                        j["contract_type"] = "CALL";
                else if(contract_type == SELL)
                        j["contract_type"] = "PUT";
                if(currency_ == "")
                        j["currency"] = currency;
                else
                        j["currency"] = currency_;

                j["duration"] = std::to_string(duration);
                if(duration_unit == SECONDS) j["duration_unit"] = "s";
                else if(duration_unit == MINUTES) j["duration_unit"] = "m";
                else if(duration_unit == HOURS) j["duration_unit"] = "h";
                else if(duration_unit == TICKS) j["duration_unit"] = "t";
                else if(duration_unit == DAYS) j["duration_unit"] = "d";
                j["symbol"] = symbol;
                return send_json(j);
        }
//------------------------------------------------------------------------------
        /** \brief Инициализировать поток процентов выплат по ставке по всем валютным парам
         * \param amount размер ставки (помните об ограничениях брокера)
         * \param duration длительность контракта
         * \param duration_unit единица измерения длительности контракта (см. DurationType)
         * \param currency валюта счета
         * \return состояние ошибки (0 в случае успеха, иначе см. ErrorType)
         */
        int init_stream_proposal(double amount,
                                 int duration,
                                 int duration_unit,
                                 std::string currency = "USD")
        {
                if(symbols_.size() == 0)
                        return NO_INIT;
                // максимальное число запросов для подписки на проценты выплат в минуту - 25
                int num_stream_proposal = 0;
                const int max_num_stream_proposal = 25;
                const float SECONDS_MINUTE = 60.0f;
                std::chrono::duration<double> diff;
                std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::steady_clock::now();
                std::chrono::time_point<std::chrono::steady_clock> stop = std::chrono::steady_clock::now();
                //
                for(size_t i = 0; i < symbols_.size(); ++i) {
                        int err_data = init_stream_proposal(symbols_[i],
                                                            amount,
                                                            BUY,
                                                            duration,
                                                            duration_unit,
                                                            currency);
                        if(err_data != OK) {
                                std::cout << "BinaryApi: init stream proposal error! Message: " <<
                                        symbols_[i] << " BUY " << std::endl;
                                return err_data;
                        } else {
                                std::cout << "BinaryApi: init stream proposal " <<
                                        symbols_[i] << " BUY " << std::endl;
                        }
                        /* проверим ограничения на подписку о процентах выплат
                         */
                        num_stream_proposal++; // увеличим число запросов
                        stop = std::chrono::steady_clock::now();
                        diff = std::chrono::duration_cast<std::chrono::seconds>(stop - start);
                        // проверим, не прошла ли минута
                        if(diff.count() < SECONDS_MINUTE) {
                                // проверим число запросов в минуту
                                if(num_stream_proposal >= max_num_stream_proposal) {
                                        std::cout << "BinaryApi: init stream proposal wait..." << std::endl;
                                        // число запросов слишком большое, ждем конца минуты
                                        while(true) {
                                                stop = std::chrono::steady_clock::now();
                                                std::chrono::duration<double> diff = std::chrono::duration_cast<std::chrono::seconds>(stop - start);
                                                if(diff.count() >= SECONDS_MINUTE) {
                                                        // минута прошла, уходим из ожидания
                                                        start = std::chrono::steady_clock::now();
                                                        stop = std::chrono::steady_clock::now();
                                                        num_stream_proposal = 0;
                                                        break;
                                                }
                                                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                                        } // while
                                } // if
                        } else {
                                // минута прошла, обнуляем переменные
                                start = std::chrono::steady_clock::now();
                                stop = std::chrono::steady_clock::now();
                                num_stream_proposal = 0;
                        }
                        /*
                         */
                        err_data = init_stream_proposal(symbols_[i],
                                                        amount,
                                                        SELL,
                                                        duration,
                                                        duration_unit,
                                                        currency);
                        if(err_data != OK) {
                                std::cout << "BinaryApi: init streamn proposal error! Message: " <<
                                        symbols_[i] << " SELL " << std::endl;
                                return err_data;
                        } else {
                                std::cout << "BinaryApi: init stream proposal " <<
                                        symbols_[i] << " SELL " << std::endl;
                        }
                        /* проверим ограничения на подписку о процентах выплат
                         */
                        num_stream_proposal++; // увеличим число запросов
                        stop = std::chrono::steady_clock::now();
                        diff = std::chrono::duration_cast<std::chrono::seconds>(stop - start);
                        // проверим, не прошла ли минута
                        if(diff.count() < SECONDS_MINUTE) {
                                // проверим число запросов в минуту
                                if(num_stream_proposal >= max_num_stream_proposal) {
                                        std::cout << "BinaryApi: init stream proposal wait..." << std::endl;
                                        // число запросов слишком большое, ждем конца минуты
                                        while(true) {
                                                stop = std::chrono::steady_clock::now();
                                                auto diff = std::chrono::duration_cast<std::chrono::seconds>(stop - start);
                                                if(diff.count() >= SECONDS_MINUTE) {
                                                        // минута прошла, уходим из ожидания
                                                        start = std::chrono::steady_clock::now();
                                                        stop = std::chrono::steady_clock::now();
                                                        num_stream_proposal = 0;
                                                        break;
                                                }
                                                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                                        } // while
                                } // if
                        } else {
                                start = std::chrono::steady_clock::now();
                                stop = std::chrono::steady_clock::now();
                                num_stream_proposal = 0;
                        }
                        /*
                         */
                }
                is_stream_proposal_ = true;
                return OK;
        }
//------------------------------------------------------------------------------
        /** \brief Получить данные потока процентов выплат
         * Проценты выплат варьируются обычно от 0 до 1.0, где 1.0 соответствует 100% выплате брокера
         * Порядок следования валютных пар зависит от порядка, указанного в массие функции init_symbols
         * \param buy_data проценты выплат по сделкам BUY для всех валютных пар
         * \param sell_data проценты выплат по сделкам SELL для всех валютных пар
         * \return состояние ошибки (0 в случае успеха, иначе см. ErrorType)
         */
        inline int get_stream_proposal(std::vector<double> &buy_data,
                                       std::vector<double> &sell_data)
        {
                if(symbols_.size() == 0 || !is_stream_proposal_) {
                        return NO_INIT;
                }
                proposal_mutex_.lock();
                buy_data = std::vector<double>(proposal_buy_.cbegin(), proposal_buy_.cend());
                sell_data = std::vector<double>(proposal_sell_.cbegin(), proposal_sell_.cend());
                proposal_mutex_.unlock();
                return OK;
        }
//------------------------------------------------------------------------------
        /** \brief Остановить поток процентов выплат
         * \return состояние ошибки (0 в случае успеха, иначе см. ErrorType)
         */
        int stop_stream_proposal()
        {
                json j;
                j["forget_all"] = "proposal";
                is_stream_proposal_ = false;
                return send_json(j);
        }
//------------------------------------------------------------------------------
        /** \brief Состояние потока процентов выплат
         * Потоки могут оборваться после сброса соединения с сервером
         * \return флаг активности потока процентов выплат
         */
        inline bool is_proposal_stream()
        {
                return is_stream_proposal_;
        }
//------------------------------------------------------------------------------
        /** \brief Состояние потока котировок
         * Потоки могут оборваться после сброса соединения с сервером
         * \return флаг активности потока котировок
         */
        inline bool is_quotations_stream()
        {
                return is_stream_quotations_;
        }
//------------------------------------------------------------------------------
        /** \brief Отправить ордер на октрытие сделки
         * Данная функция не проверяет ответ от сервера на запрос
         * \param symbol имя валютной пары
         * \param amount размер ставки
         * \param contract_type тип контракта (см. ContractType, доступно BUY и SELL)
         * \param duration длительность контракта
         * \param duration_unit единица измерения длительности контракта (см. DurationType, например MINUTES)
         * \param currency валюта счета, по умолчанию USD
         * \return состояние ошибки (OK = 0 в случае успеха, иначе см. ErrorType)
         */
        inline int send_order(std::string symbol, double amount, int contract_type, int duration, int duration_unit, std::string currency = "USD")
        {
                if(!is_authorize_)
                        return NO_AUTHORIZATION;
                // для ускорения работы JSON строку формирует самостоятельно
                std::string str_symbol = "\"" + symbol + "\"";
                std::string str_amount = format("\"%f.2\"", amount);
                std::string str_duration = format("\"%d\"", duration);
                std::string str_duration_unit;
                if(duration_unit == MINUTES) str_duration_unit = "\"m\"";
                else if(duration_unit == SECONDS) str_duration_unit = "\"s\"";
                else if(duration_unit == HOURS) str_duration_unit = "\"h\"";
                else if(duration_unit == TICKS) str_duration_unit = "\"t\"";
                else if(duration_unit == DAYS) str_duration_unit = "\"d\"";

                std::string str_contract_type;
                if(contract_type == BUY) str_contract_type = "\"CALL\"";
                else if(contract_type == SELL) str_contract_type = "\"PUT\"";
                else return INVALID_PARAMETER;

                if(currency == "") {
                        currency = currency_;
                }
                std::string message = "{\"buy\":1,\"parameters\":{\"amount\":" + str_amount +
                        ",\"basis\":\"stake\",\"contract_type\":" + str_contract_type +
                        ",\"currency\":\"" + currency + "\",\"duration\":" + str_duration +
                        ",\"duration_unit\":" + str_duration_unit +
                        ",\"symbol\":" + str_symbol + "},\"price\":" + str_amount + "}";

                connection_mutex_.lock();
                save_connection_->send(message);
                connection_mutex_.unlock();
                return OK;
        }
};

#endif // BINARY_API_HPP_INCLUDED
