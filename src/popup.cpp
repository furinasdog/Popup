#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include "popup.h"

// ============================================================
// DPI 感知支持
// 使用运行时动态加载，兼容 Windows 8.1 ~ Windows 11
// 避免在旧系统上链接失败
// ============================================================

// SetThreadDpiAwarenessContext / GetThreadDpiAwarenessContext
// 在 Windows 10 1607 (RS1) 及以上可用
typedef DPI_AWARENESS_CONTEXT (WINAPI* PFN_GetThreadDpiAwarenessContext)();
typedef DPI_AWARENESS_CONTEXT (WINAPI* PFN_SetThreadDpiAwarenessContext)(DPI_AWARENESS_CONTEXT);
typedef BOOL                  (WINAPI* PFN_SetProcessDpiAwarenessContext)(DPI_AWARENESS_CONTEXT);

static PFN_GetThreadDpiAwarenessContext s_fnGetThread = nullptr;
static PFN_SetThreadDpiAwarenessContext s_fnSetThread = nullptr;
static PFN_SetProcessDpiAwarenessContext s_fnSetProcess = nullptr;
static bool s_dpiInit = false;

static void dpi_init()
{
    if (s_dpiInit) return;
    s_dpiInit = true;

    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (!hUser32) return;

    s_fnGetThread  = reinterpret_cast<PFN_GetThreadDpiAwarenessContext>(
        GetProcAddress(hUser32, "GetThreadDpiAwarenessContext"));
    s_fnSetThread  = reinterpret_cast<PFN_SetThreadDpiAwarenessContext>(
        GetProcAddress(hUser32, "SetThreadDpiAwarenessContext"));
    s_fnSetProcess = reinterpret_cast<PFN_SetProcessDpiAwarenessContext>(
        GetProcAddress(hUser32, "SetProcessDpiAwarenessContext"));
}

// RAII 守卫：弹窗期间切换到 Per-Monitor v2，结束后自动还原
// 对宿主进程的 DPI 设置零侵入
struct DpiScope
{
    DPI_AWARENESS_CONTEXT prev = nullptr;

    DpiScope()
    {
        dpi_init();
        if (s_fnSetThread && s_fnGetThread)
        {
            prev = s_fnGetThread();
            // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 = (DPI_AWARENESS_CONTEXT)-4
            s_fnSetThread(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        }
    }

    ~DpiScope()
    {
        if (s_fnSetThread && prev)
            s_fnSetThread(prev);
    }
};

// ============================================================
// UTF-8 → UTF-16
// ============================================================
static std::wstring utf8_to_wstring(const char* str)
{
    if (!str || *str == '\0')
        return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, str, -1, nullptr, 0);
    std::wstring result(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str, -1, &result[0], len);
    return result;
}

// ============================================================
// 内部辅助
// ============================================================
static void show_box(const char* title, const char* message, UINT flags)
{
    DpiScope dpi;   // 进入 Per-Monitor v2，离开时自动还原
    std::wstring wtitle   = utf8_to_wstring(title);
    std::wstring wmessage = utf8_to_wstring(message);
    MessageBoxW(nullptr, wmessage.c_str(), wtitle.c_str(), flags | MB_SETFOREGROUND);
}

// ============================================================
// 公开接口
// ============================================================

POPUP_API void popup_info(const char* title, const char* message)
{
    show_box(title, message, MB_OK | MB_ICONINFORMATION);
}

POPUP_API void popup_warning(const char* title, const char* message)
{
    show_box(title, message, MB_OK | MB_ICONWARNING);
}

POPUP_API void popup_error(const char* title, const char* message)
{
    show_box(title, message, MB_OK | MB_ICONERROR);
}

POPUP_API int popup_ask(const char* title, const char* message)
{
    DpiScope dpi;   // 进入 Per-Monitor v2，离开时自动还原
    std::wstring wtitle   = utf8_to_wstring(title);
    std::wstring wmessage = utf8_to_wstring(message);
    int ret = MessageBoxW(nullptr, wmessage.c_str(), wtitle.c_str(),
                          MB_YESNO | MB_ICONQUESTION | MB_SETFOREGROUND);
    return (ret == IDYES) ? 1 : 0;
}
