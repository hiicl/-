#include <Windows.h>
#include <easyhook.h>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <target_process_id>" << std::endl;
        return 1;
    }

    DWORD pid = std::stoi(argv[1]);
    std::wstring dllPath = L"hook.dll"; // 确保hook.dll在相同目录

    // 注入DLL
    NTSTATUS result = RhInjectLibrary(
        pid, 
        0, 
        EASYHOOK_INJECT_DEFAULT,
        const_cast<WCHAR*>(dllPath.c_str()),
        NULL,
        NULL,
        0
    );

    if (result != 0) {
        std::cerr << "Failed to inject DLL: " << result << std::endl;
        return 1;
    }

    std::cout << "Successfully injected hook DLL into process " << pid << std::endl;
    return 0;
}
