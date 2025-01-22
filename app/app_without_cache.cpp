#include <iostream>
#include "ema-search-int.h"

int main() {
    try {
        ema_search_int_without_cache();
        return 0;
    } catch (std::runtime_error &e) {
        std::cout << e.what() << std::endl;
        return 1;
    }
}
