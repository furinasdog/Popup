#include "popup.h"

int main()
{
    popup_info("提示", "这是一条普通提示信息。");
    popup_warning("警告", "这是一条警告信息！");
    popup_error("错误", "这是一条错误信息！");

    int ans = popup_ask("询问", "你确定要继续吗？");
    if (ans)
        popup_info("结果", "你选择了【是】。");
    else
        popup_info("结果", "你选择了【否】。");

    return 0;
}
