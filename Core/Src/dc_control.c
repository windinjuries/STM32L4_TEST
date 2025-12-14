#include "main.h"
#include "cmsis_os.h"
#include "dc_control.h"

int16_t motor_speed = 0;

extern TIM_HandleTypeDef htim2;

void TC214B_Forward(uint8_t speed);
void TC214B_Stop(void);
void TC214B_Reverse(uint8_t speed);
void StartTC214BTask(void const *argument);


/**
 * @brief  Set motor direction and speed for forward
 * @param  speed: Speed value (0-255, mapped to PWM duty)
 * @retval None
 */
void TC214B_Forward(uint8_t speed)
{
    uint16_t pwm_value =  100 * speed ;               
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pwm_value); 
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 0);         
}

/**
 * @brief  Stop the motor
 * @retval None
 */
void TC214B_Stop(void)
{
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 0);
}

/**
 * @brief  Set motor direction and speed for reverse
 * @param  speed: Speed value (0-255, mapped to PWM duty)
 * @retval None
 */
void TC214B_Reverse(uint8_t speed)
{
    uint16_t pwm_value = speed * 100;
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0);         
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, pwm_value); 
}

/**
 * @brief  Function implementing the TC214B task thread.
 * @param  argument: Not used
 * @retval None
 */
void StartTC214BTask(void const *argument)
{
    // Start PWM
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);

    // Set initial duty cycle to 0
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 0);

    /* Infinite loop */
    for (;;)
    {
        if( motor_speed == 0)
        {
            TC214B_Stop();
        }
        else if( motor_speed > 0)
        {
            TC214B_Forward(motor_speed);
        }
        else
        {
            TC214B_Reverse(0 - motor_speed);
        }
        HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9);
        osDelay(1000); 
    }
}
