#pragma once

#include <string>

namespace rgb_tool::core {

int ParseInt(const std::wstring& text);
int ClampInt(int value, int low, int high);

} // namespace rgb_tool::core
