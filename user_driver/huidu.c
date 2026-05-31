#include "huidu.h"

uint8_t huidu_value[] = {0, 0, 0, 0, 0};

uint8_t get_gpio_state(GPIO_Regs *gpio_port, uint32_t gpio) {
    uint32_t high_bits = DL_GPIO_readPins(gpio_port, gpio); 
    if((high_bits & gpio) != 0) return 1;
    else return 0;
}

void huidu_get_value()
{
    huidu_value[0] = get_gpio_state(HUIDU_L2_PORT, HUIDU_L2_PIN);
    huidu_value[1] = get_gpio_state(HUIDU_L1_PORT, HUIDU_L1_PIN);
    huidu_value[2] = get_gpio_state(HUIDU_M_PORT, HUIDU_M_PIN);
    huidu_value[3] = get_gpio_state(HUIDU_R1_PORT, HUIDU_R1_PIN);
    huidu_value[4] = get_gpio_state(HUIDU_R2_PORT, HUIDU_R2_PIN);
}
extern float target_speed_1;
extern float target_speed_2;

float line_last_error = 0;
float line_kp = 78;
float line_kd = 150;
float line_base_speed = 850;//直线速度
float line_min_speed = 420;//大弯最低速
float line_slow_k = 150;//速度衰减系数
float line_max_speed = 960;//最大速度

float limit_target_speed(float speed)
{
    if(speed > line_max_speed){
        return line_max_speed;
    }
    else if(speed < 0){
        return 0;
    }
    return speed;
}

float abs_float(float value)  //绝对函数
{
    if(value < 0){
        return -value;
    }
    return value;
}

float get_line_base_speed(float line_error) //连续减速
{
    float abs_error = abs_float(line_error);
    float base_speed = line_base_speed - line_slow_k * abs_error;

    if(base_speed < line_min_speed){
        return line_min_speed;
    }

    return base_speed;
}

float get_line_error(void)
{
    int weights[5] = {-2, -1, 0, 1, 2};
    int sum = 0;
    int count = 0;

    huidu_get_value();

    for(int i = 0; i < 5; i++){
        if(huidu_value[i] == 1){
            sum += weights[i];
            count++;
        }
    }

    if(count == 0){
        return line_last_error;
    }

    return (float)sum / count;
}

float line_pd(float line_error)
{
    float turn = line_kp * line_error + line_kd * (line_error - line_last_error);
    line_last_error = line_error;
    return turn;
}

void adjust_motor()
{
    float line_error = get_line_error();
    float turn = line_pd(line_error);
    float base_speed = get_line_base_speed(line_error);

    motor_set_direction(1, 1);
    motor_set_direction(2, 1);

    if(huidu_value[0] == 1 && huidu_value[1] == 1 && huidu_value[2] == 1 && huidu_value[3] == 1 && huidu_value[4] == 1){
        target_speed_1 = 0;
        target_speed_2 = 0;
        return;
    }

    target_speed_1 = limit_target_speed(base_speed - turn);
    target_speed_2 = limit_target_speed(base_speed + turn);
}
