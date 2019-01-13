#include "BinaryAPI.hpp"
#include "BinaryApiEasy.hpp"
#include "ZstdEasy.hpp"

#define BUILD_VER 1.1

using json = nlohmann::json;

void save_json(std::string file_name, json &j);
void open_json(std::string file_name, json &j);
std::string format(const char *fmt, ...);

std::vector<std::string> get_all_symbols_binary();
std::vector<std::string> get_proposal_symbols_binary();

void make_commit(std::string disk, std::string path, std::string new_file);

int main() {
        std::cout << "build version " << (float)BUILD_VER << std::endl;

        BinaryAPI iBinaryApi;
        BinaryAPI iBinaryApiForTime;
        BinaryAPI iBinaryApiForQuotes;

        iBinaryApi.set_use_log(true);
        iBinaryApiForTime.set_use_log(true);
        iBinaryApiForQuotes.set_use_log(true);

        std::mutex make_commit_mutex_;

        json j_settings;
        open_json("settings.json", j_settings);
        std::cout << std::setw(4) << j_settings << std::endl;

        std::vector<std::string> symbols = get_all_symbols_binary();
        std::vector<std::string> symbols_proposal = get_proposal_symbols_binary();
        // инициализируем список валютных пар
        std::cout << "init_symbols..." << std::endl;
        iBinaryApi.init_symbols(symbols_proposal);
        //
        // инициализируем поток процентов выплат
        const double amount = j_settings["amount"];
        const int duration = j_settings["duration"];
        const int duration_uint = j_settings["duration_uint"];
        const std::string currency = j_settings["currency"];
        const std::string folder_name_proposal = j_settings["folder_proposal"];
        const std::string folder_name_quotes_ticks = j_settings["folder_quotes_ticks"];
        const std::string folder_name_quotes_bars = j_settings["folder_quotes_bars"];
        const std::string disk_name = j_settings["disk"];
        const std::string path = j_settings["path"];
        const std::string dictionary_quotes_ticks_file = j_settings["dictionary_quotes_ticks_file"];
        const std::string dictionary_quotes_bars_file = j_settings["dictionary_quotes_bars_file"];
        int is_use_git = j_settings["git"];
        std::string old_file_name = "";

        // сохраним список валютных пар и параметры в отдельный файл
        json j_pp;
        j_pp["amount"] = amount;
        j_pp["duration"] = duration;
        j_pp["duration_uint"] = duration_uint;
        j_pp["currency"] = currency;
        j_pp["symbols"] = symbols_proposal;

        json j_qp;
        j_qp["symbols"] = symbols;

        std::string folder_path_quotes_ticks = disk_name + ":\\" + path + "\\" + folder_name_quotes_ticks;
        std::string folder_path_quotes_bars = disk_name + ":\\" + path + "\\" + folder_name_quotes_bars;
        std::string folder_path_proposal = disk_name + ":\\" + path + "\\" + folder_name_proposal;
        std::string file_name_qt = folder_path_quotes_ticks + "\\parameters.json";
        std::string file_name_qb = folder_path_quotes_bars + "\\parameters.json";
        std::string file_name_pp = folder_path_proposal + "\\parameters.json";

        bf::create_directory(folder_path_quotes_ticks);
        bf::create_directory(folder_path_quotes_bars);
        bf::create_directory(folder_path_proposal);
        save_json(file_name_pp, j_pp);
        save_json(file_name_qb, j_qp);
        save_json(file_name_qt, j_qp);

        if(is_use_git) {
                make_commit(disk_name, path, file_name_pp);
                make_commit(disk_name, path, file_name_qb);
                make_commit(disk_name, path, file_name_qt);
        }
        //
        std::cout << "..." << std::endl;
        unsigned long long servertime_last = 0;
        while(true) {
                // для всех валютных пар
                std::vector<double> buy_data; // проценты выплат
                std::vector<double> sell_data;
                unsigned long long servertime = 0;

                if(!iBinaryApi.is_proposal_stream()) {
                        // инициализируем поток процентов выплат
                        std::cout << "init_stream_proposal..." << std::endl;
                        int err_data = iBinaryApi.init_stream_proposal(amount, duration, duration_uint, currency);
                        std::cout << "err: " << err_data << std::endl;
                        if(err_data != iBinaryApi.OK)
                                continue;
                }

                if(iBinaryApiForTime.get_servertime(servertime) != iBinaryApiForTime.OK) {
                        iBinaryApiForTime.request_servertime();
                        std::this_thread::sleep_for(std::chrono::milliseconds(450));
                        continue;
                } else {
                        if(servertime > servertime_last) {
                                servertime_last = servertime;
                        } else {
                                std::this_thread::yield();
                                continue;
                        }
                }
#               if(1)
                if(iBinaryApi.get_stream_proposal(buy_data, sell_data) == iBinaryApi.OK) {
                        // выводим последние значения котировок валютных пар
                        for(size_t i = 0; i < symbols_proposal.size(); ++i) {
                                std::cout << symbols_proposal[i] << " " << buy_data[i] << "/" << sell_data[i] << std::endl;
                        }
                        // сохраняем список
                        mkdir(folder_path_proposal.c_str());
                        xtime::DateTime iTime(servertime);
                        std::string file_chunk_name = "proposal_" +
                                                      std::to_string(iTime.day) + "_" +
                                                      std::to_string(iTime.month) + "_" +
                                                      std::to_string(iTime.year);

                        const std::string file_name = folder_path_proposal + "\\" +
                                                      file_chunk_name +
                                                      ".hex";
                        // если не выходной, сохраняем файлы
                        if(!xtime::is_day_off(servertime)) {
                                BinaryApiEasy::write_binary_proposal_file(
                                        file_name,
                                        symbols_proposal,
                                        duration,
                                        duration_uint,
                                        currency,
                                        buy_data,
                                        sell_data,
                                        servertime);
                        }

                        if(old_file_name != file_name) {
                                if(is_use_git == 1) {
                                        std::thread make_thread([&,disk_name, path, file_name, old_file_name]() {
                                                if(old_file_name == "") {
                                                        make_commit_mutex_.lock();
                                                        make_commit(disk_name, path, file_name);
                                                        make_commit_mutex_.unlock();
                                                } else {
                                                        make_commit_mutex_.lock();
                                                        make_commit(disk_name, path, old_file_name);
                                                        make_commit_mutex_.unlock();
                                                }
                                        });
                                        make_thread.detach();
                                }

                                std::thread download_thread([&, disk_name, path, folder_path_quotes_bars, folder_path_quotes_ticks, servertime, symbols, is_use_git]() {
                                        for(size_t s = 0; s < symbols.size(); ++s) {
                                                ZstdEasy::download_and_save_all_data_with_compression(
                                                        iBinaryApiForQuotes,
                                                        symbols[s],
                                                        folder_path_quotes_bars + "//" + symbols[s],
                                                        dictionary_quotes_bars_file,
                                                        servertime,
                                                        true,
                                                        BinaryApiEasy::QUOTES_BARS,
                                                        BinaryApiEasy::standart_user_function);
                                                //const int DELAY = 2000;
                                                //std::this_thread::sleep_for(std::chrono::milliseconds(DELAY));
                                        }

                                        if(is_use_git == 1) {
                                                make_commit_mutex_.lock();
                                                make_commit(disk_name, path, folder_path_quotes_bars);
                                                make_commit_mutex_.unlock();
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
                                                        BinaryApiEasy::standart_user_function);
                                                //const int DELAY = 2000;
                                                //std::this_thread::sleep_for(std::chrono::milliseconds(DELAY));
                                        }

                                        if(is_use_git == 1) {
                                                make_commit_mutex_.lock();
                                                make_commit(disk_name, path, folder_path_quotes_ticks);
                                                make_commit_mutex_.unlock();
                                        }
                                });
                                download_thread.detach();

                                old_file_name = file_name;
                        } // if
                }
#               endif
                // время сервера в GMT\UTC
                std::cout << servertime << std::endl;
                std::cout << std::endl;
                std::this_thread::yield();
        }
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

void make_commit(std::string disk, std::string path, std::string new_file)
{
        //make_commit_mutex_.lock();
        std::string str_return_hd = "cd " + disk + ":\\";
        std::cout << str_return_hd << std::endl;

        std::string str_cd_path = "cd " + path;
        std::cout << str_cd_path << std::endl;

        std::string str_git_add = "git add " + new_file;
        std::cout << str_git_add << std::endl;

        std::string str_git_commit = "git commit -a -m \"update\"";
        std::cout << str_git_commit << std::endl;

        std::string str_git_pull = "git pull";
        std::cout << str_git_pull << std::endl;

        std::string str_git_push = "git push";
        std::cout << str_git_push << std::endl;

        std::string msg = str_return_hd + " && " + str_cd_path + " && " + str_git_add + " && " + str_git_commit + " && " + str_git_pull + " & " + str_git_push;
        system(msg.c_str());
        //make_commit_mutex_.unlock();
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
