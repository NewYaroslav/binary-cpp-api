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
#include <atomic>
#include <chrono>
#include <iostream>
//------------------------------------------------------------------------------
class BinaryApi
{
public:
        using json = nlohmann::json;
        using WssClient = SimpleWeb::SocketClient<SimpleWeb::WSS>;

        // варианты ошибок API
        enum ErrorType {
                OK = 0,
                NO_AUTHORIZATION = -1,
                NO_COMMAND = -2,
                UNKNOWN_ERROR = -3,
                NO_INIT = -4,
                NO_OPEN_CONNECTION = -5,
        };

        // типы контрактов
        enum ContractType {
                BUY = 1,
                SELL = -1,
        };
        // длительности контракта
        enum DurationType {
                TICKS = 0,
                SECONDS = 1,
                MINUTES = 2,
                HOURS = 3,
                DAYS = 4,
        };
private:
        WssClient client_; // Класс клиента
        std::shared_ptr<WssClient::Connection> save_connection_; // Соединение
        std::atomic<bool> is_open_connection_; // состояние соединения
        std::string token_ = "";
        std::mutex token_mutex_;
        std::atomic<bool> is_error_token_;
        std::mutex c_mutex_;

        std::queue<std::string> send_queue_; // Очередь сообщений
        std::mutex s_mutex_;

        // параметры счета
        std::atomic<double> balance_; // Баланс счета
        std::string currency_ = ""; // Валюта счета
        std::atomic<bool> is_authorize_;
        std::mutex a_mutex_;


        // поток выплат и котировок
        std::vector<std::string> symbols_;
        std::unordered_map<std::string, int> map_symbol_;
        std::mutex m_mutex_;

        std::vector<double> proposal_buy_;
        std::vector<double> proposal_sell_;
        std::mutex p_mutex_;

        std::vector<std::vector<double>> close_data_;
        std::vector<std::vector<unsigned long long>> time_data_;
        std::mutex q_mutex_;

        std::atomic<bool> is_stream_quotations_;
        std::atomic<bool> is_stream_proposal_;
        // время сервера
        std::atomic<unsigned long long> last_time_;
        std::atomic<bool> is_last_time_;

        std::mutex array_candles_lock_;
        json array_candles_;
        std::atomic<bool> is_array_candles_;
        std::atomic<bool> is_array_candles_error_;
        std::atomic<bool> is_send_array_candles_;
//------------------------------------------------------------------------------
        int send_json_with_authorize(json &j)
        {
                if(is_authorize_) {
                        std::string message = j.dump();
                        s_mutex_.lock();
                        send_queue_.push(message);
                        s_mutex_.unlock();
                        return OK;
                }
                return NO_AUTHORIZATION;
        }
//------------------------------------------------------------------------------
        int send_json(json &j)
        {
                if(is_open_connection_) {
                        std::string message = j.dump();
                        s_mutex_.lock();
                        send_queue_.push(message);
                        s_mutex_.unlock();
                        return OK;
                }
                return NO_OPEN_CONNECTION;
        }
//------------------------------------------------------------------------------
        bool check_time_message(json &j,
                                json::iterator& j_msg_type,
                                json::iterator& j_error)
        {
                if(*j_msg_type == "time") {
                        if(j_error != j.end()) {
                                // попробуем еще раз
                                std::string message = j["echo_req"].dump();
                                if((*j_error)["code"] == "RateLimit") {
                                        // отправим сообщение повторно с задержкой
                                        std::thread([&,message]{
                                                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                                                s_mutex_.lock();
                                                send_queue_.push(message);
                                                s_mutex_.unlock();
                                        }).detach();
                                } else {
                                        s_mutex_.lock();
                                        send_queue_.push(message);
                                        s_mutex_.unlock();
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
                                json::iterator& j_msg_type,
                                json::iterator& j_error)
        {
                if(*j_msg_type == "tick") {
                        if(j_error != j.end()) {
                                if((*j_error)["code"] != "AlreadySubscribed") {
                                        // попробуем еще раз
                                        std::string message = j["echo_req"].dump();
                                        if((*j_error)["code"] == "RateLimit") {
                                                std::thread([&,message]{
                                                        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                                                        s_mutex_.lock();
                                                        send_queue_.push(message);
                                                        s_mutex_.unlock();
                                                }).detach();
                                        } else {
                                                s_mutex_.lock();
                                                send_queue_.push(message);
                                                s_mutex_.unlock();
                                        }
                                }
                        } else {
                                auto it_tick = j.find("tick");
                                double quote = atof(((*it_tick)["quote"].get<std::string>()).c_str());              // котировка
                                unsigned long long epoch = atoi(((*it_tick)["epoch"].get<std::string>()).c_str());  // время
                                std::string symbol = (*it_tick)["symbol"];                                          // символ
                                unsigned long long lastepoch = (epoch/60)*60;                                               // время послденей закрытой свечи

                                m_mutex_.lock();
                                auto it_symbol = map_symbol_.find(symbol);
                                if(it_symbol == map_symbol_.end()) {
                                        m_mutex_.unlock();
                                        return true;
                                }
                                volatile int indx = it_symbol->second;
                                m_mutex_.unlock();

                                q_mutex_.lock();
                                if(indx >= 0 && indx < (int)close_data_.size()) {
                                        if(epoch % 60 == 0) {
                                                close_data_.at(indx).push_back(quote);
                                                time_data_.at(indx).push_back(epoch);
                                        } else
                                        if(close_data_.size() > 0) {
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
                                q_mutex_.unlock();
                                //}
                        }
                        return true;
                } else {
                        return false;
                }
        }
//------------------------------------------------------------------------------
        bool check_candles_message(json &j,
                                   json::iterator& j_msg_type,
                                   json::iterator& j_error)
        {
                if(*j_msg_type == "candles") {
                        if(j_error != j.end()) {
                                if((*j_error)["code"] == "RateLimit") {
                                        // попробуем еще раз
                                        std::string message = j["echo_req"].dump();
                                        std::thread([&,message]{
                                                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                                                s_mutex_.lock();
                                                send_queue_.push(message);
                                                s_mutex_.unlock();
                                        }).detach();
                                } else {
                                        is_array_candles_ = true;
                                        is_array_candles_error_ = true;
                                }
                        } else {
                                auto it_candles = j.find("candles");

                                array_candles_lock_.lock();
                                if(it_candles != j.end())
                                        array_candles_ = *it_candles;
                                array_candles_lock_.unlock();
                                is_array_candles_ = true;
                                is_array_candles_error_ = false;
                        }
                        return true;
                } else {
                        return false;
                }
        }
//------------------------------------------------------------------------------
        bool check_authorize_message(json &j,
                                     json::iterator& j_msg_type,
                                     json::iterator& j_error)
        {
                // получили сообщение авторизации
                if(*j_msg_type == "authorize") {
                        if(j_error != j.end()) {
                                // попробуем еще раз залогиниться
                                std::string message = j["echo_req"].dump();
                                if((*j_error)["code"] == "RateLimit") {
                                        std::thread([&,message]{
                                                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                                                s_mutex_.lock();
                                                send_queue_.push(message);
                                                s_mutex_.unlock();
                                        }).detach();
                                } else {
                                        if((*j_error)["code"] == "InvalidToken") {
                                                is_error_token_ = true;
                                                return true;
                                        }
                                        s_mutex_.lock();
                                        send_queue_.push(message);
                                        s_mutex_.unlock();
                                }
                                is_authorize_ = false;
                        } else {
                                balance_ = atof((j["authorize"]["balance"].get<std::string>()).c_str());
                                a_mutex_.lock();
                                currency_ = j["authorize"]["currency"];
                                a_mutex_.unlock();
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
                                    json::iterator& j_msg_type,
                                    json::iterator& j_error)
        {
                auto it_echo_req = j.find("echo_req");
                if(*j_msg_type == "proposal" &&
                        (*it_echo_req)["subscribe"] == 1) {
                        std::string _symbol = (*it_echo_req)["symbol"];
                        m_mutex_.lock();
                        auto it_symbol = map_symbol_.find(_symbol);
                        if(it_symbol != map_symbol_.end()) {
                                volatile int indx = it_symbol->second;
                                m_mutex_.unlock();
                                double temp = 0.0;
                                if(j_error == j.end()) {
                                        auto it_proposal = j.find("proposal");
                                        double ask_price = atof(((*it_proposal)["ask_price"].get<std::string>()).c_str());
                                        double payout = atof(((*it_proposal)["payout"].get<std::string>()).c_str());
                                        temp = ask_price != 0 ? (payout/ask_price) - 1 : 0.0;
                                } else {
                                        if((*j_error)["code"] == "AlreadySubscribed") {
                                                return true;
                                        }
                                        // отправим сообщение о подписки на выплаты
                                        std::string message = j["echo_req"].dump();
                                        if((*j_error)["code"] == "RateLimit" ||
                                          (*j_error)["code"] == "ContractBuyValidationError") {
                                                // отправляем сообщение с задержкой
                                                std::thread([&,message]{
                                                        std::this_thread::sleep_for(std::chrono::milliseconds(2500));
                                                        s_mutex_.lock();
                                                        send_queue_.push(message);
                                                        s_mutex_.unlock();
                                                }).detach();
                                        } else {
                                                // отправляем сообщение мгновенно
                                                s_mutex_.lock();
                                                send_queue_.push(message);
                                                s_mutex_.unlock();
                                        }
                                }
                                //std::cout << "6" << std::endl;
                                auto it_contract_type = (*it_echo_req).find("contract_type");
                                if(*it_contract_type == "CALL") {
                                        p_mutex_.lock();
                                        proposal_buy_.at(indx) = temp;
                                        p_mutex_.unlock();
                                } else
                                if(*it_contract_type == "PUT") {
                                        p_mutex_.lock();
                                        proposal_sell_.at(indx) = temp;
                                        p_mutex_.unlock();
                                } // if
                        } else {
                                m_mutex_.unlock();
                        }
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
                        /* для ускорения заранее находим сообщения
                         * msg_type и error
                         */
                        json::iterator it_msg_type = j.find("msg_type");
                        json::iterator it_error = j.find("error");
                        // обрабатываем сообщение
                        if(check_tick_message(j, it_msg_type, it_error))
                                return;
                        if(check_proposal_message(j, it_msg_type, it_error))
                                return;
                        if(check_authorize_message(j, it_msg_type, it_error))
                                return;
                        if(check_candles_message(j, it_msg_type, it_error))
                                return;
                        if(check_time_message(j, it_msg_type, it_error))
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
                        app_id, false) , token_(token), is_open_connection_(false),
                        is_error_token_(false), is_authorize_(false),
                        is_stream_quotations_(false), is_stream_proposal_(false),
                        is_last_time_(false), is_array_candles_(false),
                        is_array_candles_error_(false), is_send_array_candles_(false),
                        last_time_(0), balance_(0)
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
                        std::lock(c_mutex_, token_mutex_);
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
                        c_mutex_.unlock();
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
                };

                client_.on_error = [&](std::shared_ptr<WssClient::Connection> /*connection*/,
                        const SimpleWeb::error_code &ec)
                {
                        is_open_connection_ = false;
                        is_authorize_ = false;
                        std::cout << "BinaryApi: Error: " <<
                                ec << ", error message: " << ec.message() << std::endl;
                };

                std::thread client_thread([&]() {
                        while(true) {
                                client_.start();
                                is_stream_quotations_ = false;
                                is_stream_proposal_ = false;
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
                                        s_mutex_.lock();
                                        if(send_queue_.size() > 0) {
                                                std::string message = send_queue_.front();
                                                send_queue_.pop();
                                                s_mutex_.unlock();
                                                //std::cout << "BinaryApi: send " <<
                                                //        message << std::endl;
                                                c_mutex_.lock();
                                                save_connection_->send(message);
                                                c_mutex_.unlock();
                                                /* проверим ограничение запросов в минуту
                                                 */
                                                num_requestes++; // увеличим число запросов в минуту
                                                stop_min = std::chrono::steady_clock::now();
                                                std::chrono::duration<double> diff;
                                                diff = std::chrono::duration_cast<std::chrono::seconds>(stop_min - start_min);
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
                                                s_mutex_.unlock();
                                                stop = std::chrono::steady_clock::now();
                                                std::chrono::duration<double> diff = std::chrono::duration_cast<std::chrono::seconds>(stop - start);
                                                const float PING_DELAY = 20.0f;
                                                if(diff.count() > PING_DELAY) {
                                                        json j;
                                                        j["ping"] = 1;
                                                        std::string message = j.dump();
                                                        s_mutex_.lock();
                                                        send_queue_.push(message);
                                                        s_mutex_.unlock();
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
                        a_mutex_.lock();
                        currency = currency_;
                        a_mutex_.unlock();
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
                array_candles_lock_.lock();
                array_candles_.clear();
                array_candles_lock_.unlock();
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
                std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::steady_clock::now();
                std::chrono::time_point<std::chrono::steady_clock> stop = std::chrono::steady_clock::now();
                while(true) {
                        std::this_thread::yield();

                        // получили историю
                        if(is_array_candles_) {
                                break;
                        }

                        stop = std::chrono::steady_clock::now();
                        std::chrono::duration<double> diff = std::chrono::duration_cast<std::chrono::seconds>(stop - start);
                        const float MAX_DELAY = 5.0f;
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
                        array_candles_lock_.lock();
                        close.resize(array_candles_.size());
                        times.resize(array_candles_.size());
                        for(size_t i = 0; i < array_candles_.size(); i++) {
                                json j = array_candles_[i];
                                close[i] = atof((j["close"].get<std::string>()).c_str());
                                std::string timestr = j["epoch"].dump();
                                times[i] = atoi(timestr.c_str());
                        }
                        array_candles_lock_.unlock();
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
                symbols_ = symbols;

                p_mutex_.lock();
                proposal_buy_.clear();
                proposal_buy_.resize(symbols.size());
                proposal_sell_.clear();
                proposal_sell_.resize(symbols.size());
                p_mutex_.unlock();

                q_mutex_.lock();
                close_data_.clear();
                time_data_.clear();
                close_data_.resize(symbols.size());
                time_data_.resize(symbols.size());
                q_mutex_.unlock();

                m_mutex_.lock();
                map_symbol_.clear();
                for(size_t i = 0; i < symbols_.size(); ++i) {
                        map_symbol_[symbols_[i]] = i;
                }
                m_mutex_.unlock();
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
                        q_mutex_.lock();
                        int err_data = get_candles(symbols_[i], init_size, close_data_.at(i), time_data_.at(i), 0, 0);
                        q_mutex_.unlock();
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

                q_mutex_.lock();
                close_data = std::vector<std::vector<double>>(close_data_.cbegin(), close_data_.cend());
                time_data = std::vector<std::vector<unsigned long long>>(time_data_.cbegin(), time_data_.cend());
                q_mutex_.unlock();
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
                p_mutex_.lock();
                buy_data = std::vector<double>(proposal_buy_.cbegin(), proposal_buy_.cend());
                sell_data = std::vector<double>(proposal_sell_.cbegin(), proposal_sell_.cend());
                p_mutex_.unlock();
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
};

#endif // BINARY_API_HPP_INCLUDED
