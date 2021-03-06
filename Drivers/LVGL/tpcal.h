/**
 * @file tpcal.h
 *
 */

#ifndef TPCAL_H
#define TPCAL_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#ifdef LV_CONF_INCLUDE_SIMPLE
#include "lvgl.h"
#include "lv_conf.h"
#else
#include "lvgl.h"
#include "lv_conf.h"
#endif
//#define USE_LV_DEMO 1
//#define USE_LV_TPCAL 1
#if USE_LV_DEMO

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create a touch pad calibration screen
 */
void tpcal_create(void);

/**********************
 *      MACROS
 **********************/

#endif /*USE_LV_TPCAL*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*TP_CAL_H*/
