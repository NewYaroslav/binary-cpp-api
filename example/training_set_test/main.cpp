#include "BinaryAPI.hpp"
#include "BinaryApiEasy.hpp"
#include "ZstdEasy.hpp"

int main() {
#if(0)
        std::cout << "start train" << std::endl;
        int err = ZstdEasy::train_zstd("..//..//frxAUDCAD","test.zstdt");
        std::cout << "stop train " << err << std::endl;
#endif
        ZstdEasy::compress_file("..//..//frxEURGBP//2015_12_14.hex", "compress5.hex", "test.zstdt");
        ZstdEasy::decompress_file("compress5.hex", "decompress5.hex", "test.zstdt");
}

