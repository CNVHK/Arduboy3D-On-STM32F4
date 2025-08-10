#include "main.h"
#include <stdint.h>

#define KEY_DEBOUNCE_TIME 5  // 消抖时间(ms)

uint8_t optTab(uint8_t currentKey)
{
    switch (currentKey)
    {
        case 0: return 0x01;
        case 1: return 0x02;
        case 2: return 0x04;
        case 5: return 0x08;
        case 11: return 0x10;
        case 15: return 0x20;
        default: return 0x00;
    }
}

uint16_t MatrixKey_Scan(void) {
    static uint8_t lastKey = 0xFF;
    static uint32_t lastTime = 0;
    uint8_t currentKey = 0xFF;
    uint8_t optKey = 0x00;
    
    // 扫描列（Y0-Y3依次输出低电平）
    for(uint8_t col = 0; col < 4; col++) {
        // 设置当前列为低电平，其他为高
        HAL_GPIO_WritePin(KEY_Y0_GPIO_Port, KEY_Y0_Pin, (col != 0) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(KEY_Y1_GPIO_Port, KEY_Y1_Pin, (col != 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(KEY_Y2_GPIO_Port, KEY_Y2_Pin, (col != 2) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(KEY_Y3_GPIO_Port, KEY_Y3_Pin, (col != 3) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        
        // 延时等待电平稳定（根据实际硬件调整）
        HAL_Delay(1);
        
        // 读取行（X0-X3）
        if(HAL_GPIO_ReadPin(KEY_X0_GPIO_Port, KEY_X0_Pin) == GPIO_PIN_RESET) 
            currentKey = col * 4 + 0;
        else if(HAL_GPIO_ReadPin(KEY_X1_GPIO_Port, KEY_X1_Pin) == GPIO_PIN_RESET) 
            currentKey = col * 4 + 1;
        else if(HAL_GPIO_ReadPin(KEY_X2_GPIO_Port, KEY_X2_Pin) == GPIO_PIN_RESET) 
            currentKey = col * 4 + 2;
        else if(HAL_GPIO_ReadPin(KEY_X3_GPIO_Port, KEY_X3_Pin) == GPIO_PIN_RESET) 
            currentKey = col * 4 + 3;

        optKey |= optTab(currentKey);
    }
    
    // 消抖处理
    if(currentKey != lastKey) {
        lastKey = currentKey;
        lastTime = HAL_GetTick();
        return 0x00; // 表示按键变化中
    }
    
    // 确认按键稳定按下
    if((HAL_GetTick() - lastTime) > KEY_DEBOUNCE_TIME) {
        return (optKey != 0x00) ? optKey : 0x00;
    }

    return 0x00;
}