#include "hid_device.h"

#include <SetupAPI.h>
#include <hidsdi.h>

#include <vector>

namespace rgb_tool::hid {

namespace {
std::wstring LastErrorText(const wchar_t* operation) {
    wchar_t* detail = nullptr;
    const DWORD code = GetLastError();
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, code, 0, reinterpret_cast<wchar_t*>(&detail), 0, nullptr);
    std::wstring text = operation;
    text += L" failed (" + std::to_wstring(code) + L")";
    if (detail != nullptr) { text += L": "; text += detail; LocalFree(detail); }
    return text;
}
}

HidDevice::~HidDevice() { Close(); }

bool HidDevice::Open(std::wstring* error) {
    GUID guid{};
    HidD_GetHidGuid(&guid);
    HDEVINFO info = SetupDiGetClassDevsW(&guid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (info == INVALID_HANDLE_VALUE) { if (error) *error = LastErrorText(L"SetupDiGetClassDevs"); return false; }

    SP_DEVICE_INTERFACE_DATA iface{};
    iface.cbSize = sizeof(iface);
    bool found = false;
    for (DWORD index = 0; SetupDiEnumDeviceInterfaces(info, nullptr, &guid, index, &iface); ++index) {
        DWORD required = 0;
        SetupDiGetDeviceInterfaceDetailW(info, &iface, nullptr, 0, &required, nullptr);
        std::vector<std::uint8_t> storage(required);
        auto* detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(storage.data());
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        if (!SetupDiGetDeviceInterfaceDetailW(info, &iface, detail, required, nullptr, nullptr)) continue;
        HANDLE candidate = CreateFileW(detail->DevicePath, GENERIC_READ | GENERIC_WRITE,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
        if (candidate == INVALID_HANDLE_VALUE) continue;
        HIDD_ATTRIBUTES attributes{};
        attributes.Size = sizeof(attributes);
        if (HidD_GetAttributes(candidate, &attributes) && attributes.VendorID == kVendorId && attributes.ProductID == kProductId) {
            handle_ = candidate;
            path_ = detail->DevicePath;
            found = true;
            break;
        }
        CloseHandle(candidate);
    }
    SetupDiDestroyDeviceInfoList(info);
    if (!found && error) *error = L"RGB controller not found (VID 0416, PID 0125).";
    return found;
}

void HidDevice::Close() {
    if (handle_ != INVALID_HANDLE_VALUE) CloseHandle(handle_);
    handle_ = INVALID_HANDLE_VALUE;
    path_.clear();
}

bool HidDevice::Send(Command command, const std::uint8_t* payload, std::uint8_t length, std::wstring* error) {
    if (!IsOpen()) { if (error) *error = L"USB HID device is not connected."; return false; }
    const auto report = MakeReport(command, ++sequence_, payload, length);
    std::array<std::uint8_t, kReportSize + 1> wire{};
    for (std::size_t i = 0; i < report.size(); ++i) wire[i + 1] = report[i];
    DWORD written = 0;
    if (!WriteFile(handle_, wire.data(), static_cast<DWORD>(wire.size()), &written, nullptr) || written != wire.size()) {
        if (error) *error = LastErrorText(L"HID WriteFile");
        Close();
        return false;
    }
    return true;
}

} // namespace rgb_tool::hid
