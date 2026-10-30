#include "prx/libSceContentDelete/include/Api.hpp"
#include "SceTypes.hpp"
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <string>

namespace {

void Require(bool value) { if (!value) std::abort(); }

template <typename TAction>
std::string LogicErrorMessage(TAction action) {
    try {
        action();
    } catch (const std::invalid_argument&) {
        return "invalid argument";
    } catch (const std::logic_error& error) {
        return error.what();
    }
    return "returned";
}

}

int main() {
    Require(sceContentDeleteTerminate() == static_cast<int>(0x809D5002u));
    Require(sceContentDeleteInitialize(nullptr) == static_cast<int>(0x809D5001u));
    Require(sceContentDeleteById(1) == static_cast<int>(0x809D5002u));
    Require(sceContentDeleteById(0) == static_cast<int>(0x809D5002u));
    Require(sceContentDeleteById(-1) == static_cast<int>(0x809D5002u));
    Require(sceContentDeleteByPath("/user/av_contents/photo/a.png") == static_cast<int>(0x809D5002u));
    Require(sceContentDeleteByPath(nullptr) == static_cast<int>(0x809D5002u));
    Require(sceContentDeleteByPath("") == static_cast<int>(0x809D5002u));

    ContentDeleteInitParam param{};
    for (const std::uint32_t invalidHeap : {0u, 0x3fffu, 0x4001u, 0x8000u, 0x10000u, 0xffffffffu}) {
        param.heap_size = invalidHeap;
        Require(sceContentDeleteInitialize(&param) == static_cast<int>(0x809D5001u));
    }
    param.heap_size = 0x4000;
    Require(sceContentDeleteInitialize(&param) == 0);
    Require(sceContentDeleteInitialize(&param) == static_cast<int>(0x809D5003u));
    Require(sceContentDeleteInitialize(nullptr) == static_cast<int>(0x809D5001u));
    for (const std::uint32_t invalidHeap : {0u, 0x3fffu, 0x4001u, 0x8000u, 0x10000u, 0xffffffffu}) {
        param.heap_size = invalidHeap;
        Require(sceContentDeleteInitialize(&param) == static_cast<int>(0x809D5001u));
    }
    param.heap_size = 0x4000;
    Require(sceContentDeleteByPath("") == static_cast<int>(0x809D5001u));

    Require(LogicErrorMessage([] { sceContentDeleteByPath(nullptr); }) == "invalid argument");

    Require(sceContentDeleteTerminate() == 0);
    Require(sceContentDeleteTerminate() == static_cast<int>(0x809D5002u));
    Require(sceContentDeleteById(1) == static_cast<int>(0x809D5002u));
    Require(sceContentDeleteById(0) == static_cast<int>(0x809D5002u));
    Require(sceContentDeleteById(-1) == static_cast<int>(0x809D5002u));
    Require(sceContentDeleteByPath(nullptr) == static_cast<int>(0x809D5002u));
    Require(sceContentDeleteByPath("/user/av_contents/photo/a.png") == static_cast<int>(0x809D5002u));
    Require(sceContentDeleteByPath("") == static_cast<int>(0x809D5002u));

    Require(sceContentDeleteInitialize(&param) == 0);
    Require(sceContentDeleteByPath("") == static_cast<int>(0x809D5001u));
    Require(LogicErrorMessage([] { sceContentDeleteByPath(nullptr); }) == "invalid argument");
    Require(sceContentDeleteTerminate() == 0);
    Require(sceContentDeleteTerminate() == static_cast<int>(0x809D5002u));
    return 0;
}
