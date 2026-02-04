#include "button_handler.h"
#include "display_manager.h"
#include "time_manager.h"
#include "system_manager.h"
#include "utils.h"
#include <ESP8266WiFi.h>
#include "config.h"

// 全局按钮状态数组声明 - 现在统一在global_config.cpp中定义
extern ButtonStateArray buttonStates;

// 外部变量声明
extern SystemState systemState;
extern DisplayState displayState;
extern SettingState settingState;
extern TimeState timeState;
extern const unsigned long DISPLAY_UPDATE_INTERVAL;

void initButtons() {
  // 定义按键引脚数组
  const uint8_t pins[] = {K1_PIN, K2_PIN, K3_PIN, K4_PIN};
  const int buttonCount = sizeof(pins) / sizeof(pins[0]);

  // 初始化每个按键的状态
  for (int i = 0; i < buttonCount; i++) {
    ButtonState& btn = buttonStates.buttons[i];
    btn.pin = pins[i];
    btn.currentState = HIGH;          // 初始状态为HIGH（未按下）
    btn.lastState = HIGH;             // 上一次状态
    btn.stableState = HIGH;           // 稳定状态（消抖后）
    btn.lastDebounceTime = 0;         // 上次消抖时间
    btn.lastPressTime = 0;            // 上次按下时间
    btn.lastReleaseTime = 0;          // 上次释放时间
    btn.isPressed = false;            // 是否按下
    btn.pressDuration = 0;            // 按下持续时间
    btn.clickCount = 0;               // 点击次数
    btn.lastProcessTime = 0;          // 上次处理时间

    // 设置引脚模式为上拉输入（按键按下时为LOW）
    pinMode(pins[i], INPUT_PULLUP);
  }
}

void updateButtonStates() {
  unsigned long currentMillis = millis();

  // 遍历所有按键
  for (int i = 0; i < 4; i++) {
    ButtonState& btn = buttonStates.buttons[i];

    // 读取当前物理状态
    bool currentState = digitalRead(btn.pin);

    // 消抖处理：只有当状态真正变化时才更新消抖时间
    if (currentState != btn.lastState) {
      btn.lastDebounceTime = currentMillis;
      btn.lastState = currentState;  // 立即更新lastState
    }

    // 计算消抖时间（使用安全的减法避免溢出问题）
    unsigned long elapsed = (currentMillis >= btn.lastDebounceTime) ?
                           (currentMillis - btn.lastDebounceTime) :
                           (0xFFFFFFFF - btn.lastDebounceTime + currentMillis);
    bool debounceExpired = (elapsed >= DEBOUNCE_DELAY);

    if (debounceExpired) {
      // 只有当稳定状态真正发生变化时才处理事件
      if (btn.stableState != currentState) {
        btn.stableState = currentState;

        // 检测按下事件 (从HIGH到LOW)
        if (btn.stableState == LOW) {
          btn.isPressed = true;
          btn.lastPressTime = currentMillis;
          btn.pressDuration = 0;
          btn.lastProcessTime = currentMillis;

          // 记录按键按下时间
          systemState.lastButtonPressTime[i] = currentMillis;
          btn.lastReleaseTime = 0;
        }
        // 检测释放事件 (从LOW到HIGH)
        else {
          if (btn.isPressed) {
            btn.lastReleaseTime = currentMillis;
            btn.pressDuration = (currentMillis >= btn.lastPressTime) ?
                               (currentMillis - btn.lastPressTime) :
                               (0xFFFFFFFF - btn.lastPressTime + currentMillis);

            // 短按事件
            if (btn.pressDuration < LONG_PRESS_TIME) {
              processButtonEvent(i, btn.pressDuration);
            }

            // 重置按下状态
            btn.isPressed = false;
            btn.pressDuration = 0;
            btn.lastProcessTime = currentMillis;
          }
        }
      }
    }

    // 长按检测（只触发一次）
    if (btn.isPressed && btn.lastPressTime > 0) {
      unsigned long currentPressDuration = (currentMillis >= btn.lastPressTime) ?
                                         (currentMillis - btn.lastPressTime) :
                                         (0xFFFFFFFF - btn.lastPressTime + currentMillis);

      if (currentPressDuration >= LONG_PRESS_TIME &&
          (currentMillis >= btn.lastProcessTime ?
           (currentMillis - btn.lastProcessTime) :
           (0xFFFFFFFF - btn.lastProcessTime + currentMillis)) >= LONG_PRESS_TIME) {
        processButtonEvent(i, currentPressDuration);
        btn.lastProcessTime = currentMillis;
      }
    }

    // 更新最后物理状态
    btn.lastState = currentState;

    // 状态清理：防止按键长时间按下导致状态卡死
    if (btn.isPressed) {
      unsigned long pressElapsed = (currentMillis >= btn.lastPressTime) ?
                                  (currentMillis - btn.lastPressTime) :
                                  (0xFFFFFFFF - btn.lastPressTime + currentMillis);
      if (pressElapsed > BUTTON_RESET_TIME) {
        bool physicalState = digitalRead(btn.pin);

        btn.isPressed = (physicalState == LOW);
        btn.pressDuration = 0;

        if (physicalState == LOW) {
          btn.lastPressTime = currentMillis;
          btn.lastReleaseTime = 0;
        } else {
          btn.lastPressTime = 0;
          btn.lastReleaseTime = currentMillis;
          btn.pressDuration = BUTTON_RESET_TIME;
        }

        btn.lastProcessTime = currentMillis;
      }
    }
  }
}

// 长按处理函数
void handleLongPress(int buttonIndex) {
  switch (buttonIndex) {
    case 1: // K2长按进入时间源设置
      if (!settingState.settingMode && !settingState.brightnessSettingMode && !settingState.timeSourceSettingMode) {
        enterTimeSourceSettingMode();
        systemState.needsRefresh = true;
      }
      break;
    case 2: // K3长按进入时间设置
      if (!settingState.settingMode && !settingState.brightnessSettingMode && !settingState.timeSourceSettingMode) {
        enterSettingMode();
        systemState.needsRefresh = true;
      }
      break;
    case 3: // K4长按重置WiFi
      if (!settingState.settingMode && !settingState.brightnessSettingMode && !settingState.timeSourceSettingMode) {
        resetToAP();
        systemState.needsRefresh = true;
      }
      break;
  }
  // 喂狗以防止在处理过程中看门狗超时
  ESP.wdtFeed();
}

// 亮度设置模式处理函数
void handleBrightnessMode(int buttonIndex) {
  switch (buttonIndex) {
    case 0: // K1键增加亮度
      updateBrightnessSetting(1);
      break;
    case 1: // K2键减少亮度
      updateBrightnessSetting(-1);
      break;
    case 2: // K3键切换亮度等级
      updateBrightnessSetting(1);
      break;
    case 3: // K4键确认并退出亮度设置模式
      exitBrightnessSettingMode();
      break;
  }
  systemState.needsRefresh = true;
}

// 时间设置模式处理函数
void handleSettingMode(int buttonIndex) {
  switch (buttonIndex) {
    case 0: // K1键增加
      updateSettingValue(1);
      break;
    case 1: // K2键减少
      updateSettingValue(-1);
      break;
    case 2: // K3键切换设置字段
      settingState.settingField = (settingState.settingField + 1) % 6; // 循环切换年月日时分秒
      break;
    case 3: // K4键确认并退出设置模式
      exitSettingMode();
      break;
  }
  systemState.needsRefresh = true;
}

// 时间源设置模式处理函数
void handleTimeSourceMode(int buttonIndex) {
  switch (buttonIndex) {
    case 1: // K2短按选择下一个时间源
      selectNextTimeSource();
      systemState.needsRefresh = true;
      break;
    case 3: // K4短按确认并退出时间源设置模式
      exitTimeSourceSettingMode();
      systemState.needsRefresh = true;
      break;
  }
}

// 常规模式处理函数
void handleNormalMode(int buttonIndex) {
  switch (buttonIndex) {
    case 1: // K2短按切换字体大小
      displayState.largeFont = !displayState.largeFont;
      systemState.needsRefresh = true;
      displayTime();
      break;
    
    case 2: // K3短按进入亮度设置
      enterBrightnessSettingMode();
      systemState.needsRefresh = true;
      break;
    
    case 3: // K4短按显示/隐藏网络状态
      displayState.showStatus = !displayState.showStatus;
      displayState.statusOverlayUntil = displayState.showStatus ? (millis() + 5000) : 0;
      systemState.needsRefresh = true;
      break;
  }
}

// 主按键事件处理函数（已拆分）
void processButtonEvent(int buttonIndex, unsigned long pressDuration) {
  bool isLongPress = pressDuration >= LONG_PRESS_TIME;
  
  // 优先处理长按事件，无论处于何种模式
  if (isLongPress) {
    handleLongPress(buttonIndex);
    return;
  }
  
  // 非长按事件（短按释放）的处理
  if (!isLongPress) {
    // 在亮度设置模式下
    if (settingState.brightnessSettingMode) {
      handleBrightnessMode(buttonIndex);
      return;
    }
    
    // 在时间设置模式下
    if (settingState.settingMode) {
      handleSettingMode(buttonIndex);
      return;
    }
    
    // 时间源设置模式下的按键处理
    if (settingState.timeSourceSettingMode) {
      handleTimeSourceMode(buttonIndex);
      return;
    }
    
    // 常规模式下的按键处理
    handleNormalMode(buttonIndex);
  }
}