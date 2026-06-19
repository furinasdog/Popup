#pragma once

#ifdef _WIN32
  #ifdef POPUP_EXPORTS
    #define POPUP_API __declspec(dllexport)
  #else
    #define POPUP_API __declspec(dllimport)
  #endif
#else
  #define POPUP_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 显示普通提示框
 * @param title   标题（UTF-8）
 * @param message 内容（UTF-8）
 */
POPUP_API void popup_info(const char* title, const char* message);

/**
 * 显示警告框
 * @param title   标题（UTF-8）
 * @param message 内容（UTF-8）
 */
POPUP_API void popup_warning(const char* title, const char* message);

/**
 * 显示错误框
 * @param title   标题（UTF-8）
 * @param message 内容（UTF-8）
 */
POPUP_API void popup_error(const char* title, const char* message);

/**
 * 显示询问框（是/否）
 * @param title   标题（UTF-8）
 * @param message 内容（UTF-8）
 * @return        1 = 用户点击"是"，0 = 用户点击"否"
 */
POPUP_API int popup_ask(const char* title, const char* message);

/**
 * 检查 CUDA 后端 DLL 是否可加载。
 *
 * 在 search_dir 目录下查找 ggml-cuda*.dll 文件，若文件存在但
 * LoadLibrary 失败（通常是因为缺少 CUDA 12 Runtime），则弹出
 * 错误提示框，引导用户下载 CUDA 12 Runtime。
 *
 * @param search_dir  搜索目录（UTF-8），为 NULL 时使用当前工作目录
 * @return 0 = 正常（DLL 不存在或已成功加载），1 = DLL 存在但加载失败
 */
POPUP_API int popup_check_cuda(const char* search_dir);

/**
 * 检查 VC++ 运行时 DLL 是否可加载。
 *
 * 在 search_dir 目录下查找 ggml-base.dll 文件，若文件存在但
 * LoadLibrary 失败（通常是因为缺少 VC++ Redistributable），则弹出
 * 错误提示框，引导用户下载 vc_redist.x64.exe。
 *
 * @param search_dir  搜索目录（UTF-8），为 NULL 时使用当前工作目录
 * @return 0 = 正常（DLL 不存在或已成功加载），1 = DLL 存在但加载失败
 */
POPUP_API int popup_check_runtime(const char* search_dir);

#ifdef __cplusplus
}
#endif
