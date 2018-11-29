![binary-cpp-api logo](doc/logo/binary-cpp-api_logo.png)

***

### Описание

Данная *header-only* библиотека содержит класс для взаимодействия с Binary.com WebSocket API v3. [https://developers.binary.com/api/](https://developers.binary.com/api/). 

На данный момент библиотека находится в разработке

### Как пользоваться?

Чтобы начать использовать BinaryApi в своей программе, необходимо после подключения всех зависимостей в проект просто добавить заголовочный файл *BinaryAPI.hpp*.

Пример программы, которая выводит на экран цены закрытия (последние три минутные свечи) с трех валютных пар (EURUSD, EURGBP, EURJPY), а также время сервера:
```C++
#include "BinaryAPI.hpp"

int main() {
        BinaryApi iBinaryApi; // класс для взаимодействия с BinaryApi
        
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

### Готовые программы для ОС Windows

В архиве bin.7z содержится программа *binary_proposal_recorder.exe*, которая записывает каждую секунду проценты выплат с валютных пар (WLDAUD...WLDUSD, AUDCAD, AUDCHF, AUDJPY, AUDNZD, AUDUSD, EURAUD, EURCAD, EURCHF, EURGBP, EURJPY, EURNZD, EURUSD, GBPAUD, GBPCAD, GBPCHF, GBPJPY)
Проценты выплат записываются для сделок PUT и CALL с временем экспирации 3 минуты. Для расчета процента используется ставка 10 USD.
Данные записываются в виде JSON строки в файлы в папке data. Каждый файл соответствует конкретной дате и название файла формируется из даты, когда он был записан (например *proposal_29_11_2018.json*). Время для каждой структуры JSON указано в виде timestamp, ключ *time*, используется время сервера Binary (GMT).
Максимальное число строк в файле соответствует количеству секунд одного дня. 
Пример JSON строки:
```json
{"amount":10.0,"currency":"USD","data":[{"buy":0.8440000000000001,"sell":0.895,"symbol":"WLDAUD"},{"buy":0.7949999999999999,"sell":0.815,"symbol":"WLDEUR"},{"buy":0.7879999999999998,"sell":0.8850000000000002,"symbol":"WLDGBP"},{"buy":0.802,"sell":0.802,"symbol":"WLDUSD"},{"buy":0.7370000000000001,"sell":0.9710000000000001,"symbol":"frxAUDCAD"},{"buy":0.8079999999999998,"sell":0.8859999999999999,"symbol":"frxAUDCHF"},{"buy":0.8350000000000002,"sell":0.887,"symbol":"frxAUDJPY"},{"buy":0.702,"sell":1.0059999999999998,"symbol":"frxAUDNZD"},{"buy":0.784,"sell":0.889,"symbol":"frxAUDUSD"},{"buy":0.855,"sell":0.815,"symbol":"frxEURAUD"},{"buy":0.7570000000000001,"sell":0.8489999999999998,"symbol":"frxEURCAD"},{"buy":0.9120000000000001,"sell":0.7030000000000001,"symbol":"frxEURCHF"},{"buy":0.8559999999999999,"sell":0.0,"symbol":"frxEURGBP"},{"buy":0.0,"sell":0.0,"symbol":"frxEURJPY"},{"buy":0.0,"sell":0.0,"symbol":"frxEURNZD"},{"buy":0.0,"sell":0.0,"symbol":"frxEURUSD"},{"buy":0.0,"sell":0.0,"symbol":"frxGBPAUD"},{"buy":0.0,"sell":0.0,"symbol":"frxGBPCAD"},{"buy":0.0,"sell":0.0,"symbol":"frxGBPCHF"},{"buy":0.0,"sell":0.0,"symbol":"frxGBPJPY"},{"buy":0.0,"sell":0.0,"symbol":"frxGBPNZD"},{"buy":0.0,"sell":0.0,"symbol":"frxNZDUSD"},{"buy":0.0,"sell":0.0,"symbol":"frxUSDCAD"},{"buy":0.0,"sell":0.0,"symbol":"frxUSDJPY"}],"data_type":"proposal","duration":3,"duration_unit":2,"time":1543463936}
```

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

### Автор

elektro yar [electroyar2@gmail.com](electroyar2@gmail.com)

