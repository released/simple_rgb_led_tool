#include "src/core/rgb_pattern.h"
#include "src/core/text_utils.h"
#include "src/services/rgb_hid_protocol.h"
#include <windows.h>
#include <cstdio>
#include <stdexcept>
#include <string>

using namespace rgb_tool::core;
static int checks = 0;
static void Check(bool result, const char* name) {
    ++checks;
    if (!result) throw std::runtime_error(name);
}
template <typename Function> static bool Rejects(Function function) {
    try { function(); } catch (const std::exception&) { return true; }
    return false;
}
static std::string Bytes(const std::wstring& path) {
    FILE* input = nullptr;
    if (_wfopen_s(&input, path.c_str(), L"rb") != 0) return "";
    std::string bytes;
    char block[4096];
    size_t count;
    while ((count = fread(block, 1u, sizeof(block), input)) > 0u) bytes.append(block, count);
    fclose(input);
    return bytes;
}
int wmain(int argc, wchar_t** argv) {
    if (argc != 2) return 2;
    const std::wstring root = argv[1];
    try {
        std::wstring error;
        Check(ParseInt(L"2147483647") == 2147483647, "INT_MAX");
        Check(ParseInt(L"-2147483648") == (-2147483647 - 1), "INT_MIN");
        Check(ParseInt(L"0xFF") == 255, "hex compatibility");
        for (const wchar_t* value : {L"2147483648", L"-2147483649", L"0xFFFFFFFF", L"999999999999999999999999999", L"-", L"0x"}) {
            Check(Rejects([&] { ParseInt(value); }), "numeric rejection");
        }
        auto pattern = MakePreset(L"Moving Dot", 4, LedShape::UShape, 2, LedDirection::Reverse);
        const auto source = GeneratePatternCode(pattern, L"g_rgb_pattern", LedOutputBackend::Llsi);
        Check(ValidatePattern(pattern, &error), "valid model");
        pattern.name = L"\x6e2c\x8a66 %;#=\\";
        const auto path = root + L"/unicode.rgbpat";
        Check(SavePatternFile(pattern, path, &error), "save unicode");
        RgbPattern loaded;
        Check(LoadPatternFile(path, &loaded, &error) && loaded.name == pattern.name, "unicode round trip");
        Check(GeneratePatternCode(loaded, L"g_rgb_pattern", LedOutputBackend::Llsi) == source, "preview/export source immutability");
        const auto accepted = loaded.name;
        const std::wstring base = L"RGB_PATTERN_V1\nled_count=2\nframe 0 duration=100\nleds=FF0000 00FF00\n";
        for (const auto& invalid : {
            L"RGB_PATTERN_V2\n" + base,
            base + L"frame 1 duration=50\n",
            std::wstring(L"RGB_PATTERN_V1\nled_count=2\nframe 0\nleds=XX0000 00FF00\n"),
            std::wstring(L"RGB_PATTERN_V1\nled_count=2\nframe 0\nleds=FF0000\n"),
            std::wstring(L"RGB_PATTERN_V1\nled_count=2\nframe 0\nleds=FF0000 00FF00 FFFFFF\n"),
            std::wstring(L"RGB_PATTERN_V1\nled_count=999999999999999999\n"),
            std::wstring(L"RGB_PATTERN_V1\nshape=Triangle\n"),
            std::wstring(L"RGB_PATTERN_V1\n")}) {
            Check(SaveTextFile(root + L"/invalid.rgbpat", invalid, &error), "write fixture");
            Check(!LoadPatternFile(root + L"/invalid.rgbpat", &loaded, &error) && loaded.name == accepted, "reject preserves accepted model");
        }
        Check(SaveTextFile(root + L"/legacy.rgbpat", base, &error), "legacy fixture");
        Check(LoadPatternFile(root + L"/legacy.rgbpat", &loaded, &error) && loaded.start_index == 0 &&
              loaded.direction == LedDirection::Forward && loaded.frames[0].leds[1].g == 255u, "V1 defaults and independent pixels");
        auto invalid = pattern;
        invalid.frames[0].leds.clear();
        Check(!SavePatternFile(invalid, path, &error), "invalid save rejected");
        Check(Rejects([&] { GenerateSourceFiles(invalid, L"g_test", LedOutputBackend::Llsi); }), "invalid generation rejected");
        invalid = pattern;
        invalid.frames.resize(kMaxPatternFrames + 1u);
        Check(!ValidatePattern(invalid, &error), "frame upper bound");
        std::wstring frames = L"RGB_PATTERN_V1\nled_count=1\n";
        for (size_t index = 0u; index <= kMaxPatternFrames; ++index) {
            frames += L"frame " + std::to_wstring(index) + L" duration=1\nleds=000000\n";
        }
        Check(SaveTextFile(root + L"/many.rgbpat", frames, &error), "frame count fixture");
        Check(!LoadPatternFile(root + L"/many.rgbpat", &loaded, &error), "parser frame upper bound");
        Check(SaveTextFile(root + L"/large.rgbpat", std::wstring(kMaxPatternFileBytes + 1u, L'x'), &error), "large fixture");
        Check(!LoadPatternFile(root + L"/large.rgbpat", &loaded, &error), "file byte upper bound");
        const auto before = Bytes(path);
        HANDLE lock = CreateFileW(path.c_str(), GENERIC_READ, 0u, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        Check(lock != INVALID_HANDLE_VALUE, "fixture lock");
        const bool saved = SaveTextFile(path, L"replacement", &error);
        CloseHandle(lock);
        Check(!saved && Bytes(path) == before, "failed replacement preserves last good");
        Check(!SaveGeneratedSourceFiles(root, {{L"../escape.c", L"x"}}, &error), "traversal rejected");
        Check(!SaveGeneratedSourceFiles(root, {{L"NUL.c", L"x"}}, &error), "reserved device rejected");
        Check(!SaveGeneratedSourceFiles(root, {{L"a.c", L"x"}, {L"A.c", L"y"}}, &error), "case collision rejected");
        CreateDirectoryW((root + L"/blocked.c").c_str(), nullptr);
        Check(SaveTextFile(root + L"/first.c", L"old", &error), "old bundle fixture");
        Check(!SaveGeneratedSourceFiles(root, {{L"first.c", L"new"}, {L"blocked.c", L"x"}}, &error) &&
              Bytes(root + L"/first.c") == "old", "prepare failure commits nothing");
        Check(SaveTextFile(root + L"/second.c", L"old second", &error), "partial bundle fixture");
        lock = CreateFileW((root + L"/second.c").c_str(), GENERIC_READ, FILE_SHARE_READ,
                           nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        Check(lock != INVALID_HANDLE_VALUE, "partial bundle lock");
        const bool complete = SaveGeneratedSourceFiles(root, {{L"first.c", L"new"}, {L"second.c", L"new second"}}, &error);
        CloseHandle(lock);
        Check(!complete && error.find(L"after 1 of 2") != std::wstring::npos &&
              Bytes(root + L"/first.c") == "new" && Bytes(root + L"/second.c") == "old second", "partial commit reported accurately");
        for (const auto& preset : PresetNames()) {
            for (int count : {1, 2, 8, 16, 24, 64}) {
                auto current = MakePreset(preset, count, LedShape::UShape, count / 2, LedDirection::Reverse, LedPathMode::Bounce);
                Check(ValidatePattern(current, &error), "preset bounds");
                for (int backend = 0; backend < 4; ++backend) {
                    const auto folder = root + L"/backend" + std::to_wstring(backend);
                    CreateDirectoryW(folder.c_str(), nullptr);
                    const auto files = GenerateSourceFiles(current, L"g_test", static_cast<LedOutputBackend>(backend));
                    Check(files.size() == 4u, "backend bundle count");
                    if (preset == L"Moving Dot" && count == 8) Check(SaveGeneratedSourceFiles(folder, files, &error), "backend output");
                }
            }
        }
        using namespace rgb_tool::hid;
        const std::uint8_t payload[] = {255, 80, 0};
        const auto report = MakeReport(Command::SetStatic, 1u, payload, 3u);
        Check(report[0] == 0xA5 && report[1] == 1 && report[2] == 3 && report[4] == 3 &&
              report[5] == 255 && report[6] == 80 && report[63] == 0xE7, "independent HID vector");
        Check(Rejects([&] { MakeReport(Command::SetStatic, 1u, nullptr, 1u); }), "null payload rejected");
        std::uint8_t maximum[59] = {};
        Check(MakeReport(Command::SetPixels, 0u, maximum, 58u)[4] == 58u, "payload maximum");
        Check(Rejects([&] { MakeReport(Command::SetPixels, 0u, maximum, 59u); }), "payload overflow rejected");
        std::printf("PASS: %d focused contract checks\n", checks);
        return 0;
    } catch (const std::exception& exception) {
        std::fprintf(stderr, "FAIL after %d checks: %s\n", checks, exception.what());
        return 1;
    }
}
