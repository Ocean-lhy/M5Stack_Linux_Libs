/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/**
 * @file main
 *
 */

/*********************
 *      INCLUDES
 *********************/
#define _DEFAULT_SOURCE /* needed for usleep() */
#include <unistd.h>
#define SDL_MAIN_HANDLED    /*To fix SDL's "undefined reference to WinMain" \
                               issue*/
#include <stdlib.h>
#include <time.h>

#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/evdev.h"
#include "lvgl/lvgl.h"
/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void hal_init(void);
static void cursor_set_hidden(bool en);
/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *      VARIABLES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_port_disp_init(void);
static void hal_init(void);
static void lv_port_indev_init(bool show_cursor);
static void cursor_set_hidden(bool en);
/**********************
 *   GLOBAL FUNCTIONS
 **********************/

// 添加计时相关的静态变量
static uint32_t elapsed_time = 0;  // 记录经过的时间（毫秒）
static bool is_running = false;    // 计时器运行状态
static lv_timer_t *timer = NULL;   // LVGL定时器

// 计时器回调函数
static void timer_cb(lv_timer_t *timer) {
    lv_obj_t *label = (lv_obj_t *)timer->user_data;
    elapsed_time += 1;  // 改为1ms更新一次
    
    // 计算时分秒毫秒
    uint32_t ms = elapsed_time % 1000;
    uint32_t seconds = (elapsed_time / 1000) % 60;
    uint32_t minutes = (elapsed_time / 60000) % 60;
    
    // 更新显示
    char buf[32];
    lv_snprintf(buf, sizeof(buf), "%02d:%02d.%03d", minutes, seconds, ms);
    lv_label_set_text(label, buf);
}

// 开始按钮回调
static void start_button_cb(lv_event_t *e) {
    if (!is_running) {
        lv_obj_t *label = lv_event_get_user_data(e);
        timer = lv_timer_create(timer_cb, 1, label);  // 改为1ms定时器
        is_running = true;
    }
}

// 暂停按钮回调
static void pause_button_cb(lv_event_t *e) {
    if (is_running && timer != NULL) {
        lv_timer_del(timer);
        timer = NULL;
        is_running = false;
    }
}

// 重置按钮回调
static void reset_button_cb(lv_event_t *e) {
    lv_obj_t *label = lv_event_get_user_data(e);
    if (is_running && timer != NULL) {
        lv_timer_del(timer);
        timer = NULL;
        is_running = false;
    }
    elapsed_time = 0;
    lv_label_set_text(label, "00:00.000");
}

void ui_init(void) {
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(
        dispp, lv_palette_main(LV_PALETTE_BLUE),
        lv_palette_main(LV_PALETTE_RED), false, &lv_font_simsun_16_cjk);
    lv_disp_set_theme(dispp, theme);

    lv_obj_t *ui_Screen1 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen1, LV_OBJ_FLAG_SCROLLABLE);

    // 创建显示时间的标签
    lv_obj_t *time_label = lv_label_create(ui_Screen1);
    // 使用更大的字体
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_32, 0);
    // 设置标签样式
    lv_obj_set_style_text_align(time_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(time_label, LV_PCT(100));  // 设置宽度为屏幕宽度
    lv_obj_set_style_pad_top(time_label, 80, 0);  // 添加上边距使显示更居中
    lv_label_set_text(time_label, "00:00.000");

    // 创建一个容器来放置按钮
    lv_obj_t *btn_container = lv_obj_create(ui_Screen1);
    lv_obj_remove_style_all(btn_container);  // 移除容器的默认样式
    lv_obj_set_size(btn_container, LV_PCT(100), 50);  // 设置容器高度
    lv_obj_align(btn_container, LV_ALIGN_BOTTOM_MID, 0, -20);  // 底部对齐
    lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);  // 设置为水平布局
    lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);  // 均匀分布

    // 创建开始按钮
    lv_obj_t *start_btn = lv_btn_create(btn_container);
    lv_obj_set_size(start_btn, 100, 45);
    lv_obj_add_event_cb(start_btn, start_button_cb, LV_EVENT_CLICKED, time_label);
    
    lv_obj_t *start_label = lv_label_create(start_btn);
    lv_obj_set_style_text_font(start_label, &lv_font_simsun_16_cjk, 0);
    lv_label_set_text(start_label, "Start");
    lv_obj_center(start_label);

    // 创建暂停按钮
    lv_obj_t *pause_btn = lv_btn_create(btn_container);
    lv_obj_set_size(pause_btn, 100, 45);
    lv_obj_add_event_cb(pause_btn, pause_button_cb, LV_EVENT_CLICKED, time_label);
    
    lv_obj_t *pause_label = lv_label_create(pause_btn);
    lv_obj_set_style_text_font(pause_label, &lv_font_simsun_16_cjk, 0);
    lv_label_set_text(pause_label, "Pause");
    lv_obj_center(pause_label);

    // 创建重置按钮
    lv_obj_t *reset_btn = lv_btn_create(btn_container);
    lv_obj_set_size(reset_btn, 100, 45);
    lv_obj_add_event_cb(reset_btn, reset_button_cb, LV_EVENT_CLICKED, time_label);
    
    lv_obj_t *reset_label = lv_label_create(reset_btn);
    lv_obj_set_style_text_font(reset_label, &lv_font_simsun_16_cjk, 0);
    lv_label_set_text(reset_label, "Clear");
    lv_obj_center(reset_label);

    lv_disp_load_scr(ui_Screen1);
}

int main(int argc, char **argv) {
    /*Initialize LVGL*/
    lv_init();

    /*Initialize the HAL (display, input devices, tick) for LVGL*/
    hal_init();

    ui_init();

    while (1) {
        /* Periodically call the lv_task handler.
         * It could be done in a timer interrupt or an OS task too.*/
        lv_timer_handler();
        usleep(3 * 1000);
    }

    return 0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * Initialize the Hardware Abstraction Layer (HAL) for LVGL
 */

static void hal_init(void) {
    lv_port_disp_init();
    lv_port_indev_init(1);
}

static void lv_port_disp_init(void) {
    /*Linux frame buffer device init*/

    fbdev_init();

    uint32_t width, height, dpi;
    fbdev_get_sizes(&width, &height, &dpi);

    printf("fbdev: %" PRIu32 " x %" PRIu32 " DPI: %" PRIu32 "\n", width, height,
           dpi);
    /*A buffer for LittlevGL to draw the screen's content*/
    uint32_t buf_size = width * height;

#if USE_DIRECT_MODE
    lv_color_t *buf = fbdev_get_fbp();
    LV_ASSERT_NULL(buf);
#else
    lv_color_t *buf = malloc(buf_size * sizeof(lv_color_t));
    LV_ASSERT_MALLOC(buf);
#endif

    /*Initialize a descriptor for the buffer*/
    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, buf, NULL, buf_size);

    /*Initialize and register a display driver*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf = &disp_buf;
#if USE_DIRECT_MODE
    disp_drv.flush_cb    = fbdev_flush_direct;
    disp_drv.direct_mode = 1;
#else
    disp_drv.flush_cb = fbdev_flush;
#endif
    disp_drv.hor_res = width;
    disp_drv.ver_res = height;
    lv_disp_drv_register(&disp_drv);

    lv_timer_set_period(_lv_disp_get_refr_timer(lv_disp_get_default()), 16);
    lv_timer_set_period(lv_anim_get_timer(), 16);
}

static void lv_port_indev_init(bool show_cursor) {
    evdev_init();

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = evdev_read;
    lv_indev_t *indev = lv_indev_drv_register(&indev_drv);

    if (show_cursor) {
        /*Set a cursor for the mouse*/
        LV_IMG_DECLARE(mouse_cursor_icon); /*Declare the image file.*/
        lv_obj_t *cursor_obj = lv_img_create(
            lv_scr_act()); /*Create an image object for the cursor */
        lv_img_set_src(cursor_obj, &mouse_cursor_icon); /*Set the image source*/
        lv_indev_set_cursor(
            indev, cursor_obj); /*Connect the image  object to the driver*/
    }

    lv_timer_set_period(lv_indev_get_read_timer(indev), 16);
}

uint32_t linux_millis(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint32_t tick = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    return tick;
}
static void cursor_set_hidden(bool en) {
    // hide cursor echo -e "\033[?25l"
    // show cursor echo -e "\033[?25h"
    int ret = system(en ? "echo -e \"\033[?25l\"" : "echo -e \"\033[?25h\"");
    LV_LOG_USER(ret == 0 ? " OK" : " ERROR");
}
