#include "prx/libSceNpCommon/include/Api.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <limits>
#include <cstdio>
#include <cstdlib>
#include <source_location>
#include <stdexcept>

static void Require(bool value, std::source_location location = std::source_location::current()) {
    if (!value) {
        std::fprintf(stderr, "check failed at line %u\n", location.line());
        std::abort();
    }
}

static void CheckRingBufCheckoutNullBlockSize() {
    std::array<std::uint8_t, 16> storage{};
    NpRingBuf ring{};
    _ZN3sce2np7RingBufC1Ev(&ring);
    Require(_ZN3sce2np7RingBuf4InitEPvm(&ring, storage.data(), storage.size()) == 0);

    bool writeThrew = false;
    try {
        (void)_ZN3sce2np7RingBuf16CheckoutForWriteEPm(&ring, nullptr);
    } catch (const std::invalid_argument&) {
        writeThrew = true;
    }
    Require(writeThrew);

    bool emptyReadThrew = false;
    try {
        (void)_ZN3sce2np7RingBuf15CheckoutForReadEPm(&ring, nullptr);
    } catch (const std::invalid_argument&) {
        emptyReadThrew = true;
    }
    Require(emptyReadThrew);
    Require(ring.size == 0 && ring.readPointer == storage.data() && ring.writePointer == storage.data());

    const std::array<std::uint8_t, 5> payload{1, 2, 3, 4, 5};
    Require(_ZN3sce2np7RingBuf5WriteEPKvm(&ring, payload.data(), payload.size()) == 0);
    const auto* const savedRead = ring.readPointer;
    const auto* const savedWrite = ring.writePointer;
    const std::size_t savedSize = ring.size;

    bool nonEmptyReadThrew = false;
    try {
        (void)_ZN3sce2np7RingBuf15CheckoutForReadEPm(&ring, nullptr);
    } catch (const std::invalid_argument&) {
        nonEmptyReadThrew = true;
    }
    Require(nonEmptyReadThrew);
    Require(ring.size == savedSize && ring.readPointer == savedRead && ring.writePointer == savedWrite &&
            ring.buf == storage.data() && ring.bufSize == storage.size());

    std::size_t blockSize = 0;
    const auto* const checkedOut = static_cast<const std::uint8_t*>(_ZN3sce2np7RingBuf15CheckoutForReadEPm(&ring, &blockSize));
    Require(checkedOut != nullptr && blockSize == payload.size() &&
            std::memcmp(checkedOut, payload.data(), payload.size()) == 0);

    std::array<std::uint8_t, 5> readBack{};
    Require(_ZN3sce2np7RingBuf4ReadEPvm(&ring, readBack.data(), readBack.size()) == 0 &&
            readBack == payload && ring.size == 0);

    _ZN3sce2np7RingBuf7DestroyEv(&ring);
    _ZN3sce2np7RingBufD1Ev(&ring);
}

static void CheckRingBufCtorNullSelf() {
    errno = 71;

    bool nullThrew = false;
    try {
        _ZN3sce2np7RingBufC1Ev(nullptr);
    } catch (const std::invalid_argument&) {
        nullThrew = true;
    }
    Require(nullThrew);
    Require(errno == 71);

    bool aliasThrew = false;
    try {
        _ZN3sce2np7RingBufC2Ev(nullptr);
    } catch (const std::invalid_argument&) {
        aliasThrew = true;
    }
    Require(aliasThrew);

    NpRingBuf ring{};
    std::memset(&ring, 0x5A, sizeof(ring));
    const auto savedVtable = ring.vtable;
    const auto savedBuf = ring.buf;
    const auto savedReadPointer = ring.readPointer;
    const auto savedWritePointer = ring.writePointer;
    const std::size_t savedSize = ring.size;
    const std::size_t savedBufSize = ring.bufSize;

    bool untouchedThrew = false;
    try {
        _ZN3sce2np7RingBufC1Ev(nullptr);
    } catch (const std::invalid_argument&) {
        untouchedThrew = true;
    }
    Require(untouchedThrew && errno == 71);
    Require(ring.vtable == savedVtable && ring.buf == savedBuf && ring.readPointer == savedReadPointer &&
            ring.writePointer == savedWritePointer && ring.size == savedSize && ring.bufSize == savedBufSize);

    std::array<std::uint8_t, 16> storage{};
    NpRingBuf live{};
    _ZN3sce2np7RingBufC1Ev(&live);
    Require(live.vtable != nullptr && live.buf == nullptr && live.readPointer == nullptr &&
            live.writePointer == nullptr && live.size == 0 && live.bufSize == 0);
    Require(_ZN3sce2np7RingBuf4InitEPvm(&live, storage.data(), storage.size()) == 0);
    _ZN3sce2np7RingBuf7DestroyEv(&live);
    _ZN3sce2np7RingBufD1Ev(&live);
}

int main() {
    constexpr int tooSmall = static_cast<int>(0x80558008);
    const std::array<unsigned char, 4> input{0, 0x7f, 0x80, 0xff};
    std::array<char, 16> output{};
    for (const auto length : {std::numeric_limits<std::size_t>::max() / 2 + 1,
                              std::numeric_limits<std::size_t>::max()}) {
        output.fill('!');
        Require(sceNpStrBuildHex(output.data() + 1, 8, input.data(), length, false) == tooSmall);
        Require(std::all_of(output.begin(), output.end(), [](char c) { return c == '!'; }));
    }
    for (const bool uppercase : {false, true}) {
        for (std::size_t count = 0; count <= input.size(); ++count) {
            for (std::size_t capacity = 0; capacity <= count * 2 + 2; ++capacity) {
                output.fill('!');
                const auto result = sceNpStrBuildHex(output.data() + 1, capacity, input.data(), count, uppercase);
                Require(output.front() == '!' && output[1 + capacity] == '!');
                if (capacity <= count * 2) {
                    Require(result == tooSmall);
                    Require(std::all_of(output.begin(), output.end(), [](char c) { return c == '!'; }));
                } else {
                    Require(result == 0 && output[1 + count * 2] == '\0');
                    const char* expected = uppercase ? "007F80FF" : "007f80ff";
                    Require(std::memcmp(output.data() + 1, expected, count * 2) == 0);
                }
            }
        }
    }
    CheckRingBufCheckoutNullBlockSize();
    CheckRingBufCtorNullSelf();
}