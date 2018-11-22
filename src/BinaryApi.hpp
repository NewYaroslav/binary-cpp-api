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
private:
        WssClient client_; // Класс клиента
        std::shared_ptr<WssClient::Connection> save_connection_; // Соединение
        std::recursive_mutex connection_lock_;
        bool is_open_connection = false;
        std::queue<std::string> send_queue_; // Очередь сообщений

        // параметры счета
        double balance_ = 0; // Баланс счета
        std::string currency_ = ""; // Валюта счета
        bool is_authorize_ = false;
        bool is_repeat_authorize_ = false;
        std::recursive_mutex balance_lock_;
        // поток выплат и котировок
        std::vector<std::string> symbols_;
        std::unordered_map<std::string, int> map_proposal_symbol_;
        std::vector<double> proposal_buy_;
        std::vector<double> proposal_sell_;
        bool is_repeat_proposa_l = false;

        std::recursive_mutex proposal_lock_;

        std::recursive_mutex send_queue_lock_;
//------------------------------------------------------------------------------
        bool check_authorize_message(json &j) {
                // получили сообщение авторизации
                if(j["msg_type"] == "authorize") {
                        if(j["error"] != nullptr) {
                                std::unique_lock<std::recursive_mutex> locker(balance_lock_);
                                is_authorize_ = false;
                                // попробуем еще раз залогиниться
                                std::string message = j["echo_req"].dump();
                                std::unique_lock<std::recursive_mutex> locker_send(send_queue_lock_);
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
        bool check_proposal_message(json &j) {
                if(j["msg_type"] == "proposal" &&
                        j["echo_req"]["subscribe"] == 1) {
                        std::string _symbol = j["echo_req"]["symbol"];
                        std::unique_lock<std::recursive_mutex> locker(proposal_lock_);
                        if(map_proposal_symbol_.find(_symbol) != map_proposal_symbol_.end()) {
                                int indx = map_proposal_symbol_[_symbol];
                                double temp = 0.0;
                                if(j["error"] == nullptr) {
                                        double ask_price = atof((j["proposal"]["ask_price"].get<std::string>()).c_str());
                                        double payout = atof((j["proposal"]["payout"].get<std::string>()).c_str());
                                        temp = ask_price != 0 ? (payout/ask_price) - 1 : 0.0;
                                } else {
                                        // отправим сообщение о подписки на выплаты

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
        void parse_json(std::string& str) {
                try {
                        json j = json::parse(str);
                        // обрабатываем сообщение
                        if(check_proposal_message(j))
                                return;
                        if(check_authorize_message(j))
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
                        app_id, false)
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
                        if(token != "") {
                            j["authorize"] = token;
                        } else {
                            j["ping"] = 1;
                        }

                        std::string message = j.dump();
                        std::cout << "BinaryApi: Sending message: \"" <<
                                message << "\"" << std::endl;
                        connection->send(message);

                        is_open_connection = true;
                };

                client_.on_message =
                        [&](std::shared_ptr<WssClient::Connection> connection,
                                std::shared_ptr<WssClient::InMessage> message)
                {
                        std::string text = message->string();
                        std::cout << "message: " << text << std::endl;
                        parse_json(text);
                };

                client_.on_close = [&](std::shared_ptr<WssClient::Connection> /*connection*/,
                        int status,
                        const std::string & /*reason*/)
                {
                        std::unique_lock<std::recursive_mutex> locker(connection_lock_);
                        is_open_connection = false;
                        std::cout << "BinaryApi: Closed connection with status code " <<
                                status << std::endl;
                };

                client_.on_error = [&](std::shared_ptr<WssClient::Connection> /*connection*/,
                        const SimpleWeb::error_code &ec)
                {
                        std::unique_lock<std::recursive_mutex> locker(connection_lock_);
                        is_open_connection = false;
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
                                if(is_open_connection) {
                                        std::unique_lock<std::recursive_mutex> locker(send_queue_lock_);
                                        if(send_queue_.size() > 0) {
                                                std::string message = send_queue_.front();
                                                send_queue_.pop();
                                                std::cout << "BinaryApi: send " <<
                                                        message << std::endl;
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
        }

        enum ErrorType {
                OK = 0,
                NO_AUTHORIZATION = -1,
        };
//------------------------------------------------------------------------------
        /** \brief Получить данные о размере депозита
         * \param balance Депозит
         * \return состояние ошибки (0 в случае успеха, иначе см. ErrorType)
         */
         inline int get_balance(double& balance)
         {
                std::unique_lock<std::recursive_mutex> locker(balance_lock_);
                if(is_authorize_) {
                        balance = balance_;
                        return OK;
                }
                return NO_AUTHORIZATION;
         }
};

#endif // BINARY_API_HPP_INCLUDED
