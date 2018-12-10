#include "BinaryAPI.hpp"
#include "BinaryApiEasy.hpp"
#include "ZstdEasy.hpp"

int main() {
#if(0)
        std::cout << "start train" << std::endl;
        int err = ZstdEasy::train_zstd("..//..//train","quotes_ticks.zstd", 100 * 1024);
        std::cout << "stop train " << err << std::endl;
        return 0;
#endif
        std::string dictionary_file = "quotes_ticks.zstd";
        ZstdEasy::compress_file("..//..//train//frxEURGBP//2015_12_14.hex", "compress6.hex", dictionary_file);
        ZstdEasy::decompress_file("compress6.hex", "decompress6.hex", dictionary_file);
        std::vector<double> prices;
        std::vector<unsigned long long> times;
        std::cout << "read_binary_quotes_compress_file" << std::endl;
        ZstdEasy::read_binary_quotes_compress_file("compress6.hex", dictionary_file, prices, times);

        //for(size_t i = 0; i < times.size(); ++i) {
        std::cout << prices.back() << " " << times.back() << std::endl;
        //}
         std::cout << "write_binary_quotes_compressed_file" << std::endl;
        ZstdEasy::write_binary_quotes_compressed_file("compress6.hex", dictionary_file, prices, times);

        std::cout << "read_binary_quotes_compress_file" << std::endl;
        ZstdEasy::read_binary_quotes_compress_file("compress6.hex", dictionary_file, prices, times);
        std::cout << prices.back() << " " << times.back() << std::endl;
}

