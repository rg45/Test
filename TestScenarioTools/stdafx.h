#pragma once

// Development/Debug utility
#define PRINT(ex) (std::cout << #ex" = " << (ex) << std::endl)

#define PRINT_CAPTION() (std::cout << "\n========== " __FUNCSIG__ " =========="<< std::endl)


