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
  uint8_t pins[] = {K1_PIN, K2_PIN, K3_PIN, K4_PIN};
  
  for (int i = 0; i < 4; i++) {
    // 添加数组边界检查
    if (i < 0 || i >= 4) continue;
    
    ButtonState& btn = buttonStates.buttons[i];
    btn.pin = pins[i];
    btn.currentState = HIGH;
    btn.lastState = HIGH;
    btn.stableState = HIGH;
    btn.lastDebounceTime = 0;
    btn.lastPressTime = 0;
    btn.lastReleaseTime = 0;
    btn.isPressed = false;
    btn.pressDuration = 0;
    btn.clickCount = 0;
    btn.lastProcessTime = 0;
    
    pinMode(pins[i], INPUT_PULLUP);
  }
}

void updateButtonStates() {
  unsigned long currentMillis = millis();
  
    for (int i = 0; i < 4; i++) {
    // 添加数组边界检查
    if (i < 0 || i >= 4) continue;
    
    ButtonState& btn = buttonStates.buttons[i];
    
    // 读取当前物理状态
    bool currentState = digitalRead(btn.pin);
    
    // 检测状态变化，更新消抖时间
    if (currentState != btn.lastState) {
      btn.lastDebounceTime = currentMillis;
    }
    
    // 检查是否已超过消抖延迟
    unsigned long elapsed = currentMillis - btn.lastDebounceTime;
    bool debounceExpired = (elapsed >= DEBOUNCE_DELAY);
    
    if (debounceExpired) {
      // 如果稳定状态发生变化
      if (btn.stableState != currentState) {
        btn.stableState = currentState;
        
        // 检测按下事件 (从HIGH到LOW)
        if (btn.stableState == LOW) {
          btn.isPressed = true;
          btn.lastPressTime = currentMillis;
          btn.pressDuration = 0;
          btn.lastProcessTime = currentMillis; // 更新处理时间以防止误重置
          
          // 记录按键按下时间，用于处理短按释放
          systemState.lastButtonPressTime[i] = currentMillis;
          
          // 重置上次释放时间，确保在按下后能正确检测释放
          btn.lastReleaseTime = 0;
        } 
        // 检测释放事件 (从LOW到HIGH)
        else {
          // 只有在确实处于按下状态时才处理释放
          if (btn.isPressed) {
            btn.lastReleaseTime = currentMillis;
            btn.pressDuration = currentMillis - btn.lastPressTime;
            
            // 根据按压时长判断是短按还是长按
            if (btn.pressDuration < LONG_PRESS_TIME) {
              // 短按事件
              processButtonEvent(i, btn.pressDuration);
            }
            
            // 重置按下状态
            btn.isPressed = false;
            btn.pressDuration = 0;
            btn.lastProcessTime = currentMillis; // 更新处理时间
          }
        }
      }
    }
    
    // 长按检测：仅在按键确实处于按下状态且未触发过长按
    if (btn.isPressed && btn.lastPressTime > 0) {
      unsigned long currentPressDuration = currentMillis - btn.lastPressTime;
      
      // 检测长按（只触发一次）
      if (currentPressDuration >= LONG_PRESS_TIME && 
          (currentMillis - btn.lastProcessTime) >= LONG_PRESS_TIME) {
        processButtonEvent(i, currentPressDuration); // 传递实际按压时间
        btn.lastProcessTime = currentMillis; // 更新处理时间，标记已处理长按
      }
    }
    
    // 更新最后物理状态
    btn.lastState = currentState;
    
    // 状态清理：如果按键长时间处于按下状态而未释放，重置状态以防止卡死
    if (btn.isPressed) {
      unsigned long pressElapsed = currentMillis - btn.lastPressTime;
      if (pressElapsed > BUTTON_RESET_TIME) {
        // 重置按键状态
        btn.isPressed = false;
        btn.pressDuration = 0;
        btn.lastPressTime = 0;
        btn.lastReleaseTime = 0;
        btn.lastProcessTime = currentMillis; // 更新处理时间以避免重复重置
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
      unsigned long currentMillis = millis();
      if (displayState.showStatus) {
        displayState.statusOverlayUntil = currentMillis + 5000;
        systemState.needsRefresh = true;
      } else {
        displayState.statusOverlayUntil = 0;
        systemState.needsRefresh = true;
      }
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