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

// ============================================================
// DLL 依赖检测
// ============================================================

// 解析搜索目录：若 search_dir 为 NULL 则回退到当前工作目录
static std::wstring resolve_search_dir(const char* search_dir)
{
    if (search_dir && search_dir[0]) {
        return utf8_to_wstring(search_dir);
    }
    wchar_t cwd[MAX_PATH];
    GetCurrentDirectoryW(MAX_PATH, cwd);
    return std::wstring(cwd);
}

// 在 wdir 下用通配符 pattern 搜索匹配的 DLL 文件，
// 对每个找到的文件尝试 LoadLibraryExW。
// 加载成功则 FreeLibrary 并继续；加载失败则调用 on_fail 回调并返回 1。
// 如果全部加载成功或没有匹配文件，返回 0。
static int check_dll_glob(const std::wstring& wdir,
                          const wchar_t*     pattern,
                          void (*on_fail)())
{
    std::wstring search = wdir + L"\\" + pattern;
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(search.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        return 0;   // 没有匹配文件
    }

    int result = 0;
    do {
        // 跳过目录项
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;

        std::wstring dll_path = wdir + L"\\" + fd.cFileName;

        // 抑制系统错误弹窗（与 ggml-backend-dl.cpp 行为一致）
        DWORD old_mode = SetErrorMode(SEM_FAILCRITICALERRORS);
        SetErrorMode(old_mode | SEM_FAILCRITICALERRORS);

        HMODULE hMod = LoadLibraryExW(
            dll_path.c_str(), NULL,
            LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);

        SetErrorMode(old_mode);

        if (!hMod) {
            // 文件存在但无法加载 —— 依赖缺失
            on_fail();
            result = 1;
            break;
        }
        FreeLibrary(hMod);
    } while (FindNextFileW(hFind, &fd));

    FindClose(hFind);
    return result;
}

// 同上，但匹配单个确定文件名而非通配符
static int check_dll_file(const std::wstring& wdir,
                          const wchar_t*     filename,
                          void (*on_fail)())
{
    std::wstring dll_path = wdir + L"\\" + filename;

    // 先检查文件是否存在
    DWORD attrs = GetFileAttributesW(dll_path.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        return 0;
    }

    DWORD old_mode = SetErrorMode(SEM_FAILCRITICALERRORS);
    SetErrorMode(old_mode | SEM_FAILCRITICALERRORS);

    HMODULE hMod = LoadLibraryExW(
        dll_path.c_str(), NULL,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);

    SetErrorMode(old_mode);

    if (!hMod) {
        on_fail();
        return 1;
    }
    FreeLibrary(hMod);
    return 0;
}

static void on_cuda_fail()
{
    popup_error(
        "CUDA Runtime 缺失",
        "无法加载 CUDA 后端 (ggml-cuda.dll)，可能未安装 CUDA 12 运行时。\n\n"
        "请下载并安装 CUDA 12 Runtime：\n"
        "https://developer.nvidia.com/cuda-downloads");
}

static void on_runtime_fail()
{
    popup_error(
        "VC++ 运行时缺失",
        "无法加载必要的运行库，可能未安装 Microsoft Visual C++ 运行时。\n\n"
        "请下载并安装 VC++ Redistributable：\n"
        "https://aka.ms/vs/18/release/vc_redist.x64.exe");
}

POPUP_API int popup_check_cuda(const char* search_dir)
{
    std::wstring wdir = resolve_search_dir(search_dir);
    return check_dll_glob(wdir, L"ggml-cuda*.dll", on_cuda_fail);
}

POPUP_API int popup_check_runtime(const char* search_dir)
{
    std::wstring wdir = resolve_search_dir(search_dir);
    return check_dll_file(wdir, L"ggml-base.dll", on_runtime_fail);
}
