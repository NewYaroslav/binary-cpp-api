![binary-cpp-api logo](doc/logo/binary-cpp-api_logo.png)

***

### Описание

Данная *header-only* библиотека содержит класс для взаимодействия с Binary.com WebSocket API v3. [https://developers.binary.com/api/](https://developers.binary.com/api/). 

На данный момент библиотека находится в разработке

### Как пользоваться?

Чтобы начать использовать BinaryAPI в своей программе, необходимо после подключения всех зависимостей в проект просто добавить заголовочный файл *BinaryAPI.hpp*.

Пример программы, которая выводит на экран цены закрытия (последние три минутные свечи) с трех валютных пар (EURUSD, EURGBP, EURJPY), а также время сервера:
```C++
#include "BinaryAPI.hpp"

int main() {
        BinaryAPI iBinaryApi; // класс для взаимодействия с BinaryApi
        
        std::vector<std::string> symbols; // массив валютных пар
        symbols.push_back("frxEURUSD");
        symbols.push_back("frxEURGBP");
        symbols.push_back("frxEURJPY");
        // инициализируем список валютных пар 
        std::cout << "init_symbols " << std::endl;
        iBinaryApi.init_symbols(symbols);
        // инициализируем поток котировок, глубина начальной истории 60 минут
        std::cout << "init_stream_quotations " << iBinaryApi.init_stream_quotations(60) << std::endl;
        // инициализируем поток процентов выплат
        std::cout << "init_stream_proposal " << iBinaryApi.init_stream_proposal(10, 3, iBinaryApi.MINUTES, "USD") << std::endl;
        // последнее полученное с сервера время
        unsigned long long servertime_last = 0;
        // начинается основной цикл программы
        std::cout << "..." << std::endl;
        while(true) {
                // для всех валютных пар
                std::vector<std::vector<double>> close_data; // цены закрытия
                std::vector<std::vector<unsigned long long>> time_data; // время открытия свечей
                std::vector<double> buy_data; // проценты выплат (ставка вверх)
                std::vector<double> sell_data; // проценты выплат (ставка вниз)
                unsigned long long servertime = 0; // время сервера

                if(iBinaryApi.get_servertime(servertime) != iBinaryApi.OK) {
                        // если время сервера не удалось получить
                        iBinaryApi.request_servertime(); // отправляем запрос на получение времени
                        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // задержка 
                        continue;
                } else {
                        // время было получено, проверим, прошла ли секунда
                        if(servertime > servertime_last) {
                                // секунда прошла, сохраним последнее полученное от сервера время
                                servertime_last = servertime;
                        } else {
                                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                                // секунда еще не прошла, вернемся
                                continue;
                        }
                }
                // проверим, удалось ли получить данные по котировкам и процентам выплат
                if(iBinaryApi.get_stream_quotations(close_data, time_data) == iBinaryApi.OK &&
                   iBinaryApi.get_stream_proposal(buy_data, sell_data) == iBinaryApi.OK) {
                        // выводим последние значения котировок валютных пар
                        for(size_t i = 0; i < symbols.size(); ++i) {
                                std::cout << symbols[i] << " " << buy_data[i] << "/" << sell_data[i] << std::endl;
                                for(size_t j = time_data[i].size() - 3; j < time_data[i].size(); ++j) {
                                        std::cout << close_data[i][j] << " " << time_data[i][j] << std::endl;
                                }
                                std::cout << "size: " << time_data[i].size() << std::endl;
                        }
                }
                // время сервера в GMT\UTC
                std::cout << servertime << std::endl;
                std::cout << std::endl;
                std::this_thread::yield();
        }
}
```

![example_1](doc/example_1.png)

+ Методы

*get_balance* - Получить данные о размере депозита

*get_currency* - Получить данные о валюте депозита

*init_stream_balance* - Инициализировать поток баланса депозита

*stop_stream_balance* - Остановить поток баланса депозита

*download_ticks* - Загрузить исторические данные тиков

*get_ticks* - Получить исторические данные тиков

*download_candles* - Загрузить исторические данные минутных свечей

*get_candles* - Получить исторические данные минутных свечей

*init_symbols* - Инициализировать список валютных пар

*init_stream_quotations* - Инициализировать поток котировок

*stop_stream_quotations* - Остановить поток котировок

*get_stream_quotations* - Получить данные потока котировок

*request_servertime* - Запрос на получение времери сервера

*get_servertime* - Получить время сервера
Данная функция вернет время сервера после вызова функции request_servertime(), а также если инициализирован поток котировок

*init_stream_proposal* - Инициализировать поток процентов выплат по ставке по всем валютным парам

*get_stream_proposal* - Получить данные потока процентов выплат
Проценты выплат варьируются обычно от 0 до 1.0, где 1.0 соответствует 100% выплате брокера

*stop_stream_proposal* - Остановить поток процентов выплат

*is_proposal_stream* - Состояние потока процентов выплат

*is_quotations_stream* - Состояние потока котировок

*send_order* - Отправить ордер на октрытие сделки

+ Типы контрактов

```С++
// типы контрактов
enum ContractType {
		BUY = 1, // опцион на покупку
		SELL = -1, // опцион на продажу
};

// длительности контракта
enum DurationType {
		TICKS = 0,
		SECONDS = 1,
		MINUTES = 2,
		HOURS = 3,
		DAYS = 4,
};

// используется, например, в методе init_stream_proposal или send_order

//...

BinaryAPI BinaryApi(token, api_id);
double amount = 10.0;
int duration = 3;
// открываем сделку
BinaryApi.send_order("frxEURUSD", amount, BUY, duration, BinaryAPI::MINUTES); 

```

+ Возможные состояния ошибок

```С++
// варианты ошибок API
enum ErrorType {
		OK = 0, // процесс завершился удачно
		NO_AUTHORIZATION = -1, // нет авторизации
		NO_COMMAND = -2, // не было команды перед использованием метода
		UNKNOWN_ERROR = -3, // неизвестная ошибка
		NO_INIT = -4, // не было инициализации перед использованием метода
		NO_OPEN_CONNECTION = -5, // нет соединения с сервером
};

std::vector<double> buy_proposal;
std::vector<double> sell_proposal;
int err = BinaryApi.get_stream_proposal(buy_proposal, sell_proposal);

if(err == BinaryAPI::OK) {
	// все в порядке, можно использовать данные в buy_proposal и sell_proposal
}

```

### Готовые программы для ОС Windows

В архиве bin.7z содержится программа *binary_proposal_recorder.exe*, которая записывает каждую секунду проценты выплат с валютных пар (WLDAUD...WLDUSD, AUDCAD, AUDCHF, AUDJPY, AUDNZD, AUDUSD, EURAUD, EURCAD, EURCHF, EURGBP, EURJPY, EURNZD, EURUSD, GBPAUD, GBPCAD, GBPCHF, GBPJPY)
Проценты выплат записываются для сделок PUT и CALL с временем экспирации 3 минуты. Для расчета процента используется ставка 10 USD.
Данные записываются в виде бинарных файлов. Каждый файл соответствует конкретной дате и название файла формируется из даты, когда он был записан (например *proposal_29_11_2018.hex*). Время для каждого сэмпла указано в виде 8 байт timestamp в конце, используется время сервера Binary (GMT).
Максимальное число сэмплов в файле соответствует количеству секунд одного дня. 

Настройки программы хранятся в JSON файле *settings.json*. Пример содержимого файла:
```java
{
	"disk": "D", // диск, на котором находится программа
	"path": "_repoz//binary_historical_data", // папка (репозиторий git), где будут храниться данные
	"amount": 10.0, // ставка для расчета процентов выплат
	"duration": 3, // время экспирации опциона
	"duration_uint": 2, // единица измерения времени (2 - минуты)
	"currency": "USD", // валюта счета 
	"folder": "proposal_data", // папка, где будут храниться данные процентов выплат
	"git": 1 // использовать git для загрузки данных в репозиторий
}
```

На данный момент данные по процентам выплат будут храниться в репозитории [https://github.com/NewYaroslav/binary_historical_data](https://github.com/NewYaroslav/binary_historical_data). 
Данные репозитория будут обновляться раз в день (приблизительно в 00:00 GMT)

### Зависимости

* *binary-cpp-api* зависит от следующих внешних библиотек / пакетов

* *Simple-WebSocket-Server* - [https://gitlab.com/eidheim/Simple-WebSocket-Server](https://gitlab.com/eidheim/Simple-WebSocket-Server)
* *Boost.Asio* или автономный *Asio* - [http://think-async.com/Asio](http://think-async.com/Asio)
* *Библиотека OpenSSL* - [http://slproweb.com/products/Win32OpenSSL.html](http://slproweb.com/products/Win32OpenSSL.html)
* *Библиотека JSON* - [https://github.com/nlohmann/json](https://github.com/nlohmann/json)
* *gcc* или *mingw* с поддержкой C++11, например - [https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win64/Personal%20Builds/mingw-builds/7.3.0/threads-posix/seh/x86_64-7.3.0-release-posix-seh-rt_v5-rev0.7z/download](https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win64/Personal%20Builds/mingw-builds/7.3.0/threads-posix/seh/x86_64-7.3.0-release-posix-seh-rt_v5-rev0.7z/download)

Все необходимые библиотеки добавлены, как субмодули, в папку lib. 

### Полезные ссылки

Просмотреть схемы JSON можно здесь - [https://github.com/binary-com/websockets/tree/gh-pages/config/v3](https://github.com/binary-com/websockets/tree/gh-pages/config/v3)

Сайт брокера - [https://www.binary.com/](https://www.binary.com/)


