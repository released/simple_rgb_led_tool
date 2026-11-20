#include "text_utils.h"

#include <cwctype>
#include <limits>
#include <stdexcept>

namespace rgb_tool::core {

int ParseInt(const std::wstring& text) {
    std::wstring s;
    size_t idx = 0;
    int base = 10;
    int sign = 1;
    unsigned int value = 0u;

    for (wchar_t ch : text) {
        if (!iswspace(ch)) {
            s.push_back(ch);
        }
    }
    if (s.empty()) {
        throw std::invalid_argument("empty integer string");
    }

    if (s[idx] == L'-' || s[idx] == L'+') {
        sign = s[idx] == L'-' ? -1 : 1;
        ++idx;
    }
    if (s.size() >= idx + 2u && s[idx] == L'0' && (s[idx + 1u] == L'x' || s[idx + 1u] == L'X')) {
        base = 16;
        idx += 2u;
    }
    if (idx == s.size()) throw std::invalid_argument("integer has no digits");
    const unsigned int limit = static_cast<unsigned int>((std::numeric_limits<int>::max)()) +
                               (sign < 0 ? 1u : 0u);

    for (; idx < s.size(); ++idx) {
        wchar_t ch = s[idx];
        int digit = -1;
        if (ch >= L'0' && ch <= L'9') {
            digit = ch - L'0';
        } else if (base == 16 && ch >= L'a' && ch <= L'f') {
            digit = 10 + ch - L'a';
        } else if (base == 16 && ch >= L'A' && ch <= L'F') {
            digit = 10 + ch - L'A';
        } else {
            throw std::invalid_argument("invalid integer character");
        }
        if (digit >= base) {
            throw std::invalid_argument("invalid digit for base");
        }
        if (value > (limit - static_cast<unsigned int>(digit)) / static_cast<unsigned int>(base)) {
            throw std::out_of_range("integer overflow");
        }
        value = value * static_cast<unsigned int>(base) + static_cast<unsigned int>(digit);
    }

    if (sign < 0 && value == limit) return (std::numeric_limits<int>::min)();
    return sign < 0 ? -static_cast<int>(value) : static_cast<int>(value);
}

int ClampInt(int value, int low, int high) {
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

} // namespace rgb_tool::core
