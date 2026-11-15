#include "prx/libSceImeDialog/include/Api.hpp"

#include <cstdio>
#include <cstdlib>

namespace {

void Require(bool value, const char* what) {
    if (value) return;
    std::fprintf(stderr, "%s\n", what);
    std::abort();
}

} // namespace

int main() {
    const int first = sceImeDialogForTestFunction();
    const int second = sceImeDialogForTestFunction();
    Require(first == static_cast<int>(0x80bc00ffu), "sceImeDialogForTestFunction did not answer ErrorInternal");
    Require(second == static_cast<int>(0x80bc00ffu), "sceImeDialogForTestFunction changed its error on a repeated call");
    return 0;
}
