#include "BinaryAPI.hpp"
#include "BinaryApiEasy.hpp"
#include "ZstdEasy.hpp"

#define BUILD_VER 1.2

using json = nlohmann::json;

void save_json(std::string file_name, json &j);
void open_json(std::string file_name, json &j);

std::vector<std::string> get_all_symbols_binary();

void user_function_quotes_ticks(std::string str, std::vector<double> &prices, std::vector<unsigned long long> &times, unsigned long long timestamp)
{
        std::cout << "write binary ticks file: " << str << std::endl;
}

void user_function_quotes_bars(std::string str, std::vector<double> &prices, std::vector<unsigned long long> &times, unsigned long long timestamp)
{
        std::cout << "write binary quotes file: " << str << std::endl;
}

int main() {
        std::cout << "build version " << (float)BUILD_VER << std::endl;
        json j_settings;
        open_json("settings.json", j_settings);
        std::cout << std::setw(4) << j_settings << std::endl;

        std::vector<std::string> symbols = get_all_symbols_binary();

        const std::string folder_name_quotes_ticks = j_settings["folder_quotes_ticks"];
        const std::string folder_name_quotes_bars = j_settings["folder_quotes_bars"];
        const std::string disk_name = j_settings["disk"];
        const std::string path = j_settings["path"];
        const std::string dictionary_quotes_ticks_file = j_settings["dictionary_quotes_ticks_file"];
        const std::string dictionary_quotes_bars_file = j_settings["dictionary_quotes_bars_file"];

        // сохраним список валютных пар и параметры в отдельный файл
        json j_qp;
        j_qp["symbols"] = symbols;
        // получим директории и имена файлов
        std::string folder_path_quotes_ticks = disk_name + ":\\" + path + "\\" + folder_name_quotes_ticks;
        std::string folder_path_quotes_bars = disk_name + ":\\" + path + "\\" + folder_name_quotes_bars;
        std::string file_name_qt = folder_path_quotes_ticks + "\\parameters.json";
        std::string file_name_qb = folder_path_quotes_bars + "\\parameters.json";
        // созазтм необходимые директории
        bf::create_directory(folder_path_quotes_ticks);
        bf::create_directory(folder_path_quotes_bars);
        // сохраним файлы настроек
        save_json(file_name_qb, j_qp);
        save_json(file_name_qt, j_qp);

        unsigned long long servertime = xtime::get_unix_timestamp();
        // запустим загрузку котировок в несколько потоков
        int num_threads = std::thread::hardware_concurrency();
        std::cout << "hardware concurrency: " << num_threads << std::endl;
        std::vector<std::thread> threads(num_threads);
        for(int t = 0; t < num_threads; ++t) {
                threads[t] = std::thread([&, disk_name, path, folder_path_quotes_bars, folder_path_quotes_ticks, servertime, symbols, t, num_threads]() {
                        BinaryAPI iBinaryApiForQuotes;
                        iBinaryApiForQuotes.set_use_log(true);
                        for(size_t s = t; s < symbols.size(); s += num_threads) {
                                bool is_skip_day_off = true;
                                // проверим, есть ли смысл загружать котировки за выходные дни
                                if(symbols[s] == "R_100" || symbols[s] == "R_50" || symbols[s] == "R_25" || symbols[s] == "R_10") {
                                        is_skip_day_off = false;
                                }
                                ZstdEasy::download_and_save_all_data_with_compression(
                                        iBinaryApiForQuotes,
                                        symbols[s],
                                        folder_path_quotes_ticks + "//" + symbols[s],
                                        dictionary_quotes_ticks_file,
                                        servertime,
                                        is_skip_day_off,
                                        BinaryApiEasy::QUOTES_TICKS,
                                        user_function_quotes_ticks);

                                ZstdEasy::download_and_save_all_data_with_compression(
                                        iBinaryApiForQuotes,
                                        symbols[s],
                                        folder_path_quotes_bars + "//" + symbols[s],
                                        dictionary_quotes_bars_file,
                                        servertime,
                                        true,
                                        BinaryApiEasy::QUOTES_BARS,
                                        user_function_quotes_bars);
                        } // for s
                });
                //download_thread.detach();
        } // for t
        for(int t = 0; t < num_threads; ++t) {
                threads[t].join();
        }
        std::cout << "The program has completed" << std::endl;
        std::cin;
        return 0;
}

std::vector<std::string> get_all_symbols_binary() {
        std::vector<std::string> vsymbol;
        vsymbol.push_back("WLDAUD");
        vsymbol.push_back("WLDEUR");
        vsymbol.push_back("WLDGBP");
        vsymbol.push_back("WLDUSD");

        vsymbol.push_back("frxAUDCAD");
        vsymbol.push_back("frxAUDCHF");
        vsymbol.push_back("frxAUDJPY");
        vsymbol.push_back("frxAUDNZD");
        vsymbol.push_back("frxAUDUSD");

        vsymbol.push_back("frxEURAUD");
        vsymbol.push_back("frxEURCAD");
        vsymbol.push_back("frxEURCHF");
        vsymbol.push_back("frxEURGBP");
        vsymbol.push_back("frxEURJPY");
        vsymbol.push_back("frxEURNZD");
        vsymbol.push_back("frxEURUSD");

        vsymbol.push_back("frxGBPAUD");
        vsymbol.push_back("frxGBPCAD");
        vsymbol.push_back("frxGBPCHF");
        vsymbol.push_back("frxGBPJPY");

        vsymbol.push_back("frxGBPNOK");
        vsymbol.push_back("frxGBPNZD");
        vsymbol.push_back("frxGBPPLN");
        vsymbol.push_back("frxGBPUSD");

        vsymbol.push_back("frxNZDJPY");
        vsymbol.push_back("frxNZDUSD");

        vsymbol.push_back("frxUSDCAD");
        vsymbol.push_back("frxUSDCHF");
        vsymbol.push_back("frxUSDJPY");
        vsymbol.push_back("frxUSDNOK");
        vsymbol.push_back("frxUSDPLN");
        vsymbol.push_back("frxUSDSEK");

        vsymbol.push_back("R_100");
        vsymbol.push_back("R_50");
        vsymbol.push_back("R_25");
        vsymbol.push_back("R_10");
        return vsymbol;
}

void save_json(std::string file_name, json &j) {
        std::ofstream file(file_name);
        file << std::setw(4) << j << std::endl;
        file.close();
}

void open_json(std::string file_name, json &j) {
        std::ifstream file(file_name);
        file >> j;
        file.close();
}
