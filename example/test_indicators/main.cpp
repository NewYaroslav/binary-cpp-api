#include "IndicatorsEasy.hpp"
#include "HistoricalDataEasy.hpp"

int main() {
        std::string path = "..//..//train_2//quotes_bars_data//frxAUDCAD";
        HistoricalDataEasy::HistoricalData hist(path);
        hist.read_all_data();

        int PERIOD_MA = 10;
        int PERIOD_MW = 10;
        int PERIOD_RSI = 10;
        IndicatorsEasy::WMA<double> wma(PERIOD_MA);
        IndicatorsEasy::SMA<double> sma(PERIOD_MA);
        IndicatorsEasy::EMA<double> ema(PERIOD_MA);
        IndicatorsEasy::MW<double> mw(PERIOD_MW);
        IndicatorsEasy::RSI<double, IndicatorsEasy::EMA<double>> rsi(PERIOD_RSI);
        const int CANDLE_SIZE = 60;

        int status = 0;
        while(status != hist.END_OF_DATA) {
                double price = 0;
                unsigned long long timestamp = 0;
                hist.get_price(price, timestamp, status, CANDLE_SIZE);
                std::cout << "price: " << price << std::endl;
                if(status == hist.SKIPPING_DATA) {
                        wma.clear();
                        sma.clear();
                        ema.clear();
                        mw.clear();
                        rsi.clear();
                }
                if(status == hist.NORMAL_DATA) {
                        double wma_out, sma_out, ema_out, rsi_out;
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
                        std::vector<double> mw_out;
                        if(mw.update(price, mw_out) == hist.OK) {
                                std::cout << "mw:";
                                for(int i = 0; i < PERIOD_MW; ++i) {
                                        std::cout << " " << mw_out[i];
                                }
                                std::cout << std::endl;
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
                }
                std::cout << std::endl;
        }
        return 0;
}

