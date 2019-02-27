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

#ifndef FANNEASY_HPP_INCLUDED
#define FANNEASY_HPP_INCLUDED
//------------------------------------------------------------------------------
#include "fann.h"
#include "floatfann.h"
#include <vector>
//------------------------------------------------------------------------------
namespace FannEasy
{
//------------------------------------------------------------------------------
                enum ErrorType {
                        OK = 0,
                        NO_INIT = -4,
                        INVALID_PARAMETER = -6,
                        DATA_NOT_AVAILABLE = -7,
                        NO_TIMESTAMP = - 14,
                };
//------------------------------------------------------------------------------
        /** \brief Зарезервировать память для двумерного массива
         * \param x количество строк
         * \param y длина строки
         * \return указатель на двумерный массив
        */
        fann_type **reserve_data(int x, int y)
        {
                fann_type **data = new fann_type*[x];
                for(int i = 0; i < x; i++) {
                        data[i] = new fann_type[y]; // Кол-во нейроннов в слое
                }
                return data;
        }
//------------------------------------------------------------------------------
        /** \brief Очистить память двумерного массива
         * \param x количество строк
         * \param data указатель на двумерный массив
        */
        void delete_data(int x, fann_type **data)
        {
                for(int i = 0; i < x; i++) {
                        delete[] data[i];
                }
                delete[] data;
        }
//------------------------------------------------------------------------------
        /** \brief Установить данные двумерного массива
         * \param in вектор данных (содержание строки)
         * \param data указатель на двумерный массив
         * \param x строка двумерного массива
        */
        template<class T>
        void set_data(std::vector<T> &in, fann_type **data, int x)
        {
                for(int i = 0; i < (int)in.size(); i++) {
                        data[x][i] = in[i];
                }
        }
//------------------------------------------------------------------------------
        /** \brief Установить данные для двух массивов
         * \param in вектор входных данных нейросети (содержание строки)
         * \param out вектор выходных данных нейросети (содержание строки)
         * \param dataIn указатель на двумерный массив входных данных нейросети
         * \param dataOut указатель на двумерный массив выходных данных нейросети
         * \param x строка двумерного массива
        */
        template<class T, class T2>
        void set_data(std::vector<T> &in, std::vector<T2> &out, fann_type **data_in, fann_type **data_out, int x)
        {
                for(int i = 0; i < (int)in.size(); i++) {
                        data_in[x][i] = in[i];
                }
                for(int i = 0; i < (int)out.size(); i++) {
                        data_out[x][i] = out[i];
                }
        }
//------------------------------------------------------------------------------
        /** \brief Преобразовать данные вектора в данные для нейронной сети
         * \param in вектор данных
         * \return указатель на массив
        */
        template<class T>
        fann_type *conversion(std::vector<T> &in)
        {
                fann_type *out = (fann_type*)malloc(in.size() * sizeof(fann_type));
                for(int i = 0; i < (int)in.size(); i++) {
                        out[i] = in[i];
                }
                return out;
        }
//------------------------------------------------------------------------------
        /** @brief Преобразовать данные нейронной сети в вектор
         * \param in вектор данных
         * \return указатель на массив
        */
        template<class T>
        std::vector<T> conversion(fann_type *in, int n)
        {
                std::vector<T> out(n);
                for(int i = 0; i < n; i++) {
                        out[i] = in[i];
                }
                return out;
        }
//------------------------------------------------------------------------------
        /** \brief Преобразовать данные вектора в данные нейронной сети
         * \param in данные вектора
         * \param out данные нейронной сети
         */
        template<class T>
        void conversion(std::vector<T> &in, fann_type *out)
        {
                for(int i = 0; i < (int)in.size(); i++) {
                        out[i] = in[i];
                }
        }
//------------------------------------------------------------------------------
        /** \brief Преобразовать данные нейронной сети в вектор
         * \param in данные нейронной сети
         * \param n количество данных нейронной сети
         * \param out данные вектора
         */
        template<class T>
        void conversion(fann_type *in, int n, std::vector<T> &out)
        {
                for(int i = 0; i < n; i++) {
                        out[i] = in[i];
                }
        }
//------------------------------------------------------------------------------
        /** \brief Запустить нейросеть
         * Функция возвращает максимальный выход нейросети, или в случае неопределенности -1
         * \param ann нейросеть
         * \param in вектор данных
         * \return номер максимального выхода нейросети или  -1 в случае неопределенного состояния
        */
        template<class T>
        int run_ann(struct fann *ann, std::vector<T> &in) {
                fann_type *input = conversion(in);
                fann_type *calc_out = fann_run(ann, input);
                int num_out = fann_get_num_output(ann);
                fann_type max_out = 0;
                const int UNDEFINED_STATE = -1;
                int pos = UNDEFINED_STATE;
                for(int i = 0; i < num_out; i++) {
                        if(calc_out[i] > max_out) {
                                max_out = calc_out[i];
                                pos = i;
                        }
                }
                free(input);
                return pos;
        }
//------------------------------------------------------------------------------
        class BaseAnn {
//------------------------------------------------------------------------------
        protected:
                struct fann* ann = NULL;
                fann_type* input = NULL;
                is_init = false;
                int num_input = 0;
                int num_outpuut = 0;
//------------------------------------------------------------------------------
        public:
                virtual BaseNet()
                {

                }
//------------------------------------------------------------------------------
                virtual BaseNet(std::string path)
                {
                        ann = fann_create_from_file(path.c_str());
                        num_input = fann_get_num_input(ann);
                        num_outpuut = fann_get_num_output(ann);
                        input = (fann_type*)malloc(num_input * sizeof(fann_type));
                        is_init = true;
                }
//------------------------------------------------------------------------------
                virtual ~BaseNet()
                {
                        if(is_init) {
                                fann_destroy(ann);
                                free(input);
                        }
                }
//------------------------------------------------------------------------------
                virtual int updata(std::vector<double>& in, std::vector<double>& out)
                {
                        if(!is_init) {
                                return NO_INIT;
                        }
                        if((int)in.size() != num_input) {
                                return INVALID_PARAMETER;
                        }
                        conversion(in, input);
                        fann_type* calc_out = fann_run(ann, input);
                        out.resize(num_output);
                        for(int i = 0; i < nOutput; i++) {
                                out[i] = calc_out[i];
                        }
                }
//------------------------------------------------------------------------------
        };
//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------
#endif // FANNEASY_HPP_INCLUDED
