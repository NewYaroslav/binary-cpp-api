#ifndef WAVEASY_HPP_INCLUDED
#define WAVEASY_HPP_INCLUDED

#include "xwave.h"
#include <string>
//------------------------------------------------------------------------------
namespace WavEasy
{
//------------------------------------------------------------------------------
        /** \brief Класс для записи звука
        */
        class SoundRecording
        {
        private:
//------------------------------------------------------------------------------
                xwave_wave_file wave_file;      /**< Структура файла wav */
                int bits_per_sample_;           /**< Количество бит в сэмпле */
                bool is_init = false;
//------------------------------------------------------------------------------
        public:
//------------------------------------------------------------------------------
                enum ErrorType {
                        OK = 0,
                        NO_INIT = -4,
                };
//------------------------------------------------------------------------------
                /** \brief Инициализировать класс
                 * \param file_name имя файла
                 * \param sample_rate Частота дискретизации
                 * \param bits_per_sample Количество бит в сэмпле
                 */
                SoundRecording(std::string file_name, int sample_rate = 48000, int bits_per_sample = 16)
                {
                        const int num_channels = 1;
                        std::string file_name_norm_len;
                        if(file_name.size() > XWAVE_MAX_PATH) {
                                const int END_STR_LEN = 5;
                                file_name_norm_len = file_name.substr(0,XWAVE_MAX_PATH-END_STR_LEN);
                                file_name_norm_len += ".wav";
                        } else {
                                file_name_norm_len = file_name + ".wav";
                        }
                        if(bits_per_sample != 8 && bits_per_sample != 16 && bits_per_sample != 32)
                                return;
                        // инициализируем структуру
                        xwave_init_wave_file(&wave_file, file_name_norm_len.c_str(), sample_rate, bits_per_sample, num_channels);
                        bits_per_sample_ = bits_per_sample;
                        is_init = true;
                }
//------------------------------------------------------------------------------
                /** \brief Обновить WAV файл
                 * \param input входной сигнал в диапазоне от -1 до 1
                 * \return вернет 0 в случае успеха
                 */
                int update(double input)
                {
                        if(!is_init)
                                return NO_INIT;
                        long data;
                        if(input > 1.0)
                                input = 1.0;
                        if(input < -1.0)
                                input = -1.0;
                        if(bits_per_sample_ == 8) {
                                data = ((input + 1.0) / 2.0) * 255;
                        } else
                        if(bits_per_sample_ == 16) {
                                data = input * 32767;
                        } else
                        if(bits_per_sample_ == 32) {
                                data = input * 2147483647;
                        }
                        xwave_write_sample_wave_file(&wave_file, &data);
                        return OK;
                }
//------------------------------------------------------------------------------
                /** \brief Сохранить и закрыть файл
                 * \return вернет 0 в случае успеха
                 */
                int close()
                {
                        if(is_init) {
                                // сохраним изменения в файле и закроем его
                                xwave_close_wave_file(&wave_file);
                                is_init = false;
                                return OK;
                        }
                        return NO_INIT;
                }
//------------------------------------------------------------------------------
                ~SoundRecording()
                {
                        close();
                }
//------------------------------------------------------------------------------
            };
//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------
#endif // WAVEASY_HPP_INCLUDED
