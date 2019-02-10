#include "imgui.h"
#include "imgui-SFML.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/CircleShape.hpp>

#include "BinaryOptionsEasy.hpp"

#include <thread>
#include <mutex>
#include <cstdlib>
#include <atomic>
#include <random>
//------------------------------------------------------------------------------
std::mutex data_mutex;
std::atomic<bool> is_test;
std::atomic<bool> is_test_ok;
//------------------------------------------------------------------------------
void check_min_max(double& value, double min_value, double max_value);
double frand(double fmin, double fmax);
//------------------------------------------------------------------------------
int main()
{
    BinaryOptionsEasy::TestDataIn iTestOptions(1000, 1, 0.5);
    BinaryOptionsEasy::TestDataOut iTestDataKelly;
    BinaryOptionsEasy::TestDataOut iTestDataMartingale;
    BinaryOptionsEasy::TestDataOut iTestDataMiller;
    std::default_random_engine re;
    is_test = false;
    is_test_ok = false;
    double profit = 80;
    double strategy_eff = 60;
    double martingale_stake = 1;
    double win_rate = 57;
    //
    sf::RenderWindow window(sf::VideoMode(1200, 720), "Martingale does not work");
    window.setFramerateLimit(60);
    ImGui::SFML::Init(window);

    sf::Clock deltaClock;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);

            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        ImGui::Begin("Martingale does not work");

        ImGui::Text("Trading options");
        ImGui::PushItemWidth(128);
        ImGui::InputDouble("Deposit", &iTestOptions.start_depo, 1, 100, "%.0f");
        if(iTestOptions.start_depo < iTestOptions.min_amount*2) iTestOptions.start_depo = iTestOptions.min_amount*2;
        ImGui::InputDouble("Binary options payout (%)", &profit, 1, 1, "%.1f");
        check_min_max(profit, 0, 200);
        ImGui::InputDouble("Minimum order size", &iTestOptions.min_amount, 1, 100, "%.1f");
        check_min_max(iTestOptions.min_amount, 0, iTestOptions.start_depo/2);
        ImGui::InputDouble("Win rate (%)", &win_rate, 1, 100, "%.1f");
        check_min_max(win_rate, 0, 100);
        ImGui::PopItemWidth();
        ImGui::Separator();
        ImGui::Text("Kelly criterion");
        ImGui::PushItemWidth(128);
        ImGui::InputDouble("Strategy effectiveness (%)", &strategy_eff, 1, 1, "%.1f");
        check_min_max(strategy_eff, 0, 100);
        iTestOptions.average_strategy_eff = strategy_eff / 100.0;
        ImGui::InputDouble("Attenuation coefficient", &iTestOptions.kelly_criterion_attenuation, 0.01, 1, "%.2f");
        check_min_max(iTestOptions.kelly_criterion_attenuation, 0, 1);
        ImGui::Text("Mathematical expectation of profit: %.3f", BinaryOptionsEasy::calc_math_expectation_profit(
                win_rate/100.0,
                profit/100.0));
        ImGui::Text("Deposit interest rate: %.3f %%", BinaryOptionsEasy::calc_deposit_rate_kelly_criterion(
                iTestOptions.average_strategy_eff, profit/100.0,
                iTestOptions.kelly_criterion_attenuation) * 100.0);
        ImGui::Text("Minimum win rate: %.2f %%", BinaryOptionsEasy::calc_min_strategy_eff(profit/100.0) * 100.0);
        ImGui::PopItemWidth();
        ImGui::Separator();
        ImGui::Text("Martingale");
        ImGui::PushItemWidth(128);
        ImGui::InputInt("Number of martingale steps", &iTestOptions.martingale_max_step, 1, 1);
        if(iTestOptions.martingale_max_step < 1) iTestOptions.martingale_max_step = 1;
        ImGui::InputDouble("Coefficient for martingale strategy", &iTestOptions.martingale_coeff, 0.001, 0.1, "%.3f");
        check_min_max(iTestOptions.martingale_coeff, 1, 10);
        ImGui::InputDouble("Deposit interest rate (%)", &martingale_stake, 0.01, 1, "%.1f");
        check_min_max(martingale_stake, 100.0 * iTestOptions.min_amount/iTestOptions.start_depo, 100.0);
        iTestOptions.martingale_stake = martingale_stake/100.0;
        ImGui::PopItemWidth();
        ImGui::Separator();
        if(ImGui::Button("Start testing trading") && !is_test) {
                is_test_ok = false;
                is_test = true;
                std::thread test_thread([&,iTestOptions,profit,win_rate]() {
                        BinaryOptionsEasy::TradingSimulator iSimulator;
                        std::uniform_real_distribution<double> unif(0,1.0);
                        //std::default_random_engine re;
                        const int MAX_DEALS = 10000;
                        double _profit = profit/100.0;
                        double _win_rate = win_rate/100.0;
                        for(int i = 0; i < MAX_DEALS; ++i) {
                                int status = unif(re) < _win_rate ? BinaryOptionsEasy::WIN : BinaryOptionsEasy::LOSS;
                                iSimulator.open_virtual_deal(_profit, i, status);
                        }
                        //data_mutex.lock();
                        iSimulator.test_kelly_criterion(iTestOptions,iTestDataKelly);
                        iSimulator.test_martingale(iTestOptions,iTestDataMartingale);
                        iSimulator.test_miller_system(iTestOptions,iTestDataMiller);
                        //data_mutex.unlock();
                        is_test_ok = true;
                        is_test = false;
                });
                test_thread.detach();
        }
        ImGui::End();

        if(is_test_ok) {
                const int PLOT_W = 512;
                const int PLOT_H = 128*2;
                ImGui::Begin("Results (Kelly criterion)");
                ImGui::Text("Gain: %.1f", iTestDataKelly.gain);
                ImGui::Text("Wins: %d Losses: %d Deals: %d", iTestDataKelly.num_wins, iTestDataKelly.num_losses, iTestDataKelly.num_deals);
                ImGui::Text("Win rate: %.1f %%", iTestDataKelly.eff * 100);
                if(iTestDataKelly.array_depo.size() > 0) {
                        float* array_depo = &iTestDataKelly.array_depo[0];
                        ImGui::PlotLines("##Kelly criterion", array_depo, iTestDataKelly.array_depo.size(), 0, "Kelly criterion", FLT_MAX, FLT_MAX, ImVec2(PLOT_W,PLOT_H));
                }
                ImGui::End();

                ImGui::Begin("Results (Martingale)");
                ImGui::Text("Gain: %.1f", iTestDataMartingale.gain);
                ImGui::Text("Wins: %d Losses: %d Deals: %d", iTestDataMartingale.num_wins, iTestDataMartingale.num_losses, iTestDataMartingale.num_deals);
                ImGui::Text("Win rate: %.1f %%", iTestDataMartingale.eff * 100);
                if(iTestDataMartingale.array_depo.size() > 0) {
                        float* array_depo = &iTestDataMartingale.array_depo[0];
                        ImGui::PlotLines("##Martingale", array_depo, iTestDataMartingale.array_depo.size(), 0, "Martingale", FLT_MAX, FLT_MAX, ImVec2(PLOT_W,PLOT_H));
                }
                ImGui::End();

                ImGui::Begin("Results (Miller System)");
                ImGui::Text("Gain: %.1f", iTestDataMiller.gain);
                ImGui::Text("Wins: %d Losses: %d Deals: %d", iTestDataMiller.num_wins, iTestDataMiller.num_losses, iTestDataMiller.num_deals);
                ImGui::Text("Win rate: %.1f %%", iTestDataMiller.eff * 100);
                if(iTestDataMiller.array_depo.size() > 0) {
                        float* array_depo = &iTestDataMiller.array_depo[0];
                        ImGui::PlotLines("##Miller System", array_depo, iTestDataMiller.array_depo.size(), 0, "Miller System", FLT_MAX, FLT_MAX, ImVec2(PLOT_W,PLOT_H));
                }
                ImGui::End();
        }

        window.clear();
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
}

void check_min_max(double& value, double min_value, double max_value)
{
    if(value > max_value) value = max_value;
    else
    if(value < min_value) value = min_value;
}

double frand(double fmin, double fmax)
{
    double f = (double)rand() / RAND_MAX;
    return fmin + f * (fmax - fmin);
}
