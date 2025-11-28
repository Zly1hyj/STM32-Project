#include "stm32f10x.h"

#include "sys.h"
#include "led.h"
#include "bmp.h"
#include "oled.h"
#include "adc.h"
#include "ds18b20.h"
#include "timer.h"
#include "delay.h"
#include "usart.h"
#include "usart2.h"
#include "cJSON.h"

int LED_STATUS = 0;
int FAN_STATUS = 0;

char WIFIName[] = "ESP8266-WIFI";
char WIFIpwd[] = "12345678";

// 【注意】TDS阈值不合适，建议将tds_up改为180, tds_down改为90
int temp_up = 35,temp_down = 15;      // 温度上下限
int tds_up = 90,tds_down = 30;        // 水质上下限

// int maxHighLevel = 5; // 该变量在超声波方案中使用，此处已无需使用

int feedTime = 43200;                // 喂食时间 (间隔)
int cacheFeedTime = 43200;           // 缓存喂食时间    
int sendDataTime = 3;                // 数据发送时间
int cacheSendTime = 3;               // 缓存数据发送时间

// --- GPIO 定义 ---
#define LED         PAout(2)       // LED
#define BUZZ        PAout(4)       // 蜂鸣器

// 【定义确认】JD1=排水, JD2=加水
#define JD1         PAout(8)       // 继电器1-抽水(排水)
#define JD2         PAout(5)       // 继电器2-放水(加水)
#define JD3         PAout(12)      // 继电器3-加热
#define JD4         PAout(7)       // 继电器4-降温
#define JD5         PAout(6)       // 继电器5-增氧

#define KEY_EDIT    PBin(13)       // 设置按钮
#define KEY_NEXT    PBin(14)       // 切换按钮
#define KEY_SWIT    PBin(15)       // 下一页按钮
#define KEY_ADD     PAin(11)       // 加一按钮
#define KEY_DEC     PBin(12)       // 减一按钮

// --- 新增：液位传感器引脚定义 ---
// 注意：传感器输出低电平(0)代表有水，高电平(1)代表无水
#define LEVEL_HIGH  PBin(8)        // 高水位传感器输入引脚
#define LEVEL_LOW   PBin(9)        // 低水位传感器输入引脚

#define SG90_CLOSE  185            // 舵机关闭
#define SG90_OPEN   195            // 舵机打开

// --- 状态变量 ---
int handleFlag = 0;        // 判断上位机发送的数据需要处理哪一个
int sendFlag = 0;          // 定时器时间到了该位置一表示发送数据
int paramFlag = 1;         // 是否开启参数检查
int changeWaterFlag = 0;   // 换水标志位 (0:不换水, 1:正在排水, 2:正在抽水)
int feedFlag = 0;          // 是否喂食
int feedEndFlag = 0;       // 喂食结束    
int water_level_state = 1; // 水位状态 (0:过低, 1:正常, 2:过高)
int initFlag = 0;          // ESP8266初始化标志位
int alarmFlag = 0;         // 全局报警标志位
int tds_water_change_lock = 0; // 【新增】TDS换水锁定标志 (0=解锁, 1=锁定)

unsigned char temp = 0; // 温度

// --- 函数声明 ---
void paramCheck( void );         // 检查参数是否超过
void handleData( void );         // 上位机数据处理
void DisplayUI( void );          // 固定页面UI渲染
void paramEdit( void );          // 阈值参数设置
void editUiDisplay( int pageIndex ); // 设置页面UI初始化
void runAlter(int cursor,int count); // 执行参数修改
void Level_Sensor_Init(void);        // 新增：液位传感器GPIO初始化

extern char *USARTx_RX_BUF;
extern float TDS_value;    

int main(void)
{
    int time = 0;
    delay_init();
    LED_Init();
    OLED_Init();
    Adc_Init();
    DS18B20_Init();
    Level_Sensor_Init(); // 初始化液位传感器GPIO
    uart_init(115200);
    timeInit(4999,7199);
    timeSendInit(9999,7199);
    timePwmInit(199,7199);
    
    TIM_SetCompare4(TIM2,SG90_CLOSE); // 关闭喂食
    OLED_ShowChLength(38,16,47,3);    // 显示启动中    
    // ESP8266Init(WIFIName,WIFIpwd);
    OLED_Clear();
    
    while(1){
        DisplayUI();
        if( time++ > 5 ){
            time = 0;
            temp = getTemperture();
            TDS_Value_Conversion();
        }
        
        // --- 温度显示 ---
        OLED_ShowNum(45,0,temp,2,16,1);
        
        // --- 水位状态显示 ---
        // 动态显示：0=过低，1=正常，2=过高
        switch (water_level_state){
            case 0: // 过低
                OLED_ShowChinese(45,16,91,16,1); // 过
                OLED_ShowChinese(61,16,93,16,1); // 低
                break;
            case 2: // 过高
                OLED_ShowChinese(45,16,91,16,1); // 过
                OLED_ShowChinese(61,16,92,16,1); // 高
                break;
            default: // 正常
                OLED_ShowChinese(45,16,94,16,1); // 正
                OLED_ShowChinese(61,16,95,16,1); // 常
                break;
        }
        OLED_ShowNum(45,32,TDS_value,3,16,1);
        OLED_ShowNum(76,48,cacheFeedTime,5,16,1); // 喂食倒计时
        OLED_Refresh();
        
        // 开始喂食
        if( feedFlag ){
            TIM_SetCompare4(TIM2,SG90_OPEN);
            if( feedEndFlag ){
                feedFlag = 0;
                feedEndFlag = 0;
                feedTime = cacheFeedTime;
                TIM_SetCompare4(TIM2,SG90_CLOSE);
            }
        }
        
        // 进入设置页面
        if( !KEY_EDIT ){      
            while( !KEY_EDIT ); // 防抖
            paramEdit();
            if( !paramFlag ) OLED_ShowChLength(105,2,62,1);
            else OLED_ShowString(105,2,"  ",16,1);
        }
        
        // 切换参数提醒设置
        if( !KEY_NEXT ){      
            while( !KEY_NEXT );
            paramFlag = !paramFlag;  
            if( !paramFlag ){ // 关闭提示
                JD1=0; JD2=0; JD3=0; JD4=0; JD5=0; BUZZ=0; LED=0;
                OLED_ShowChLength(105,2,62,1);
            } else {
                OLED_ShowString(105,2,"  ",16,1);
            }
            OLED_Refresh();           
        }
        
        // 上位机更改数据
        if( handleFlag ) handleData();
        
        // 3s上传一次数据
        if( sendFlag && 1){      
            // 注意: 此处将 level 替换为 water_level_state
            ESP8266Pub(temp, water_level_state, TDS_value);
            sendFlag = 0;
            sendDataTime = cacheSendTime;
        }
        
        // 检查参数是否超出范围（开启了参数检查）
        if( paramFlag ) paramCheck();    
        
        delay_ms(100);
    }
}

// 初始化液位传感器GPIO口
void Level_Sensor_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE); // 使能GPIOB时钟

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 设置为上拉输入
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

// 绘制固定不变的UI画面
void DisplayUI( void ){
    OLED_ShowChLength(0,0,11,2);
    OLED_ShowChLength(0,16,87,2);
    OLED_ShowChLength(0,32,67,2);
    OLED_ShowChLength(0,48,75,4);
    
    OLED_ShowString(33,0,":",16,1);
    OLED_ShowString(33,16,":",16,1);
    OLED_ShowString(33,32,":",16,1);
    OLED_ShowString(65,48,":",16,1);

    OLED_ShowChLength(65,1,21,1);
    OLED_ShowString(80,16," ",16,1); // 清空旧单位 "CM"
    OLED_ShowString(74,32,"PPM",16,1);
    OLED_ShowString(118,48,"S",16,1); // 已修正坐标
}

// 处理上位机数据
void handleData( void ){
    cJSON *data = NULL;
    cJSON *upData = NULL;
    cJSON *downData = NULL;
    
    data = cJSON_Parse( (const char *)USARTx_RX_BUF );
    
    switch( handleFlag ){
        case 1: LED = 1; break;
        case 2: LED = 0; break;
        case 3: /* ... */ break;
        case 4: /* ... */ break;
        case 5:    
            upData = cJSON_GetObjectItem(data,"temp_up");
            downData = cJSON_GetObjectItem(data,"temp_down");
            temp_up = upData->valueint;
            temp_down = downData->valueint;
            break;
        case 6: // 原level_up/down的功能已移除，可作他用
            break;
        case 7:
            upData = cJSON_GetObjectItem(data,"tds_up");
            downData = cJSON_GetObjectItem(data,"tds_down");
            tds_up = upData->valueint;
            tds_down = downData->valueint;
            break;
    }
    clearCache();
    cJSON_Delete(data);
    handleFlag = 0;
    USART_RX_STA = 0;
}

// 参数设置页面 (恢复双页)
void paramEdit( void ){
    int pageIndex = 0, cursor = 0, alterTip = 0, beforeCursor = 0;
    OLED_Clear();
    OLED_ShowString(112,0,"<",16,1);
    
    while( 1 ){
        editUiDisplay(pageIndex); // UI显示
        
        if( !KEY_NEXT ){
            while( !KEY_NEXT );
            beforeCursor = (cursor % 4) * 16;
            cursor++;
            cursor = (cursor % 4) + 4 * pageIndex;
            alterTip = (cursor % 4) * 16;
            OLED_ShowString(112,beforeCursor," ",16,1);
            OLED_ShowString(112,alterTip,"<",16,1);
        }
        
        if( !KEY_SWIT ){
            while(!KEY_SWIT);
            pageIndex = (++pageIndex) % 2;
            cursor = 4 * pageIndex;
            OLED_Clear();
            OLED_ShowString(112,0,"<",16,1);
            OLED_Refresh();           
        }
        
        if( !KEY_ADD ){
            while(!KEY_ADD);
            runAlter(cursor,1);
        }
        
        if( !KEY_DEC ){
            while(!KEY_DEC);
            runAlter(cursor,-1);
        }
        
        if( !KEY_EDIT ){
            while( !KEY_EDIT );
            OLED_Clear();
            break;
        }
        
        if( handleFlag ) handleData();
        OLED_Refresh();    
    }
}

// 执行参数修改
void runAlter(int cursor,int count){
    int countResult;
    
    switch( cursor ){
        case 0: // temp_up
            countResult = temp_up + count;
            temp_up = countResult > 0 ? countResult : 0;
            break;
        case 1: // temp_down
            countResult = temp_down + count;
            temp_down = countResult > 0 ? countResult : 0;
            break;
        case 2: // tds_up
            countResult = tds_up + (count * 10);
            tds_up = countResult > 0 ? countResult : 0;
            break;
        case 3: // tds_down
            countResult = tds_down + (count * 10);
            tds_down = countResult > 0 ? countResult : 0;
            break;
        case 4: // cacheFeedTime
            countResult = cacheFeedTime + (count * 5);
            cacheFeedTime = countResult > 0 ? countResult : 0;
            break;
        case 5: // cacheSendTime
            countResult = cacheSendTime + count;
            cacheSendTime = countResult > 0 ? countResult : 0;
            break;
        case 6: // 空
        case 7: // 空
            break;
    }
}

// 设置页面UI (恢复双页)
void editUiDisplay( int pageIndex ){
    if( pageIndex == 0){
        OLED_ShowChLength(0,0,23,4);  // 温度上限
        OLED_ShowChLength(0,16,27,4); // 温度下限
        OLED_ShowChLength(0,32,67,4); // 水质上限
        OLED_ShowChLength(0,48,71,4); // 水质下限

        OLED_ShowString(65,0,":",16,1);
        OLED_ShowString(65,16,":",16,1);
        OLED_ShowString(65,32,":",16,1);
        OLED_ShowString(65,48,":",16,1);
        
        OLED_ShowNum(76,0,temp_up,3,16,1);
        OLED_ShowNum(76,16,temp_down,3,16,1);
        OLED_ShowNum(76,32,tds_up,4,16,1);
        OLED_ShowNum(76,48,tds_down,4,16,1);
    } else if ( pageIndex == 1) {
        OLED_ShowChLength(0,0,75,4);  // 喂食时间
        OLED_ShowChLength(0,16,79,4); // 发送时间 (修正)

        OLED_ShowString(65,0,":",16,1);
        OLED_ShowString(65,16,":",16,1);
        
        OLED_ShowNum(76,0,cacheFeedTime,5,16,1); // 已修正为5位
        OLED_ShowNum(76,16,cacheSendTime,4,16,1);
    }
    OLED_Refresh();
}

// ===================================================================
// 【函数已替换】
// 检查各参数是否超出范围 (已修复JD1/JD2逻辑 和 TDS换水死循环BUG)
// ===================================================================
void paramCheck( void ){
    // --- 错误修复: 将变量声明移至函数顶部 ---
    int is_level_high;
    int is_level_low;
	
    alarmFlag = 0; // 在每次检查开始时，重置全局报警标志
    
    // 读取高低水位传感器的状态
    // 传感器有水时输出0, 无水时输出1. 为方便理解，我们反转一下逻辑。
    is_level_high = !LEVEL_HIGH; // is_level_high=1 代表高位有水
    is_level_low = !LEVEL_LOW;   // is_level_low=1 代表低位有水

    // --- 1. 水位检查与控制 (已按JD1=排水, JD2=加水修正) ---
    if( changeWaterFlag == 0 ){ // 仅在非自动换水模式下执行
        if( is_level_high ){
            // 水位过高
            JD1 = 1; // 开启排水
            JD2 = 0; // 停止加水
            alarmFlag = 1; // 触发报警
            water_level_state = 2; // 更新状态为 "过高"
        } else if ( !is_level_low ){
            // 水位过低
            JD1 = 0; // 停止排水
            JD2 = 1; // 开启加水
            alarmFlag = 1; // 触发报警
            water_level_state = 0; // 更新状态为 "过低"
        } else {
            // 水位正常
            JD1 = 0; // 停止排水
            JD2 = 0; // 停止加水
            water_level_state = 1; // 更新状态为 "正常"
        }
    }

    // --- 2. 温度检查与控制 ---
    if( temp && temp >= temp_up){
        // 温度过高
        JD4 = 1; // 开启降温
        JD3 = 0;
        alarmFlag = 1; // 触发报警
    } else if( temp && temp <= temp_down ){
        // 温度过低
        JD3 = 1; // 开启加热
        JD4 = 0;
        alarmFlag = 1; // 触发报警
    } else {
        // 温度正常
        JD3 = 0;
        JD4 = 0;
    }

    // --- 3. 水质检查与自动换水 (已修复BUG) ---
    
    // 【新增】如果TDS值恢复正常，则解锁
    if (TDS_value < tds_up) {
        tds_water_change_lock = 0;
    }

    // 【修改】如果TDS超标，并且【未被锁定】，则启动换水流程
    if( TDS_value && TDS_value >= tds_up && tds_water_change_lock == 0 ){
        if( changeWaterFlag == 0) {
            changeWaterFlag = 1; // 启动换水，进入排水阶段
            JD2 = 0; // 确保加水泵(JD2)关闭
        }
    }
    // 检查TDS是否过低 (触发报警)
    else if ( TDS_value && TDS_value <= tds_down ) {
        alarmFlag = 1; // 触发报警
    }
    
    // -- 换水流程状态机 (已按JD1=排水, JD2=加水修正) --
    if( changeWaterFlag == 1 ){ // 状态1: 正在排水
        JD1 = 1; // 打开排水(JD1)
        JD2 = 0; // 确保加水(JD2)关闭
        alarmFlag = 1; // 换水过程本身就是一种需要提示的状态
        if( !is_level_low ){ // 如果低水位传感器检测到无水
            changeWaterFlag = 2; // 进入加水阶段
        }
    } else if( changeWaterFlag == 2 ){ // 状态2: 正在加水
        JD1 = 0; // 关闭排水(JD1)
        JD2 = 1; // 打开加水(JD2)
        alarmFlag = 1; // 换水过程本身就是一种需要提示的状态
        if( is_level_high ){ // 如果高水位传感器检测到有水
            JD2 = 0; // 停止加水(JD2)
            changeWaterFlag = 0; // 换水结束，返回正常监控模式
            
            tds_water_change_lock = 1; // 【关键】换水结束，立即“锁定”TDS检查
        }
    }

    // --- 4. 增氧逻辑控制 (JD5 - 气泵) ---
    if( feedFlag == 1 ){
        // 4.1: 如果正在喂食，关闭增氧
        JD5 = 0;
    } else {
        // 4.2: 默认情况，保持增氧开启
        JD5 = 1;
    }
    
    // 4.3: 安全覆盖逻辑：如果温度过高，强制开启增氧
    if( temp && temp >= temp_up ){
        JD5 = 1; // 高温时必须增氧
    }

    // --- 5. 统一声光报警控制 ---
    if( alarmFlag ){
        // 如果有任何异常，则启动声光报警
        BUZZ = 1;
        LED = ~LED; // LED灯闪烁
    } else {
        // 如果一切正常，则关闭所有报警
        BUZZ = 0;
        LED = 0;    // LED灯关闭
    }
}
