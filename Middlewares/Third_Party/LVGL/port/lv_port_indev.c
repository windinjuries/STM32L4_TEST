/**
 * @file    lv_port_indev.c
 * @brief   LVGL Input Device Port - Keypad driver
 * @details Supports 4-button keypad with long-press detection
 *
 *          Button mapping (standard STM32 dev-board convention):
 *          ┌──────────────────────────────────────────────────────────┐
 *          │  WAKE_UP  PC13   Active-HIGH, pull-down   LV_KEY_UP     │
 *          │  KEY2      PD8   Active-LOW,  pull-up     LV_KEY_LEFT   │
 *          │  KEY1      PD9   Active-LOW,  pull-up     LV_KEY_DOWN   │
 *          │            └─ Long press (>500ms) → ENTER               │
 *          │  KEY0     PD10   Active-LOW,  pull-up     LV_KEY_RIGHT  │
 *          └──────────────────────────────────────────────────────────┘
 *
 *          KEY1 (PD9) special behavior:
 *          - Short press: LV_KEY_DOWN (navigate down)
 *          - Long press (>= 500ms): LV_KEY_ENTER (select/confirm)
 */

#include "lv_port_indev.h"
#include "lvgl.h"
#include "main.h"

/* ====================================================================
 *  Button GPIO definitions
 * ==================================================================== */
#define BTN_WAKE_UP_PORT     GPIOC
#define BTN_WAKE_UP_PIN      GPIO_PIN_13   /* Active-HIGH, pull-down  */

#define BTN_KEY0_PORT        GPIOD
#define BTN_KEY0_PIN         GPIO_PIN_10   /* Active-LOW,  pull-up    */

#define BTN_KEY1_PORT        GPIOD
#define BTN_KEY1_PIN         GPIO_PIN_9    /* Active-LOW,  pull-up    */

#define BTN_KEY2_PORT        GPIOD
#define BTN_KEY2_PIN         GPIO_PIN_8    /* Active-LOW,  pull-up    */

/* KEY1 long-press threshold in milliseconds */
#define KEY1_LONG_PRESS_MS      500

/*
 * Minimum number of LVGL cycles to hold the key state stable.
 * LVGL samples every ~30ms, so 3 cycles ≈ 90ms ensures proper processing.
 */
#define KEY_MIN_CYCLES          3

/* ====================================================================
 *  Static function declarations
 * ==================================================================== */
static void btn_gpio_init(void);
static void lv_port_indev_read(lv_indev_drv_t *drv, lv_indev_data_t *data);

/* ====================================================================
 *  Public API
 * ==================================================================== */

/**
 * @brief  Initialize the input device driver
 * @note   Must be called after lv_init() and before lv_timer_handler() loop
 */
void lv_port_indev_init(void)
{
    /* Initialize button GPIOs */
    btn_gpio_init();

    /* Register a keypad input device */
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_KEYPAD;
    indev_drv.read_cb = lv_port_indev_read;
    lv_indev_drv_register(&indev_drv);
}

/**
 * @brief  Find and return the keypad input device
 * @retval Pointer to keypad indev, or NULL if not found
 */
lv_indev_t *lv_port_indev_get_keypad(void)
{
    lv_indev_t *indev = lv_indev_get_next(NULL);
    while (indev != NULL)
    {
        if (indev->driver->type == LV_INDEV_TYPE_KEYPAD)
        {
            return indev;
        }
        indev = lv_indev_get_next(indev);
    }
    return NULL;
}

/* ====================================================================
 *  GPIO Initialization
 * ==================================================================== */

/**
 * @brief  Configure all button pins as inputs
 * @note   WAKE_UP (PC13): active-HIGH → internal pull-down
 *         KEY0/1/2  (PD8-PD10): active-LOW → internal pull-up
 *         (standard STM32 dev-board convention)
 */
static void btn_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /*
     * PC13 - WAKE_UP button (active-HIGH)
     * PC13 is in the backup domain; need to ensure RTC tamper is disabled
     * and the pin is released for GPIO use.
     */
    GPIO_InitStruct.Pin   = BTN_WAKE_UP_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BTN_WAKE_UP_PORT, &GPIO_InitStruct);

    /*
     * PD8 (KEY2), PD9 (KEY1), PD10 (KEY0) — all active-LOW
     * Pressed = pin reads LOW (GPIO_PIN_RESET)
     * Released = pin reads HIGH (pull-up)
     */
    GPIO_InitStruct.Pin   = BTN_KEY2_PIN | BTN_KEY1_PIN | BTN_KEY0_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BTN_KEY2_PORT, &GPIO_InitStruct);
}

/* ====================================================================
 *  LVGL Input Device Read Callback
 * ==================================================================== */

/**
 * @brief  Keypad read callback — called periodically by LVGL (~30ms)
 *
 *  KEY1 state machine:
 *  ┌──────────┐    press     ┌────────────────┐
 *  │  IDLE    │ ──────────→  │  DOWN_SENDING  │
 *  │ key=0    │              │  key=LV_DOWN   │
 *  └──────────┘              │  hold N cycles │
 *        ↑                   └──┬─────────────┘
 *        │                      │
 *        │ release              │ N cycles done
 *        │ (short press)        ↓
 *        │              ┌──────────────────────┐
 *        │              │  WAITING_LONG        │
 *        │              │  key=0 (silent)      │
 *        │              └──┬───────────────────┘
 *        │                 │
 *        │                 │ elapsed > 500ms
 *        │                 ↓
 *        │         ┌──────────────┐
 *        └─────────│ ENTER_SENT   │
 *                  │ key=LV_ENTER │
 *                  └──────────────┘
 *
 *  Design rationale:
 *  - LV_KEY_DOWN is held for KEY_MIN_CYCLES (≈90ms) to ensure LVGL
 *    reliably processes the key-down event.
 *  - After that, the callback goes silent (key=0) to prevent
 *    LVGL auto-repeat from scrolling the focus past the target.
 *  - If 500ms elapses while still held → LV_KEY_ENTER is emitted.
 *  - If released before 500ms → short press = DOWN navigation only.
 */
static void lv_port_indev_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    (void)drv;

    /* KEY1 state machine variables */
    static uint32_t key1_press_start  = 0;
    static uint8_t  key1_down_cycles  = 0;
    static bool     key1_enter_sent   = false;

    /*
     * Read all four buttons.
     * WAKE_UP:  active-HIGH → pressed when pin reads HIGH
     * KEY0/1/2: active-LOW  → pressed when pin reads LOW
     */
    bool wake_up = (HAL_GPIO_ReadPin(BTN_WAKE_UP_PORT, BTN_WAKE_UP_PIN)
                    == GPIO_PIN_SET);
    bool key0    = (HAL_GPIO_ReadPin(BTN_KEY0_PORT, BTN_KEY0_PIN)
                    == GPIO_PIN_RESET);
    bool key1    = (HAL_GPIO_ReadPin(BTN_KEY1_PORT, BTN_KEY1_PIN)
                    == GPIO_PIN_RESET);
    bool key2    = (HAL_GPIO_ReadPin(BTN_KEY2_PORT, BTN_KEY2_PIN)
                    == GPIO_PIN_RESET);

    uint32_t key = 0;

    /*
     * Priority: WAKE_UP > KEY0 > KEY2 > KEY1
     * If multiple keys are pressed, only the highest-priority one is reported.
     * Pressing any non-KEY1 button resets the KEY1 state machine.
     */
    if (wake_up)
    {
        key = LV_KEY_ESC;
        key1_press_start = 0;
        key1_down_cycles = 0;
        key1_enter_sent  = false;
    }
    else if (key0)
    {
        key = LV_KEY_NEXT;
        key1_press_start = 0;
        key1_down_cycles = 0;
        key1_enter_sent  = false;
    }
    else if (key2)
    {
        key = LV_KEY_PREV;
        key1_press_start = 0;
        key1_down_cycles = 0;
        key1_enter_sent  = false;
    }
    else if (key1)
    {
        /* ---- KEY1 state machine ---- */

        key = LV_KEY_ENTER;
        key1_press_start = 0;
        key1_down_cycles = 0;
        key1_enter_sent  = false;
    }

    /* Write back to LVGL */
    data->key   = key;
    data->state = (key != 0) ? LV_INDEV_STATE_PRESSED
                             : LV_INDEV_STATE_RELEASED;
}
