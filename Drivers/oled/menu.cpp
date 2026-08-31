#include "menu.hpp"
#include "drv_oled.hpp"
#include "drv_key.hpp"
#include "drv_CubeFOC.hpp"
#include "board.hpp"
#include "task_Control.hpp"

// LiPo 电池节数 (电压分级阈值 = 节数 x 单节阈值)
#define MENU_BAT_CELLS 4

// 单节阈值 (V), 与参考工程 DrvINA226 一致
static constexpr float LOW_POWER_VEL  = 3.70f;
static constexpr float MID_POWER_VEL  = 3.80f;
static constexpr float HIGH_POWER_VEL = 4.10f;
static constexpr float FULL_POWER_VEL = 4.20f;

MenuBatState menu_battery_state(float voltage)
{
    const float cells = (float)MENU_BAT_CELLS;

    if (voltage <= 0.0f || voltage >= cells * 4.5f)
        return MenuBatState::MEASURE_ERR;   // 无有效测量

    if (voltage < cells * LOW_POWER_VEL)
        return MenuBatState::LOW_POWER;

    if (voltage < cells * MID_POWER_VEL)
        return MenuBatState::MID_POWER;

    if (voltage < cells * HIGH_POWER_VEL)
        return MenuBatState::HIGH_POWER;

    if (voltage < cells * FULL_POWER_VEL)
        return MenuBatState::FULL_POWER;

    return MenuBatState::MEASURE_ERR;
}

// =============================================================================
// 姿态显示页: roll / pitch / yaw (弧度)
// =============================================================================
MenuState menu_att_page(void)
{
    auto *oled = drv_oled();
    auto *key3 = drv_key(BOARD_KEY_3);

    float roll = 0, pitch = 0, yaw = 0;
    if (g_att_mutex != NULL && osMutexAcquire(g_att_mutex, 0) == osOK)
    {
        roll  = g_attitude.roll;
        pitch = g_attitude.pitch;
        yaw   = g_attitude.yaw;
        osMutexRelease(g_att_mutex);
    }

    oled->showString(0, 16, "roll:");
    oled->showString(0, 32, "pitch:");
    oled->showString(0, 48, "yaw:");

    oled->showFloat(58, 16, roll);
    oled->showFloat(58, 32, pitch);
    oled->showFloat(58, 48, yaw);

    if (key3 && key3->getEvent() == DrvKey::Event::ShortPress)
        return MenuState::MAIN_PAGE_STATE;

    return MenuState::ATT_PAGE_STATE;
}

// =============================================================================
// 任务选择页: 平衡模式(0) / 单点保持(1) / 轮速测试(4)
// =============================================================================
MenuState menu_control_page(void)
{
    auto *oled = drv_oled();
    auto *key1 = drv_key(BOARD_KEY_1);
    auto *key2 = drv_key(BOARD_KEY_2);
    auto *key3 = drv_key(BOARD_KEY_3);

    // 菜单项 -> 控制模式号映射 (与 mavlink 遥控指令一致)
    const int8_t MENU_COUNT = 3;
    const int16_t VISIBLE_Y[3] = {18, 32, 46};
    const char *menu_names[3] = {"平衡模式", "单点保持", "轮速测试"};
    const uint8_t mode_map[3] = {0, 1, 4};

    static int8_t selected_idx = 0;
    static int8_t scroll_top   = 0;

    for (int8_t i = 0; i < 3; i++)
    {
        int8_t item_idx = scroll_top + i;
        if (item_idx < MENU_COUNT)
            oled->showChinese(0, VISIBLE_Y[i], menu_names[item_idx]);
    }

    if (key1 && key1->getEvent() == DrvKey::Event::ShortPress)
    {
        selected_idx = (int8_t)((selected_idx - 1 + MENU_COUNT) % MENU_COUNT);
        if (selected_idx < scroll_top)                 scroll_top = selected_idx;
        if (selected_idx >= scroll_top + 3)            scroll_top = (int8_t)(selected_idx - 2);
    }

    if (key2 && key2->getEvent() == DrvKey::Event::ShortPress)
    {
        selected_idx = (int8_t)((selected_idx + 1) % MENU_COUNT);
        if (selected_idx < scroll_top)                 scroll_top = selected_idx;
        if (selected_idx >= scroll_top + 3)            scroll_top = (int8_t)(selected_idx - 2);
    }

    int16_t draw_frame_y_pos = VISIBLE_Y[selected_idx - scroll_top];
    oled->drawRectangle(0, draw_frame_y_pos, 120, 15, 0);

    if (key3)
    {
        DrvKey::Event evt = key3->getEvent();
        if (evt == DrvKey::Event::ShortPress)
            return MenuState::MAIN_PAGE_STATE;

        if (evt == DrvKey::Event::LongPress)
        {
            g_control_mode = mode_map[selected_idx];
            if (g_controlModeSem != NULL)
                osSemaphoreRelease(g_controlModeSem);

            return MenuState::CONTROL_RUNNING_STATE;
        }
    }

    return MenuState::CONTROL_PAGE_STATE;
}

// =============================================================================
// 固件版本页
// =============================================================================
MenuState menu_firmware_page(void)
{
    auto *oled = drv_oled();
    auto *key3 = drv_key(BOARD_KEY_3);

    oled->showString(0, 16, "Version: V1.0.0");

    if (key3 && key3->getEvent() == DrvKey::Event::ShortPress)
        return MenuState::MAIN_PAGE_STATE;

    return MenuState::FIRMWARE_PAGE_STATE;
}

// =============================================================================
// 任务运行中页: 显示当前模式, 短按 key3 退出
// =============================================================================
MenuState menu_control_running_page(void)
{
    auto *oled = drv_oled();
    auto *key3 = drv_key(BOARD_KEY_3);

    oled->showChinese(0, 0, "当前模式:");

    // 显示权威模式字 (菜单/遥控最后写入者)
    uint8_t mode = g_control_mode;

    // 模式已被遥控停机/保护触发清除 -> 显示停止并自动返回主菜单
    if (mode == 0xFF)
    {
        oled->showChinese(0, 18, "已停止");
        return MenuState::MAIN_PAGE_STATE;
    }

    switch (mode)
    {
        case 0:  oled->showChinese(0, 18, "平衡模式"); break;
        case 1:  oled->showChinese(0, 18, "单点保持"); break;
        case 4:  oled->showChinese(0, 18, "轮速测试"); break;
        default: oled->showChinese(0, 18, "未选择");   break;
    }

    oled->showString(0, 40, "key3: back");

    if (key3)
    {
        // 重新采样按键, 避免控制任务抢占导致事件丢失
        key3->update();
        DrvKey::Event evt = key3->getEvent();
        if (evt == DrvKey::Event::ShortPress)
        {
            g_control_mode = 0xFF;   // 退出任务模式
            if (g_controlModeSem != NULL)
                osSemaphoreRelease(g_controlModeSem);

            return MenuState::CONTROL_PAGE_STATE;
        }
    }

    return MenuState::CONTROL_RUNNING_STATE;
}

// =============================================================================
// 陀螺仪零偏校准页: 长按 key3 启动校准
// =============================================================================
MenuState menu_gyro_calib_page(void)
{
    auto *oled = drv_oled();
    auto *key3 = drv_key(BOARD_KEY_3);

    oled->showChinese(0, 0, "陀螺仪校准");

    oled->showChinese(0, 18, "状态:");
    if (g_gyro_calibrated)
        oled->showChinese(36, 18, "已校准");
    else
        oled->showChinese(36, 18, "未校准");

    oled->showChinese(0, 36, "长按key3开始");

    if (key3)
    {
        DrvKey::Event evt = key3->getEvent();
        if (evt == DrvKey::Event::ShortPress)
            return MenuState::MAIN_PAGE_STATE;

        if (evt == DrvKey::Event::LongPress)
        {
            if (g_calibSem != NULL)
            {
                g_calib_command = 1;       // 陀螺仪零偏校准
                osSemaphoreRelease(g_calibSem);
            }
        }
    }

    return MenuState::GYRO_CALIB_PAGE_STATE;
}

// =============================================================================
// 电调编号设置页: 2 路电调
// 注意: 发送设置命令时, CAN 总线上务必只接目标电调, 否则编号会被覆盖
// =============================================================================
MenuState menu_esc_index_page(void)
{
    auto *oled  = drv_oled();
    auto *esc   = drv_cubefoc();
    auto *key1  = drv_key(BOARD_KEY_1);
    auto *key2  = drv_key(BOARD_KEY_2);
    auto *key3  = drv_key(BOARD_KEY_3);

    const int8_t MENU_COUNT = 2;
    const int16_t VISIBLE_Y[3] = {18, 32, 46};
    const char *menu_names[MENU_COUNT] = {"设为电调1", "设为电调2"};

    static int8_t selected_idx = 0;
    static int8_t scroll_top   = 0;
    static bool   set_success  = false;

    if (!set_success)
    {
        for (int8_t i = 0; i < 3; i++)
        {
            int8_t item_idx = scroll_top + i;
            if (item_idx < MENU_COUNT)
                oled->showChinese(0, VISIBLE_Y[i], menu_names[item_idx]);
        }

        if (key1 && key1->getEvent() == DrvKey::Event::ShortPress)
        {
            selected_idx = (int8_t)((selected_idx - 1 + MENU_COUNT) % MENU_COUNT);
            if (selected_idx < scroll_top)      scroll_top = selected_idx;
            if (selected_idx >= scroll_top + 3) scroll_top = (int8_t)(selected_idx - 2);
        }

        if (key2 && key2->getEvent() == DrvKey::Event::ShortPress)
        {
            selected_idx = (int8_t)((selected_idx + 1) % MENU_COUNT);
            if (selected_idx < scroll_top)      scroll_top = selected_idx;
            if (selected_idx >= scroll_top + 3) scroll_top = (int8_t)(selected_idx - 2);
        }

        int16_t draw_frame_y_pos = VISIBLE_Y[selected_idx - scroll_top];
        oled->drawRectangle(0, draw_frame_y_pos, 120, 15, 0);

        if (key3)
        {
            DrvKey::Event evt = key3->getEvent();
            if (evt == DrvKey::Event::ShortPress)
            {
                selected_idx = 0;
                scroll_top   = 0;
                return MenuState::MAIN_PAGE_STATE;
            }

            if (evt == DrvKey::Event::LongPress && esc)
            {
                // 目标电调编号 (1-based)
                uint8_t targets[MENU_COUNT] = {ESC1_Index + 1, ESC2_Index + 1};
                esc->set_esc_index_command(targets[selected_idx]);
                set_success = true;
            }
        }
    }
    else
    {
        oled->showChinese(0, 18, "设置成功");
        if (key3 && key3->getEvent() == DrvKey::Event::ShortPress)
        {
            selected_idx = 0;
            scroll_top   = 0;
            set_success  = false;
            return MenuState::MAIN_PAGE_STATE;
        }
    }

    return MenuState::ESC_INDEX_PAGE_STATE;
}
