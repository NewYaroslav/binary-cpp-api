#include "IndicatorsEasy.hpp"
#include "NormalizationEasy.hpp"
#include "CorrelationEasy.hpp"
#include "HistoricalDataEasy.hpp"

int main() {
        std::string path = "..//..//train_2//quotes_bars_data//frxEURGBP";
        HistoricalDataEasy::HistoricalData hist(path);
        hist.read_all_data();

        int PERIOD_MA = 10;
        int PERIOD_MW = 10;
        int PERIOD_RSI = 5;
        int PERIOD_BB = 20;
        double STD_DEV_BB = 2;
        IndicatorsEasy::WMA<double> wma(PERIOD_MA);
        IndicatorsEasy::SMA<double> sma(PERIOD_MA);
        IndicatorsEasy::EMA<double> ema(PERIOD_MA);
        IndicatorsEasy::MW<double> mw(PERIOD_MW);
        IndicatorsEasy::RSI<double, IndicatorsEasy::EMA<double>> rsi(PERIOD_RSI);
        IndicatorsEasy::BollingerBands<double> bb(PERIOD_BB, STD_DEV_BB);
        IndicatorsEasy::DetectorWaveform<double> dw(100);
        /*
                30, 240, 0.6
        */
        const int CANDLE_SIZE = 60;

        int status = 0;
        while(status != hist.END_OF_DATA) {
                double price = 0;
                unsigned long long timestamp = 0;
                hist.get_price(price, timestamp, status, CANDLE_SIZE);
                //std::cout << "price: " << price << std::endl;
                if(status == hist.SKIPPING_DATA) {
                        wma.clear();
                        sma.clear();
                        ema.clear();
                        mw.clear();
                        rsi.clear();
                        bb.clear();
                        dw.clear();
                }
                if(status == hist.NORMAL_DATA) {
                        double wma_out, sma_out, ema_out, rsi_out, bb_tl, bb_ml, bb_bl;
                        double dw_out;
                        if(rsi.update(price, rsi_out) ==  hist.OK) {
                                //std::cout << "rsi: " << rsi_out << std::endl;
                        }
                        if(dw.update(price, dw_out, 20) == hist.OK) {
                                //std::cout << "dw: " << dw_out << std::endl;
                                if(abs(rsi_out) > 80)
                                if(abs(dw_out) > 0.8) {
                                        int state = 0;
                                        if(dw_out > 0) {
                                                hist.check_binary_option(state, hist.SELL, 180, timestamp);
                                        } else
                                        if(dw_out < 0) {
                                                hist.check_binary_option(state, hist.BUY, 180, timestamp);
                                        }
                                        static int num_sum  = 0;
                                        static double sum = 0;
                                        if(state == 1) {
                                                sum += 1.0;
                                        }
                                        num_sum++;
                                        std::cout << "eff " << (sum / (double)num_sum) << std::endl;
                                        std::cout << num_sum << std::endl;
                                        std::cout << xtime::get_str_unix_date_time(timestamp) << std::endl;
                                        //int zz;
                                        //std::cin >> zz;
                                        //std::cout << zz << std::endl;
                                }
                        }
#                       if (0)
                        if(wma.update(price, wma_out) == hist.OK) {
                                std::cout << "wma: " << wma_out << std::endl;
                        }
                        if(sma.update(price, sma_out) == hist.OK) {
                                std::cout << "sma: " << sma_out << std::endl;
                        }
                        if(ema.update(price, ema_out) == hist.OK) {
                                std::cout << "ema: " << ema_out << std::endl;
                        }
                        if(rsi.update(price, rsi_out) ==  hist.OK) {
                                std::cout << "rsi: " << rsi_out << std::endl;
                        }
                        if(bb.update(price, bb_tl, bb_ml, bb_bl) == hist.OK) {
                                std::cout << "bb: " << bb_tl << " " << bb_ml << " " << bb_bl << std::endl;
                        }
                        std::vector<double> mw_out;
                        if(mw.update(price, mw_out) == hist.OK) {
                                std::cout << "mw:";
                                for(int i = 0; i < PERIOD_MW; ++i) {
                                        std::cout << " " << mw_out[i];
                                }
                                std::cout << std::endl;
                                std::vector<double> mw_norm_out;
                                NormalizationEasy::calculate_min_max(mw_out, mw_norm_out, NormalizationEasy::MINMAX_1_1);
                                std::cout << "mw norm:";
                                for(size_t i = 0; i < mw_norm_out.size(); ++i) {
                                        std::cout << " " << mw_norm_out[i];
                                        mw_norm_out[i] += i * 0.1;
                                }
                                std::cout << std::endl;

                                double p = 0;
                                CorrelationEasy::calculate_spearman_rank_correlation_coefficient(mw_out, mw_norm_out, p);
                                std::cout << "spearman corr: " << p << std::endl;

                                double temp;
                                mw.get_std_data(temp);
                                std::cout << "mw std: " << temp << std::endl;
                                mw.get_average_data(temp);
                                std::cout << "mw aver: " << temp << std::endl;
                                mw.get_max_data(temp);
                                std::cout << "mw max: " << temp << std::endl;
                                mw.get_min_data(temp);
                                std::cout << "mw min: " << temp << std::endl;
                                std::cout << std::endl;
                        }
#                       endif
                }
                //std::cout << std::endl;
        }
        return 0;
}

