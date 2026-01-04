
#ifndef MATH_UTILS_HPP
#define MATH_UTILS_HPP

#include <cmath>

class MathUtils {
public:
    static double squareRoot(double num) {
        return std::sqrt(num);
    }

    static double power(double base, double exponent) {
        return std::pow(base, exponent);
    }

    static unsigned long long factorial(unsigned int num) {
        if (num <= 1) return 1;
        return num * factorial(num - 1);
    }
};

#endif
