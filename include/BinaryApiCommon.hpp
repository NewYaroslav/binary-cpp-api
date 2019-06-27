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
#ifndef COMMON_HPP_INCLUDED
#define COMMON_HPP_INCLUDED
//------------------------------------------------------------------------------
namespace BinaryApiCommon {
        /// Набор возможных состояний ошибки
        enum ErrorType {
                OK = 0,                         ///< Ошибок нет, все в порядке
                NO_AUTHORIZATION = -1,
                NO_COMMAND = -2,
                UNKNOWN_ERROR = -3,             ///< Неопределенная ошибка
                NO_INIT = -4,                   ///< Не было инициализации, поэтому метод класса не может быть использован
                INVALID_PARAMETER = -6,         ///< Один из параметров неверно указан
                DATA_NOT_AVAILABLE = -7,        ///< Данные не доступны
                NOT_ALL_DATA_DOWNLOADED = -8,   ///< Загружены не все данные

                NOT_OPEN_FILE = -9,             ///< Файл не был открыт
                NOT_WRITE_FILE = -10,           ///< Файл нельзя записать
                NOT_COMPRESS_FILE = -11,        ///< Файл нельзя сжать
                NOT_DECOMPRESS_FILE = -12,      ///< Файл нельзя разорхивировать
                DATA_SIZE_ERROR = -13,          ///< Ошибка размера данных

                NO_TIMESTAMP = - 14,            ///< Нет временной метки
                FILE_CANNOT_OPENED = -15,       ///< Файл не может быть открыт
                INDICATOR_NOT_READY_TO_WORK = -16,      ///< Индикатор не готов к работе
        };
//------------------------------------------------------------------------------
        /// Типы контрактов
        enum ContractType {
                BUY = 1,        ///< Контракт на покупку, ставка\прогноз вверх
                SELL = -1,      ///< Контракт на продажу, ставка\прогноз вверх
                UP = 1,         ///< Прогноз на ставку вверх (BUY)
                DN = -1,        ///< Прогноз на ставку вниз  (SELL)
                NO_FORECAST = 0,///< Нет прогноза
        };
//------------------------------------------------------------------------------
        /// Состояния опционов
        enum OptionStatus {
                WIN = 1,        ///< Прибыльный бинарный опцион\победа
                LOSS = -1,      ///< Убыточный бинарный опцион\провал
                NEUTRAL = 0,    ///< Опцион завершился в ничью
        };
//------------------------------------------------------------------------------
        /// Типы котировок
        enum QuotesType {
                QUOTES_TICKS = 0,       ///< Котировки тиков
                QUOTES_BARS = 1,        ///< Котировки баров
        };
//------------------------------------------------------------------------------
        /// Типы состояний
        enum StateType {
                END_OF_DATA = 2,        ///< Конец данных
                SKIPPING_DATA = -2,     ///< Пропуск данных (нет цен за указанный период)
                NORMAL_DATA = 0,        ///< Нормальный доступ к данным
        };
//------------------------------------------------------------------------------
        /// Типы нормализации данных
        enum NormalizationType {
                MINMAX_0_1 = 0, ///< Данные приводятся к уровню от 0 до 1
                MINMAX_1_1 = 1, ///< Данные приводятся к уровню от -1 до 1
        };
}
#endif // COMMON_HPP_INCLUDED
