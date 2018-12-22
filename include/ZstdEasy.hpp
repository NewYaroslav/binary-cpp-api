#ifndef ZSTDEASY_HPP_INCLUDED
#define ZSTDEASY_HPP_INCLUDED

#include "banana_filesystem.hpp"
#include "dictBuilder/zdict.h"
#include "zstd.h"

//#define ZSTD_EASY_USE_BINARY_API

#ifdef ZSTD_EASY_USE_BINARY_API
#include "BinaryAPI.hpp"
#include "BinaryApiEasy.hpp"
#endif

namespace ZstdEasy
{

        enum ErrorType {
                OK = 0,
                NOT_OPEN_FILE = -9,
                NOT_WRITE_FILE = -10,
                NOT_COMPRESS_FILE = -11,
                NOT_DECOMPRESS_FILE = -12,
                DATA_SIZE_ERROR = -13,
        };

        enum QuotesType {
                QUOTES_TICKS = 0,
                QUOTES_BARS = 1,
        };

//------------------------------------------------------------------------------
        /** \brief Тренируйте словарь из массива образцов
        * \param path путь к файлам
        * \param file_name имя файл словаря, который будет сохранен по окончанию обучения
        * \return венет 0 в случае успеха
        */
        int train_zstd(std::string path, std::string file_name, size_t dict_buffer_capacit = 102400)
        {
                std::vector<std::string> files_list;
                bf::get_list_files(path, files_list, true);
                size_t all_files_size = 0;
                void *samples_buffer = NULL;

                size_t num_files = 0;
                size_t *samples_size = NULL;
                for(size_t i = 0; i < files_list.size(); ++i) {
                        int file_size = bf::get_file_size(files_list[i]);
                        if(file_size > 0) {
                                num_files++;
                                size_t start_pos = all_files_size;
                                all_files_size += file_size;
                                std::cout << "buffer size: " << all_files_size << std::endl;
                                samples_buffer = realloc(samples_buffer, all_files_size);
                                samples_size = (size_t*)realloc((void*)samples_size, num_files * sizeof(size_t));
                                samples_size[num_files - 1] = file_size;
                                int err = bf::load_file(files_list[i], samples_buffer, all_files_size, start_pos);
                                if(err != file_size) {
                                        std::cout << "load file: error, " << files_list[i] << std::endl;
                                        if(samples_buffer != NULL)
                                                free(samples_buffer);
                                        if(samples_size != NULL)
                                                free(samples_size);
                                        return NOT_OPEN_FILE;
                                } else {
                                        std::cout << "load file: " << files_list[i] << std::endl;
                                }
                        } else {
                                std::cout << "buffer size: error, " << files_list[i] << std::endl;
                                if(samples_buffer != NULL)
                                        free(samples_buffer);
                                if(samples_size != NULL)
                                        free(samples_size);
                                return NOT_OPEN_FILE;
                        }
                }
                void *dict_buffer = NULL;
                //size_t dict_buffer_capacit = 1024*100;
                dict_buffer = malloc(dict_buffer_capacit);
                size_t file_size = ZDICT_trainFromBuffer(dict_buffer, dict_buffer_capacit, samples_buffer, samples_size, num_files);
                size_t err = bf::write_file(file_name, dict_buffer, file_size);
                return err > 0 ? OK : NOT_WRITE_FILE;
        }
//------------------------------------------------------------------------------
        /** \brief Сжать файл с использованием словаря
         * \param input_file файл, который надо сжать
         * \param output_file файл, в который сохраним данные
         * \param dictionary_file файл словаря для сжатия
         * \param compress_level уровень сжатия файла
         * \return вернет 0 в случае успеха
         */
        int compress_file(std::string input_file,
                          std::string output_file,
                          std::string dictionary_file,
                          int compress_level = ZSTD_maxCLevel())
        {
                int input_file_size = bf::get_file_size(input_file);
                if(input_file_size <= 0)
                        return NOT_OPEN_FILE;

                int dictionary_file_size = bf::get_file_size(dictionary_file);
                if(dictionary_file_size <= 0)
                        return NOT_OPEN_FILE;

                void *input_file_buffer = NULL;
                input_file_buffer = malloc(input_file_size);

                void *dictionary_file_buffer = NULL;
                dictionary_file_buffer = malloc(dictionary_file_size);

                bf::load_file(dictionary_file, dictionary_file_buffer, dictionary_file_size);
                bf::load_file(input_file, input_file_buffer, input_file_size);

                size_t compress_file_size = ZSTD_compressBound(input_file_size);
                void *compress_file_buffer = NULL;
                compress_file_buffer = malloc(compress_file_size);

                ZSTD_CCtx* const cctx = ZSTD_createCCtx();
                size_t compress_size = ZSTD_compress_usingDict(
                        cctx,
                        compress_file_buffer,
                        compress_file_size,
                        input_file_buffer,
                        input_file_size,
                        dictionary_file_buffer,
                        dictionary_file_size,
                        compress_level
                        );

                if(ZSTD_isError(compress_size)) {
                        std::cout << "error compressin: " << ZSTD_getErrorName(compress_size) << std::endl;
                        ZSTD_freeCCtx(cctx);
                        free(compress_file_buffer);
                        free(dictionary_file_buffer);
                        free(input_file_buffer);
                        return NOT_COMPRESS_FILE;
                }

                bf::write_file(output_file, compress_file_buffer, compress_size);

                ZSTD_freeCCtx(cctx);
                free(compress_file_buffer);
                free(dictionary_file_buffer);
                free(input_file_buffer);
                return OK;
        }
//------------------------------------------------------------------------------
        /** \brief Записать сжатый файл
         * \param file_name имя файла
         * \param dictionary_file файл словаря для сжатия
         * \param buffer буфер, который запишем
         * \param buffer_size размер буфера
         * \param compress_level уровень сжатия файла
         * \return вернет 0 в случае успеха
         */
        int write_compressed_file(std::string file_name,
                                  std::string dictionary_file,
                                  void *buffer,
                                  size_t buffer_size,
                                  int compress_level = ZSTD_maxCLevel())
        {
                int dictionary_file_size = bf::get_file_size(dictionary_file);
                if(dictionary_file_size <= 0)
                        return NOT_OPEN_FILE;

                void *dictionary_file_buffer = NULL;
                dictionary_file_buffer = malloc(dictionary_file_size);

                bf::load_file(dictionary_file, dictionary_file_buffer, dictionary_file_size);

                size_t compress_file_size = ZSTD_compressBound(buffer_size);
                void *compress_file_buffer = NULL;
                compress_file_buffer = malloc(compress_file_size);

                ZSTD_CCtx* const cctx = ZSTD_createCCtx();
                size_t compress_size = ZSTD_compress_usingDict(
                        cctx,
                        compress_file_buffer,
                        compress_file_size,
                        buffer,
                        buffer_size,
                        dictionary_file_buffer,
                        dictionary_file_size,
                        compress_level
                        );

                if(ZSTD_isError(compress_size)) {
                        std::cout << "error compressin: " << ZSTD_getErrorName(compress_size) << std::endl;
                        ZSTD_freeCCtx(cctx);
                        free(compress_file_buffer);
                        free(dictionary_file_buffer);
                        //free(input_file_buffer);
                        return NOT_COMPRESS_FILE;
                }

                bf::write_file(file_name, compress_file_buffer, compress_size);

                ZSTD_freeCCtx(cctx);
                free(compress_file_buffer);
                free(dictionary_file_buffer);
                return OK;
        }
//------------------------------------------------------------------------------
        /** \brief Записать сжатый файл
         * \param file_name имя файла
         * \param dictionary_file файл словаря для сжатия
         * \param prices котировки
         * \param times временные метки
         * \param compress_level уровень сжатия файла
         * \return вернет 0 в случае успеха
         */
        int write_binary_quotes_compressed_file(std::string file_name,
                                                std::string dictionary_file,
                                                std::vector<double> &prices,
                                                std::vector<unsigned long long> &times,
                                                int compress_level = ZSTD_maxCLevel())
        {
                if(times.size() != prices.size())
                        return DATA_SIZE_ERROR;
                size_t buffer_size = prices.size() * sizeof(double) +
                        times.size() * sizeof(unsigned long long);
                void *buffer = NULL;
                buffer = malloc(buffer_size);
                size_t buffer_offset = 0;
                unsigned long data_size = times.size();
                std::memcpy(buffer + buffer_offset, &data_size, sizeof(data_size));
                buffer_offset += sizeof(data_size);
                for(unsigned long i = 0; i < data_size; ++i) {
                        std::memcpy(buffer + buffer_offset, &prices[i], sizeof(double));
                        buffer_offset += sizeof(double);
                        std::memcpy(buffer + buffer_offset, &times[i], sizeof(unsigned long long));
                        buffer_offset += sizeof(unsigned long long);
                }
                int err = write_compressed_file(file_name, dictionary_file, buffer, buffer_size, compress_level);
                free(buffer);
                return err;
        }
//------------------------------------------------------------------------------
        /** \brief Декомпрессия файла
         * \param input_file сжатый файл
         * \param output_file файл, в который сохраним данные
         * \param dictionary_file файл словаря для декомпресии
         * \return вернет 0 в случае успеха
         */
        int decompress_file(std::string input_file,
                            std::string output_file,
                            std::string dictionary_file)
        {
                int input_file_size = bf::get_file_size(input_file);
                if(input_file_size <= 0)
                        return NOT_OPEN_FILE;

                int dictionary_file_size = bf::get_file_size(dictionary_file);
                if(dictionary_file_size <= 0)
                        return NOT_OPEN_FILE;

                void *input_file_buffer = NULL;
                input_file_buffer = malloc(input_file_size);

                void *dictionary_file_buffer = NULL;
                dictionary_file_buffer = malloc(dictionary_file_size);

                bf::load_file(dictionary_file, dictionary_file_buffer, dictionary_file_size);
                bf::load_file(input_file, input_file_buffer, input_file_size);

                unsigned long long const decompress_file_size = ZSTD_getFrameContentSize(input_file_buffer, input_file_size);
                if (decompress_file_size == ZSTD_CONTENTSIZE_ERROR) {
                        std::cout << input_file << " it was not compressed by zstd." << std::endl;
                        free(dictionary_file_buffer);
                        free(input_file_buffer);
                        return NOT_DECOMPRESS_FILE;
                } else
                if (decompress_file_size == ZSTD_CONTENTSIZE_UNKNOWN) {
                        std::cout << input_file << " original size unknown." << std::endl;
                        free(dictionary_file_buffer);
                        free(input_file_buffer);
                        return NOT_DECOMPRESS_FILE;
                }
                void *decompress_file_buffer = NULL;
                decompress_file_buffer = malloc(decompress_file_size);

                ZSTD_DCtx* const dctx = ZSTD_createDCtx();
                size_t const decompress_size = ZSTD_decompress_usingDict(
                        dctx,
                        decompress_file_buffer,
                        decompress_file_size,
                        input_file_buffer,
                        input_file_size,
                        dictionary_file_buffer,
                        dictionary_file_size);

                if(ZSTD_isError(decompress_size)) {
                        std::cout << "error decompressin: " << ZSTD_getErrorName(decompress_size) << std::endl;
                        ZSTD_freeDCtx(dctx);
                        free(decompress_file_buffer);
                        free(dictionary_file_buffer);
                        free(input_file_buffer);
                        return NOT_DECOMPRESS_FILE;
                }

                bf::write_file(output_file, decompress_file_buffer, decompress_size);

                ZSTD_freeDCtx(dctx);
                free(decompress_file_buffer);
                free(dictionary_file_buffer);
                free(input_file_buffer);
                return OK;
        }
//------------------------------------------------------------------------------
        /** \brief Считать данные из сжатого файла
         * \param file_name имя файла
         * \param dictionary_file файл словаря для декомпресии
         * \param buffer буфер, в который запишем
         * \param buffer_size размер буфера
         * \return вернет 0 в случае успеха
         */
        int read_compressed_file(std::string file_name,
                                 std::string dictionary_file,
                                 void *&buffer,
                                 size_t &buffer_size)
        {
                int input_file_size = bf::get_file_size(file_name);
                if(input_file_size <= 0)
                        return NOT_OPEN_FILE;

                int dictionary_file_size = bf::get_file_size(dictionary_file);
                if(dictionary_file_size <= 0)
                        return NOT_OPEN_FILE;

                void *input_file_buffer = NULL;
                input_file_buffer = malloc(input_file_size);

                void *dictionary_file_buffer = NULL;
                dictionary_file_buffer = malloc(dictionary_file_size);

                bf::load_file(dictionary_file, dictionary_file_buffer, dictionary_file_size);
                bf::load_file(file_name, input_file_buffer, input_file_size);

                unsigned long long const decompress_file_size = ZSTD_getFrameContentSize(input_file_buffer, input_file_size);
                if (decompress_file_size == ZSTD_CONTENTSIZE_ERROR) {
                        std::cout << file_name << " it was not compressed by zstd." << std::endl;
                        free(dictionary_file_buffer);
                        free(input_file_buffer);
                        return NOT_DECOMPRESS_FILE;
                } else
                if (decompress_file_size == ZSTD_CONTENTSIZE_UNKNOWN) {
                        std::cout << file_name << " original size unknown." << std::endl;
                        free(dictionary_file_buffer);
                        free(input_file_buffer);
                        return NOT_DECOMPRESS_FILE;
                }

                buffer = malloc(decompress_file_size);

                ZSTD_DCtx* const dctx = ZSTD_createDCtx();
                size_t const decompress_size = ZSTD_decompress_usingDict(
                        dctx,
                        buffer,
                        decompress_file_size,
                        input_file_buffer,
                        input_file_size,
                        dictionary_file_buffer,
                        dictionary_file_size);

                if(ZSTD_isError(decompress_size)) {
                        std::cout << "error decompressin: " << ZSTD_getErrorName(decompress_size) << std::endl;
                        ZSTD_freeDCtx(dctx);
                        free(buffer);
                        free(dictionary_file_buffer);
                        free(input_file_buffer);
                        buffer_size = 0;
                        return NOT_DECOMPRESS_FILE;
                }
                buffer_size = decompress_size;
                ZSTD_freeDCtx(dctx);
                free(dictionary_file_buffer);
                free(input_file_buffer);
                return OK;
        }
//------------------------------------------------------------------------------
        /** \brief Читать сжатый файл котировок
         * \param file_name имя файла
         * \param dictionary_file файл словаря для декомпресии
         * \param prices котировки
         * \param times временные метки
         * \return вернет 0 в случае успеха
         */
        int read_binary_quotes_compress_file(std::string file_name,
                                             std::string dictionary_file,
                                             std::vector<double> &prices,
                                             std::vector<unsigned long long> &times)
        {
                void *buffer = NULL;
                size_t buffer_size = 0;
                int err = read_compressed_file(file_name, dictionary_file, buffer, buffer_size);
                if(err == OK) {
                        size_t buffer_offset = 0;
                        unsigned long data_size = 0;
                        std::memcpy(&data_size, buffer + buffer_offset, sizeof(data_size));
                        buffer_offset += sizeof(data_size);
                        prices.resize(data_size);
                        times.resize(data_size);
                        for(unsigned long i = 0; i < data_size; ++i) {
                                std::memcpy(&prices[i], buffer + buffer_offset, sizeof(double));
                                buffer_offset += sizeof(double);
                                std::memcpy(&times[i], buffer + buffer_offset, sizeof(unsigned long long));
                                buffer_offset += sizeof(unsigned long long);
                        }
                        if(buffer != NULL) {
                                free(buffer);
                        }
                        return OK;
                } else {
                        if(buffer != NULL) {
                                free(buffer);
                        }
                        return err;
                }
        }
//------------------------------------------------------------------------------
#       ifdef ZSTD_EASY_USE_BINARY_API
        /** \brief Скачать и сохранить все доступыне данные по котировкам
         * \param api Класс BinaryAPI
         * \param symbol валютная пара
         * \param path директория, куда сохраняются данные
         * \param dictionary_file файл словаря для декомпресии
         * \param timestamp временная метка, с которой начинается загрузка данных
         * \param is_skip_day_off флаг пропуска выходных дней, true если надо пропускать выходные
         * \param type тип загружаемых данных, QUOTES_BARS - минутные бары, QUOTES_TICKS - тики (как правило период 1 секунда)
         * \param user_function - функтор
         */
        int download_and_save_all_data_with_compression(
                                       BinaryAPI &api,
                                       std::string symbol,
                                       std::string path,
                                       std::string dictionary_file,
                                       unsigned long long timestamp,
                                       bool is_skip_day_off = true,
                                       int type = QUOTES_BARS,
                                       void (*user_function)(std::string,
                                        std::vector<double> &,
                                        std::vector<unsigned long long> &,
                                        unsigned long long) = NULL)
        {
                bf::create_directory(path);
                xtime::DateTime iTime(timestamp);
                iTime.hour = iTime.seconds = iTime.minutes = 0;
                unsigned long long stop_time = iTime.get_timestamp() - xtime::SEC_DAY;
                if(is_skip_day_off) {
                        while(xtime::is_day_off(stop_time)) {
                                stop_time -= xtime::SEC_DAY;
                        }
                }
                int err = BinaryAPI::OK;
                int num_download = 0;
                int num_errors = 0;
                while(true) {
                        // сначала выполняем проверку
                        std::string file_name = path + "//" +
                                BinaryApiEasy::get_file_name_from_date(stop_time) + ".zstd";

                        if(bf::check_file(file_name)) {
                                if(is_skip_day_off) {
                                        stop_time -= xtime::SEC_DAY;
                                        while(xtime::is_day_off(stop_time)) {
                                                stop_time -= xtime::SEC_DAY;
                                        }
                                } else {
                                        stop_time -= xtime::SEC_DAY;
                                }
                                continue;
                        }

                        //std::cout << xtime::get_str_unix_date_time(stop_time) << std::endl;
                        // загружаем файл
                        std::vector<double> _prices;
                        std::vector<unsigned long long> _times;
                        if(type == QUOTES_BARS) {
                                err = api.get_candles_without_limits(
                                        symbol,
                                        _prices,
                                        _times,
                                        stop_time,
                                        stop_time + xtime::SEC_DAY - 1);
                        } else
                        if(type == QUOTES_TICKS) {
                                err = api.get_ticks_without_limits(
                                        symbol,
                                        _prices,
                                        _times,
                                        stop_time,
                                        stop_time + xtime::SEC_DAY - 1);
                        }
                        if(is_skip_day_off) {
                                stop_time -= xtime::SEC_DAY;
                                while(xtime::is_day_off(stop_time)) {
                                        stop_time -= xtime::SEC_DAY;
                                }
                        } else {
                                stop_time -= xtime::SEC_DAY;
                        }
                        if(err == BinaryAPI::OK && _times.size() > 0) { // данные получены
                                //std::cout << "write_binary_quotes_file: " << file_name << " " << xtime::get_str_unix_date_time(_times.back()) << std::endl;
                                if(user_function != NULL)
                                        user_function(file_name, _prices, _times, stop_time);
                                write_binary_quotes_compressed_file(file_name, dictionary_file, _prices, _times);
                                num_download++;
                                num_errors = 0;
                        } else {
                                num_errors++;
                                //std::cout << "num_errors: " << num_errors << std::endl;
                        }
                        const int MAX_ERRORS = 30;
                        if(num_errors > MAX_ERRORS) {
                                break;
                        }
                }
                if(num_download == 0) {
                        if(err != BinaryAPI::OK)
                                return err;
                        return BinaryApiEasy::NOT_ALL_DATA_DOWNLOADED;
                }
                return BinaryAPI::OK;
        }
#       endif
//------------------------------------------------------------------------------
}

#endif // ZSTDEASY_HPP_INCLUDED
