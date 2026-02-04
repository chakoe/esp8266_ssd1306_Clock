#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include "global_config.h"

// 按钮事件类型
enum ButtonEventType {
  SHORT_PRESS,
  LONG_PRESS,
  RELEASE
};

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

#endif