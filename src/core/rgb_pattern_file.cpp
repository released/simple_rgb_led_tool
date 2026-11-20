#include "rgb_pattern.h"

#include <algorithm>
#include <cstdio>
#include <cwctype>
#include <sstream>
#include <stdexcept>
#include <memory>
#include <set>
#include <windows.h>
#include <objbase.h>

#pragma comment(lib, "ole32.lib")

#include "text_utils.h"

namespace rgb_tool::core {
namespace {

std::wstring Trim(const std::wstring& text) {
    size_t begin = 0u;
    size_t end = text.size();
    while (begin < end && iswspace(text[begin])) {
        ++begin;
    }
    while (end > begin && iswspace(text[end - 1u])) {
        --end;
    }
    return text.substr(begin, end - begin);
}

bool StartsWith(const std::wstring& text, const std::wstring& prefix) {
    return text.size() >= prefix.size() && text.compare(0u, prefix.size(), prefix) == 0;
}

std::wstring ValueAfter(const std::wstring& line, const std::wstring& prefix) {
    if (!StartsWith(line, prefix)) {
        return L"";
    }
    return Trim(line.substr(prefix.size()));
}

int HexDigit(wchar_t ch) {
    if (ch >= L'0' && ch <= L'9') {
        return ch - L'0';
    }
    if (ch >= L'a' && ch <= L'f') {
        return 10 + ch - L'a';
    }
    if (ch >= L'A' && ch <= L'F') {
        return 10 + ch - L'A';
    }
    return -1;
}

bool ParseColorToken(const std::wstring& token, RgbColor* color) {
    int digits[6] = {};
    size_t start = 0u;
    size_t i = 0u;

    if (color == nullptr) {
        return false;
    }
    if (token.size() >= 2u && token[0] == L'0' && (token[1] == L'x' || token[1] == L'X')) {
        start = 2u;
    }
    if (token.size() - start != 6u) {
        return false;
    }
    for (i = 0u; i < 6u; ++i) {
        digits[i] = HexDigit(token[start + i]);
        if (digits[i] < 0) {
            return false;
        }
    }
    color->r = static_cast<std::uint8_t>((digits[0] << 4) | digits[1]);
    color->g = static_cast<std::uint8_t>((digits[2] << 4) | digits[3]);
    color->b = static_cast<std::uint8_t>((digits[4] << 4) | digits[5]);
    return true;
}

std::wstring JoinPath(const std::wstring& directory, const std::wstring& file_name) {
    if (directory.empty()) {
        return file_name;
    }
    if (directory.back() == L'\\' || directory.back() == L'/') {
        return directory + file_name;
    }
    return directory + L"\\" + file_name;
}

class StagedTextFile {
public:
    ~StagedTextFile() {
        if (handle_ != INVALID_HANDLE_VALUE) CloseHandle(handle_);
        if (!temporary_.empty()) DeleteFileW(temporary_.c_str());
    }

    bool Prepare(const std::wstring& path, const std::wstring& text) {
        if (path.empty() || path.find(L'\0') != std::wstring::npos) return false;
        wchar_t full_path[32768] = {};
        const DWORD length = GetFullPathNameW(path.c_str(), 32768u, full_path, nullptr);
        if (length == 0u || length >= 32768u || text.size() > 32u * 1024u * 1024u) return false;
        target_ = full_path;
        const DWORD attributes = GetFileAttributesW(target_.c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES &&
            (attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) != 0u) return false;
        existed_ = attributes != INVALID_FILE_ATTRIBUTES;
        if (!existed_ && GetLastError() != ERROR_FILE_NOT_FOUND) return false;
        if (existed_ && !TargetIdentity(&original_)) return false;

        GUID identity{};
        wchar_t guid[40] = {};
        if (FAILED(CoCreateGuid(&identity)) || StringFromGUID2(identity, guid, 40) == 0) return false;
        const std::wstring candidate = target_.substr(0u, target_.find_last_of(L"\\/") + 1u) +
                                       L".rgb-export-" + guid + L".tmp";
        handle_ = CreateFileW(candidate.c_str(), GENERIC_WRITE, 0u, nullptr, CREATE_NEW,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
        if (handle_ == INVALID_HANDLE_VALUE) return false;
        temporary_ = candidate;
        const int bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(),
                                              static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
        if (!text.empty() && bytes <= 0) return false;
        std::string utf8(static_cast<size_t>(bytes), '\0');
        if (bytes > 0 && WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(),
            static_cast<int>(text.size()), utf8.data(), bytes, nullptr, nullptr) != bytes) return false;
        DWORD written = 0u;
        const bool saved = WriteFile(handle_, utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr) &&
                           written == utf8.size() && FlushFileBuffers(handle_);
        const bool closed = CloseHandle(handle_) != FALSE;
        handle_ = INVALID_HANDLE_VALUE;
        return saved && closed;
    }

    bool Commit() {
        if (existed_) {
            BY_HANDLE_FILE_INFORMATION current{};
            if (!TargetIdentity(&current) ||
                original_.dwVolumeSerialNumber != current.dwVolumeSerialNumber ||
                original_.nFileIndexHigh != current.nFileIndexHigh || original_.nFileIndexLow != current.nFileIndexLow ||
                original_.nFileSizeHigh != current.nFileSizeHigh || original_.nFileSizeLow != current.nFileSizeLow ||
                CompareFileTime(&original_.ftLastWriteTime, &current.ftLastWriteTime) != 0) return false;
        }
        const bool committed = existed_
            ? ReplaceFileW(target_.c_str(), temporary_.c_str(), nullptr, 0u, nullptr, nullptr) != FALSE
            : MoveFileExW(temporary_.c_str(), target_.c_str(), MOVEFILE_WRITE_THROUGH) != FALSE;
        if (committed) temporary_.clear();
        return committed;
    }

private:
    bool TargetIdentity(BY_HANDLE_FILE_INFORMATION* identity) const {
        HANDLE file = CreateFileW(target_.c_str(), FILE_READ_ATTRIBUTES,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
            FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        if (file == INVALID_HANDLE_VALUE) return false;
        const bool read = GetFileInformationByHandle(file, identity) != FALSE &&
            (identity->dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY)) == 0u;
        const bool closed = CloseHandle(file) != FALSE;
        return read && closed;
    }

    BY_HANDLE_FILE_INFORMATION original_{};
    HANDLE handle_ = INVALID_HANDLE_VALUE;
    std::wstring target_;
    std::wstring temporary_;
    bool existed_ = false;
};

bool ReadPatternText(const std::wstring& path, std::wstring* text) {
    if (path.empty() || path.find(L'\0') != std::wstring::npos) return false;
    HANDLE input = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (input == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER length{};
    if (!GetFileSizeEx(input, &length) || length.QuadPart <= 0 ||
        length.QuadPart > static_cast<LONGLONG>(kMaxPatternFileBytes)) {
        CloseHandle(input);
        return false;
    }
    std::string bytes;
    try { bytes.resize(static_cast<size_t>(length.QuadPart)); }
    catch (...) { CloseHandle(input); throw; }
    DWORD received = 0u;
    const bool read = ReadFile(input, bytes.data(), static_cast<DWORD>(bytes.size()), &received, nullptr) &&
                      received == bytes.size();
    const bool closed = CloseHandle(input) != FALSE;
    if (!read || !closed || bytes.find('\0') != std::string::npos) return false;
    UINT encoding = CP_UTF8;
    DWORD flags = MB_ERR_INVALID_CHARS;
    int chars = MultiByteToWideChar(encoding, flags, bytes.data(), static_cast<int>(bytes.size()), nullptr, 0);
    if (chars == 0) {
        // Legacy V1 files were written with the Windows ANSI code page.
        encoding = CP_ACP;
        flags = 0u;
        chars = MultiByteToWideChar(encoding, flags, bytes.data(), static_cast<int>(bytes.size()), nullptr, 0);
    }
    if (chars <= 0) return false;
    text->resize(static_cast<size_t>(chars));
    if (MultiByteToWideChar(encoding, flags, bytes.data(), static_cast<int>(bytes.size()), text->data(), chars) != chars) return false;
    if (!text->empty() && text->front() == 0xFEFF) text->erase(0u, 1u);
    return true;
}

} // namespace

bool SaveTextFile(const std::wstring& path, const std::wstring& text, std::wstring* error) {
    StagedTextFile output;
    if (!output.Prepare(path, text) || !output.Commit()) {
        if (error != nullptr) *error = L"Output could not be committed: " + path;
        return false;
    }
    if (error != nullptr) error->clear();
    return true;
}

bool SaveGeneratedSourceFiles(const std::wstring& directory, const std::vector<GeneratedSourceFile>& files, std::wstring* error) {
    size_t i = 0u;
    std::set<std::wstring> names;
    std::vector<std::unique_ptr<StagedTextFile>> staged;

    if (directory.empty() || directory.find(L'\0') != std::wstring::npos || files.empty()) {
        if (error != nullptr) {
            *error = L"Output directory is empty.";
        }
        return false;
    }
    for (const GeneratedSourceFile& file : files) {
        std::wstring name = file.file_name;
        if (name.empty() || name == L"." || name == L".." ||
            name.find_first_of(L"\\/:<>\"|?*") != std::wstring::npos ||
            name.find(L'\0') != std::wstring::npos || name.back() == L'.' || name.back() == L' ') {
            if (error != nullptr) *error = L"Invalid generated filename.";
            return false;
        }
        for (wchar_t& ch : name) ch = static_cast<wchar_t>(towupper(ch));
        const std::wstring stem = name.substr(0u, name.find(L'.'));
        const bool device = stem == L"CON" || stem == L"PRN" || stem == L"AUX" || stem == L"NUL" ||
            (stem.size() == 4u && (stem.compare(0u, 3u, L"COM") == 0 || stem.compare(0u, 3u, L"LPT") == 0) &&
             stem[3] >= L'1' && stem[3] <= L'9');
        if (device || name.find_first_of(L"\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F") != std::wstring::npos) {
            if (error != nullptr) *error = L"Reserved or invalid generated filename.";
            return false;
        }
        if (!names.insert(name).second) {
            if (error != nullptr) *error = L"Duplicate generated filename.";
            return false;
        }
    }
    for (i = 0u; i < files.size(); ++i) {
        const std::wstring path = JoinPath(directory, files[i].file_name);
        auto output = std::make_unique<StagedTextFile>();
        if (!output->Prepare(path, files[i].text)) {
            if (error != nullptr) *error = L"Output preparation failed; no files committed: " + path;
            return false;
        }
        staged.push_back(std::move(output));
    }
    for (i = 0u; i < staged.size(); ++i) {
        if (!staged[i]->Commit()) {
            if (error != nullptr) *error = L"Output commit failed after " + std::to_wstring(i) +
                L" of " + std::to_wstring(staged.size()) + L" files. Committed files remain updated: " + files[i].file_name;
            return false;
        }
    }
    if (error != nullptr) error->clear();
    return true;
}

bool SavePatternFile(const RgbPattern& pattern, const std::wstring& path, std::wstring* error) {
    if (!ValidatePattern(pattern, error)) return false;
    std::wstringstream ss;
    size_t frame_index = 0u;

    ss << L"RGB_PATTERN_V1\r\n";
    ss << L"name=" << pattern.name << L"\r\n";
    ss << L"led_count=" << pattern.led_count << L"\r\n";
    ss << L"shape=" << ShapePersistName(pattern.shape) << L"\r\n";
    ss << L"start_index=" << pattern.start_index << L"\r\n";
    ss << L"direction=" << DirectionPersistName(pattern.direction) << L"\r\n";
    ss << L"path_mode=" << PathModePersistName(pattern.path_mode) << L"\r\n";
    for (frame_index = 0u; frame_index < pattern.frames.size(); ++frame_index) {
        const PatternFrame& frame = pattern.frames[frame_index];
        size_t led = 0u;
        ss << L"frame " << frame_index << L" duration=" << static_cast<unsigned int>(frame.duration_ms) << L"\r\n";
        ss << L"leds=";
        for (led = 0u; led < frame.leds.size(); ++led) {
            const RgbColor& c = frame.leds[led];
            wchar_t color_text[16] = {};
            swprintf_s(color_text, L"%02X%02X%02X",
                       static_cast<unsigned int>(c.r),
                       static_cast<unsigned int>(c.g),
                       static_cast<unsigned int>(c.b));
            if (led > 0u) {
                ss << L" ";
            }
            ss << color_text;
        }
        ss << L"\r\n";
    }
    return SaveTextFile(path, ss.str(), error);
}

bool LoadPatternFile(const std::wstring& path, RgbPattern* out, std::wstring* error) {
    std::wstring text;
    RgbPattern pattern;
    int current_frame = -1;
    bool saw_header = false;
    bool saw_leds = false;

    if (out == nullptr) {
        if (error != nullptr) {
            *error = L"Output pointer is null.";
        }
        return false;
    }
    pattern.frames.clear();
    try {
        if (!ReadPatternText(path, &text)) {
            if (error != nullptr) *error = L"Cannot read pattern (maximum 4 MiB): " + path;
            return false;
        }
        std::wistringstream input(text);
        std::wstring raw;
        while (std::getline(input, raw)) {
            if (raw.size() >= 8192u) throw std::invalid_argument("line too long");
            const std::wstring line = Trim(raw);
            if (line.empty()) {
                continue;
            }
            if (!saw_header) {
                if (line != L"RGB_PATTERN_V1") {
                    if (error != nullptr) {
                        *error = L"Unsupported pattern file header.";
                    }
                    return false;
                }
                saw_header = true;
                continue;
            }

            if (StartsWith(line, L"name=")) {
                pattern.name = ValueAfter(line, L"name=");
            } else if (StartsWith(line, L"led_count=")) {
                if (current_frame >= 0) throw std::invalid_argument("late LED count");
                pattern.led_count = ClampInt(ParseInt(ValueAfter(line, L"led_count=")), kMinLedCount, kMaxLedCount);
            } else if (StartsWith(line, L"shape=")) {
                const std::wstring value = ValueAfter(line, L"shape=");
                pattern.shape = ParseShapeName(value);
                std::wstring upper = value;
                for (wchar_t& ch : upper) ch = static_cast<wchar_t>(towupper(ch));
                if (pattern.shape == LedShape::Line && upper != L"LINE") throw std::invalid_argument("shape");
            } else if (StartsWith(line, L"start_index=")) {
                pattern.start_index = ParseInt(ValueAfter(line, L"start_index="));
            } else if (StartsWith(line, L"direction=")) {
                std::wstring value = ValueAfter(line, L"direction=");
                for (wchar_t& ch : value) ch = static_cast<wchar_t>(towupper(ch));
                pattern.direction = ParseDirectionName(value);
                if (pattern.direction == LedDirection::Forward && value != L"FORWARD") throw std::invalid_argument("direction");
            } else if (StartsWith(line, L"path_mode=")) {
                std::wstring value = ValueAfter(line, L"path_mode=");
                for (wchar_t& ch : value) ch = static_cast<wchar_t>(towupper(ch));
                pattern.path_mode = ParsePathModeName(value);
                if (pattern.path_mode == LedPathMode::Loop && value != L"LOOP") throw std::invalid_argument("motion");
            } else if (StartsWith(line, L"frame ")) {
                if ((current_frame >= 0 && !saw_leds) || pattern.frames.size() >= kMaxPatternFrames) {
                    throw std::invalid_argument("missing pixels or too many frames");
                }
                const size_t pos = line.find(L"duration=");
                const std::wstring index = Trim(line.substr(6u, pos == std::wstring::npos ? pos : pos - 6u));
                if (ParseInt(index) != static_cast<int>(pattern.frames.size())) throw std::invalid_argument("frame index");
                PatternFrame frame;
                frame.duration_ms = kDefaultFrameDurationMs;
                frame.leds.assign(static_cast<size_t>(pattern.led_count), RgbColor{});
                if (pos != std::wstring::npos) {
                    frame.duration_ms = static_cast<std::uint16_t>(
                        ClampInt(ParseInt(line.substr(pos + 9u)), 1, 60000));
                }
                pattern.frames.push_back(frame);
                current_frame = static_cast<int>(pattern.frames.size()) - 1;
                saw_leds = false;
            } else if (StartsWith(line, L"leds=")) {
                if (current_frame < 0 || saw_leds) throw std::invalid_argument("unexpected pixels");
                std::wistringstream ls(ValueAfter(line, L"leds="));
                std::wstring token;
                size_t led = 0u;
                while (ls >> token) {
                    RgbColor color;
                    if (led >= pattern.frames[static_cast<size_t>(current_frame)].leds.size() || !ParseColorToken(token, &color)) {
                        throw std::invalid_argument("invalid pixel");
                    }
                    pattern.frames[static_cast<size_t>(current_frame)].leds[led] = color;
                    ++led;
                }
                if (led != static_cast<size_t>(pattern.led_count)) throw std::invalid_argument("pixel count");
                saw_leds = true;
            }
        }
    } catch (...) {
        if (error != nullptr) {
            *error = L"Invalid pattern field, frame sequence, pixels, or resource limit.";
        }
        return false;
    }

    if (!saw_header) {
        if (error != nullptr) {
            *error = L"Empty pattern file.";
        }
        return false;
    }
    if (pattern.frames.empty() || !saw_leds) {
        if (error != nullptr) *error = L"Pattern has no complete frames.";
        return false;
    }
    EnsurePatternShape(&pattern, pattern.led_count, pattern.shape);
    if (!ValidatePattern(pattern, error)) return false;
    *out = std::move(pattern);
    return true;
}

} // namespace rgb_tool::core
