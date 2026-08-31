#pragma once

#include <cmsis_os2.h>
#include <stdint.h>

// =============================================================================
// OLED 菜单 (移植自 Cube-ADRC 工程 Drivers/oled/menu.cpp, 适配平衡小车)
//
// 页面状态机:
//   MAIN_PAGE  -> 主菜单 (姿态显示/任务控制/固件版本/陀螺仪校准/电调编号设置)
//   ATT_PAGE   -> 姿态显示 (roll/pitch/yaw)
//   CONTROL_PAGE -> 任务选择 (平衡模式/单点保持/轮速测试)
//   FIRMWARE_PAGE -> 固件版本
//   CONTROL_RUNNING -> 任务运行中 (短按 key3 退出)
//   GYRO_CALIB_PAGE -> 陀螺仪零偏校准
//   ESC_INDEX_PAGE  -> 电调编号设置 (2 路)
//
// 按键: key1=上移, key2=下移, key3=短按返回/长按确认
// =============================================================================

enum class MenuState : uint8_t
{
    MAIN_PAGE_STATE       = 1,
    ATT_PAGE_STATE        = 2,
    CONTROL_PAGE_STATE    = 3,
    FIRMWARE_PAGE_STATE   = 4,
    CONTROL_RUNNING_STATE = 5,
    GYRO_CALIB_PAGE_STATE = 6,
    ESC_INDEX_PAGE_STATE  = 7,
};

// 电池图标状态 (按电压分级, 无 INA226, 取 ESC 总线电压)
enum class MenuBatState : uint8_t
{
    MEASURE_ERR = 0,
    FULL_POWER  = 1,
    HIGH_POWER  = 2,
    MID_POWER   = 3,
    LOW_POWER   = 4,
};

// 电压 -> 电池图标状态 (LiPo, 按 MENU_BAT_CELLS 节串联折算)
MenuBatState menu_battery_state(float voltage);

MenuState menu_att_page(void);
MenuState menu_control_page(void);
MenuState menu_firmware_page(void);
MenuState menu_control_running_page(void);
MenuState menu_gyro_calib_page(void);
MenuState menu_esc_index_page(void);
