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

    // 设置引脚模式为上拉输入（按键按下时为LOW）
    pinMode(pins[i], INPUT_PULLUP);

    // 强制所有状态重置为未按下（HIGH），确保初始化时逻辑状态与物理状态一致
    // 这样可以避免设备长期运行后按键状态与物理状态不一致的问题
    btn.lastState = HIGH;
    btn.stableState = HIGH;
    btn.isPressed = false;
    btn.lastDebounceTime = millis();  // 使用当前时间初始化消抖时间
    btn.lastPressTime = 0;            // 上次按下时间
    btn.pressDuration = 0;            // 按下持续时间
    btn.longPressTriggered = false;   // 初始化长按触发标志
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
      btn.lastState = currentState;  // 立即更新 lastState
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

        // 检测按下事件 (从 HIGH 到 LOW)
        if (btn.stableState == LOW) {
          btn.isPressed = true;
          btn.lastPressTime = currentMillis;
          btn.pressDuration = 0;
          btn.longPressTriggered = false;  // 重置长按触发标志

          // 记录按键按下时间
          systemState.lastButtonPressTime[i] = currentMillis;
          
          // 为 K4 按键添加调试日志
#ifdef ENABLE_DEBUG_LOGS
          if (i == 3) {  // K4 对应索引 3
            LOG_DEBUG("K4 pressed detected, stableState=LOW, isPressed=true");
          }
#endif
        }
        // 检测释放事件 (从 LOW 到 HIGH)
        else {
          if (btn.isPressed) {
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
            btn.longPressTriggered = false;  // 重置长按触发标志，确保下次长按可正常触发
            
            // 为 K4 按键添加调试日志
#ifdef ENABLE_DEBUG_LOGS
            if (i == 3) {  // K4 对应索引 3
              LOG_DEBUG("K4 released, pressDuration=%lu ms", btn.pressDuration);
            }
#endif
          }
        }
      }
    }

    // 长按检测（只触发一次）
    if (btn.isPressed && btn.lastPressTime > 0 && !btn.longPressTriggered) {
      unsigned long currentPressDuration = (currentMillis >= btn.lastPressTime) ?
                                         (currentMillis - btn.lastPressTime) :
                                         (0xFFFFFFFF - btn.lastPressTime + currentMillis);

      // 检测长按事件
      if (currentPressDuration >= LONG_PRESS_TIME) {
        processButtonEvent(i, currentPressDuration);
        btn.longPressTriggered = true;  // 标记长按已触发，防止重复触发
        
        // 为 K4 按键添加调试日志
#ifdef ENABLE_DEBUG_LOGS
        if (i == 3) {  // K4 对应索引 3
          LOG_DEBUG("K4 long press detected, duration=%lu ms", currentPressDuration);
        }
#endif
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

        // 仅当物理状态确实已释放时才重置，避免误判
        if (physicalState == HIGH) {
          // 计算按下持续时间并触发事件，确保事件不丢失
          btn.pressDuration = BUTTON_RESET_TIME;

          // 触发按键事件（如果是短按）
          if (btn.pressDuration < LONG_PRESS_TIME) {
            processButtonEvent(i, btn.pressDuration);
          }

          btn.isPressed = false;
          btn.longPressTriggered = false;  // 重置长按触发标志，防止状态卡死
          btn.lastPressTime = 0;
        } else {
          // 如果仍然被按下，更新按下时间以避免重复触发清理
          btn.lastPressTime = currentMillis;
          btn.pressDuration = 0;
          // 注意：不重置 longPressTriggered，防止长按事件被重复触发
          // （如 resetToAP 在 nonBlockingDelay 期间被再次调用）
        }
      }
    }
  }

  // 检查设置模式是否超时
  checkSettingModeTimeout();
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
      } else {
        // 设置模式下长按K4无效，显示提示避免用户困惑
        displayError("设置中", "请先退出设置");
      }
      break;
  }
  // 喂狗以防止在处理过程中看门狗超时
  ESP.wdtFeed();
}

// 亮度设置模式处理函数
void handleBrightnessMode(int buttonIndex) {
  settingState.settingModeEnterTime = millis();  // 更新操作时间
  switch (buttonIndex) {
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
  settingState.settingModeEnterTime = millis();  // 更新操作时间
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
  settingState.settingModeEnterTime = millis();  // 更新操作时间
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
      if (displayState.showStatus) {
        // 无符号加法自然溢出，后续与millis()的比较仍然正确
        unsigned long currentMillis = millis();
        displayState.statusOverlayUntil = currentMillis + 5000;
      } else {
        displayState.statusOverlayUntil = 0;
      }
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

// 检查设置模式是否超时
void checkSettingModeTimeout() {
  unsigned long currentMillis = millis();

  // 检查时间设置模式超时
  if (settingState.settingMode) {
    unsigned long elapsed = (currentMillis >= settingState.settingModeEnterTime) ?
                           (currentMillis - settingState.settingModeEnterTime) :
                           (0xFFFFFFFF - settingState.settingModeEnterTime + currentMillis);
    if (elapsed >= SETTING_MODE_TIMEOUT) {
      exitSettingMode();
      systemState.needsRefresh = true;
    }
  }

  // 检查亮度设置模式超时
  if (settingState.brightnessSettingMode) {
    unsigned long elapsed = (currentMillis >= settingState.settingModeEnterTime) ?
                           (currentMillis - settingState.settingModeEnterTime) :
                           (0xFFFFFFFF - settingState.settingModeEnterTime + currentMillis);
    if (elapsed >= SETTING_MODE_TIMEOUT) {
      exitBrightnessSettingMode();
      systemState.needsRefresh = true;
    }
  }

  // 检查时间源设置模式超时
  if (settingState.timeSourceSettingMode) {
    unsigned long elapsed = (currentMillis >= settingState.settingModeEnterTime) ?
                           (currentMillis - settingState.settingModeEnterTime) :
                           (0xFFFFFFFF - settingState.settingModeEnterTime + currentMillis);
    if (elapsed >= SETTING_MODE_TIMEOUT) {
      exitTimeSourceSettingMode();
      systemState.needsRefresh = true;
    }
  }
}