#include "CorrelationEasy.hpp"
#include <iostream>

int main() {
        for(int i = 0; i < 30; ++i) {
                if(CorrelationEasy::table_critical_t_points[i*6 + 1] != CorrelationEasy::get_critical_t_points(CorrelationEasy::BI_CRITICAl_AREA_0P05, i + 1)) {
                        std::cout << "error! " << (i + 1) << std::endl;
                }
        }
        for(int i = 30; i < 47; ++i) {
                int degrees_freedom = ((i - 30) * 10) + 30 + 1;
                if(CorrelationEasy::table_critical_t_points[(i - 1)*6 + 1] != CorrelationEasy::get_critical_t_points(CorrelationEasy::BI_CRITICAl_AREA_0P05, degrees_freedom)) {
                        std::cout << "error! " << degrees_freedom << std::endl;
                        std::cout << "t " << CorrelationEasy::table_critical_t_points[(i - 1)*6 + 1] << std::endl;
                        std::cout << "g " << CorrelationEasy::get_critical_t_points(CorrelationEasy::BI_CRITICAl_AREA_0P05, degrees_freedom) << std::endl;
                }
        }
        for(int i = 47; i < 52; ++i) {
                int degrees_freedom = ((i - 47) * 20) + 200 + 1;
                if(CorrelationEasy::table_critical_t_points[(i - 1)*6 + 1] != CorrelationEasy::get_critical_t_points(CorrelationEasy::BI_CRITICAl_AREA_0P05, degrees_freedom)) {
                        std::cout << "error! " << degrees_freedom << std::endl;
                        std::cout << "t " << CorrelationEasy::table_critical_t_points[(i - 1)*6 + 1] << std::endl;
                        std::cout << "g " << CorrelationEasy::get_critical_t_points(CorrelationEasy::BI_CRITICAl_AREA_0P05, degrees_freedom) << std::endl;
                }
        }
        for(int i = 52; i < 59; ++i) {
                int degrees_freedom = ((i - 52) * 100) + 300 + 99;
                if(CorrelationEasy::table_critical_t_points[(i - 1)*6 + 1] != CorrelationEasy::get_critical_t_points(CorrelationEasy::BI_CRITICAl_AREA_0P05, degrees_freedom)) {
                        std::cout << "error! " << degrees_freedom << std::endl;
                        std::cout << "t " << CorrelationEasy::table_critical_t_points[(i - 1)*6 + 1] << std::endl;
                        std::cout << "g " << CorrelationEasy::get_critical_t_points(CorrelationEasy::BI_CRITICAl_AREA_0P05, degrees_freedom) << std::endl;
                }
        }
        for(int i = 59; i < 68; ++i) {
                int degrees_freedom = ((i - 59) * 1000) + 1000 + 1;
                if(CorrelationEasy::table_critical_t_points[(i - 1)*6 + 1] != CorrelationEasy::get_critical_t_points(CorrelationEasy::BI_CRITICAl_AREA_0P05, degrees_freedom)) {
                        std::cout << "error! " << degrees_freedom << std::endl;
                        std::cout << "t " << CorrelationEasy::table_critical_t_points[(i - 1)*6 + 1] << std::endl;
                        std::cout << "g " << CorrelationEasy::get_critical_t_points(CorrelationEasy::BI_CRITICAl_AREA_0P05, degrees_freedom) << std::endl;
                }
        }


        std::vector<double> x = {1,0,7,0,-1,0,5};
        std::cout << "size x " << x.size() << std::endl;

        std::vector<double> xp(x.size());
        CorrelationEasy::calculate_spearmen_ranking(x, xp);
        std::cout << "size xp " << xp.size() << std::endl;
        for(size_t i = 0; i < xp.size(); ++i) {
                std::cout << xp[i] << std::endl;
        }
        double summ = std::accumulate(xp.begin(), xp.end(), double(0));
        std::cout << "summ  " << summ << std::endl;
        double n = xp.size();
        double calc_summ = ((n + 1.0) * n) / 2.0;
        std::cout << "calc_summ  " << calc_summ << std::endl;
        CorrelationEasy::calculate_reshaping_ranks(xp);

        for(size_t i = 0; i < xp.size(); ++i) {
                std::cout << xp[i]  << std::endl;
        }
        summ = std::accumulate(xp.begin(), xp.end(), double(0));
        std::cout << "summ  " << summ << std::endl;


        std::cout << "repetitions_rank " << CorrelationEasy::calculate_repetitions_rank(xp) << std::endl;

        std::vector<double> x1 = {1,2,3,4,5,6,7};
        std::vector<double> y1 = {1,2,3,4,1,6,7};
        double cc;
        CorrelationEasy::calculate_spearman_rank_correlation_coefficient(x1, y1, cc);
        std::cout << "spearmam " << cc << std::endl;
        double t_criterion;
        t_criterion = CorrelationEasy::calculate_significance_correlation_coefficient_t_criterion(cc, x1.size());
        std::cout << "t_criterion " << t_criterion << std::endl;

        std::cout << "check " << CorrelationEasy::check_correlation_coefficient_t_criterion(cc, CorrelationEasy::BI_CRITICAl_AREA_0P05, x1.size());
        return 0;
}

