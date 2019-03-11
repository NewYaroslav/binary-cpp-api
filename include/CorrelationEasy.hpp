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
#ifndef CORRELATIONEASY_HPP_INCLUDED
#define CORRELATIONEASY_HPP_INCLUDED

#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>

namespace CorrelationEasy
{
//------------------------------------------------------------------------------
        /// Набор возможных состояний ошибки
        enum ErrorType {
                OK = 0,
                NO_INIT = -4,
                INVALID_PARAMETER = -6,
        };
//------------------------------------------------------------------------------
        /** \brief Коэффициент корреляции Пирсона
         * Коэффициент корреляции Пирсона характеризует существование линейной зависимости между двумя величинами
         * \param x первая выборка данных
         * \param y вторая выборка данных
         * \param rxy коэффициент корреляции Пирсона (от -1 до +1)
         * \return вернет 0 в случае успеха
         */
        template<class T1, class T2, class T3>
        int calculate_pearson_correlation_coefficient(std::vector<T1>& x, std::vector<T2> &y, T3 &rxy)
        {
                if(x.size() != y.size() || x.size() == 0) {
                        return INVALID_PARAMETER;
                }
                T1 xm = std::accumulate(x.begin(), x.end(), T1(0));
                T2 ym = std::accumulate(y.begin(), y.end(), T2(0));
                xm /= (T1)x.size();
                ym /= (T2)y.size();
                T3 sum = 0, sumx2 = 0, sumy2 = 0;
                for(size_t i = 0; i < x.size(); ++i) {
                        T1 dx = x - xm;
                        T2 dy = y - ym;
                        sum += dx * dy;
                        sumx2 += dx * dx;
                        sumy2 += dy * dy;
                }
                if(sumx2 == 0 || sumy2 == 0) {
                        return INVALID_PARAMETER;
                }
                rxy = sum / std::sqrt(sumx2 * sumy2);
                return OK;
        }
//------------------------------------------------------------------------------
        /**@brief Quick sort function (Быстрая сортировка Хоара)
         * Данная функция нужна для коэффициента корреляции Спирмена
         */
        template<typename T1, typename T2>
        void quick_sort(std::vector<T1> &a, std::vector<T2> &ra, long l, long r)
        {
                long i = l, j = r;
                T1 temp, p;
                p = a[ l + (r - l)/2 ];
                do {
                        while(a[i] < p) i++;
                        while(a[j] > p) j--;
                        if (i <= j) {
                                temp = a[i];
                                a[i] = a[j];
                                a[j] = temp;
                                ra[i] = j;
                                ra[j] = i;
                                i++; j--;
                        }
                } while(i <= j);
                if(i < r)
                        quick_sort(a, ra, i, r);
                if(l < j)
                        quick_sort(a, ra, l , j);
        };
//------------------------------------------------------------------------------
        /**@brief Quick sort function (Быстрая сортировка Хоара)
         * Данная функция нужна для коэффициента корреляции Спирмена
         */
        template<typename T1, typename T2>
        void quick_sort(std::vector<T1> &a, std::vector<T2> &ra)
        {
                std::vector<T1> _a = a;
                quick_sort(_a, ra, 0, _a.size() - 1);
        }
//------------------------------------------------------------------------------
        /** \brief Коэффициент корреляции Спирмена
         * Коэффициент корреляции Спирмена - мера линейной связи между случайными величинами.
         * Корреляция Спирмена является ранговой, то есть для оценки силы связи используются не численные значения, а соответствующие им ранги.
         * Коэффициент инвариантен по отношению к любому монотонному преобразованию шкалы измерения.
         * \param x первая выборка данных
         * \param y вторая выборка данных
         * \param p коэффициент корреляции Спирмена (от -1 до +1)
         * \return вернет 0 в случае успеха
         */
        template<class T1, class T2, class T3>
        int calculate_spearman_rank_correlation_coefficient(std::vector<T1>& x, std::vector<T2> &y, T3 &p)
        {
                if(x.size() != y.size() || x.size() == 0) {
                        return INVALID_PARAMETER;
                }
                // найдем ранги элементов
                std::vector<int> rx(x.size());
                std::vector<int> ry(y.size());
                quick_sort(x, rx);
                quick_sort(y, ry);
                T3 sum = 0;
                for(size_t i = 0; i < x.size(); ++i) {
                        T3 diff = rx[i] - ry[i];
                        sum += diff * diff;
                }

                T3 n = x.size();
                p = 1.0 - (6.0 /(n * (n - 1) * (n + 1))) * sum;
                return OK;
        }
//------------------------------------------------------------------------------
}

#endif // CORRELATIONEASY_HPP_INCLUDED
