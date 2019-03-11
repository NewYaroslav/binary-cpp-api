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
#ifndef ALGORITHMSEASY_HPP_INCLUDED
#define ALGORITHMSEASY_HPP_INCLUDED
//------------------------------------------------------------------------------
#include <algorithm>
#include <vector>
//------------------------------------------------------------------------------
namespace AlgorithmsEasy
{
//------------------------------------------------------------------------------
        /** \brief Расстояние Левенштайна между двумя векторами
         * \code
                const std::string src = "125";
                const std::string dst = "521";
                const std::string::size_type distance = calc_levenstein_distance(src, dst);
         * \endcode
         * \param src вектор для сравннеия
         * \param dst исходный вектор с которым сравниваем
         * \return расстояние Левенштайна
         */
        template<class T>
        typename T::value_type calc_levenstein_distance(const T &src, const T &dst)
        {
                const typename T::size_type m = src.size();
                const typename T::size_type n = dst.size();
                if(m == 0)
                        return n;
                if(n == 0)
                        return m;
                std::vector<std::vector<class T::value_type>> matrix(m + 1, std::vector<class T::value_type> (n + 1));
                for(typename T::size_type i = 0; i <= m; ++i) {
                        matrix [i].resize(n + 1);
                        matrix [i][0] = i;
                }
                for(typename T::size_type j = 0; j <= n; ++j) {
                        matrix[0][j] = j;
                }
                typename T::value_type above_cell, left_cell, diagonal_cell, cost;
                for(typename T::size_type i = 1; i <= m; ++i) {
                        for(typename T::size_type j = 1; j <= n; ++j) {
                                cost = src[i-1] == dst[j-1] ? 0 : 1;
                                above_cell = matrix[i-1][j];
                                left_cell = matrix[i][j-1];
                                diagonal_cell = matrix[i-1][j-1];
                                matrix[i][j] = std::min(std::min(above_cell + 1, left_cell + 1), diagonal_cell + cost);
                        }
                }
                return matrix[m][n];
        }
//------------------------------------------------------------------------------
        /** \brief
         *
         * \param
         * \param
         * \return
         *
         */
        template<typename T>
        typename T::size_type calc_generalized_levenstein_distance(
                const T &source,
                const T &target,
                typename T::size_type insert_cost = 1,
                typename T::size_type delete_cost = 1,
                typename T::size_type replace_cost = 1)
        {
                if (source.size() > target.size()) {
                        return calc_generalized_levenstein_distance(target, source, delete_cost, insert_cost, replace_cost);
                }

                using TSizeType = typename T::size_type;
                const TSizeType min_size = source.size(), max_size = target.size();
                std::vector<TSizeType> lev_dist(min_size + 1);

                lev_dist[0] = 0;
                for (TSizeType i = 1; i <= min_size; ++i) {
                        lev_dist[i] = lev_dist[i - 1] + delete_cost;
                }

                for (TSizeType j = 1; j <= max_size; ++j) {
                        TSizeType previous_diagonal = lev_dist[0], previous_diagonal_save;
                        lev_dist[0] += insert_cost;

                        for (TSizeType i = 1; i <= min_size; ++i) {
                                previous_diagonal_save = lev_dist[i];
                                if (source[i - 1] == target[j - 1]) {
                                        lev_dist[i] = previous_diagonal;
                                } else {
                                        lev_dist[i] = std::min(std::min(lev_dist[i - 1] + delete_cost, lev_dist[i] + insert_cost), previous_diagonal + replace_cost);
                                }
                                previous_diagonal = previous_diagonal_save;
                        }
                }

                return lev_dist[min_size];
        }
//------------------------------------------------------------------------------
        template<class T>
        calc_dtw(const std::vector<T> &q, const std::vector<T> &c)
        {
                size_t n = q.size();
                size_t m = c.size();
                if(n == 0 || m == 0)
                        return;
                std::vector<std::vector<T>> distance_matrix(n, std::vector<T>(m));
                // находим матрицу расстояний
                for(size_t i = 0; i < n; ++i) {
                        for(size_t j = 0; j < m; ++j) {
                                T temp = q[i] - c[j];
                                distance_matrix[i][j] = temp * temp;
                        }
                }
                std::vector<std::vector<T>> transformation_matrix(n, std::vector<T>(m));
                // находим матрицу трансформаций
                transformation_matrix[0][0] = distance_matrix[0][0];
                for(size_t j = 1; j < m; ++j) {
                        transformation_matrix[0][j] = distance_matrix[0][j] +
                                transformation_matrix[0][j - 1];
                }
                for(size_t i = 1; i < n; ++i) {
                        transformation_matrix[i][0] = distance_matrix[i][0] +
                                transformation_matrix[i - 1][0];
                }
                for(size_t i = 1; i < n; ++i) {
                        for(size_t j = 1; j < m; ++j) {
                                transformation_matrix[i][j] = distance_matrix[i][j] +
                                        std::min(std::min(transformation_matrix[i - 1][j],
                                                transformation_matrix[i - 1][j - 1]),
                                                transformation_matrix[i][j - 1]);
                        }
                }
                // ищем оптимальный путь
                size_t pos_n = n - 1, pos_m = m - 1;
                std::vector<T> w;
                w.push_back(transformation_matrix[pos_n][pos_m]);
                while(true) {
                        if(pos_n == 0 && pos_m == 0)
                                break;
                        if(pos_n == 0) {
                                w.push_back(transformation_matrix[pos_n][pos_m--]);
                                continue;
                        }
                        if(pos_m == 0) {
                                w.push_back(transformation_matrix[pos_n--][pos_m]);
                                continue;
                        }
                        T a = transformation_matrix[pos_n - 1][pos_m - 1];
                        T b = transformation_matrix[pos_n ][pos_m - 1];
                        T c = transformation_matrix[pos_n - 1][pos_m];

                        if(a <= b) {
                                if(a <= c) {
                                        w.push_back(a);
                                        pos_n--;
                                        pos_m--;
                                } else {
                                        w.push_back(c);
                                        pos_n--;
                                }
                        } else
                        if(b < a) {
                                if(b < c) {


                                }
                        }
                }
        }
//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------
#endif // ALGORITHMSEASY_HPP_INCLUDED
