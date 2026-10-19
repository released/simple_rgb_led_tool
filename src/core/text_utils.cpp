#include "text_utils.h"

#include <cwctype>
#include <stdexcept>

namespace rgb_tool::core {

int ParseInt(const std::wstring& text) {
    std::wstring s;
    size_t idx = 0;
    int base = 10;
    int sign = 1;
    long long value = 0;

    for (wchar_t ch : text) {
        if (!iswspace(ch)) {
            s.push_back(ch);
        }
    }
    if (s.empty()) {
        throw std::invalid_argument("empty integer string");
    }

    if (s.size() > 2u && s[0] == L'0' && (s[1] == L'x' || s[1] == L'X')) {
        base = 16;
        idx = 2u;
    }
    if (idx < s.size() && s[idx] == L'-') {
        sign = -1;
        ++idx;
    }

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
        value = value * base + digit;
    }

    return static_cast<int>(value * sign);
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
