/**
 * @file tpcal.c
 *
 * TOUCHPAD CALIBRATION
 * ---------------------
 *
 * This application creates a GUI and instruct the user
 * to click the four corners to get data for touchpad calibration.
 *
 * - You display driver should have two functions: `xxx_read` and `xxx_set_cal_data`.
 * - At first run run the touchpad is not calibrated therefore your `xxx_read` function should provide raw data.
 * - When the user touched all four corners you should call the `xxx_set_cal_data` function in
 * ` TP_CAL_STATE_WAIT_LEAVE` state. As arguments you should pass `point[0]`, `point[1]`, `point[2]` and `point[3]`
 *   which are the coordinates read on corner pressing.
 * - `xxx_set_cal_data` should mark the display as calibrated, save the raw coordinates
 *    and use them in the upcoming calls of `xxx_read` to adjust the coordinates.
 * - A simple equation to adjust the coordinates: x_cal = ((x_act - x1_saved) * lcd_hor_res) / (x2_saved - x1_saved);
 *      - x_cal: the calibrated X coordinate
 *      - x_act: the currently measured X coordinate
 *      - x1_saved, x2_saved: The raw X coordinates saved as calibration data
 */

/*********************
 *      INCLUDES
 *********************/
#include "tpcal.h"
#if USE_LV_TPCAL
#include <stdio.h>

/*********************
 *      DEFINES
 *********************/
#define CIRCLE_SIZE      20
#define CIRCLE_OFFSET    20
#define TP_MAX_VALUE     5000
#define TOUCH_NUMBER     3

/**********************
 *      TYPEDEFS
 **********************/
typedef enum {
    TP_CAL_STATE_INIT,
    TP_CAL_STATE_WAIT_TOP_LEFT,
    TP_CAL_STATE_WAIT_TOP_RIGHT,
    TP_CAL_STATE_WAIT_BOTTOM_RIGHT,
    TP_CAL_STATE_WAIT_BOTTOM_LEFT,
    TP_CAL_STATE_WAIT_LEAVE,
    TP_CAL_STATE_READY,
} tp_cal_state_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void get_avr_value(lv_point_t * p);
static lv_res_t btn_click_action(lv_obj_t * scr);

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_point_t point[4]; /*Calibration points: [0]: top-left; [1]: top-right, [2]: bottom-right, [3]: bottom-left */
static lv_point_t avr[TOUCH_NUMBER]; /*Storage point to calculate average*/

static tp_cal_state_t state;
static lv_obj_t * prev_scr;
static lv_obj_t * big_btn;
static lv_obj_t * label_main;
static lv_obj_t * circ_area;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * Create a touch pad calibration screen
 */
void tpcal_create(void)
{
    state = TP_CAL_STATE_INIT;

    prev_scr = lv_scr_act();

    lv_obj_t * scr = lv_obj_create( NULL);
    lv_obj_set_size(scr, TP_MAX_VALUE, TP_MAX_VALUE);
    lv_scr_load(scr);

    /*Create a big transparent button screen to receive clicks*/
    big_btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(big_btn, TP_MAX_VALUE, TP_MAX_VALUE);
    lv_obj_add_event_cb(big_btn,btn_click_action,NULL);


    label_main = lv_label_create(lv_scr_act(), NULL);
    char buf[64];
    sprintf(buf, "Click the circle in\n"
            "upper left-hand corner\n"
            "%u left", TOUCH_NUMBER);
    lv_label_set_text(label_main, buf);
    lv_label_set_align(label_main, LV_LABEL_ALIGN_CENTER);

    lv_obj_set_pos(label_main, (LV_HOR_RES - lv_obj_get_width(label_main)) / 2,
                   (LV_VER_RES - lv_obj_get_height(label_main)) / 2);


    static lv_style_t style_circ;
    lv_style_copy(&style_circ, &lv_style_pretty_color);
    style_circ.body.radius = LV_RADIUS_CIRCLE;

    circ_area = lv_obj_create(lv_scr_act(), NULL);
    lv_obj_set_size(circ_area, CIRCLE_SIZE, CIRCLE_SIZE);
    lv_obj_set_style(circ_area, &style_circ);
    lv_obj_set_click(circ_area, false);

#if USE_LV_ANIMATION
    lv_anim_t a;
    a.var = circ_area;
    a.start = LV_HOR_RES / 2;
    a.end = CIRCLE_OFFSET;
    a.fp = (lv_anim_fp_t)lv_obj_set_x;
    a.path = lv_anim_path_linear;
    a.end_cb = NULL;
    a.act_time = -500;
    a.time = 200;
    a.playback = 0;
    a.playback_pause = 0;
    a.repeat = 0;
    a.repeat_pause = 0;
    lv_anim_create(&a);

    a.start = LV_VER_RES / 2;
    a.end = CIRCLE_OFFSET;
    a.fp = (lv_anim_fp_t)lv_obj_set_y;
    a.end_cb = NULL;
    a.time = 200;
    lv_anim_create(&a);
#else
    lv_obj_set_pos(circ_area, CIRCLE_OFFSET, CIRCLE_OFFSET);
#endif

    state = TP_CAL_STATE_WAIT_TOP_LEFT;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void get_avr_value(lv_point_t * p)
{
    int32_t x_sum = 0;
    int32_t y_sum = 0;
    uint8_t i = 0;
    for(; i < TOUCH_NUMBER ; i++) {
        x_sum += avr[i].x;
        y_sum += avr[i].y;
    }
    p->x = x_sum / TOUCH_NUMBER;
    p->y = y_sum / TOUCH_NUMBER;
}

static lv_res_t btn_click_action(lv_obj_t * scr)
{
    (void) scr;    /*Unused*/

    static uint8_t touch_nb = TOUCH_NUMBER;

    if(state == TP_CAL_STATE_WAIT_TOP_LEFT) {
        char buf[64];
        touch_nb--;
        lv_indev_t * indev = lv_indev_get_act();
        lv_indev_get_point(indev, &avr[touch_nb]);

        if(!touch_nb) {
            touch_nb = TOUCH_NUMBER;
            get_avr_value(&point[0]);
            sprintf(buf, "x: %d\ny: %d", point[0].x, point[0].y);
            lv_obj_t * label_coord = lv_label_create(lv_scr_act(), NULL);
            lv_label_set_text(label_coord, buf);
            sprintf(buf, "Click the circle in\n"
                    "upper right-hand corner\n"
                    " %u Left", TOUCH_NUMBER);
#if USE_LV_ANIMATION
            lv_anim_t a;
            a.var = circ_area;
            a.start = CIRCLE_OFFSET;
            a.end = LV_HOR_RES - CIRCLE_SIZE - CIRCLE_OFFSET;
            a.fp = (lv_anim_fp_t)lv_obj_set_x;
            a.path = lv_anim_path_linear;
            a.end_cb = NULL;
            a.act_time = 0;
            a.time = 200;
            a.playback = 0;
            a.playback_pause = 0;
            a.repeat = 0;
            a.repeat_pause = 0;
            lv_anim_create(&a);

            a.start = CIRCLE_OFFSET;
            a.end = CIRCLE_OFFSET;
            a.fp = (lv_anim_fp_t)lv_obj_set_y;
            a.end_cb = NULL;
            a.time = 200;
            lv_anim_create(&a);
#else
            lv_obj_set_pos(circ_area, LV_HOR_RES - CIRCLE_SIZE - CIRCLE_OFFSET, CIRCLE_OFFSET);
#endif
            state = TP_CAL_STATE_WAIT_TOP_RIGHT;
        } else {
            sprintf(buf, "Click the circle in\n"
                    "upper left-hand corner\n"
                    " %u Left", touch_nb);
        }
        lv_label_set_text(label_main, buf);
        lv_obj_set_pos(label_main, (LV_HOR_RES - lv_obj_get_width(label_main)) / 2,
                       (LV_VER_RES - lv_obj_get_height(label_main)) / 2);


    } else if(state == TP_CAL_STATE_WAIT_TOP_RIGHT) {
        char buf[64];
        touch_nb--;
        lv_indev_t * indev = lv_indev_get_act();
        lv_indev_get_point(indev, &avr[touch_nb]);

        if(!touch_nb) {
            touch_nb = TOUCH_NUMBER;
            get_avr_value(&point[1]);
            sprintf(buf, "x: %d\ny: %d", point[1].x, point[1].y);
            lv_obj_t * label_coord = lv_label_create(lv_scr_act(), NULL);
            lv_label_set_text(label_coord, buf);
            lv_obj_set_pos(label_coord, LV_HOR_RES - lv_obj_get_width(label_coord), 0);
            sprintf(buf, "Click the circle in\n"
                    "lower right-hand corner\n"
                    " %u Left", TOUCH_NUMBER);
#if USE_LV_ANIMATION
            lv_anim_t a;
            a.var = circ_area;
            a.start = LV_HOR_RES - CIRCLE_SIZE - CIRCLE_OFFSET;
            a.end = LV_HOR_RES - CIRCLE_SIZE - CIRCLE_OFFSET;
            a.fp = (lv_anim_fp_t)lv_obj_set_x;
            a.path = lv_anim_path_linear;
            a.end_cb = NULL;
            a.act_time = 0;
            a.time = 200;
            a.playback = 0;
            a.playback_pause = 0;
            a.repeat = 0;
            a.repeat_pause = 0;
            lv_anim_create(&a);

            a.start = CIRCLE_OFFSET;
            a.end = LV_VER_RES - CIRCLE_SIZE - CIRCLE_OFFSET;
            a.fp = (lv_anim_fp_t)lv_obj_set_y;
            a.end_cb = NULL;
            a.time = 200;
            lv_anim_create(&a);
#else
            lv_obj_set_pos(circ_area, LV_HOR_RES - CIRCLE_SIZE - CIRCLE_OFFSET, LV_VER_RES - CIRCLE_SIZE - CIRCLE_OFFSET);
#endif
            state = TP_CAL_STATE_WAIT_BOTTOM_RIGHT;
        } else {
            sprintf(buf, "Click the circle in\n"
                    "upper right-hand corner\n"
                    " %u Left", touch_nb);
        }
        lv_label_set_text(label_main, buf);
        lv_obj_set_pos(label_main, (LV_HOR_RES - lv_obj_get_width(label_main)) / 2,
                       (LV_VER_RES - lv_obj_get_height(label_main)) / 2);

    } else if(state == TP_CAL_STATE_WAIT_BOTTOM_RIGHT) {
        char buf[64];
        touch_nb--;
        lv_indev_t * indev = lv_indev_get_act();
        lv_indev_get_point(indev, &avr[touch_nb]);

        if(!touch_nb) {
            touch_nb = TOUCH_NUMBER;
            get_avr_value(&point[2]);
            sprintf(buf, "x: %d\ny: %d", point[2].x, point[2].y);
            lv_obj_t * label_coord = lv_label_create(lv_scr_act(), NULL);
            lv_label_set_text(label_coord, buf);
            sprintf(buf, "Click the circle in\n"
                    "lower left-hand corner\n"
                    " %u Left", TOUCH_NUMBER);
            lv_obj_set_pos(label_coord, LV_HOR_RES - lv_obj_get_width(label_coord),
                           LV_VER_RES - lv_obj_get_height(label_coord));
#if USE_LV_ANIMATION
            lv_anim_t a;
            a.var = circ_area;
            a.start = LV_HOR_RES - CIRCLE_SIZE - CIRCLE_OFFSET;
            a.end = CIRCLE_OFFSET;
            a.fp = (lv_anim_fp_t)lv_obj_set_x;
            a.path = lv_anim_path_linear;
            a.end_cb = NULL;
            a.act_time = 0;
            a.time = 200;
            a.playback = 0;
            a.playback_pause = 0;
            a.repeat = 0;
            a.repeat_pause = 0;
            lv_anim_create(&a);

            a.start = LV_VER_RES - CIRCLE_SIZE - CIRCLE_OFFSET;
            a.end = LV_VER_RES - CIRCLE_SIZE - CIRCLE_OFFSET;
            a.fp = (lv_anim_fp_t)lv_obj_set_y;
            a.end_cb = NULL;
            a.time = 200;
            lv_anim_create(&a);
#else
            lv_obj_set_pos(circ_area, CIRCLE_OFFSET, LV_VER_RES - CIRCLE_SIZE - CIRCLE_OFFSET);
#endif
            state = TP_CAL_STATE_WAIT_BOTTOM_LEFT;
        } else {
            sprintf(buf, "Click the circle in\n"
                    "lower right-hand corner\n"
                    " %u Left", touch_nb);
        }
        lv_label_set_text(label_main, buf);
        lv_obj_set_pos(label_main, (LV_HOR_RES - lv_obj_get_width(label_main)) / 2,
                       (LV_VER_RES - lv_obj_get_height(label_main)) / 2);
    } else if(state == TP_CAL_STATE_WAIT_BOTTOM_LEFT) {
        char buf[64];
        touch_nb--;
        lv_indev_t * indev = lv_indev_get_act();
        lv_indev_get_point(indev, &avr[touch_nb]);

        if(!touch_nb) {
            touch_nb = TOUCH_NUMBER;
            get_avr_value(&point[3]);
            sprintf(buf, "x: %d\ny: %d", point[3].x, point[3].y);
            lv_obj_t * label_coord = lv_label_create(lv_scr_act(), NULL);
            lv_label_set_text(label_coord, buf);
            lv_obj_set_pos(label_coord, 0, LV_VER_RES - lv_obj_get_height(label_coord));
            sprintf(buf, "Click the screen\n"
                    "to leave calibration");
            lv_obj_del(circ_area);
            state = TP_CAL_STATE_WAIT_LEAVE;
        } else {
            sprintf(buf, "Click the circle in\n"
                    "lower left-hand corner\n"
                    " %u Left", touch_nb);
        }
        lv_label_set_text(label_main, buf);
        lv_obj_set_pos(label_main, (LV_HOR_RES - lv_obj_get_width(label_main)) / 2,
                       (LV_VER_RES - lv_obj_get_height(label_main)) / 2);
    } else if(state == TP_CAL_STATE_WAIT_LEAVE) {
        lv_scr_load(prev_scr);

        /*
         * TODO Process 'p' points here to calibrate the touch pad
         * Offset will be: CIRCLE_SIZE/2 + CIRCLE_OFFSET
         */

        /*
         * TODO: you can change the calibrate input callback here e.g:
         *  lv_indev_t *indev = lv_indev_get_act();
         *  indev->driver.read = xxxx_input_get_calib;
         */

        state = TP_CAL_STATE_READY;

    } else if(state == TP_CAL_STATE_READY) {
    }

    return LV_RES_OK;
}

#endif /*USE_LV_TPCAL*/
