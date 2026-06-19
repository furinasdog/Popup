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

#ifdef __cplusplus
}
#endif
