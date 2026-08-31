#include "task_Display.hpp"
#include "drv_oled.hpp"
#include "drv_key.hpp"
#include "menu.hpp"
#include "mavlink.hpp"
#include "task_Control.hpp"

void StartDisplayTask(void *argument)
{
    auto *oled = drv_oled();
    auto *key1 = drv_key(BOARD_KEY_1);
    auto *key2 = drv_key(BOARD_KEY_2);
    auto *key3 = drv_key(BOARD_KEY_3);

    // 主菜单项
    const int8_t  MENU_COUNT = 5;
    const int16_t VISIBLE_Y[3] = {18, 32, 46};
    const char *menu_names[5] = {
        "姿态显示",
        "任务控制",
        "固件版本",
        "陀螺仪校准",
        "电调编号设置"
    };

    int16_t draw_frame_y_pos = 0;
    int8_t  selected_idx = 0;
    int8_t  scroll_top   = 0;

    MenuState main_menu_flag = MenuState::MAIN_PAGE_STATE;
    MenuBatState bat_status = MenuBatState::MEASURE_ERR;
    float bat_voltage = 0.0f;

    // 启动 LOGO
    oled->clear();
    oled->displayLogo();
    oled->update();
    osDelay(1500);

    // 预采样一次按键, 清除上电抖动事件
    if (key1) { key1->update(); key1->getEvent(); }
    if (key2) { key2->update(); key2->getEvent(); }
    if (key3) { key3->update(); key3->getEvent(); }

    uint32_t next_wake = osKernelGetTickCount();

    while (1)
    {
        oled->clear();

        // 更新按键状态 (供菜单页内 getEvent 读取)
        if (key1) key1->update();
        if (key2) key2->update();
        if (key3) key3->update();

        // 取最新的显示传感数据帧 (ESC 总线电压)
        if (g_dispSensorQueue != NULL)
        {
            MavSensorData_t sensor_data;
            while (osMessageQueueGet(g_dispSensorQueue, &sensor_data, NULL, 0) == osOK)
                bat_voltage = sensor_data.voltage;
        }

        // 获取电池状态 (按电压分级)
        bat_status = menu_battery_state(bat_voltage);

        // ===================== 主菜单 =====================
        if (main_menu_flag == MenuState::MAIN_PAGE_STATE)
        {
            for (int8_t i = 0; i < 3; i++)
            {
                int8_t item_idx = scroll_top + i;
                if (item_idx < MENU_COUNT)
                {
                    oled->showChinese(0, VISIBLE_Y[i], menu_names[item_idx]);

                    // 陀螺仪校准状态指示
                    if (item_idx == 3 && g_gyro_calibrated)
                        oled->showString(96, VISIBLE_Y[i], "OK");
                }
            }

            // key1 短按: 上移
            if (key1 && key1->getEvent() == DrvKey::Event::ShortPress)
            {
                selected_idx = (int8_t)((selected_idx - 1 + MENU_COUNT) % MENU_COUNT);
                if (selected_idx < scroll_top)      scroll_top = selected_idx;
                if (selected_idx >= scroll_top + 3) scroll_top = (int8_t)(selected_idx - 2);
            }

            // key2 短按: 下移
            if (key2 && key2->getEvent() == DrvKey::Event::ShortPress)
            {
                selected_idx = (int8_t)((selected_idx + 1) % MENU_COUNT);
                if (selected_idx < scroll_top)      scroll_top = selected_idx;
                if (selected_idx >= scroll_top + 3) scroll_top = (int8_t)(selected_idx - 2);
            }

            // 选择框
            draw_frame_y_pos = VISIBLE_Y[selected_idx - scroll_top];
            if (draw_frame_y_pos != 0)
                oled->drawRectangle(0, draw_frame_y_pos, 120, 15, 0);

            // key3 长按: 进入子界面
            if (key3 && key3->getEvent() == DrvKey::Event::LongPress)
            {
                switch (selected_idx)
                {
                    case 0: main_menu_flag = MenuState::ATT_PAGE_STATE;        break;
                    case 1: main_menu_flag = MenuState::CONTROL_PAGE_STATE;    break;
                    case 2: main_menu_flag = MenuState::FIRMWARE_PAGE_STATE;   break;
                    case 3: main_menu_flag = MenuState::GYRO_CALIB_PAGE_STATE; break;
                    case 4: main_menu_flag = MenuState::ESC_INDEX_PAGE_STATE;  break;
                }

                // 长按后松手检测
                while (key3 && key3->isPressed())
                {
                    key3->update();
                    osDelay(10);
                }
            }
        }

        // ===================== 子界面分发 =====================
        switch (main_menu_flag)
        {
            case MenuState::ATT_PAGE_STATE:             main_menu_flag = menu_att_page();             break;
            case MenuState::CONTROL_PAGE_STATE:         main_menu_flag = menu_control_page();         break;
            case MenuState::FIRMWARE_PAGE_STATE:        main_menu_flag = menu_firmware_page();        break;
            case MenuState::CONTROL_RUNNING_STATE:      main_menu_flag = menu_control_running_page(); break;
            case MenuState::GYRO_CALIB_PAGE_STATE:      main_menu_flag = menu_gyro_calib_page();      break;
            case MenuState::ESC_INDEX_PAGE_STATE:       main_menu_flag = menu_esc_index_page();       break;
            default: break;
        }

        // ===================== 顶栏: 蓝牙 + 电池图标 =====================
        if (MAVLink::get_mavlink_connect_status())
            oled->showBLEON();
        else
            oled->showBLEOFF();

        switch (bat_status)
        {
            case MenuBatState::FULL_POWER: oled->fullBattery();   break;
            case MenuBatState::HIGH_POWER: oled->highBattery();   break;
            case MenuBatState::MID_POWER:  oled->middleBattery(); break;
            case MenuBatState::LOW_POWER:  oled->lowBattery();    break;
            default: break;
        }

        oled->update();

        next_wake += 30U;
        osDelayUntil(next_wake);
    }
}
