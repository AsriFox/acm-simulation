#include <iostream>
#include "Tools/version.h"
#include "aff3ct.hpp"

int main(int, char**) {
    std::cout << "Using AFF3CT " << aff3ct::tools::version() << std::endl;
    return 0;
}
