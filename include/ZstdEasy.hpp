#ifndef ZSTDEASY_HPP_INCLUDED
#define ZSTDEASY_HPP_INCLUDED

#include "banana_filesystem.hpp"
#include "dictBuilder/zdict.h"
#include "zstd.h"

namespace ZstdEasy
{

        enum ErrorType {
                OK = 0,
                NOT_OPEN_FILE = -9,
                NOT_WRITE_FILE = -10,
                NOT_COMPRESS_FILE = -11,
                NOT_DECOMPRESS_FILE = -12,
        };

//------------------------------------------------------------------------------
        /** \brief Тренируйте словарь из массива образцов
        * \param path путь к файлам
        * \param file_name имя файл словаря, который будет сохранен по окончанию обучения
        * \return венет 0 в случае успеха
        */
        int train_zstd(std::string path, std::string file_name)
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
                size_t dict_buffer_capacit = 1024*100;
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
                        return NOT_COMPRESS_FILE;
                }

                bf::write_file(output_file, decompress_file_buffer, decompress_size);

                ZSTD_freeDCtx(dctx);
                free(decompress_file_buffer);
                free(dictionary_file_buffer);
                free(input_file_buffer);
        }
}

#endif // ZSTDEASY_HPP_INCLUDED
