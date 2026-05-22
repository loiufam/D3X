#ifndef COUNT_RESULT_H_
#define COUNT_RESULT_H_

#include <gmpxx.h>

#include <algorithm>
#include <string>

struct CountResult {
    mpz_class count;

    CountResult(unsigned long c) : count(c) {}

    CountResult(const mpz_class& c) : count(c) {}

    CountResult(const std::string& s) : count(s) {}

    CountResult() : count(0) {}

    bool isZero() const {
        return count == 0;
    }

    bool isFailure() const {
        return isZero();
    }

    std::string toString() const {
        return count.get_str(10);
    }

    std::string toScientificString(int precision = 6) const {
        std::string str = count.get_str(10);
        if (str.length() <= 1) return str;

        std::string result = "";
        result += str[0];
        result += ".";

        int availableFracLen = static_cast<int>(str.length()) - 1;
        int actualFracLen = std::min(availableFracLen, precision);
        result += str.substr(1, actualFracLen);

        if (actualFracLen < precision) {
            result.append(precision - actualFracLen, '0');
        }

        result += "e+" + std::to_string(availableFracLen);
        return result;
    }

    CountResult operator*(const CountResult& other) const {
        if (this->isZero() || other.isZero()) {
            return CountResult(0);
        }
        return CountResult(this->count * other.count);
    }

    CountResult operator+(const CountResult& other) const {
        if (this->isZero()) return other;
        if (other.isZero()) return *this;
        return CountResult(this->count + other.count);
    }

    CountResult& operator+=(const CountResult& other) {
        if (other.isZero()) return *this;
        count += other.count;
        return *this;
    }
};

#endif  // COUNT_RESULT_H_