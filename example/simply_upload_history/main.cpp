#include "BinaryAPI.hpp"
#include "BinaryApiEasy.hpp"
#include "ZstdEasy.hpp"

#define BUILD_VER 1.0

using json = nlohmann::json;

void save_json(std::string file_name, json &j);
void open_json(std::string file_name, json &j);
std::string format(const char *fmt, ...);

std::vector<std::string> get_all_symbols_binary();
std::vector<std::string> get_proposal_symbols_binary();

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
        BinaryAPI iBinaryApiForQuotes;
        iBinaryApiForQuotes.set_use_log(true);

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

        std::string folder_path_quotes_ticks = disk_name + ":\\" + path + "\\" + folder_name_quotes_ticks;
        std::string folder_path_quotes_bars = disk_name + ":\\" + path + "\\" + folder_name_quotes_bars;
        std::string file_name_qt = folder_path_quotes_ticks + "\\parameters.json";
        std::string file_name_qb = folder_path_quotes_bars + "\\parameters.json";

        bf::create_directory(folder_path_quotes_ticks);
        bf::create_directory(folder_path_quotes_bars);
        save_json(file_name_qb, j_qp);
        save_json(file_name_qt, j_qp);

        std::cout << "..." << std::endl;
        unsigned long long servertime = xtime::get_unix_timestamp();
        std::thread download_thread([&, disk_name, path, folder_path_quotes_bars, folder_path_quotes_ticks, servertime, symbols]() {
                for(size_t s = 0; s < symbols.size(); ++s) {
                        ZstdEasy::download_and_save_all_data_with_compression(
                                iBinaryApiForQuotes,
                                symbols[s],
                                folder_path_quotes_bars + "//" + symbols[s],
                                dictionary_quotes_bars_file,
                                servertime,
                                true,
                                BinaryApiEasy::QUOTES_BARS,
                                user_function_quotes_bars);
                }
                for(size_t s = 0; s < symbols.size(); ++s) {
                        ZstdEasy::download_and_save_all_data_with_compression(
                                iBinaryApiForQuotes,
                                symbols[s],
                                folder_path_quotes_ticks + "//" + symbols[s],
                                dictionary_quotes_ticks_file,
                                servertime,
                                true,
                                BinaryApiEasy::QUOTES_TICKS,
                                user_function_quotes_ticks);
                }
        });
        download_thread.join();
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
        return vsymbol;
}

std::vector<std::string> get_proposal_symbols_binary() {
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

        ///vsymbol.push_back("frxGBPNOK");
        vsymbol.push_back("frxGBPNZD");
        ///vsymbol.push_back("frxGBPPLN");
        ///vsymbol.push_back("frxGBPUSD");

        ///vsymbol.push_back("frxNZDJPY");
        vsymbol.push_back("frxNZDUSD");

        vsymbol.push_back("frxUSDCAD");
        ///vsymbol.push_back("frxUSDCHF");
        vsymbol.push_back("frxUSDJPY");
        ///vsymbol.push_back("frxUSDNOK");
        ///vsymbol.push_back("frxUSDPLN");
        ///vsymbol.push_back("frxUSDSEK");
        return vsymbol;
}

std::string format(const char *fmt, ...)
{
        va_list args;
        va_start(args, fmt);
        std::vector<char> v(1024);
        while (true)
        {
                va_list args2;
                va_copy(args2, args);
                int res = vsnprintf(v.data(), v.size(), fmt, args2);
                if ((res >= 0) && (res < static_cast<int>(v.size())))
                {
                    va_end(args);
                    va_end(args2);
                    return std::string(v.data());
                }
                size_t size;
                if (res < 0)
                    size = v.size() * 2;
                else
                    size = static_cast<size_t>(res) + 1;
                v.clear();
                v.resize(size);
                va_end(args2);
        }
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
