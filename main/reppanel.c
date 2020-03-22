//
// Created by cyber on 16.02.20.
//

#include <lvgl/src/lv_core/lv_style.h>
#include <lvgl/src/lv_core/lv_obj.h>
#include <lvgl/src/lv_objx/lv_label.h>
#include <lvgl/lvgl.h>
#include <custom_themes/lv_theme_rep_panel_light.h>
#include "reppanel.h"
#include "reppanel_process.h"
#include "reppanel_machine.h"
#include "reppanel_info.h"
#include "esp32_wifi.h"
#include "reppanel_macros.h"
#include "reppanel_jobstatus.h"
#include "reppanel_console.h"
#include "reppanel_jobselect.h"
#include <stdio.h>

void draw_header(lv_obj_t *parent_screen);

void draw_main_menu(lv_obj_t *parent_screen);

/**********************
 *  STATIC VARIABLES
 **********************/

uint8_t reppanel_conn_status = REPPANEL_NO_CONNECTION;

lv_obj_t *process_scr;  // screen for the process settings
lv_obj_t *machine_scr;
lv_obj_t *mainmenu_scr; // screen for the main_menue
lv_obj_t *info_scr;     // screen for the info
lv_obj_t *macro_scr;    // macro screen
lv_obj_t *jobstatus_scr;
lv_obj_t *jobselect_scr;
lv_obj_t *console_scr;

lv_obj_t *label_status;
lv_obj_t *label_chamber_temp;
lv_obj_t *main_menu_button;
lv_obj_t *console_button;
lv_obj_t *label_connection_status;

char reppanel_status[MAX_REPRAP_STATUS_LEN];
char reppanel_chamber_temp[MAX_REPRAP_STATUS_LEN];
char reppanel_job_progess[MAX_PREPANEL_TEMP_LEN];

int heater_states[MAX_NUM_TOOLS];
int num_heaters = 1;
bool job_running = false;

void rep_panel_ui_create() {
    lv_theme_t *th = lv_theme_reppanel_light_init(210, &reppanel_font_roboto_regular_22);
    lv_theme_set_current(th);

    mainmenu_scr = lv_cont_create(NULL, NULL);
    lv_cont_set_layout(mainmenu_scr, LV_LAYOUT_COL_M);
    draw_header(mainmenu_scr);
    draw_main_menu(mainmenu_scr);
    lv_scr_load(mainmenu_scr);
}

static void display_mainmenu_event(lv_obj_t *obj, lv_event_t event) {
    if (event == LV_EVENT_RELEASED) {
        if (mainmenu_scr) lv_obj_del(mainmenu_scr);
        mainmenu_scr = lv_cont_create(NULL, NULL);
        lv_cont_set_layout(mainmenu_scr, LV_LAYOUT_COL_M);
        draw_header(mainmenu_scr);
        draw_main_menu(mainmenu_scr);
        lv_scr_load(mainmenu_scr);
    }
}

static void close_conn_info_event_handler(lv_obj_t *obj, lv_event_t event) {
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_mbox_start_auto_close(obj, 0);
    }
}

static void connection_info_event(lv_obj_t *obj, lv_event_t event) {
    if (event == LV_EVENT_RELEASED) {
        static const char *btns[] = {"Close", ""};
        char conn_txt[200];
        get_connection_info(conn_txt);
        lv_obj_t *mbox1 = lv_mbox_create(lv_layer_top(), NULL);
        lv_mbox_set_text(mbox1, conn_txt);
        lv_mbox_add_btns(mbox1, btns);
        lv_obj_set_width(mbox1, 250);
        lv_obj_set_event_cb(mbox1, close_conn_info_event_handler);
        lv_obj_align(mbox1, NULL, LV_ALIGN_CENTER, 0, 0); /*Align to the corner*/
    }
}

static void display_console_event(lv_obj_t *obj, lv_event_t event) {
    if (event == LV_EVENT_RELEASED) {
        if (console_scr) lv_obj_del(console_scr);
        console_scr = lv_cont_create(NULL, NULL);
        lv_cont_set_layout(console_scr, LV_LAYOUT_COL_M);
        draw_header(console_scr);
        draw_console(console_scr);
        lv_scr_load(console_scr);
    }
}

void display_jobstatus() {
    if (jobstatus_scr) lv_obj_del(jobstatus_scr);
    jobstatus_scr = lv_cont_create(NULL, NULL);
    lv_cont_set_layout(jobstatus_scr, LV_LAYOUT_COL_M);
    draw_header(jobstatus_scr);
    draw_jobstatus(jobstatus_scr);
    lv_scr_load(jobstatus_scr);
}

/**
 * Draw main header at top of screen showing button for main menu navigation
 * @param parent_screen Parent screen to draw elements on
 */
void draw_header(lv_obj_t *parent_screen) {
    lv_obj_t *cont_header = lv_cont_create(parent_screen, NULL);
    lv_cont_set_fit2(cont_header, LV_FIT_FLOOD, LV_FIT_TIGHT);
    lv_cont_set_layout(cont_header, LV_LAYOUT_OFF);
    lv_obj_align(cont_header, parent_screen, LV_ALIGN_IN_TOP_MID, 0, 0);

    lv_obj_t *cont_header_left = lv_cont_create(cont_header, NULL);
    lv_cont_set_fit(cont_header_left, LV_FIT_TIGHT);
    lv_cont_set_layout(cont_header_left, LV_LAYOUT_ROW_M);
    lv_obj_set_event_cb(cont_header_left, display_mainmenu_event);
    lv_obj_align(cont_header_left, cont_header, LV_ALIGN_IN_TOP_LEFT, 10, 200);


    LV_IMG_DECLARE(mainmenubutton);
    static lv_style_t style_main_button;
    lv_style_copy(&style_main_button, &lv_style_plain);
    style_main_button.image.color = LV_COLOR_BLACK;
    style_main_button.image.intense = LV_OPA_50;
    style_main_button.text.color = lv_color_hex3(0xaaa);

    main_menu_button = lv_imgbtn_create(cont_header_left, NULL);
    lv_imgbtn_set_src(main_menu_button, LV_BTN_STATE_REL, &mainmenubutton);
    lv_imgbtn_set_src(main_menu_button, LV_BTN_STATE_PR, &mainmenubutton);
    lv_imgbtn_set_src(main_menu_button, LV_BTN_STATE_TGL_REL, &mainmenubutton);
    lv_imgbtn_set_src(main_menu_button, LV_BTN_STATE_TGL_PR, &mainmenubutton);
    lv_imgbtn_set_style(main_menu_button, LV_BTN_STATE_PR, &style_main_button);
    lv_imgbtn_set_style(main_menu_button, LV_BTN_STATE_TGL_PR, &style_main_button);
    lv_imgbtn_set_toggle(main_menu_button, false);
    lv_obj_set_event_cb(main_menu_button, display_mainmenu_event);

    label_status = lv_label_create(cont_header_left, NULL);
    static lv_style_t style_status_label;
    lv_style_copy(&style_status_label, &lv_style_plain);
    style_status_label.text.color = REP_PANEL_DARK_ACCENT;
    style_status_label.text.font = &reppanel_font_roboto_bold_24;
    lv_obj_set_style(label_status, &style_status_label);
    lv_label_set_text(label_status, reppanel_status);

    lv_obj_t *cont_header_right = lv_cont_create(cont_header, NULL);
    lv_cont_set_fit(cont_header_right, LV_FIT_TIGHT);
    lv_cont_set_layout(cont_header_right, LV_LAYOUT_ROW_M);
    lv_obj_align(cont_header_right, cont_header, LV_ALIGN_IN_TOP_RIGHT, -120, 12);

    lv_obj_t *click_cont = lv_cont_create(cont_header_right, NULL);
    lv_cont_set_fit(click_cont, LV_FIT_TIGHT);
    label_connection_status = lv_label_create(click_cont, NULL);
    lv_label_set_recolor(label_connection_status, true);
    update_rep_panel_conn_status();
    lv_obj_set_event_cb(click_cont, connection_info_event);

    lv_obj_t *img_chamber_tmp = lv_img_create(cont_header_right, NULL);
    LV_IMG_DECLARE(chamber_tmp);
    lv_img_set_src(img_chamber_tmp, &chamber_tmp);

    label_chamber_temp = lv_label_create(cont_header_right, NULL);
    lv_label_set_text_fmt(label_chamber_temp, "%.01f°%c",
                          reprap_tools[current_visible_tool_indx].temp_buff[reprap_tools[current_visible_tool_indx].temp_hist_curr_pos],
                          get_temp_unit());

    LV_IMG_DECLARE(consolebutton);
    static lv_style_t style_console_button;
    lv_style_copy(&style_console_button, &lv_style_plain);
    style_console_button.image.color = LV_COLOR_BLACK;
    style_console_button.image.intense = LV_OPA_50;
    style_console_button.text.color = lv_color_hex3(0xaaa);

    console_button = lv_imgbtn_create(cont_header_right, NULL);
    lv_imgbtn_set_src(console_button, LV_BTN_STATE_REL, &consolebutton);
    lv_imgbtn_set_src(console_button, LV_BTN_STATE_PR, &consolebutton);
    lv_imgbtn_set_src(console_button, LV_BTN_STATE_TGL_REL, &consolebutton);
    lv_imgbtn_set_src(console_button, LV_BTN_STATE_TGL_PR, &consolebutton);
    lv_imgbtn_set_style(console_button, LV_BTN_STATE_PR, &style_console_button);
    lv_imgbtn_set_style(console_button, LV_BTN_STATE_TGL_PR, &style_console_button);
    lv_imgbtn_set_toggle(console_button, true);
    lv_obj_set_event_cb(console_button, display_console_event);
}

static void main_menu_event_handler(lv_obj_t *obj, lv_event_t event) {
    if (event == LV_EVENT_RELEASED) {
        const char *txt = lv_btnm_get_active_btn_text(obj);
        printf("%s was pressed\n", txt);
        update_rep_panel_conn_status();
        if (strcmp(txt, "Process") == 0) {
            if (process_scr) lv_obj_del(process_scr);
            process_scr = lv_cont_create(NULL, NULL);
            lv_cont_set_layout(process_scr, LV_LAYOUT_COL_M);
            draw_header(process_scr);
            draw_process(process_scr);
            lv_scr_load(process_scr);
        } else if (strcmp(txt, "Machine") == 0) {
            if (machine_scr) lv_obj_del(machine_scr);
            machine_scr = lv_cont_create(NULL, NULL);
            lv_cont_set_layout(machine_scr, LV_LAYOUT_COL_M);
            draw_header(machine_scr);
            draw_machine(machine_scr);
            lv_scr_load(machine_scr);
        } else if (strcmp(txt, "Info") == 0) {
            if (info_scr) lv_obj_del(info_scr);
            info_scr = lv_cont_create(NULL, NULL);
            lv_cont_set_layout(info_scr, LV_LAYOUT_COL_M);
            draw_header(info_scr);
            draw_info(info_scr);
            lv_scr_load(info_scr);
        } else if (strcmp(txt, "Macros") == 0) {
            if (macro_scr) lv_obj_del(macro_scr);
            macro_scr = lv_cont_create(NULL, NULL);
            lv_cont_set_layout(macro_scr, LV_LAYOUT_COL_M);
            draw_header(macro_scr);
            draw_macro(macro_scr);
            lv_scr_load(macro_scr);
        } else if (strcmp(txt, "Job") == 0) {
            if (job_running) {
                display_jobstatus();
            } else {
                if (jobselect_scr) lv_obj_del(jobselect_scr);
                jobselect_scr = lv_cont_create(NULL, NULL);
                lv_cont_set_layout(jobselect_scr, LV_LAYOUT_COL_M);
                draw_header(jobselect_scr);
                draw_jobselect(jobselect_scr);
                lv_scr_load(jobselect_scr);
            }
        }
    }
}

void update_rep_panel_conn_status() {
    switch (reppanel_conn_status) {
        default:
        case REPPANEL_NO_CONNECTION:     // no connection
            lv_label_set_text_fmt(label_connection_status, "#e84e43 "LV_SYMBOL_WARNING"#");
            break;
        case REPPANEL_WIFI_CONNECTED:     // connected wifi
            lv_label_set_text_fmt(label_connection_status, REP_PANEL_DARK_ACCENT_STR" "LV_SYMBOL_WIFI);
            break;
        case REPPANEL_WIFI_DISCONNECTED:     // disconnected wifi
            lv_label_set_text_fmt(label_connection_status, "#e84e43 "LV_SYMBOL_WIFI);
            break;
        case REPPANEL_WIFI_RECONNECTING:     // reconnecting wifi
            lv_label_set_text_fmt(label_connection_status, "#e89e43 "LV_SYMBOL_REFRESH);
            break;
        case REPPANEL_UART_CONNECTED:     // working UART
            lv_label_set_text_fmt(label_connection_status, REP_PANEL_DARK_ACCENT_STR" "LV_SYMBOL_USB);
            break;
    }
}

static const char *main_menu_button_map[] = {"Process", "Job", "Machine", "\n", "Macros", "Info", ""};

void draw_main_menu(lv_obj_t *parent_screen) {
    lv_obj_t *btnm1 = lv_btnm_create(parent_screen, NULL);
    lv_btnm_set_map(btnm1, main_menu_button_map);
    lv_btnm_set_btn_width(btnm1, 10, 2);        /*Make "Action1" twice as wide as "Action2"*/
    lv_obj_align(btnm1, NULL, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_event_cb(btnm1, main_menu_event_handler);
}