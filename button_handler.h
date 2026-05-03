#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include "global_config.h"

extern ButtonStateArray buttonStates;

// 函数声明
void initButtons();
void updateButtonStates();
void processButtonEvent(int buttonIndex, unsigned long pressDuration);

// 拆分后的辅助函数
void handleLongPress(int buttonIndex);
void handleBrightnessMode(int buttonIndex);
void handleSettingMode(int buttonIndex);
void handleTimeSourceMode(int buttonIndex);
void handleNormalMode(int buttonIndex);
void checkSettingModeTimeout();  // 检查设置模式超时

#endif