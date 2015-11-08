
#include <iostream>
#include <random>
#include <array>
#include <fletcher.h>

int main(int argc, char* argv[])
{
    using namespace LibFDupe;

    std::default_random_engine generator;
    generator.seed(2222);
    //std::uniform_int_distribution<uint32_t> distribution(0,255);
    std::uniform_int_distribution<uint32_t> distribution(0,(1ul<<32)-1);

    std::array<uint32_t,2048> p={};

    for (auto& r : p) {
        r = distribution(generator);
    }

    for (auto it = p.begin(); it != p.end(); it++) {
        // std::cout << (int)(*it) << " ";
    }
    std::cout << std::endl;

    auto c = fletcher64(reinterpret_cast<uint32_t*>(&p[0]), 512);
    std::cout << c.lower << ":" << c.upper << std::endl;
    std::cout << std::endl;

    const auto c1 = fletcher64(reinterpret_cast<uint32_t*>(&p[0]), 200);
    const auto c2 = fletcher64(reinterpret_cast<uint32_t*>(&p[200]), 100);
    const auto c3 = fletcher64(reinterpret_cast<uint32_t*>(&p[300]), 212);
    //std::cout << "C1: " << c1.lower << ":" << c1.upper << std::endl;
    auto c4 = c1;
    c4 << c2 << c3;
    std::cout << c4.lower << ":" << c4.upper << std::endl;

    c4 = { 0, 0, 0 };
    for (int i = 0; i < 1500000; i++) {
        c4 << fletcher64(reinterpret_cast<uint32_t*>(&p[0]), 1024);
    }
    std::cout << c4.lower << ":" << c4.upper << std::endl;
}

