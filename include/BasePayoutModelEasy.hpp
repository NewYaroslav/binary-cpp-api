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
#ifndef BASEPAYOUTMODELEASY_HPP_INCLUDED
#define BASEPAYOUTMODELEASY_HPP_INCLUDED
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
namespace BasePayoutModelEasy
{
        class BasePayout
        {
        public:
//------------------------------------------------------------------------------
                virtual BasePayout()
                {

                }
//------------------------------------------------------------------------------
                /** \brief Инициализировать класс процентов выплат
                 * \param paths массив файлов настроек для модели процентов выплат
                 * \return вернет 0 в случае успеха
                 */
                virtual int init(std::vector<std::string> paths)
                {
                        return 0;
                }
//------------------------------------------------------------------------------
                /** \brief Инициализировать базовый класс процентов выплат
                 * \param paths массив файлов настроек для модели процентов выплат
                 */
                virtual BasePayout(std::vector<std::string> paths)
                {
                        init(paths);
                }
//------------------------------------------------------------------------------
                /** \brief Получить процент выплат
                 * \param payout ссылка на процент выплат
                 * \param timestamp временная метка
                 * \param contract_type тип контракта
                 * \param symbol_indx индекс символа (номер валютной пары)
                 * \return вернет 0 в случае успеха
                 */
                virtual int get_payout(double& payout, unsigned long long timestamp, int contract_type, int symbol_indx)
                {
                        return 0;
                }
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
        };
//------------------------------------------------------------------------------
        class OlympTradePayout : public BasePayout
        {

        };
}
//------------------------------------------------------------------------------
#endif // BASEPAYOUTMODELEASY_HPP_INCLUDED
