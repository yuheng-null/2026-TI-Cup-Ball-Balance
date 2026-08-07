# STM32F103标准库电赛完全指南


## 第一章 学前必读：电赛与STM32学习方法论

### 1.1 TI杯电赛到底比什么？
全国大学生电子设计竞赛（TI杯）每两年举办一次，是含金量最高的大学生电子类竞赛之一。**STM32是90%以上控制类、仪器类题目的核心控制器**。

电赛一等奖需要的能力金字塔：
```
┌─────────────────────────────────┐
│  系统设计+调试能力+临场发挥     │  ← 一等奖关键
├─────────────────────────────────┤
│  控制算法(PID)+数字信号处理     │  ← 区分二等奖和一等奖
├─────────────────────────────────┤
│  外设驱动+模块使用+通信协议     │  ← 三等奖到二等奖
├─────────────────────────────────┤
│  GPIO+定时器+ADC+串口基础       │  ← 入门门槛
├─────────────────────────────────┤
│  C语言+数电模电基础             │  ← 你现在的位置
└─────────────────────────────────┘
```

**重要认知**：
- 电赛不是比谁用的芯片高级，而是比谁能在4天3夜里稳定做出题目要求的功能
- STM32F103C8T6虽然"老"，但性能足够应对95%以上的电赛题目，而且资料最多、最稳定
- 标准库虽然被ST"停止维护"，但在电赛中是最好用的——学习曲线平缓、代码直观、调试方便，比HAL库更适合竞赛快速开发
- **不要一开始就追求寄存器开发**：先把标准库用熟、把外设原理搞懂，再去看寄存器，你会发现寄存器就是标准库的本质

### 1.2 给你的学习路线建议
| 阶段     | 时间 | 目标                  | 必须动手做的实验                                            |
| -------- | ---- | --------------------- | ----------------------------------------------------------- |
| 第一阶段 | 2周  | 搞定基础外设          | 点亮LED、按键输入、串口打印、定时器中断、PWM呼吸灯          |
| 第二阶段 | 3周  | 搞定所有外设+常用模块 | ADC采集电位器、OLED显示、超声波测距、舵机控制、直流电机驱动 |
| 第三阶段 | 3周  | 掌握核心算法          | PID控制电机转速、编码器测速、各种数字滤波、状态机写复杂逻辑 |
| 第四阶段 | 4周  | 综合项目+真题训练     | 做2-3个历年控制类真题（比如倒立摆、平衡车、智能小车）       |

**学习铁律**：
1. **每学一个外设，必须写代码验证**：不要只看视频/文档，眼睛会了手不会
2. **每遇到一个问题，必须搞懂为什么**：不要复制粘贴代码就完事，报错了必须找到根本原因
3. **学会看官方文档**：《STM32F10x参考手册》是最好的教材，比任何教程都准确
4. **建立自己的代码模板库**：电赛时时间就是生命，成熟的代码模板能帮你省几个小时

### 1.3 你需要准备的硬件
- STM32F103C8T6最小系统板 ×2（备用一个防止烧坏）
- ST-Link V2下载器
- 面包板+杜邦线（公对公、公对母、母对母各20根）
- 基本元器件：LED、电阻、按键、电位器、蜂鸣器
- 常用模块：0.96寸OLED(I2C)、HC-SR04超声波、SG90舵机、L298N/TB6612电机驱动、直流减速电机+编码器、MPU6050陀螺仪
- 稳压电源模块（3.3V/5V）
- 示波器（如果实验室有，一定要学会用；没有的话用逻辑分析仪代替）

---

## 第二章 前置知识篇：你必须先搞懂的基础
很多人学STM32学不下去，根本原因不是STM32难，而是前置知识有漏洞。我会用最通俗的语言把你需要的基础补全。

### 2.1 数字电路核心知识（1小时就能搞懂）
STM32是数字芯片，所有引脚都是数字引脚（除了ADC专用引脚），你必须先理解数字逻辑。

#### 2.1.1 什么是高电平和低电平？
- 数字世界只有两种状态：0和1，对应电路里的**低电平**和**高电平**
- 对于STM32F103：
  - **低电平**：0V ~ 0.8V，被识别为逻辑0
  - **高电平**：2.0V ~ 3.3V，被识别为逻辑1
  - **0.8V~2.0V之间是不确定区**：引脚电压在这个范围可能被识别为0也可能是1，这就是为什么电路不能虚焊、不能接错！
- **致命常识**：STM32是**3.3V器件**！所有IO口最大只能承受3.6V电压！绝对不能接5V！接5V大概率会烧坏芯片！

> **电赛踩坑教训**：很多新手把5V的Arduino模块直接接STM32引脚，一接就烧。记住：只要和STM32引脚相连的信号，必须是3.3V！如果模块是5V输出，必须做电平转换！

#### 2.1.2 GPIO的四种基本输入输出模式
你以后天天和GPIO打交道，这四种模式必须刻在脑子里：

| 模式         | 通俗解释                                                                             | 电赛常用场景                                               |
| ------------ | ------------------------------------------------------------------------------------ | ---------------------------------------------------------- |
| **推挽输出** | 引脚真正被芯片主动驱动：输出高就是3.3V，输出低就是0V，驱动能力强（最大25mA）         | 点亮LED、驱动蜂鸣器、输出控制信号、模拟通信时序            |
| **开漏输出** | 引脚只能输出低电平，高电平需要外部上拉电阻才能输出。好处是可以线与、可以兼容不同电平 | I2C通信、需要电平转换的场合、多个设备共用一条总线          |
| **上拉输入** | 引脚默认状态是高电平（芯片内部接了一个40K左右的电阻到VCC），外部拉低时变成低电平     | 按键输入（按键一端接引脚一端接地，没按下时是高，按下是低） |
| **下拉输入** | 引脚默认状态是低电平（芯片内部接了一个电阻到GND），外部拉高时变成高电平              | 较少用，某些特定传感器输入                                 |
| **浮空输入** | 引脚什么都不接，电平完全由外部决定，高阻态                                           | USART RX引脚、SPI MISO引脚、ADC输入                        |

> **为什么按键用上拉输入？**
> 如果按键用浮空输入，没按下的时候引脚是悬空的，就像天线一样会接收空间电磁波，电平乱跳，你会发现按键没按也会触发。上拉输入就是用内部电阻把引脚"拉"到高电平，没按下时稳定是高，按下时按键直接把引脚接到GND变成低，非常稳定。

#### 2.1.3 二进制、十六进制与位操作
STM32编程90%的操作都是在操作寄存器的某一位，位运算是C语言和单片机的灵魂。

**必须熟练掌握的位操作（C语言）**：
```c
// 假设我们有一个32位的寄存器REG，我们想操作它的第n位（从0开始数，最低位是第0位）

// 1. 把某一位设置为1（置位），其他位不变
REG |= (1 << n);
// 例子：把第5位设为1：REG |= (1<<5);  等价于 REG |= 0x20;

// 2. 把某一位设置为0（清零），其他位不变
REG &= ~(1 << n);
// 例子：把第3位清零：REG &= ~(1<<3); 等价于 REG &= 0xFFFFFFF7;

// 3. 翻转某一位（0变1，1变0）
REG ^= (1 << n);

// 4. 读取某一位的值（0或1）
uint8_t bit_value = (REG >> n) & 1;

// 5. 同时设置多个位
REG |= (1<<n) | (1<<m);  // 同时把第n位和第m位置1

// 6. 把连续几位设置为特定值（比如把第4-6位设置为二进制101）
REG &= ~(7 << 4);        // 先把第4、5、6位清零（7是二进制111，左移4位就是0b1110000）
REG |= (5 << 4);         // 再把5（二进制101）左移4位写进去
```

> **为什么要用位操作而不是直接给寄存器赋值？**
> 因为寄存器的每一位都有独立的含义，你不能直接写REG = 0x20; 这样会把其他所有位都变成0，破坏其他配置。位操作的好处是**只改变你想改的位，其他位保持不变**。这是单片机编程最核心的技巧，没有之一。

**十六进制速记**：
- 1位十六进制 = 4位二进制
- 0x0 = 0000, 0x1=0001, ..., 0xF=1111
- 以后看寄存器描述，看到0x40010800这样的地址不要慌，就是个32位的数字而已

### 2.2 模拟电路核心知识（够用就行，不用深究）
电赛很多题目需要采集模拟信号、输出模拟信号，模电不用学太深入，掌握以下概念足够了：

#### 2.2.1 什么是ADC？
- **ADC = Analog to Digital Converter，模数转换器**
- 作用：把模拟电压（0~3.3V之间的连续值）转换成数字量（0~4095之间的整数，因为STM32F1的ADC是12位的）
- 换算公式：`实际电压 = ADC读取值 * 3.3V / 4096`
- 例子：ADC读到2048，电压就是2048*3.3/4096 = 1.65V

#### 2.2.2 什么是DAC？
- **DAC = Digital to Analog Converter，数模转换器**
- 作用：和ADC相反，把数字量转换成模拟电压输出
- STM32F103C8T6**没有DAC**！C8T6是中容量产品，DAC只在大容量产品（比如ZET6）上才有。需要模拟输出怎么办？用PWM+RC滤波模拟DAC，后面会详细讲。

#### 2.2.3 什么是PWM？为什么它能当DAC用？
- **PWM = Pulse Width Modulation，脉冲宽度调制**
- 简单说就是快速开关一个引脚：一会儿输出高，一会儿输出低，高电平的时间占整个周期的比例叫**占空比**
- 如果PWM频率足够高（比如10kHz以上），加一个简单的RC低通滤波电路，就能把方波变成平滑的直流电压！占空比0%就是0V，占空比100%就是3.3V，占空比50%就是1.65V
- **这是电赛最常用的技巧！** 因为PWM比真DAC便宜、好用、通道多，电机调速、舵机控制、LED调光全靠它。

#### 2.2.4 上拉电阻和下拉电阻
- 上拉电阻：把信号"拉"向高电平的电阻，一端接VCC一端接信号
- 下拉电阻：把信号"拉"向低电平的电阻，一端接GND一端接信号
- 作用：给不确定的信号一个默认稳定状态，防止悬空干扰
- STM32内部已经集成了上拉/下拉电阻，大多数时候不需要外接，配置寄存器就行

### 2.3 C语言进阶（单片机专用版）
你说你有C语言基础，但课本上教的C语言和单片机用的C语言有很大区别，以下是你必须熟练掌握的：

#### 2.3.1 基本数据类型（STM32标准库专用）
标准库已经帮你定义好了这些类型，以后直接用，不要用int/char/long，因为不同编译器长度可能不一样：
```c
typedef unsigned           int uint32_t;   // 32位无符号整数，最常用
typedef unsigned short     int uint16_t;   // 16位无符号整数
typedef unsigned char      int uint8_t;    // 8位无符号整数
typedef signed             int int32_t;    // 32位有符号整数
typedef signed short       int int16_t;    // 16位有符号整数
typedef signed char        int int8_t;     // 8位有符号整数

typedef uint8_t  u8;    // 很多老工程师喜欢用这个简写
typedef uint16_t u16;
typedef uint32_t u32;
```

#### 2.3.2 宏定义#define
宏定义是单片机编程用的最多的语法，用来定义常量、寄存器位、简单操作：
```c
// 定义常量
#define LED_PIN     GPIO_Pin_0
#define LED_PORT    GPIOC
#define SYSTEM_CLOCK 72000000

// 定义带参数的宏（类似函数，但没有函数调用开销，非常快）
#define LED_ON()    GPIO_SetBits(LED_PORT, LED_PIN)
#define LED_OFF()   GPIO_ResetBits(LED_PORT, LED_PIN)
#define DELAY_MS(x) Delay_ms(x)

// 以后写代码，所有引脚定义、参数都用宏定义！不要在代码里写死数字！
// 好处：改硬件的时候只需要改宏定义，不用翻遍整个代码找数字
```

#### 2.3.3 结构体struct
标准库所有外设初始化都是用结构体，你必须理解结构体：
```c
// 定义一个GPIO初始化结构体类型
typedef struct {
    uint16_t GPIO_Pin;    // 引脚号
    GPIOSpeed_TypeDef GPIO_Speed;  // 速度
    GPIOMode_TypeDef GPIO_Mode;    // 模式
} GPIO_InitTypeDef;

// 使用的时候：定义一个结构体变量，给成员赋值，然后传给初始化函数
int main(void) {
    GPIO_InitTypeDef GPIO_InitStructure;  // 定义结构体变量
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;     // 配置引脚0
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;  // 速度50MHz
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;    // 推挽输出
    
    GPIO_Init(GPIOC, &GPIO_InitStructure);  // 把结构体地址传给初始化函数
}
```

#### 2.3.4 指针（不用怕，单片机里指针很简单）
单片机里指针99%的用途就是：
1. 传递结构体/数组给函数（避免拷贝整个数组，效率高）
2. 直接访问寄存器地址（标准库已经帮你做好了，你不用自己写）
3. 函数指针（做状态机、回调函数用，后面讲）

你只要记住：`&变量名`是取变量的地址，`*指针变量`是取指针指向地址里的值就行。

#### 2.3.5 枚举enum和typedef
枚举就是给数字起名字，让代码更好读：
```c
// 标准库是这么定义GPIO模式的，本质就是#define，但更有条理
typedef enum { 
    GPIO_Mode_AIN = 0x0,           // 模拟输入
    GPIO_Mode_IN_FLOATING = 0x04,  // 浮空输入
    GPIO_Mode_IPD = 0x28,          // 下拉输入
    GPIO_Mode_IPU = 0x48,          // 上拉输入
    GPIO_Mode_Out_OD = 0x14,       // 开漏输出
    GPIO_Mode_Out_PP = 0x10,       // 推挽输出
    GPIO_Mode_AF_OD = 0x1C,        // 复用开漏
    GPIO_Mode_AF_PP = 0x18         // 复用推挽
} GPIOMode_TypeDef;

// 好处：你写GPIO_Mode_Out_PP比写0x10好记100倍，而且编译器会帮你检查错误
```

#### 2.3.6 volatile关键字（非常重要！）
这是单片机C语言最容易踩坑的地方！
- volatile的意思是"易变的"，告诉编译器：这个变量可能在程序意料之外被改变，不要优化这个变量的读写！
- 什么地方必须加volatile？
  1. 中断服务函数里会修改的全局变量
  2. 寄存器映射的变量（标准库已经帮你加了）
  3. 多线程/RTOS中多个任务访问的变量
```c
// 例子：中断里会改变的标志位，必须加volatile
volatile uint8_t flag = 0;

void EXTI0_IRQHandler(void) {
    if(EXTI_GetITStatus(EXTI_Line0) != RESET) {
        flag = 1;  // 中断里修改flag
        EXTI_ClearITPendingBit(EXTI_Line0);
    }
}

int main(void) {
    while(1) {
        if(flag == 1) {  // 如果flag没加volatile，编译器可能会优化成死循环，因为它觉得flag在main里从来没被改过
            // 处理事件
            flag = 0;
        }
    }
}
```
> **电赛踩坑教训**：很多新手遇到"中断明明触发了，但是主循环里检测不到标志位"的问题，90%都是因为标志位没加volatile！编译器优化把变量读操作优化掉了！

#### 2.3.7 静态变量static
static在单片机里非常有用：
- 函数内部的static变量：只会初始化一次，下次进入函数值保持不变（类似全局变量，但只在这个函数里能访问）
- 全局变量/函数加static：限制这个变量/函数只能在当前.c文件里访问，防止多文件重名冲突
```c
void delay_ms(uint16_t ms) {
    static uint32_t last_time = 0;  // 这个变量不会每次调用都重置为0
    // ...
}
```

#### 2.3.8 头文件包含与多文件编程
电赛代码不可能都写在main.c里，必须分模块：
- 每个外设/模块写一个.c文件和一个.h文件
- .h文件里放函数声明、宏定义、类型定义
- .c文件里放具体实现
- 头文件必须加防重复包含的宏：
```c
// 这是led.h文件的标准写法
#ifndef __LED_H
#define __LED_H

#include "stm32f10x.h"  // 所有头文件都要包含标准库头文件

void LED_Init(void);
void LED_On(void);
void LED_Off(void);

#endif
```
- 哪个.c文件要用LED的函数，就在开头`#include "led.h"`就行

---

## 第三章 STM32入门篇：芯片架构与开发环境

### 3.1 STM32F103C8T6芯片详解
先认识一下你天天用的这块芯片：

| 参数     | 值                                            | 电赛意义                                                     |
| -------- | --------------------------------------------- | ------------------------------------------------------------ |
| 内核     | ARM Cortex-M3                                 | 32位内核，比51单片机快几十倍                                 |
| 主频     | 最高72MHz                                     | 1秒能执行7200万条指令，足够做复杂控制和算法                  |
| Flash    | 64KB                                          | 存程序代码，足够写非常复杂的程序（电赛程序一般几KB到几十KB） |
| RAM      | 20KB                                          | 存运行时变量，一般够用，注意不要定义太大的数组               |
| GPIO     | 48个引脚，最多37个通用IO                      | 足够接很多模块                                               |
| 定时器   | 3个通用定时器(TIM2/3/4) + 1个高级定时器(TIM1) | 电赛最核心的外设，电机、PWM、输入捕获全靠它                  |
| ADC      | 2个12位ADC，最多10个外部通道                  | 采集模拟信号，最多1us转换一次                                |
| 通信接口 | 3个USART, 2个SPI, 2个I2C, 1个CAN, 1个USB      | 接各种模块和通信                                             |
| DMA      | 12个通道                                      | 高速数据传输，不占用CPU                                      |
| 供电     | 2.0V~3.6V，典型3.3V                           | 再次强调：不能接5V！                                         |

> **为什么C8T6是电赛神芯？**
> 便宜（几块钱一片）、资料多、足够用、稳定。很多人觉得F1性能不够，其实72MHz的M3内核做双轮平衡车、倒立摆都完全没问题，做四轴飞行器都有人用。电赛比的是谁能稳定实现功能，不是比谁的芯片性能强。

### 3.2 STM32标准库到底是什么？
很多人用了很久标准库都不知道它本质是什么，我给你讲透：

**标准库 = ST公司帮你写好的、操作寄存器的函数集合**

你可以把它理解为一个驱动层：
- 最底层是硬件：芯片里的寄存器，就是一些特定地址的内存，往里面写特定的值就能控制硬件
- 中间层是标准库：ST把几百个寄存器的操作封装成了一个个函数，比如`GPIO_Init()`、`TIM_SetCompare1()`，你不用记寄存器地址和每一位是什么意思，调用函数就行
- 最上层是你的应用代码：调用标准库函数实现你要的功能

**为什么要用标准库而不是直接写寄存器？**
1. 开发快：写寄存器要翻手册查每一位，调用函数一行搞定
2. 可读性好：`GPIO_SetBits(GPIOC, GPIO_Pin_13)`一看就知道是点亮PC13的LED，写寄存器`GPIOC->ODR |= (1<<13);`也能实现，但新手看不懂
3. 不容易错：标准库经过了几百万工程师十几年的验证，基本没有BUG
4. 移植方便：同系列芯片代码基本不用改

**为什么不推荐新手一开始学寄存器？**
- 学习曲线太陡，一开始就面对几百个寄存器很容易放弃
- 电赛时间宝贵，标准库足够你拿一等奖
- 等你把标准库用熟了，外设原理搞懂了，回头看寄存器你会发现非常简单，就是把函数里的操作直接写出来而已

> **正确的学习态度**：用标准库，但要知道这个函数内部操作了什么寄存器、改了哪些位。不要做只会调函数、出了问题完全不知道怎么查的"调库侠"。

### 3.3 标准库工程结构详解
打开一个标准库工程，你会看到很多文件夹，我给你讲清楚每个文件夹是干嘛的：

```
你的工程文件夹/
├── Libraries/           # 库文件，不用改
│   ├── CMSIS/           # ARM内核相关文件
│   │   ├── core_cm3.h   # Cortex-M3内核定义
│   │   ├── system_stm32f10x.c/h  # 系统时钟配置
│   │   └── startup_stm32f10x_md.s  # 启动文件！非常重要
│   └── FWlib/           # 标准库外设函数
│       ├── inc/         # 所有外设头文件：stm32f10x_gpio.h, stm32f10x_tim.h...
│       └── src/         # 所有外设源文件：stm32f10x_gpio.c, stm32f10x_tim.c...
├── User/                # 用户代码，你主要写这里
│   ├── main.c           # 主函数，程序入口
│   ├── stm32f10x_conf.h # 库配置文件，用来指定你要用哪些外设
│   ├── stm32f10x_it.c   # 中断服务函数都写在这里！
│   └── stm32f10x_it.h
├── Hardware/            # 你自己写的模块驱动：led.c, oled.c, motor.c...
├── Output/              # 编译输出的hex文件
└── Listings/            # 编译生成的列表文件
```

#### 3.3.1 启动文件startup_stm32f10x_md.s详解
这是整个工程第一个执行的文件，是汇编写的，你不用改，但要知道它做了什么：
1. 初始化堆栈指针SP
2. 初始化程序计数器PC，指向复位中断服务函数
3. 设置中断向量表：所有中断发生时跳转到哪个函数
4. 调用SystemInit()函数配置系统时钟为72MHz
5. 调用__main()，最终跳转到你写的main()函数

> **重要**：C8T6是中容量产品，启动文件必须用`startup_stm32f10x_md.s`！md=medium density。如果用成hd（大容量）或者ld（小容量）的，程序会跑飞或者根本下载不进去。

#### 3.3.2 stm32f10x_conf.h 库配置文件
这个文件用来告诉编译器你要用哪些外设，不用的外设可以注释掉，减小代码体积：
```c
// 把你要用的外设头文件包含进来，不用的注释掉
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_adc.h"
#include "stm32f10x_usart.h"
// #include "stm32f10x_can.h"  // 不用CAN就注释掉
// ...
```
还有一个重要的宏：`USE_STDPERIPH_DRIVER`，必须在Keil的C/C++选项里定义这个宏，否则标准库函数不会被编译。

### 3.4 STM32程序执行流程详解
很多新手写了半天代码，不知道程序是怎么跑起来的，我给你理清楚：

```
上电复位
  ↓
启动文件startup_stm32f10x_md.s执行
  ↓
调用SystemInit()：配置时钟树，把主频设为72MHz
  ↓
跳转到main()函数
  ↓
执行你写的各种初始化函数：GPIO_Init(), TIM_Init()...
  ↓
进入while(1)主循环，永远循环执行里面的代码
  ↓
如果有中断发生，暂停主循环，跳转到对应的中断服务函数执行
  ↓
中断服务函数执行完，回到主循环被打断的地方继续执行
```

**最核心的认知**：
- STM32程序是**前后台系统**：
  - **后台**：while(1)大循环，一直在跑，处理主要业务逻辑
  - **前台**：中断，有事件发生时打断主循环，快速处理紧急事件
- main函数里的while(1)是必须的！程序绝对不能从main函数返回，因为返回之后没有地方去，会跑飞。
- 中断服务函数一定要短！不要在中断里写延时、不要在中断里处理复杂逻辑，中断里只做最紧急的事（比如存数据、置标志位），复杂逻辑放到主循环里处理。

### 3.5 一个最基础的标准库程序模板
先给你一个标准的程序模板，以后所有程序都基于这个模板写：

```c
#include "stm32f10x.h"

// 简单的延时函数，后面会教你写更精确的
void Delay_ms(uint32_t ms) {
    uint32_t i,j;
    for(i=0; i<ms; i++)
        for(j=0; j<10000; j++);
}

int main(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 第一步：开启GPIO时钟！！！（新手最容易忘的一步！）
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    
    // 第二步：配置GPIO引脚
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;  // C13引脚，核心板上的LED
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  // 推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    
    // 主循环
    while(1) {
        GPIO_ResetBits(GPIOC, GPIO_Pin_13);  // LED亮（C13低电平亮）
        Delay_ms(500);
        GPIO_SetBits(GPIOC, GPIO_Pin_13);    // LED灭
        Delay_ms(500);
    }
}
```

> **新手第一大坑：忘记开时钟！**
> STM32为了省电，所有外设默认都是关闭时钟的，时钟就是外设的心跳，时钟不开，外设完全不工作，你怎么配置寄存器都没用。90%的新手第一次写代码都会忘记开时钟，然后LED不亮，查几个小时都找不到原因。
> 
> **记住**：配置任何外设之前，第一步永远是开它的时钟！GPIO在APB2总线上，USART1在APB2，其他定时器、USART2/3在APB1，后面会详细讲时钟树。

---

### 3.6 Keil MDK 新建标准库工程完整步骤

这是每个 STM32 初学者必须掌握的第一个技能。你可能会觉得"新建工程有什么难的"，但标准库工程的手动搭建涉及十几个文件的正确放置和配置，一步出错就编译不过。我会把每一步的目的都讲清楚。

#### 3.6.1 准备工作：你需要哪些文件？

一个标准库工程需要的文件分三类：

```
第一类：CMSIS 内核文件（ARM 提供，不需要修改）
  ├── core_cm3.c / core_cm3.h      ← Cortex-M3 内核定义
  ├── system_stm32f10x.c / .h       ← 系统时钟配置函数（SystemInit 在这里）
  └── startup_stm32f10x_md.s        ← 启动文件（汇编写的）

第二类：标准库外设文件（ST 提供，不需要修改）
  ├── stm32f10x.h                   ← 外设寄存器地址和位定义（最重要的头文件）
  ├── stm32f10x_conf.h              ← 库配置头文件（你选要用哪些外设）
  ├── stm32f10x_gpio.c / .h         ← GPIO 库
  ├── stm32f10x_rcc.c / .h          ← 时钟库
  ├── stm32f10x_tim.c / .h          ← 定时器库
  ├── stm32f10x_usart.c / .h        ← 串口库
  ├── stm32f10x_adc.c / .h          ← ADC 库
  ├── stm32f10x_dma.c / .h          ← DMA 库
  ├── ...（你需要什么外设就加什么）
  ├── misc.c / .h                   ← NVIC（中断控制器）配置函数
  └── stm32f10x_it.c / .h           ← 中断服务函数模板

第三类：你自己的代码
  ├── main.c                        ← 主函数
  ├── led.c / led.h                 ← 你的模块驱动
  └── ...
```

**关键提示**：这些标准库文件从哪里来？从 ST 官网下载 STM32F10x 标准固件库（STSW-STM32054），解压后就能找到。或者直接用正点原子、野火提供的工程模板，把 Library 文件夹复制过来。

#### 3.6.2 新建 Keil 工程的详细步骤

以下步骤假设你在 Windows 上使用 Keil MDK-ARM V5。

**步骤1：创建工程文件夹结构**

先在电脑上创建一个总文件夹，比如 `STM32_Template`，里面创建以下子文件夹：

```
STM32_Template/
├── Libraries/
│   ├── CMSIS/              ← 放 CMSIS 内核文件
│   └── FWlib/              ← 放标准库外设文件
│       ├── inc/            ← 所有外设的 .h 头文件
│       └── src/            ← 所有外设的 .c 源文件
├── User/                   ← 放你自己的代码
├── Hardware/               ← 放你写的模块驱动
├── Output/                 ← 编译输出（.hex 文件生成在这里）
└── Listings/               ← 编译中间文件（.lst, .map）
```

**步骤2：打开 Keil，新建工程**

```
Keil 菜单栏 → Project → New uVision Project...
→ 导航到 STM32_Template/ 文件夹
→ 文件名填 "Template"（或任何你喜欢的名字）
→ 保存
```

弹出 "Select Device" 对话框：
```
→ 搜索 "STM32F103C8"
→ 选中 STM32F103C8（在 STMicroelectronics 下面）
→ 点击 OK
```

弹出 "Manage Run-Time Environment" 对话框：
```
→ 直接点 Cancel（我们用标准库，不用 Keil 自带的软件包）
```

**步骤3：添加文件到工程**

在 Keil 左侧的 Project 面板中，右键 `Target 1` → `Manage Project Items...`：

```
创建以下 Groups（组）：
  - CMSIS     （放内核文件）
  - FWlib     （放标准库源文件）
  - User      （放 main.c 等）
  - Hardware  （放你的模块驱动）

然后给每个 Group 添加对应的文件：
```

| Group        | 要添加的文件                                                                                                                                                                                   |
| ------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **CMSIS**    | `core_cm3.c`<br>`system_stm32f10x.c`<br>`startup_stm32f10x_md.s`（注意选 .s 类型）                                                                                                             |
| **FWlib**    | `stm32f10x_gpio.c`<br>`stm32f10x_rcc.c`<br>`stm32f10x_tim.c`<br>`stm32f10x_usart.c`<br>`stm32f10x_adc.c`<br>`stm32f10x_dma.c`<br>`misc.c`<br>...（你需要什么外设就加什么，不用的不加也能编译） |
| **User**     | `main.c`（需要自己新建并保存到 User/ 文件夹）<br>`stm32f10x_it.c`                                                                                                                              |
| **Hardware** | 你写的 `led.c`, `oled.c` 等模块文件                                                                                                                                                            |

**步骤4：配置工程选项（Keil 魔术棒）**

点击工具栏上的 "魔术棒" 图标（Options for Target），或按 `Alt+F7`：

**4a. Target 选项卡**：
```
→ 确认 Xtal(MHz) = 8.0（外部晶振频率）
→ 其他默认即可
```

**4b. Output 选项卡**：
```
→ 勾选 "Create HEX File"（生成 .hex 文件，用于下载到芯片）
→ 点 "Select Folder for Objects..." → 选择 Output/ 文件夹
→ 这样 .hex 文件会生成在 Output/ 而不是散落在工程根目录
```

**4c. Listing 选项卡**：
```
→ 点 "Select Folder for Listings..." → 选择 Listings/ 文件夹
```

**4d. C/C++ 选项卡**（这是最关键的配置！）：
```
→ 在 "Define" 框中填入（这是三个宏定义，用逗号分隔）：
     STM32F10X_MD,USE_STDPERIPH_DRIVER
  含义：
    STM32F10X_MD           → 告诉库：芯片是中容量（C8T6 是 MD）
                            MD = Medium Density（64KB Flash）
                            其他选项：LD=小容量(32KB), HD=大容量(256KB+)
    USE_STDPERIPH_DRIVER   → 告诉库：我们要用标准库外设驱动
                            如果没有这个宏，stm32f10x_conf.h 里的外设头文件不会被包含

→ 在 "Include Paths" 中添加以下路径（头文件搜索路径）：
     .\Libraries\CMSIS
     .\Libraries\FWlib\inc
     .\User
     .\Hardware
  这些路径告诉编译器去哪些文件夹找 .h 头文件

→ Optimization 优化级别：Level 0 (-O0)（调试阶段用最低优化）
  比赛时可以改为 -O2 或 -Os 提高运行速度

→ 勾选 "C99 Mode"（支持 C99 标准，如 for(int i=0;...) 的写法）
```

**4e. Debug 选项卡**：
```
→ 右侧选择 "ST-Link Debugger"（用 ST-Link 下载和调试）
→ 点击旁边的 "Settings" 按钮：
   → Debug 子选项卡 → Port: SW（SWD 接口）
   → Flash Download 子选项卡 → 勾选 "Reset and Run"（下载后自动复位运行）
   → 确认 Programming Algorithm 中只有 "STM32F10x Med-density Flash"
     （如果不是，点 Add 添加，选 128KB 的那个——虽然 C8T6 只有 64KB，但算法用 128KB 的兼容）
```

**步骤5：新建 main.c 并写入最简单的代码**

在 Keil 中：`File → New`，粘贴以下代码，然后保存到 `User/main.c`：

```c
#include "stm32f10x.h"

int main(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 开 GPIOC 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    
    // 配置 PC13 为推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    
    while(1)
    {
        // 软件空转延时（粗略延时）
        for(uint32_t i = 0; i < 500000; i++);
        GPIO_ResetBits(GPIOC, GPIO_Pin_13);  // LED 亮
        
        for(uint32_t i = 0; i < 500000; i++);
        GPIO_SetBits(GPIOC, GPIO_Pin_13);    // LED 灭
    }
}
```

**步骤6：编译和下载**

```
→ 按 F7 编译（Build）
→ 确认下方 Build Output 窗口显示 "0 Error(s), 0 Warning(s)"
  （如果有一个关于 inline 的 warning 可以忽略）
→ 连接 ST-Link 到电脑，连接 ST-Link 到核心板的 SWD 接口
→ 按 F8 下载（Download / Load）
→ 按开发板上的复位键，或重新上电
→ LED 开始闪烁 → 成功！
```

#### 3.6.3 常见编译错误与解决

| 错误信息                                                 | 原因                               | 解决方法                                               |
| -------------------------------------------------------- | ---------------------------------- | ------------------------------------------------------ |
| `error: #5: cannot open source file "stm32f10x.h"`       | 头文件搜索路径没配                 | 在 C/C++ 选项卡的 Include Paths 中添加对应路径         |
| `error: #20: identifier "GPIO_InitTypeDef" is undefined` | 忘记定义 `USE_STDPERIPH_DRIVER` 宏 | 在 C/C++ 的 Define 中添加 `USE_STDPERIPH_DRIVER`       |
| `error: #35: #error directive: "Please select..."`       | 芯片型号宏没定义或定义错了         | Define 中确保有 `STM32F10X_MD`（注意是_MD不是_MD_VL）  |
| `Undefined symbol SystemInit`                            | 缺少 `system_stm32f10x.c`          | 把 `system_stm32f10x.c` 添加到 CMSIS Group             |
| `Undefined symbol __main`                                | 缺少启动文件                       | 把 `startup_stm32f10x_md.s` 添加到 CMSIS Group         |
| `Error: L6218E: Undefined symbol xxx`                    | 链接错误：某个函数只有声明没有实现 | 把对应的 `.c` 文件加入工程。如 `stm32f10x_gpio.c` 没加 |
| `The code size exceeds the limit...`                     | 代码超过 64KB Flash                | 减小代码量，或换更大容量的芯片                         |

> **关于启动文件的选择（重要！）**：
> - C8T6 是**中容量**（64KB Flash）→ 用 `startup_stm32f10x_md.s`
> - C6T6 是小容量（32KB）→ 用 `startup_stm32f10x_ld.s`
> - ZET6 是大容量（512KB）→ 用 `startup_stm32f10x_hd.s`
> - **选错了会怎样？** 中断向量表的位置和大小不同，程序可能一上电就跑飞（HardFault）。

#### 3.6.4 工程模板的建立和复用

建好一个能编译成功的工程后，**把整个文件夹打包成 .zip 备份**。以后每次开始新项目：

```
1. 解压模板 .zip
2. 改文件夹名
3. 在 Keil 中打开，改工程名（右键 Target 1 → Rename）
4. 在 Hardware/ 下添加新模块的 .c/.h 文件
5. 开始写代码
```

**这就是电赛老司机的套路——成熟的工程模板是效率的保证。** 比赛只有 4 天 3 夜，花 10 分钟从零配工程是不值得的。

---

## 第四章 核心基础篇：时钟系统与GPIO
### 4.1 时钟系统RCC——整个芯片的心脏
时钟系统是STM32最核心、也是新手最懵的部分。我用最通俗的话给你讲透。

#### 4.1.1 为什么需要时钟？
数字电路是靠时钟脉冲一步步工作的，每来一个时钟脉冲，电路就执行一次操作。时钟就像乐队的节拍器，没有节拍器，所有人都乱套了。
- 时钟频率越高，芯片跑的越快，但功耗越大、发热越大
- STM32之所以复杂，是因为它有多个时钟源，可以给不同外设配置不同的时钟频率，兼顾性能和功耗

#### 4.1.2 STM32F103的五个时钟源
1. **HSI**：内部高速RC时钟，8MHz，芯片内部自带，精度不高，上电默认用这个
2. **HSE**：外部高速时钟，我们一般接8MHz的外部晶振，精度很高
3. **LSI**：内部低速RC时钟，40kHz，给独立看门狗用
4. **LSE**：外部低速时钟，32.768kHz，给RTC实时时钟用
5. **PLL**：锁相环，用来倍频！可以把HSE/HSI的时钟倍频到最高72MHz，这就是我们的系统主频

#### 4.1.3 时钟树结构（重点！）
我给你简化一下，你只要记住最常用的路径：
```
外部8MHz晶振(HSE)
  ↓
PLL倍频：8MHz × 9 = 72MHz （这就是为什么系统时钟是72M）
  ↓
系统时钟SYSCLK = 72MHz
  ├─→ AHB总线时钟 = 72MHz （HCLK，给内存、DMA、内核用）
  └─→ APB1总线时钟 = 36MHz （PCLK1，给TIM2-7、USART2-5、SPI2/3、I2C用）
      └─→ APB1上的定时器时钟 = 72MHz！（注意！APB1预分频如果不是1，定时器时钟自动×2）
  └─→ APB2总线时钟 = 72MHz （PCLK2，给GPIO、USART1、SPI1、TIM1、ADC用）
      └─→ APB2上的定时器时钟 = 72MHz
      └─→ ADC时钟 = PCLK2 / 6 = 12MHz （ADC最大不能超过14MHz）
```

> **电赛最常用的时钟配置**：外部8MHz晶振，PLL倍频到72MHz，APB1二分频到36MHz，APB2不分频72MHz。这就是标准库SystemInit()默认帮你配好的，你不用改，知道怎么来的就行。

#### 4.1.4 时钟相关函数（你会用到的）
```c
// 开启APB2总线上的外设时钟（GPIO、AFIO、USART1、TIM1、ADC1/2、SPI1）
RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE); // 一次开多个

// 开启APB1总线上的外设时钟（TIM2-4、USART2-3、SPI2/I2C等）
RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
```

> **时钟开启口诀**：
> - GPIO、串口1、高级定时器TIM1、SPI1、ADC → APB2
> - 通用定时器TIM2/3/4、串口2/3、I2C → APB1
> - 开时钟是所有外设配置的第一步！忘开时钟，外设必不工作！


---

## 第四章 核心基础篇：时钟系统与GPIO（续）

### 4.2 GPIO深入详解——从原理到实战
GPIO（通用输入输出）是你用的最多的外设，没有之一。LED、按键、模块控制信号、模拟通信时序，全靠GPIO。我会从最底层原理讲起，让你彻底搞懂GPIO。

#### 4.2.1 GPIO的内部结构原理
先看GPIO引脚的内部结构框图，这张图你理解了，GPIO所有模式你就都懂了：
```
                        VDD
                         │
                         ├─→ 上拉电阻(P-MOS)
                         │
    输入驱动器 ←── 施密特触发器 ←──┼─── 引脚
                         │
                         ├─→ 下拉电阻(N-MOS)
                         │
                        GND
                         │
    输出驱动器 ←── 位设置/清除寄存器 ←─ P-MOS和N-MOS管
```

**每个GPIO引脚内部都有两个MOS管**：
- 上面的P-MOS管导通时，引脚接VDD（3.3V）
- 下面的N-MOS管导通时，引脚接GND（0V）
- 两个都不导通时，引脚是高阻态（浮空）

这就是为什么叫"推挽输出"：P-MOS把引脚"推"到高电平，N-MOS把引脚"挽"到低电平。

#### 4.2.2 八种GPIO模式的本质（彻底搞懂）
标准库定义了8种GPIO模式，我一个个给你讲本质，以及什么时候用：

| 模式                               | 内部状态                                                       | 电赛使用场景                                  | 注意事项                                                                       |
| ---------------------------------- | -------------------------------------------------------------- | --------------------------------------------- | ------------------------------------------------------------------------------ |
| **GPIO_Mode_AIN 模拟输入**         | 上下拉都断开，施密特触发器关闭，信号直接进ADC                  | ADC采集电压、DAC输出                          | 用ADC的时候必须设这个模式！否则施密特触发器会把模拟信号整形，ADC采到的就是错的 |
| **GPIO_Mode_IN_FLOATING 浮空输入** | 上下拉都断开，施密特触发器打开                                 | USART RX引脚、SPI MISO引脚、外部中断引脚      | 悬空时电平不确定，容易受干扰，按键绝对不能用这个                               |
| **GPIO_Mode_IPD 下拉输入**         | 内部下拉电阻接GND，施密特触发器打开                            | 较少用，特定传感器输入                        | 默认低电平，外部信号拉高才是高                                                 |
| **GPIO_Mode_IPU 上拉输入**         | 内部上拉电阻接VDD，施密特触发器打开                            | 按键输入（一端接GND）、I2C SCL/SDA默认        | 这是按键的标准配置！没按下时稳定高电平，按下接地变低                           |
| **GPIO_Mode_Out_OD 开漏输出**      | P-MOS始终断开，只有N-MOS工作，只能输出低电平，高电平靠外部上拉 | I2C通信、电平转换、线与逻辑                   | 开漏输出本身不能输出高电平！必须外接上拉电阻，或者用内部上拉                   |
| **GPIO_Mode_Out_PP 推挽输出**      | P-MOS和N-MOS都工作，强驱动输出高低电平                         | LED、蜂鸣器、普通输出引脚、模拟时序           | 驱动能力最强，最大25mA，不要直接驱动电机等大电流设备                           |
| **GPIO_Mode_AF_OD 复用开漏输出**   | P-MOS断开，N-MOS由外设控制，不是输出数据寄存器                 | I2C的SCL/SDA引脚（复用功能）                  | 用硬件I2C的时候必须设这个模式                                                  |
| **GPIO_Mode_AF_PP 复用推挽输出**   | P-MOS和N-MOS都由外设控制                                       | USART TX引脚、SPI MOSI/SCK引脚、定时器PWM输出 | 用硬件外设输出的时候必须设这个模式！不要用普通推挽输出                         |

> **为什么复用功能要单独的模式？**
> 普通输出模式下，引脚电平由GPIO输出数据寄存器(ODR)控制；复用输出模式下，引脚电平由对应的外设（比如定时器、串口）直接控制，GPIO的ODR寄存器不起作用。比如你用定时器输出PWM，必须把引脚设为复用推挽，否则定时器的信号到不了引脚上。

#### 4.2.3 GPIO对应的总线时钟（必须记牢）
**为什么GPIO都挂在APB2总线上？**
因为APB2是高速总线，最高72MHz，GPIO需要快速翻转，所以挂在高速总线上。APB1是低速总线，最高36MHz，给那些不需要太高速度的外设用。

所有GPIO端口（GPIOA、GPIOB、GPIOC、GPIOD...）都挂在**APB2总线**上，所以配置任何GPIO之前，必须开启APB2总线上对应的时钟：
```c
// 开GPIOA时钟
RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
// 开GPIOB时钟
RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
// 开GPIOC时钟（核心板LED一般在PC13）
RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
// 一次开多个：GPIOA + GPIOB + USART1
RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_USART1, ENABLE);
```

> **电赛踩坑血泪史**：
> 1. 我见过无数新手，配置PA9/PA10做串口1，只开了USART1的时钟，没开GPIOA的时钟，结果串口死活发不出数据，查了一整天！记住：**用哪个引脚，就必须开对应GPIO端口的时钟**，不管这个引脚做什么用。
> 2. 还有人开时钟开错总线：把GPIO的时钟用RCC_APB1PeriphClockCmd开，结果当然是不工作。GPIO永远在APB2！

#### 4.2.4 GPIO常用标准库函数详解
所有GPIO函数都在`stm32f10x_gpio.h`和`stm32f10x_gpio.c`里，常用的就这几个：

##### 1. GPIO初始化函数
```c
void GPIO_Init(GPIO_TypeDef* GPIOx, GPIO_InitTypeDef* GPIO_InitStruct);
```
- 第一个参数：GPIOA/GPIOB/GPIOC...
- 第二个参数：GPIO初始化结构体指针
- 结构体三个成员：
  - GPIO_Pin：引脚号，GPIO_Pin_0 ~ GPIO_Pin_15，可以或运算同时配置多个引脚
  - GPIO_Speed：输出速度，只有输出模式下有效：
    - GPIO_Speed_2MHz：2MHz，低速，干扰小
    - GPIO_Speed_10MHz：10MHz，中速
    - GPIO_Speed_50MHz：50MHz，高速，电赛一般都选这个
  - GPIO_Mode：就是上面讲的8种模式

**例子：配置PC13为推挽输出（接LED）**
```c
GPIO_InitTypeDef GPIO_InitStructure;
RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE); // 第一步永远是开时钟！

GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
GPIO_Init(GPIOC, &GPIO_InitStructure);
```

##### 2. 设置引脚为高电平
```c
void GPIO_SetBits(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
// 例子：PC13输出高电平
GPIO_SetBits(GPIOC, GPIO_Pin_13);
// 同时设置多个引脚
GPIO_SetBits(GPIOA, GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2);
```
本质：往GPIOx->BSRR寄存器的对应位写1，原子操作，不会被中断打断。

##### 3. 设置引脚为低电平
```c
void GPIO_ResetBits(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
// 例子：PC13输出低电平（核心板LED亮）
GPIO_ResetBits(GPIOC, GPIO_Pin_13);
```
本质：往GPIOx->BRR寄存器的对应位写1，也是原子操作。

##### 4. 翻转引脚电平
```c
void GPIO_WriteBit(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, BitAction BitVal);
// 自己写翻转函数，标准库没有现成的
#define GPIO_TogglePin(GPIOx, GPIO_Pin)  GPIO_WriteBit(GPIOx, GPIO_Pin, (BitAction)(1 - GPIO_ReadOutputDataBit(GPIOx, GPIO_Pin)))
// 用法：翻转PC13
GPIO_TogglePin(GPIOC, GPIO_Pin_13);
```

##### 5. 读取引脚输入电平
```c
uint8_t GPIO_ReadInputDataBit(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
// 例子：读取PA0按键是否按下
uint8_t key_value = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0);
// 返回值：Bit_SET(1)是高电平，Bit_RESET(0)是低电平
```

##### 6. 读取引脚输出电平
```c
uint8_t GPIO_ReadOutputDataBit(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
// 读的是你之前输出的寄存器值，不是引脚实际电平
```

#### 4.2.5 GPIO电赛实战1：LED闪烁
核心板上一般PC13接了一个LED，低电平点亮：
```c
#include "stm32f10x.h"

void Delay_ms(uint32_t ms) {
    uint32_t i,j;
    for(i=0; i<ms; i++)
        for(j=0; j<7200; j++); // 72MHz下大概1ms，后面会教精确延时
}

void LED_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    
    GPIO_SetBits(GPIOC, GPIO_Pin_13); // 默认熄灭
}

int main(void) {
    LED_Init();
    while(1) {
        GPIO_ResetBits(GPIOC, GPIO_Pin_13); // 亮
        Delay_ms(500);
        GPIO_SetBits(GPIOC, GPIO_Pin_13);   // 灭
        Delay_ms(500);
    }
}
```

#### 4.2.6 GPIO电赛实战2：按键输入（带消抖）
按键接PA0，一端接PA0，一端接GND，配置上拉输入：
```c
void KEY_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 上拉输入！
    // 输入模式不用配置速度
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

// 按键扫描函数，返回1表示按下，0表示没按下
uint8_t KEY_Scan(void) {
    static uint8_t key_up = 1; // 按键松开标志
    if(key_up && GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0) { // 按键按下
        Delay_ms(20); // 消抖！机械按键按下会抖动20ms左右
        key_up = 0;
        if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0) {
            return 1; // 确认按下
        }
    } else if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 1) { // 按键松开
        key_up = 1;
    }
    return 0;
}
```

> **为什么要消抖？**
> 机械按键不是理想开关，按下和松开的时候会有十几毫秒的抖动，电平在0和1之间跳变，如果不消抖，按一次按键会被识别成按了好几次。消抖方法：检测到电平变化后延时20ms再读一次，如果还是那个电平才确认是真的按下了。

#### 4.2.7 GPIO电赛实战3：矩阵键盘
电赛经常需要4x4矩阵键盘接16个按键，只用8个GPIO，节省引脚：
- 原理：4根行线配置为上拉输入，4根列线配置为推挽输出
- 扫描方法：依次拉低每一根列线，然后读取行线，如果某根行线变低，说明对应行列交叉的按键按下了

#### 4.2.8 GPIO常见坑总结
1. **忘记开时钟**：90%新手第一坑，记住配置任何东西先开时钟
2. **引脚模式配置错误**：ADC用成了浮空输入、PWM输出用成了普通推挽、I2C用成了推挽输出
3. **5V信号接3.3V引脚**：一接就烧，比如HC-SR04超声波的Echo脚输出5V，必须分压或者电平转换再接STM32
4. **引脚电流过大**：单个引脚最大25mA，整个芯片所有引脚加起来最大150mA，不要直接接电机、继电器等大电流设备，必须用三极管/MOS管/驱动芯片
5. **按键没上拉**：浮空输入的按键电平乱跳，必须用上拉输入或者外接上拉电阻
6. **PC13/PC14/PC15的特殊性**：这三个引脚是RTC域的，驱动能力弱，只能接LED，不能接大电流设备，做输入输出最好用PA/PB口

---

## 第五章 中断系统——单片机的灵魂
中断是单片机最重要的机制，没有之一。电赛里按键、定时器、串口、ADC、外部信号触发，全靠中断。不会用中断，你永远只能写轮询的"裸奔"程序，做不了复杂项目。

### 5.1 什么是中断？为什么需要中断？
想象一下你在宿舍写代码：
- **轮询方式**：你每隔1分钟就去门口看外卖到了没，大部分时间你都白跑，浪费时间
- **中断方式**：你安心写代码，外卖到了外卖员敲门（中断信号），你停下手里的活去开门拿外卖（中断服务函数），拿完回来继续写代码（回到主程序被打断的地方）

这就是中断的好处：**CPU不用一直轮询等待事件发生，可以专心做自己的事，事件发生时自动打断CPU去处理，处理完再回来**。

#### 5.1.1 中断相关的基本概念
- **中断源**：能产生中断信号的外设/事件，比如定时器溢出、串口收到数据、按键按下、ADC转换完成
- **中断优先级**：如果同时来了两个中断怎么办？优先级高的先执行，优先级低的等一等
- **中断服务函数(ISR)**：中断发生后跳去执行的函数，执行完回到主程序
- **中断嵌套**：高优先级中断可以打断正在执行的低优先级中断，先执行高优先级的，再回来执行低优先级的
- **中断向量表**：一张表，存了每个中断对应的中断服务函数入口地址，中断发生时内核自动查表跳转到对应函数

### 5.2 NVIC嵌套向量中断控制器——中断的大管家
NVIC是Cortex-M3内核自带的中断控制器，所有中断的优先级管理、使能、挂起都是它管的。

#### 5.2.1 STM32F103的中断优先级
STM32F1用了4位二进制来表示优先级，所以一共0~15共16个优先级，数值越小优先级越高！
- 这4位又被分成了两组：**抢占优先级**和**响应优先级（子优先级）**
- 通过NVIC_PriorityGroupConfig()函数来配置怎么分组，电赛一般用分组2：2位抢占优先级，2位子优先级

| 优先级分组           | 抢占优先级位数 | 子优先级位数 | 抢占优先级范围 | 子优先级范围 |
| -------------------- | -------------- | ------------ | -------------- | ------------ |
| NVIC_PriorityGroup_0 | 0位            | 4位          | 只有0          | 0~15         |
| NVIC_PriorityGroup_1 | 1位            | 3位          | 0~1            | 0~7          |
| NVIC_PriorityGroup_2 | 2位            | 2位          | 0~3            | 0~3          | 电赛最常用 |
| NVIC_PriorityGroup_3 | 3位            | 1位          | 0~7            | 0~1          |
| NVIC_PriorityGroup_4 | 4位            | 0位          | 0~15           | 只有0        |

**抢占优先级和子优先级的区别（非常重要！）**：
- **抢占优先级高**：可以打断正在执行的低抢占优先级中断，形成中断嵌套
- **子优先级高**：抢占优先级相同的中断同时到来时，子优先级高的先执行；**不能嵌套**！
- 如果两个中断的抢占优先级和子优先级都一样，哪个先来哪个先执行

> **电赛优先级配置经验**：
> - 最高优先级（抢占0）：紧急事件，比如电机过流保护、紧急停止按钮
> - 次高优先级（抢占1）：定时器中断（PWM输出、编码器计数、控制循环）
> - 中等优先级（抢占2）：ADC转换完成、外部中断（超声波捕获、红外接收）
> - 最低优先级（抢占3）：串口接收、按键处理、普通定时任务
>
> 记住：一个工程里优先级分组只配置一次！一般在main函数最开头配置，不要中途改分组。

#### 5.2.2 NVIC相关函数
```c
// 配置优先级分组，整个工程只调用一次
NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

// 配置某个中断的优先级并使能
NVIC_InitTypeDef NVIC_InitStructure;
NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn; // 中断源，定时器2中断
NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; // 抢占优先级1
NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1; // 子优先级1
NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; // 使能这个中断
NVIC_Init(&NVIC_InitStructure);
```

> **中断源通道名怎么找？**
> 在`stm32f10x.h`头文件里找IRQn_Type枚举类型，所有中断通道名都在里面定义，比如：
> - EXTI0_IRQn：外部中断线0
> - TIM2_IRQn：定时器2全局中断
> - USART1_IRQn：串口1全局中断
> - ADC1_2_IRQn：ADC1/2全局中断

### 5.3 EXTI外部中断——引脚电平变化触发中断
EXTI就是专门管GPIO引脚外部中断的，当引脚电平发生变化时触发中断，电赛里用来接按键、超声波Echo脚、红外接收、霍尔传感器等。

#### 5.3.1 EXTI的原理
- STM32F103有19个外部中断线，其中GPIO相关的是EXTI0~EXTI15共16根线
- **所有GPIO端口的Pin_x都共用EXTIx中断线**：PA0、PB0、PC0...都接在EXTI0上，同一时间只能有一个端口的Pin0用EXTI0
- 每个中断线可以单独配置触发方式：
  - 上升沿触发：电平从低变高的时候触发
  - 下降沿触发：电平从高变低的时候触发
  - 双边沿触发：电平变化就触发

#### 5.3.2 EXTI对应的总线时钟
**EXTI和AFIO（复用功能IO）都挂在APB2总线上！**
使用外部中断必须开启AFIO时钟，因为引脚中断映射是AFIO管的：
```c
RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE); // 外部中断必须开AFIO时钟！
```

> **新手大坑**：用外部中断只开了GPIO时钟，没开AFIO时钟，结果中断死活进不去，查几个小时都找不到原因。记住：用EXTI外部中断，必须开AFIO时钟！

#### 5.3.3 EXTI配置步骤（标准流程）
1. 开时钟：GPIOx时钟 + AFIO时钟
2. 配置GPIO引脚为输入模式（上拉/下拉/浮空，看电路）
3. 把GPIO引脚映射到对应的EXTI中断线
4. 配置EXTI中断线的触发方式、使能
5. 配置NVIC，设置优先级，使能中断
6. 编写中断服务函数

#### 5.3.4 EXTI电赛实战：按键外部中断
PA0接按键，按下时触发下降沿中断：
```c
void KEY_EXTI_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    // 1. 开时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
    
    // 2. 配置PA0为上拉输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 3. 把PA0映射到EXTI0
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);
    
    // 4. 配置EXTI0
    EXTI_InitStructure.EXTI_Line = EXTI_Line0;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt; // 中断模式
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; // 下降沿触发
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
    
    // 5. 配置NVIC
    NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

// 6. 中断服务函数！函数名必须和启动文件里的一致！
// 启动文件里定义的EXTI0中断函数名就是EXTI0_IRQHandler，写错了中断不会执行
volatile uint8_t key_flag = 0;
void EXTI0_IRQHandler(void) {
    if(EXTI_GetITStatus(EXTI_Line0) != RESET) { // 确认是EXTI0触发的中断
        Delay_ms(20); // 按键消抖
        if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0) {
            key_flag = 1; // 只置标志位，不做复杂处理！
        }
        EXTI_ClearITPendingBit(EXTI_Line0); // 必须清除中断挂起位！否则会一直进中断
    }
}
```

> **中断服务函数编写铁则**：
> 1. **函数名绝对不能写错**：必须和启动文件`startup_stm32f10x_md.s`里定义的一模一样，写错了就等于没写，中断发生时会跳到死循环。
> 2. **进入中断第一时间检查中断标志位**：确认是你要的中断触发的。
> 3. **退出中断前必须清除中断标志位**：如果不清除，中断标志一直置位，退出后会立刻再次进入中断，表现为程序一直卡在中断里。
> 4. **中断服务函数要尽可能短**：不要在里面写延时、不要写复杂逻辑、不要用串口发大量数据，只做最紧急的事（置标志位、存数据），复杂处理丢到主循环里做。
> 5. **中断里修改的全局变量必须加volatile**：防止编译器优化导致主循环读不到变化。

#### 5.3.5 EXTI中断线和函数名对应表
| 中断线        | 对应引脚    | 中断服务函数名       |
| ------------- | ----------- | -------------------- |
| EXTI0         | Pin0        | EXTI0_IRQHandler     |
| EXTI1         | Pin1        | EXTI1_IRQHandler     |
| EXTI2         | Pin2        | EXTI2_IRQHandler     |
| EXTI3         | Pin3        | EXTI3_IRQHandler     |
| EXTI4         | Pin4        | EXTI4_IRQHandler     |
| EXTI5~EXTI9   | Pin5~Pin9   | EXTI9_5_IRQHandler   | 注意！这5个线共用一个中断函数 |
| EXTI10~EXTI15 | Pin10~Pin15 | EXTI15_10_IRQHandler | 这6个线共用一个中断函数       |

> **重要**：EXTI5-9和EXTI10-15是共用中断通道的，如果你同时用了Pin5和Pin6的外部中断，它们会进同一个中断服务函数，你必须在函数里检查是哪个中断线触发的。

### 5.4 中断编程最佳实践（电赛一等奖经验）
1. **不要在中断里做任何耗时操作**：比如printf打印、LCD显示、长延时，这些都会导致中断响应不及时，丢数据或者系统崩溃
2. **中断优先级不要都设成一样**：紧急的中断优先级设高一点，不重要的设低一点
3. **不要滥用中断**：不是什么都要用中断，比如普通按键扫描用主循环定时扫描就够了，不一定非要外部中断
4. **标志位机制**：中断里只做"事件发生了"的标记，具体处理放在主循环，这是最稳妥的架构
5. **中断里不要调用不可重入函数**：比如printf、malloc、标准库的一些函数，否则会出问题

---

## 第五章补充：SysTick系统定时器——精确延时的基石

### 5S.1 什么是SysTick？为什么必须先学它？

在正式学习第六章的通用定时器（TIM2/3/4）之前，你必须先掌握 **SysTick（系统滴答定时器）**。理由有三：

1. **SysTick 比 TIM 简单十倍**：TIM 有几十个寄存器，光是 PWM 就有七八种模式；而 SysTick 只有 4 个寄存器，功能单纯——就是一个 24 位的倒计时计数器。先用 SysTick 建立"定时器"的概念，再学 TIM 就轻松很多。

2. **SysTick 是 Cortex-M3 内核自带的**：它不是 ST 公司设计的，而是 ARM 公司设计 Cortex-M3 内核时内置的。这意味着所有 Cortex-M3/M4/M7 芯片（无论 ST、NXP、TI）都有 SysTick，学会了这辈子都能用。

3. **SysTick 是实现精确延时的最佳工具**：前面我们用 `for(i=0; i<ms; i++) for(j=0; j<10000; j++);` 这种"软件空转"做延时。这种方法有几个致命问题：
   - **不精确**：不同编译器优化级别、不同主频下，延时时长会变
   - **阻塞式**：延时期间 CPU 什么都不能干，白白浪费算力
   - **不可移植**：换个芯片就得重新调整循环次数

   用 SysTick 做延时，精确到微秒级，而且代码换个 Cortex-M 芯片一样用。

#### 5S.1.1 SysTick 在芯片中的位置

很多新手搞不清 SysTick 和 TIM 的区别，我用一张图说明：

```
┌──────────────────────────────────────────────────┐
│              Cortex-M3 内核（ARM设计）             │
│  ┌────────────────────────────────────────────┐  │
│  │  SysTick 定时器 ← 内核自带，24位递减计数器   │  │
│  │  NVIC 中断控制器 ← 内核自带                  │  │
│  │  MPU 内存保护单元（C8T6没有）                │  │
│  └────────────────────────────────────────────┘  │
│                      │                           │
│              AHB 总线（72MHz）                     │
│                      │                           │
│  ┌────────────────────────────────────────────┐  │
│  │        STM32 外设（ST设计）                  │  │
│  │  TIM1/2/3/4 定时器  ← 芯片外设，16位         │  │
│  │  GPIO / USART / ADC / SPI ...               │  │
│  └────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────┘
```

**关键区别**：
- **SysTick**：内核内部的定时器，24位，只能递减，没有输入输出引脚，专做"系统心跳"
- **TIMx**：芯片外设，16位（或32位），可递增/递减，有输入捕获和 PWM 输出引脚，功能丰富

> **类比理解**：SysTick 就像你电脑上的系统时钟——只能看时间；TIM 就像多功能手表——既能看时间、又能当秒表、还能当闹钟。

#### 5S.1.2 SysTick 的时钟源

SysTick 的时钟有两个选择（通过 `SysTick->CTRL` 寄存器的第 2 位选择）：

| 时钟源               | 频率  | 说明                                                 |
| -------------------- | ----- | ---------------------------------------------------- |
| **AHB 时钟（HCLK）** | 72MHz | SysTick 挂在 AHB 总线上。如果 AHB 不分频，就是 72MHz |
| **AHB 时钟 ÷ 8**     | 9MHz  | 72MHz ÷ 8 = 9MHz。省电模式或者需要低频计数的场景     |

**电赛 99% 的情况都用 72MHz**（`SysTick_CLKSource_HCLK`），因为：
- 我们需要微秒级精确延时，72MHz 意味着每个计数周期 = 1/72MHz ≈ 13.9ns，精度极高
- 24 位计数器的最大值是 2^24 - 1 = 16,777,215，用 72MHz 时钟最大延时 = 16,777,215 / 72M ≈ 0.233 秒。需要用循环或扩展技巧来获得更长的延时。

> **注意**：SysTick **不需要** `RCC_APBxPeriphClockCmd` 来开启时钟！因为它不挂在 APB 总线上，它是内核自带的，只要芯片上电它就有时钟。但你需要通过 `SysTick_CLKSourceConfig()` 来选择它的时钟源。

---

### 5S.2 SysTick 的四个寄存器（深入本质）

SysTick 只有 4 个寄存器，都在 `core_cm3.h` 中定义。标准库把它们封装在结构体 `SysTick_TypeDef` 中，通过指针 `SysTick` 来访问（这个指针指向地址 `0xE000E010`）。

#### 5S.2.1 CTRL 控制及状态寄存器（SysTick->CTRL）

地址偏移：0x00，这是 SysTick 的"总开关"和"状态指示器"。

| 位  | 名称      | 含义                                                                                                    |
| --- | --------- | ------------------------------------------------------------------------------------------------------- |
| 0   | ENABLE    | SysTick 使能位。**写 1**：计数器开始递减。**写 0**：计数器停止                                          |
| 1   | TICKINT   | 中断使能位。**写 1**：计数器减到 0 时产生 SysTick 中断。**写 0**：不产生中断                            |
| 2   | CLKSOURCE | 时钟源选择。**写 1**：AHB 时钟（72MHz）。**写 0**：AHB÷8（9MHz）                                        |
| 16  | COUNTFLAG | 计数到 0 标志。当计数器从 1 减到 0 时，硬件自动置 1。**读这个位会返回当前标志状态；读完后硬件自动清 0** |

**最关键的一点**：`COUNTFLAG` 是"读了就自动清零"的标志位，这和 TIM 的中断标志不同（TIM 需要手动写 0 清除）。这是 ARM 设计的巧妙之处——读一下就知道有没有到时间，不需要额外的清除操作。

**标准库的封装函数**：
```c
// 使能 SysTick（开计数器）
SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;   // 或写为：SysTick_CounterCmd(SysTick_Counter_Enable);
// 实际上标准库没有 SysTick_CounterCmd 这个函数，我们直接操作寄存器或自己封装
```

#### 5S.2.2 LOAD 自动重装载寄存器（SysTick->LOAD）

地址偏移：0x04，存放"重装载值"。

- 这是一个 **24 位**寄存器，有效值范围 0x000001 ~ 0xFFFFFF（即 1 ~ 16,777,215）
- 计数器每次从 LOAD 值开始往下递减
- 当减到 0 时，下一个时钟周期自动把 LOAD 值重新加载到计数器，然后继续递减

**关键公式**：如果 SysTick 时钟是 72MHz，那么每个计数的周期是 1/72μs ≈ 0.0139μs。要得到 1μs 的延时，需要 72 个计数；要得到 1ms 的延时，需要 72000 个计数。

$$
\text{LOAD值} = \frac{\text{所需时间（秒）} \times \text{SysTick时钟频率（Hz）}}{1} = \frac{1\text{ms} \times 72\text{MHz}}{1} = 72000
$$

> **为什么是 `所需时间 × 频率`，而不是 `所需时间 × 频率 - 1`？**
> 因为 SysTick 的工作方式是：从 LOAD 值加载到计数器，然后**立即开始计数**。设 LOAD=72000，计数过程是 72000 → 71999 → ... → 1 → 0（共 72000 个时钟周期）。而 TIM 的工作方式是 CNT 从 0 数到 ARR，数了 ARR+1 次才溢出。这是 SysTick 和 TIM 的重要区别！

#### 5S.2.3 VAL 当前计数值寄存器（SysTick->VAL）

地址偏移：0x08，存放当前计数器的值。

- 读这个寄存器：返回当前还剩多少计数
- 写这个寄存器：**写任何值都会把它清零！** 同时 `COUNTFLAG` 也会被清零
- 这个特性非常有用：重新开始计时只需要写 VAL，不需要停掉 SysTick

#### 5S.2.4 CALIB 校准寄存器（SysTick->CALIB）

地址偏移：0x0C，存放校准信息。**电赛中基本不用**，它告诉你用 AHB÷8 时钟时 10ms 需要多少计数。大多数 Cortex-M3 芯片这个寄存器的值是 9000（因为 9MHz × 10ms = 90000，实际存 9000 可能是缩写了），但 STM32F103 的这个值是 9000，不总是靠谱的，我们直接算。

---

### 5S.3 SysTick 精确延时实现（逐行详解）

现在我们来写一个精确到微秒的延时函数。我会把每一行代码的目的、原理都讲清楚。

#### 5S.3.1 微秒级延时（最底层）

```c
/**
 * @brief  微秒级延时函数
 * @param  us: 要延时的微秒数，最大约 233000us（约233ms）
 *           因 为 LOAD 是 24 位的，最大 16777215，
 *           16777215 / 72 = 233016us
 * @note   这个函数是阻塞式的，延时期间 CPU 不能做别的事
 *         但它非常精确，误差在 ±1 个时钟周期（约14ns）以内
 */
void Delay_us(uint32_t us)
{
    // ===== 第1步：关闭 SysTick，确保配置过程中不会有意外中断 =====
    // SysTick->CTRL 的第0位是 ENABLE 位，写 0 关闭
    // 为什么用 &= ~ 而不是直接 =0？
    // 因为 CTRL 寄存器还有其他位（TICKINT、CLKSOURCE），直接=0会把它们也清零
    // 我们只想关闭计数，不想改其他配置
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    // SysTick_CTRL_ENABLE_Msk 是一个宏，定义在 core_cm3.h 中：
    // #define SysTick_CTRL_ENABLE_Msk  (1UL << 0)
    // 所以 &= ~(1UL<<0) 的意思就是"把第0位清零，其他位保持不变"
    
    // ===== 第2步：设置重装载值 =====
    // 我们用的是 72MHz 时钟（后面会配置），每个计数 = 1/72μs
    // 要延时 us 微秒，需要的计数 = us × 72
    // 举例：us=1000（1ms），需要 72000 个计数
    // 为什么减1？后面解释
    SysTick->LOAD = us * 72 - 1;
    // 详细说明这个公式：
    // us * 72：因为72MHz下1μs=72个时钟周期
    // -1：因为 LOAD 设置为 N 时，计数过程是 N→N-1→...→1→0，共 N+1 个周期
    //     我们要 us*72 个周期，所以 LOAD = us*72-1
    // 例如：us=10，需要720个周期，LOAD=719，计数：719→718→...→1→0，共720次
    
    // ===== 第3步：清空当前计数器值 =====
    // 写任何值到 VAL 都会清零 VAL，同时会清除 COUNTFLAG
    // 这样确保从 LOAD 值开始递减，而不是从某个随机中间值开始
    SysTick->VAL = 0UL;
    // 0UL 意思是"无符号长整型的 0"，UL 后缀确保类型匹配
    
    // ===== 第4步：配置时钟源并开启 SysTick =====
    // 设置：使用 AHB 时钟（72MHz）+ 不产生中断 + 使能计数
    // SysTick_CTRL_CLKSOURCE_Msk 是第2位掩码：(1UL << 2)
    // SysTick_CTRL_ENABLE_Msk   是第0位掩码：(1UL << 0)
    // 注意：第1位(TICKINT)不设置，意味着不产生中断
    SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
    // 等价于：SysTick->CTRL |= (1<<2) | (1<<0);  // 时钟源=AHB，使能计数
    // 为什么不写成 SysTick->CTRL = ...？
    // 因为 CTRL 可能还有其他位需要保留，用 |= 只设置我们需要的位
    
    // ===== 第5步：等待计数完成（轮询方式）=====
    // COUNTFLAG 在计数器减到 0 时由硬件自动置 1
    // 读 CTRL 寄存器检查第16位（COUNTFLAG）
    // while 循环的条件：COUNTFLAG == 0，说明还没数完，继续等
    while(!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk));
    // 详细解读这行：
    // SysTick_CTRL_COUNTFLAG_Msk = (1UL << 16)  // 第16位掩码
    // SysTick->CTRL & (1<<16)：取出第16位的值
    // 如果第16位是0（没数完），!(0) = 1（真），继续循环
    // 如果第16位是1（数完了），!(非0) = 0（假），退出循环
    // 这个读操作会自动清除 COUNTFLAG！这是 Cortex-M3 设计好的
    
    // ===== 第6步：关闭 SysTick，省电 =====
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    // 延时结束，关掉计数器，省电。下次调用时会重新配置。
}
```

> **这个函数的核心设计思想**：
> 1. **轮询而非中断**：用 `while` 循环等待 `COUNTFLAG` 变 1。优点是简单、不需要配 NVIC、不占用中断优先级。缺点是阻塞式——CPU 在等待期间什么都不能做。但对于初始化阶段的延时（比如等外设稳定、等 OLED 上电），这是最合适的。
>
> 2. **每次都重新配置**：先关 SysTick → 设 LOAD → 清 VAL → 开 SysTick → 等待 → 关 SysTick。看起来繁琐，但保证了每次延时的 LOAD 值都精确对应请求的微秒数。如果你想让 SysTick 一直跑（作为系统时基），就应该用第二种模式——中断模式，下文会讲。
>
> 3. **最大延时限制**：LOAD 是 24 位的，最大值 16777215。在 72MHz 下：16777215 / 72 ≈ 233016μs ≈ 233ms。如果需要延时 500ms 怎么办？要么循环调用 `Delay_us(1000)` 500 次，要么使用下面的毫秒延时函数。

#### 5S.3.2 毫秒级延时（在微秒基础上封装）

```c
/**
 * @brief  毫秒级延时函数
 * @param  ms: 要延时的毫秒数
 * @note   内部循环调用 Delay_us(1000)，每次延时1ms再检查是否到总时长
 *         这样绕过了 24 位 LOAD 的最大值限制
 */
void Delay_ms(uint32_t ms)
{
    // 最简单的实现：循环 ms 次，每次延时 1000us
    while(ms--)
    {
        Delay_us(1000);  // 每次精确延时 1ms
    }
    // 为什么不用一次性延时 ms*1000 微秒？
    // 因为 ms=500 时，us=500000，us*72=36000000，超过了 24 位的 16777215
    // 分段延时：每次 1ms，循环 500 次，就绕过了限制
}
```

> **进阶优化**：上面的 `Delay_ms` 每次调用 `Delay_us(1000)` 都会执行一次"关 SysTick → 配 LOAD → 清 VAL → 开 SysTick → 等 → 关 SysTick"的完整流程。如果延时 500ms，就要重复 500 次。能不能优化？
>
> 可以！直接把 LOAD 设到最大值（16777215），每次等待约 233ms，然后检查还差多少。不过对电赛来说，`Delay_ms` 通常用在初始化阶段（延时几十到几百毫秒），多循环几十次完全不是问题。只有在主循环里频繁调用的延时才需要优化，而主循环的延时我们后面会用 SysTick 中断来做（非阻塞式）。

#### 5S.3.3 SysTick 中断模式——系统时基（最重要的部分！）

上面讲的 `Delay_us` 和 `Delay_ms` 都是**阻塞式**的——延时期间 CPU 只能空等。在电赛实际程序中，我们绝大多数时间需要的是**非阻塞式**延时：CPU 可以同时做别的事，到时间了自动触发。

这就需要 **SysTick 中断模式**。这也是你之前看到 `volatile uint32_t sys_time = 0;` 的完整实现原理。

**硬件工作流程**：
```
1. 配置 SysTick：LOAD=71999（72MHz ÷ 72000 = 1kHz = 1ms）
2. 使能 TICKINT（允许中断）
3. 使能计数器
4. 每 1ms：
   LOAD=71999 → 计数器从71999开始递减 → 减到0 → 硬件自动：
   a) 重新加载 LOAD 值
   b) 置位 COUNTFLAG
   c) 产生 SysTick 中断请求 → NVIC 响应 → 执行 SysTick_Handler()
   d) 在中断里 sys_time++  （全程自动，不占用主循环CPU时间）
```

**完整代码实现（逐行解释每一个字）**：

```c
// ===== 第一步：在文件顶部定义全局变量 =====
// volatile 关键字：告诉编译器"这个变量可能在任何时候被中断修改"
// 不加 volatile 的后果：编译器优化可能把 sys_time 缓存在寄存器里，
//   主循环永远读不到中断里修改的新值！这是非常隐蔽的 bug。
// __IO 是标准库定义的宏，等价于 volatile，定义在 core_cm3.h：
//   #define __IO  volatile
// 用 __IO 比直接用 volatile 更"标准库风格"，编译器效果完全一样
__IO uint32_t sys_time = 0;   // 系统运行总时间，单位：毫秒（ms）
// 为什么用 uint32_t？
// uint32_t 最大值 4294967295，即 2^32-1
// 4294967295 ms ≈ 49.7 天
// 电赛封箱测试最多几小时，绝对不会溢出，所以不需要处理溢出

__IO uint32_t sys_time_us = 0; // 系统运行总时间，单位：微秒（us），进阶用法

/**
 * @brief  初始化 SysTick 为 1ms 中断模式
 * @note   调用后，全局变量 sys_time 每 1ms 自动加 1
 *         这是整个系统时基的"心脏"，只初始化一次
 *         
 *         关键计算：
 *         目标中断频率 = 1000Hz（即每1ms中断一次）
 *         SysTick时钟 = 72MHz = 72,000,000 Hz
 *         LOAD值 = 72,000,000 / 1000 - 1 = 72000 - 1 = 71999
 *         
 *         SysTick 是 24 位递减计数器，LOAD 写 71999
 *         计数过程：71999 → 71998 → ... → 1 → 0
 *         共 72000 个时钟周期，每个周期 1/72μs
 *         总时间 = 72000 × (1/72)μs = 1000μs = 1ms ✓
 */
void SysTick_Init(void)
{
    // ----- 第1小步：配置 SysTick 时钟源 -----
    // 选择 AHB 时钟（72MHz）作为 SysTick 的时钟
    // 标准库函数 SysTick_CLKSourceConfig() 内部实现：
    //   if(SysTick_CLKSource == SysTick_CLKSource_HCLK)
    //       SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk;   // 第2位置1
    //   else
    //       SysTick->CTRL &= ~SysTick_CTRL_CLKSOURCE_Msk;  // 第2位清0
    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);
    // 注意：此时 SysTick 还没有被使能，只是选好了时钟源
    // 如果选择 SysTick_CLKSource_HCLK_Div8，时钟就是 9MHz
    
    // ----- 第2小步：设置重装载值 -----
    // 标准库函数 SysTick_SetReload() 内部就是：
    //   SysTick->LOAD = value;
    // 为什么是 72000-1=71999，前面已经详细推导过了
    SysTick_SetReload(72000 - 1);
    // 等价于：SysTick->LOAD = 71999;
    
    // ----- 第3小步：清空当前计数器 -----
    // SysTick_CounterCmd() 的清零操作内部就是：
    //   SysTick->VAL = 0;
    // 写任何值到 VAL 寄存器都会清零计数器
    SysTick_CounterCmd(SysTick_Counter_Clear);
    // 等价于：SysTick->VAL = 0UL;
    
    // ----- 第4小步：配置 SysTick 中断优先级 -----
    // SysTick 中断的优先级和其他中断一样，由 NVIC 管理
    // SysTick_IRQn 是 SysTick 中断在 NVIC 中的编号（值为 -1，特殊处理）
    // 在 core_cm3.h 中：#define SysTick_IRQn  -1
    // 因为它是内核中断，不是外设中断，编号是负数
    
    // NVIC_SetPriority() 是 CMSIS 提供的函数，设置中断优先级
    // 参数1：中断编号（SysTick_IRQn = -1）
    // 参数2：优先级值，0~15 之间（在优先级分组为 NVIC_PriorityGroup_2 时有效范围）
    // 数值越小优先级越高！
    NVIC_SetPriority(SysTick_IRQn, 1);  // 抢占优先级=0, 子优先级=1（分组2下）
    // 设得比较高（数值小），因为它提供整个系统时基，不能被打断太久
    // 用 NVIC_SetPriority 而不是 NVIC_Init，因为 SysTick 是内核中断，
    // NVIC_Init 用于外设中断，SysTick 有自己的优先级配置方式
    
    // ----- 第5小步：使能 SysTick 并开启中断 -----
    // SysTick_CTRL_ENABLE_Msk   = (1<<0)：使能计数器
    // SysTick_CTRL_TICKINT_Msk  = (1<<1)：使能中断
    // SysTick_CTRL_CLKSOURCE_Msk = (1<<2)：选择 AHB 时钟（前面已配置）
    // 三个位同时置1：
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk    // 开始计数！
                  |  SysTick_CTRL_TICKINT_Msk   // 允许中断！
                  |  SysTick_CTRL_CLKSOURCE_Msk; // 确认时钟源
    // 这一行之后，SysTick 就开始每 1ms 产生一次中断，
    // 每次中断都会调用 SysTick_Handler() 函数
    
    // 也可以用标准库函数：
    // SysTick_ITConfig(ENABLE);  // 使能中断
    // SysTick_CounterCmd(SysTick_Counter_Enable);  // 使能计数器
    // 两行等价于上面一行，效果一样
}

/**
 * @brief  SysTick 中断服务函数
 * @note   - 函数名必须是 SysTick_Handler，这是启动文件里定死的！
 *         - 每 1ms 自动被硬件调用一次
 *         - 函数体必须极其简短，只做最基本的计数操作
 *         - 千万不要在里面加延时、printf、复杂计算！
 */
void SysTick_Handler(void)
{
    // 这个函数名来自启动文件 startup_stm32f10x_md.s 中的中断向量表：
    //   DCD  SysTick_Handler   ; SysTick Handler
    // 如果你写成 SysTickHandler 或 systick_handler，中断发生时找不到函数，
    // 会跳转到默认的空循环处理函数，你的 sys_time 永远不会增加！
    
    sys_time++;  // 系统时间每 1ms 自增 1
    // 等价于：sys_time = sys_time + 1;
    // 因为 sys_time 是 __IO（volatile）修饰的，
    // 编译器不会优化这行，一定会从内存读、加1、写回内存
    
    // 如果需要微秒级系统时间：
    // sys_time_us += 1000;  // 每1ms加1000us，但这不是真正微秒级，是粗粒度的
    // 真正的微秒级系统时基需要用 TIM 定时器（1MHz计数频率），后面定时器章节会讲
}
```

**在 main 函数中的使用**：

```c
int main(void)
{
    // ===== 最早初始化：SysTick =====
    // 必须在所有需要延时的地方之前初始化！
    SysTick_Init();   // 之后 sys_time 每 1ms 自动 +1
    
    // ===== 使用范例 1：精确阻塞延时 =====
    // 等待 500ms，用 sys_time 实现，比 Delay_ms(500) 精确得多
    // Delay_ms(500) 会受编译器优化影响，用 sys_time 的延时是精确的 500ms
    uint32_t t_start = sys_time;              // 记录开始时间
    while(sys_time - t_start < 500);           // 等待，直到经过500ms
    // 这行代码的含义：
    // sys_time - t_start 计算已经过去了多少毫秒
    // 如果小于 500，继续循环等待
    // 一旦 >= 500，退出循环
    // 
    // 重要：这个延时仍然是阻塞式的（while 循环卡住 CPU）
    // 它只是比软件空转精确，但不能解决"CPU被占用"的问题
    
    // ===== 使用范例 2：非阻塞周期性任务（这才是正确姿势！）=====
    // 关键思想：不卡在 while 里等，而是"到了时间就做，做完继续往下"
    // 这样 CPU 可以同时处理多个不同周期的任务
    uint32_t t_10ms = 0;   // 10ms 任务的时间基准
    uint32_t t_50ms = 0;   // 50ms 任务的时间基准
    uint32_t t_100ms = 0;  // 100ms 任务的时间基准
    // 为什么用 static 或者放在 while 外面？
    // 这些变量必须保持值，不能每次循环重新归零
    
    while(1)
    {
        // ---- 每 1ms 执行的任务（频率最高，只做最紧急的事）----
        // 不需要额外变量，因为 sys_time 本身就是 1ms 精度
        // 例如：编码器值读取（但一般会放定时器中断里做）
        
        // ---- 每 10ms 执行一次：PID 控制计算 ----
        if(sys_time - t_10ms >= 10)    // 距离上次执行已经过去 10ms
        {
            t_10ms = sys_time;          // 更新基准时间！（这行非常关键，不能忘）
            // 如果写的是 t_10ms += 10，那如果有延迟错过了一次，后续周期会偏移
            // 写 t_10ms = sys_time，这次晚了下一次自动修正，不会累积误差
            PID_Compute();              // 在这里做 PID 计算
        }
        // 详细解读 if 条件：
        // sys_time 是当前时间，t_10ms 是上次执行的时间
        // sys_time - t_10ms：距离上次执行过去了多少毫秒
        // >= 10：如果已经过了 10ms 或更多，就进入执行
        // 为什么不用 == 10？
        // 因为主循环执行速度不确定，不一定每次都能精确卡在 10ms
        // 可能到 11ms 才检测到，用 >= 确保不会漏掉执行
        
        // ---- 每 50ms 执行一次：读传感器数据 ----
        if(sys_time - t_50ms >= 50)
        {
            t_50ms = sys_time;
            Sensor_Read();              // 读超声波、红外等传感器
        }
        
        // ---- 每 100ms 执行一次：刷新 OLED 显示 ----
        if(sys_time - t_100ms >= 100)
        {
            t_100ms = sys_time;
            OLED_Refresh();             // 更新屏幕显示
        }
        
        // ---- 非周期性的：事件驱动的处理 ----
        if(key_flag)                    // 按键标志位（在外部中断中置位）
        {
            key_flag = 0;               // 清除标志
            Key_Process();              // 处理按键事件
        }
        
        if(usart_rx_flag)               // 串口收到数据标志
        {
            usart_rx_flag = 0;          // 清除标志
            USART_Process();            // 处理串口数据
        }
    }
}
```

> **非阻塞任务调度的核心哲学**：
> 
> 上面这个 while(1) 结构，是电赛复杂程序的标准架构。它的核心思想就是 **"合作式多任务"**：
> - 把不同频率的任务放在同一个主循环里
> - 每个任务用 `if(sys_time - t >= period)` 判断是否到执行时间
> - 每个任务执行时间都很短（微秒级），执行完立刻退出，让其他任务有机会执行
> - **绝对不要**在任何任务里写 `while` 死等或者长延时
> 
> 这个架构看着简单，但它是所有 RTOS（实时操作系统）的雏形。RTOS 的内核本质上就是在做同一件事——给不同任务分配 CPU 时间。只不过 RTOS 用的是抢占式调度，我们这里用的是合作式调度。
> 
> **和 RTOS 的比较**：
> - 合作式调度（我们用的）：每个任务要主动退出，如果一个任务卡死了，整个系统卡死
> - 抢占式调度（RTOS）：操作系统强制切换任务，即使一个任务死循环，高优先级任务仍能运行
> 
> 电赛任务一般不超过 10 个，合作式调度完全够用，不需要引入 RTOS 的复杂度。**电赛一等奖的作品绝大多数都是这种合作式调度架构，不是 FreeRTOS。**

---

### 5S.4 SysTick 延时函数的进阶写法

前面 `Delay_us` 和 `Delay_ms` 用的是"裸操作寄存器"的方式，下面给出使用标准库封装的等价写法，你可能更容易理解：

```c
// 使用标准库函数的微秒延时（和裸寄存器写法完全等价）
void Delay_us_STD(uint32_t us)
{
    // SysTick_CounterCmd(SysTick_Counter_Disable) 内部：
    //   SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    SysTick_CounterCmd(SysTick_Counter_Disable);
    
    // SysTick_SetReload(value) 内部：
    //   SysTick->LOAD = value;
    SysTick_SetReload(us * 72 - 1);
    
    // SysTick_CounterCmd(SysTick_Counter_Clear) 内部：
    //   SysTick->VAL = 0UL;
    SysTick_CounterCmd(SysTick_Counter_Clear);
    
    // SysTick_CounterCmd(SysTick_Counter_Enable) 内部：
    //   SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
    SysTick_CounterCmd(SysTick_Counter_Enable);
    
    // SysTick_GetFlagStatus(SysTick_FLAG_COUNT) 内部：
    //   return (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) ? SET : RESET;
    while(SysTick_GetFlagStatus(SysTick_FLAG_COUNT) == RESET);
    
    SysTick_CounterCmd(SysTick_Counter_Disable);
}
```

> **标准库函数 vs 寄存器操作的对比**：
> 两种写法本质完全一样，标准库函数只是"套了一层壳"：
> - 标准库写法：`SysTick_CounterCmd(SysTick_Counter_Disable);` → 一看就知道是关闭计数器
> - 寄存器写法：`SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;` → 需要知道 CTRL 的第 0 位是使能位
> 
> **我的建议**：初期用标准库（可读性好，不容易写错），后期如果你有心深入研究，去看看这些标准库函数的内部实现（在 `core_cm3.h` 里），你会发现它们就是简单的寄存器操作。理解了这个，你就算是真正"知其所以然"了。

---

### 5S.5 SysTick 常见坑与调试

#### 坑1：中断函数名写错
```c
// ❌ 错误！启动文件里定义的是 SysTick_Handler，不是 SysTickHandler
void SysTickHandler(void) { ... }

// ❌ 错误！大小写敏感，systick_handler 不等于 SysTick_Handler
void systick_handler(void) { ... }

// ✅ 正确！
void SysTick_Handler(void) { ... }
```

**排查方法**：在 `SysTick_Handler` 里加一个 LED 翻转，看 LED 是否闪烁。如果不闪，说明函数名错了，中断根本没进来。

#### 坑2：忘记配 NVIC 优先级
```c
// ❌ 只配置了 SysTick 的 LOAD 和使能，没配 NVIC
SysTick_SetReload(71999);
SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_TICKINT_Msk;
// 结果：中断根本不触发！因为 NVIC 默认可能禁止了 SysTick 中断

// ✅ 正确做法：配置 LOAD 之前先配 NVIC
NVIC_SetPriority(SysTick_IRQn, 1);  // 不要忘了这行！
```

#### 坑3：中断里做了耗时操作
```c
// ❌ 糟糕的做法！中断里延时，整个系统卡死
void SysTick_Handler(void)
{
    sys_time++;
    Delay_ms(100);  // 中断里延时 = 自杀！中断不能等！
    OLED_Refresh();  // 刷新屏幕要几十ms，绝对不能在中断里做！
}

// ✅ 正确做法：只做计数，通知主循环
void SysTick_Handler(void)
{
    sys_time++;
}
```

#### 坑4：sys_time 没有加 volatile
```c
// ❌ 致命错误！没加 volatile
uint32_t sys_time = 0;
// 编译器看到 main 里从没改过 sys_time，只读不写
// 于是优化成：把 sys_time 的值缓存在寄存器 R5 里
// 每次 while(sys_time - t < 500) 就读 R5
// 但 R5 永远不会被 SysTick_Handler 更新！
// 结果：死循环！

// ✅ 正确！必须加 __IO 或 volatile
__IO uint32_t sys_time = 0;  // 或者 volatile uint32_t sys_time = 0;
```

#### 坑5：Delay_us 的参数过大
```c
Delay_us(500000);  // ❌ 500ms = 500000μs，500000*72=36000000 > 16777215
// LOAD 只有 24 位，最大存 16777215！
// 解决方案：用 Delay_ms(500) 代替
```

---

### 5S.6 SysTick 学习自检清单

学完这部分，你应该能回答以下问题：
1. SysTick 和 TIM 定时器有什么本质区别？（内核 vs 外设，24位 vs 16位，递减 vs 可递增递减）
2. SysTick 的时钟从哪里来？可以用哪些时钟源？
3. LOAD 寄存器的计算公式是什么？为什么 72MHz 下 1ms 中断要写 LOAD=71999？
4. `volatile` 关键字在 `sys_time` 变量上为什么是必须的？
5. 非阻塞任务调度 `if(sys_time - t >= period)` 和 `Delay_ms()` 有什么本质不同？
6. SysTick 中断服务函数的名字能随便写吗？为什么？
7. COUNTFLAG 和 TIM 的中断标志位清除方式有什么不同？

如果你有任何一个答不上来，请回到对应小节重新学习，确保彻底搞懂再往下走。SysTick 是整个系统时基的基石，必须地基打牢。

---

## 第六章 定时器——电赛最核心的外设（上）
定时器是STM32最复杂、最强大、电赛用的最多的外设，没有之一。电机PWM调速、舵机控制、精确延时、输入捕获测频率/脉宽、编码器接口测速、定时执行控制算法，全靠定时器。可以说，定时器玩得溜不溜，直接决定了你电赛能不能拿一等奖。

### 6.1 定时器基本概念
#### 6.1.1 定时器是什么？
你可以把定时器理解为一个**自动数数的计数器**：
- 给它一个时钟源，每来一个时钟脉冲，计数器的值就加1（或者减1）
- 当计数器的值数到你设定的最大值（自动重装值ARR）时，计数器清零重新开始数，同时触发一个"更新中断"
- 这就实现了定时：如果时钟是1MHz，ARR设为999，那么每1ms就会触发一次中断

#### 6.1.2 STM32F103的定时器分类
| 定时器类型 | 编号           | 位宽 | 挂在哪个总线 | 总线时钟 | 定时器实际时钟 | 功能                                                 |
| ---------- | -------------- | ---- | ------------ | -------- | -------------- | ---------------------------------------------------- |
| 高级定时器 | TIM1           | 16位 | APB2         | 72MHz    | 72MHz          | 带死区插入、互补输出，专门用来驱动无刷电机、三相电机 |
| 通用定时器 | TIM2/TIM3/TIM4 | 16位 | APB1         | 36MHz    | **72MHz**      | 最常用！定时中断、PWM输出、输入捕获、编码器接口      |
| 基本定时器 | TIM6/TIM7      | 16位 | APB1         | 36MHz    | 72MHz          | 只有定时功能，没有输入输出，C8T6没有这两个           |

> **为什么APB1总线时钟是36MHz，定时器时钟却是72MHz？**
> 这是STM32时钟树的一个特殊设计：当APBx预分频器系数不是1的时候，定时器时钟自动乘以2。我们APB1预分频是2（72MHz→36MHz），所以APB1上的定时器时钟就是36×2=72MHz，和APB2上的定时器一样快。
>
> **记住**：所有定时器（不管APB1还是APB2），时钟都是72MHz！这个是计算定时时间的基础，算错了定时时间就不对。

#### 6.1.3 定时器计数时钟的计算（重点！）
定时器的时钟不是直接72MHz给计数器，中间还有一个**预分频器PSC**：
```
定时器时钟源（72MHz）
  ↓
预分频器PSC：把时钟分频，分频系数 = PSC + 1
  ↓
计数器时钟CK_CNT = 72MHz / (PSC + 1)
  ↓
计数器CNT从0开始往上数，每个CK_CNT脉冲加1
  ↓
数到自动重装值ARR时，产生更新事件，CNT清零重新数
  ↓
溢出时间（定时时间）Tout = (ARR + 1) * (PSC + 1) / 72MHz
```

**这个公式是定时器一切功能的基础，必须背下来！**

举几个例子：
- 想要1ms中断一次：72MHz/(71+1) = 1MHz，也就是计数器1us加1，ARR=999，数1000次就是1000us=1ms
- 想要10us中断一次：PSC=71，ARR=9 → 10个1us就是10us
- 想要1s中断一次：PSC=7199，ARR=9999 → 72MHz/(7199+1)=10kHz，也就是0.1ms加1，数10000次就是1000ms=1s

> **PSC和ARR都是16位寄存器，取值范围都是0~65535**，所以最大定时时间是65536*65536/72M ≈ 59.6秒，足够电赛用了。

### 6.2 基本定时中断功能
最基础的用法：定时触发中断，执行固定周期的任务，比如1ms进一次中断，给系统提供时基，或者10ms执行一次PID控制算法。

#### 6.2.1 定时中断配置步骤（以TIM2为例，1ms中断）
1. 开时钟：TIM2挂在APB1总线上！所以开APB1的时钟
2. 配置时基单元：PSC预分频器、ARR自动重装值、计数模式
3. 配置NVIC，设置定时器中断优先级，使能中断
4. 使能定时器更新中断
5. 使能定时器计数器
6. 编写定时器中断服务函数

#### 6.2.2 代码实现
```c
// TIM2定时中断初始化，1ms进一次中断
void TIM2_Int_Init(void) {
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    // 1. 开时钟：TIM2在APB1总线！
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    
    // 2. 配置时基
    TIM_TimeBaseStructure.TIM_Period = 999;        // ARR自动重装值
    TIM_TimeBaseStructure.TIM_Prescaler = 71;      // PSC预分频器
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; // 时钟分频，不用管，默认就行
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; // 向上计数模式
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    
    // 3. 配置NVIC
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; // 定时器优先级设高一点
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // 4. 使能更新中断
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    
    // 5. 使能定时器
    TIM_Cmd(TIM2, ENABLE);
}

// 定时器2中断服务函数
volatile uint32_t sys_time = 0; // 系统运行时间，单位ms
void TIM2_IRQHandler(void) {
    if(TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) { // 检查更新中断标志
        sys_time++; // 1ms加1，这就是系统时基
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update); // 清除中断标志
    }
}
```

> **电赛神技巧：系统时基**
> 用一个定时器做1ms中断，维护一个全局的sys_time变量，从开机开始1ms加1，这就是你的系统时间。以后所有和时间相关的功能都可以用它实现：
> - 精确延时：`void Delay_ms(uint32_t ms) { uint32_t t = sys_time; while(sys_time - t < ms); }` 这个延时比软件延时精确100倍，而且不会被中断打断太多
> - 非阻塞延时：不用delay函数卡住主循环，比如"LED每隔500ms翻转一次"：
>   ```c
>   if(sys_time - last_time > 500) {
>       last_time = sys_time;
>       GPIO_TogglePin(GPIOC, GPIO_Pin_13);
>   }
>   ```
> - 任务调度：不同任务按不同周期执行，比如10ms读一次传感器，100ms刷新一次OLED，10ms执行一次PID
>
> 这是做复杂项目的基础架构，比写一堆delay_ms()高级多了，delay_ms()是阻塞式的，会卡住整个程序，用系统时基做非阻塞延时，主循环可以一直跑，响应其他事件。

#### 6.2.3 定时器定时时间计算练习
我出几个题，你自己算一下，确保你真的懂了：
1. PSC=7199，ARR=9，定时时间是多少？ 答案：(9+1)*(7199+1)/72M = 10*7200/72M = 1ms
2. PSC=0，ARR=71，定时时间是多少？ 答案：72*1/72M = 1us
3. 想要20ms定时一次，PSC和ARR可以怎么设？ 答案：PSC=71，ARR=19999 → 20000*1us=20ms，或者其他组合，只要(ARR+1)*(PSC+1)=1440000就行

### 6.3 PWM输出功能——电机/舵机/LED调光必备
PWM是电赛用的最多的定时器功能，直流电机调速、舵机角度控制、LED呼吸灯、模拟DAC输出，全是PWM。

#### 6.3.1 PWM原理
PWM就是脉冲宽度调制，前面讲过：定时器自动在引脚上输出方波，你可以控制方波的频率和占空比：
- **频率**：方波一秒钟有多少个周期，单位Hz
- **占空比**：一个周期内高电平时间占整个周期的比例，0%~100%
- STM32的PWM是硬件自动输出的，不需要CPU干预，配置好之后就一直输出，CPU该干嘛干嘛，非常稳定。

#### 6.3.2 PWM频率和占空比计算
PWM的频率由PSC和ARR决定，和定时中断一样：
- PWM频率 = 72MHz / ((PSC + 1) * (ARR + 1))
- 占空比由比较寄存器CCR决定：占空比 = CCR / (ARR + 1) * 100%
  - CCR=0：占空比0%，一直输出低电平
  - CCR=ARR：占空比100%，一直输出高电平
  - CCR=ARR/2：占空比50%，高低各一半

**电赛常用PWM参数**：
- 直流电机驱动：PWM频率一般10kHz~20kHz，频率太低电机会啸叫，频率太高开关损耗大
- 舵机控制：频率50Hz（周期20ms），高电平时间0.5ms~2.5ms对应0°~180°
- LED呼吸灯：频率100Hz以上，人眼就看不出闪烁了

#### 6.3.3 PWM输出对应的引脚（重要！）
定时器的每个通道都对应固定的GPIO引脚，不是随便哪个引脚都能输出PWM的！我把最常用的TIM2/TIM3/TIM4的引脚映射列出来，你必须记牢：

| 定时器       | 通道1      | 通道2      | 通道3       | 通道4       | 总线 |
| ------------ | ---------- | ---------- | ----------- | ----------- | ---- |
| TIM2         | PA0 / PA15 | PA1 / PB3  | PA2 / PB10  | PA3 / PB11  | APB1 |
| TIM3         | PA6 / PB4  | PA7 / PB5  | PB0 / PC8   | PB1 / PC9   | APB1 |
| TIM4         | PB6 / PD12 | PB7 / PD13 | PB8 / PD14  | PB9 / PD15  | APB1 |
| TIM1（高级） | PA8 / PE9  | PA9 / PE11 | PA10 / PE13 | PA11 / PE14 | APB2 |

> **为什么要记这个？**
> 很多新手想在PB0输出PWM，结果配置了TIM2的PWM，当然出不来，因为PB0是TIM3_CH3的引脚，不是TIM2的。用PWM之前先查表，看你用的引脚对应哪个定时器的哪个通道，然后配置对应的定时器通道。
>
> 引脚重映射：有些定时器通道可以映射到其他引脚上，通过AFIO配置，比如TIM3_CH1默认是PA6，可以重映射到PB4，这个等你遇到了再查手册就行，一开始先用默认引脚。

#### 6.3.4 PWM输出配置步骤（以TIM3_CH3输出10kHz PWM为例，PB0引脚）
1. 开时钟：GPIOB时钟（APB2）+ TIM3时钟（APB1）+ AFIO时钟（如果需要重映射）
2. 配置PB0为**复用推挽输出**（AF_PP）！PWM是外设输出，必须用复用推挽，普通推挽输出不行
3. 配置定时器时基：PSC和ARR，算好频率
4. 配置PWM输出模式：常用PWM1模式，向上计数时CNT<CCR输出高，否则低
5. 使能定时器比较输出
6. 设置CCR寄存器的值，也就是初始占空比
7. 使能定时器

#### 6.3.5 代码实现
```c
// TIM3 CH3 PWM初始化，PB0引脚，10kHz PWM
void TIM3_PWM_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;
    
    // 1. 开时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    
    // 2. 配置PB0为复用推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // 复用推挽！
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // 3. 配置时基：10kHz PWM
    // 频率 = 72M / ((71+1)*(99+1)) = 72M / (72*100) = 10kHz
    TIM_TimeBaseStructure.TIM_Period = 99;         // ARR=99
    TIM_TimeBaseStructure.TIM_Prescaler = 71;      // PSC=71
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
    
    // 4. 配置PWM模式
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; // PWM1模式
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; // 输出使能
    TIM_OCInitStructure.TIM_Pulse = 0; // 初始CCR=0，占空比0
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; // 高电平有效
    TIM_OC3Init(TIM3, &TIM_OCInitStructure); // 通道3初始化，注意函数名是TIM_OC3Init！
    
    // 5. 使能比较输出预装载
    TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);
    // 6. 使能定时器
    TIM_Cmd(TIM3, ENABLE);
}

// 设置占空比函数，duty范围0~100
void TIM3_SetDuty(uint8_t duty) {
    if(duty > 100) duty = 100;
    TIM_SetCompare3(TIM3, duty); // CCR = duty，因为ARR=99，duty=50就是50%占空比
}
```

> **通道对应的函数名**：
> - 通道1：TIM_OC1Init()、TIM_SetCompare1()
> - 通道2：TIM_OC2Init()、TIM_SetCompare2()
> - 通道3：TIM_OC3Init()、TIM_SetCompare3()
> - 通道4：TIM_OC4Init()、TIM_SetCompare4()
> 新手经常犯的错：用通道3却调用了TIM_OC1Init，结果PWM出不来。

#### 6.3.6 舵机PWM控制实战
舵机是电赛最常用的执行器，控制信号是50Hz PWM（周期20ms）：
- 0.5ms高电平 → 0度
- 1.5ms高电平 → 90度
- 2.5ms高电平 → 180度

我们用TIM2_CH1（PA0）输出舵机PWM：
```c
void Servo_Init(void) {
    // 50Hz PWM：72M / ((71+1)*(19999+1)) = 72M/(72*20000) = 50Hz，周期20ms
    // PSC=71 → 计数器1us加1，ARR=19999 → 20000us=20ms
    // CCR的值就是高电平时间，单位us：500~2500对应0~180度
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    TIM_TimeBaseStructure.TIM_Period = 19999;
    TIM_TimeBaseStructure.TIM_Prescaler = 71;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 1500; // 默认90度
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(TIM2, &TIM_OCInitStructure);
    
    TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);
    TIM_Cmd(TIM2, ENABLE);
}

// 设置舵机角度，angle:0~180
void Servo_SetAngle(uint8_t angle) {
    if(angle > 180) angle = 180;
    uint16_t ccr = 500 + angle * 200 / 180; // 0度500，180度2500
    TIM_SetCompare1(TIM2, ccr);
}
```

> **舵机控制注意事项**：
> 1. 舵机一定要单独供电！不要用STM32的3.3V或者5V给舵机供电，舵机转动时电流很大，会把STM32电源拉垮导致复位
> 2. 舵机地线必须和STM32共地，否则PWM信号没有回路，舵机不转
> 3. 不要让舵机长时间堵转，会烧坏舵机

---


## 第六章 定时器——电赛最核心的外设（中）

### 6.4 输入捕获功能——测脉宽、测频率、超声波测距
输入捕获是定时器的另一个核心功能，电赛里用来测方波频率、测脉冲宽度、超声波测距、红外解码、霍尔传感器测速等。

#### 6.4.1 输入捕获原理
输入捕获简单说就是：**当引脚上出现指定的电平跳变时，定时器自动把当前计数器CNT的值锁存到CCR寄存器里，同时触发中断**。

举个例子，测一个高电平脉冲的宽度：
1. 配置为上升沿捕获，当引脚从低变高时，捕获CNT的值记为t1，同时自动切换为下降沿捕获
2. 当引脚从高变低时，捕获CNT的值记为t2，同时自动切换回上升沿捕获
3. 脉冲宽度 = (t2 - t1) × 计数器计数周期
4. 如果t2 < t1说明计数器溢出了，加上溢出次数×ARR就行

这个过程完全是硬件自动完成的，不需要CPU一直去读引脚电平，精度非常高，可以精确到1us级别，比你在主循环里读引脚准100倍。

#### 6.4.2 输入捕获的通道对应关系
和PWM输出一样，输入捕获通道和PWM输出通道是同一个引脚，还是那套引脚映射表：
- TIM2_CH1 → PA0
- TIM3_CH3 → PB0
- 等等，和PWM引脚完全一致

输入捕获的时钟和定时器其他功能一样，都是72MHz，PSC和ARR还是和之前一样配置。

#### 6.4.3 电赛最常用：HC-SR04超声波测距
超声波模块是电赛避障、测距的标配，它的工作原理：
1. Trig引脚给至少10us的高电平触发信号
2. 模块自动发送8个40kHz超声波
3. 超声波遇到障碍物反射回来，Echo引脚输出高电平
4. **Echo引脚高电平的时间 × 声速(340m/s) / 2 = 距离**

这就是标准的输入捕获测脉宽的应用，我们用TIM2_CH2（PA1）接Echo引脚，PA0接Trig引脚。

#### 6.4.4 输入捕获配置步骤（超声波测距为例）
1. 开时钟：GPIOA（APB2）+ TIM2（APB1）
2. 配置PA0为推挽输出（Trig），PA1为浮空输入（Echo）
3. 配置定时器时基：PSC=71，ARR=65535 → 计数器1us加1，最大能测65.5ms的脉宽，对应距离11米左右，足够用
4. 配置输入捕获通道：上升沿捕获、映射关系、分频
5. 配置NVIC，开启捕获中断和更新中断（溢出中断）
6. 编写中断服务函数，处理上升沿、下降沿捕获和溢出
7. 封装测距函数

#### 6.4.5 代码实现
```c
// 超声波输入捕获相关变量
volatile uint8_t  echo_state = 0; // 0:等待上升沿, 1:等待下降沿
volatile uint32_t echo_time  = 0; // 高电平时间，单位us
volatile uint8_t  echo_done  = 0; // 测量完成标志

void HCSR04_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_ICInitTypeDef TIM_ICInitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    // 1. 开时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    
    // 2. GPIO配置
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0; // Trig
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1; // Echo
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; // 浮空输入
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    GPIO_ResetBits(GPIOA, GPIO_Pin_0); // Trig默认低
    
    // 3. 定时器时基配置：1us计数一次
    TIM_TimeBaseStructure.TIM_Period = 65535;
    TIM_TimeBaseStructure.TIM_Prescaler = 71;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    
    // 4. 输入捕获配置
    TIM_ICInitStructure.TIM_Channel = TIM_Channel_2; // 通道2
    TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising; // 先上升沿捕获
    TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI; // 直接映射到TI2
    TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1; // 不分频，每个跳变都捕获
    TIM_ICInitStructure.TIM_ICFilter = 0x03; // 滤波器，滤除干扰，非常重要！
    TIM_ICInit(TIM2, &TIM_ICInitStructure);
    
    // 5. NVIC配置
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // 6. 开启中断：捕获中断 + 更新中断（溢出）
    TIM_ITConfig(TIM2, TIM_IT_CC2 | TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM2, ENABLE);
}

// 定时器2中断服务函数
void TIM2_IRQHandler(void) {
    static uint16_t cnt = 0; // 溢出计数器
    
    // 溢出中断处理
    if(TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
        if(echo_state == 1) { // 正在等下降沿才计数溢出
            cnt++;
            if(cnt >= 5) { // 超过5*65.5ms=327ms还没收到回波，说明超出量程
                echo_done = 2; // 超时错误
                echo_state = 0;
                cnt = 0;
            }
        }
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }
    
    // 捕获中断处理
    if(TIM_GetITStatus(TIM2, TIM_IT_CC2) != RESET) {
        if(echo_state == 0) { // 上升沿捕获到
            cnt = 0;
            TIM_SetCounter(TIM2, 0); // 计数器清零，从0开始计时
            echo_state = 1;
            // 切换为下降沿捕获
            TIM_OC2PolarityConfig(TIM2, TIM_ICPolarity_Falling);
        } else if(echo_state == 1) { // 下降沿捕获到
            echo_time = TIM_GetCapture2(TIM2) + (uint32_t)cnt * 65536;
            echo_state = 0;
            echo_done = 1; // 测量完成
            // 切回上升沿捕获
            TIM_OC2PolarityConfig(TIM2, TIM_ICPolarity_Rising);
        }
        TIM_ClearITPendingBit(TIM2, TIM_IT_CC2);
    }
}

// 触发一次测量，返回距离，单位mm，返回0xFFFF表示超时
uint16_t HCSR04_GetDistance(void) {
    uint32_t t;
    echo_done = 0;
    // 发送10us高电平触发
    GPIO_SetBits(GPIOA, GPIO_Pin_0);
    Delay_us(20);
    GPIO_ResetBits(GPIOA, GPIO_Pin_0);
    
    // 等待测量完成，超时300ms
    t = sys_time;
    while(echo_done == 0 && sys_time - t < 300);
    
    if(echo_done == 1) {
        // 距离(mm) = 时间(us) * 340m/s / 2 = time * 0.34mm/us / 2 = time * 0.17
        return (uint16_t)(echo_time * 17 / 100);
    } else {
        return 0xFFFF; // 超时
    }
}
```

> **超声波测距电赛经验**：
> 1. **一定要开滤波器**：TIM_ICFilter设为0x03，相当于滤除3个时钟周期以内的尖峰干扰，否则超声波模块容易被电机、电源噪声干扰，跳变很多
> 2. **不要连续测距**：两次测距间隔至少60ms以上，否则上一次的回波会干扰下一次
> 3. **多次测量取中值**：连续测5次，去掉最大值最小值取平均，结果会稳定很多
> 4. **盲区**：2cm以内是盲区，太近的距离测不准
> 5. **声速受温度影响**：如果题目对精度要求高，要加温度补偿：声速 = 331.5 + 0.6×温度

### 6.5 编码器接口——电机测速神器（平衡车/小车必备）
这是定时器最强大的功能之一，专门用来接旋转编码器测电机转速和位置，电赛做智能车、平衡车、倒立摆必用。硬件编码器接口比你用外部中断自己计数准100倍，还不会丢脉冲。

#### 6.5.1 编码器原理
增量式旋转编码器有两个输出引脚A相和B相，相位差90度：
- 电机正转时，A相超前B相90度
- 电机反转时，B相超前A相90度
- 电机转一圈，A和B各输出固定数量的脉冲（比如减速电机一般是带编码器，一圈几百个脉冲）

STM32的定时器编码器接口可以硬件自动处理A/B相的信号：
- 自动根据相位差判断正转还是反转
- 正转时CNT自动加1，反转时CNT自动减1
- 完全不需要CPU干预，不会丢脉冲，最高可以处理几十MHz的脉冲
- 还可以设置4倍频：A相上升沿、下降沿，B相上升沿、下降沿都计数，一圈的脉冲数×4，精度提高4倍

#### 6.5.2 编码器接口的引脚
编码器接口只能用CH1和CH2两个通道，固定引脚：
- TIM2编码器：CH1=PA0，CH2=PA1
- TIM3编码器：CH1=PA6，CH2=PA7
- TIM4编码器：CH1=PB6，CH2=PB7

> **为什么不用CH3/CH4？** 编码器接口需要两个通道判断方向，只有CH1和CH2支持编码器模式。

#### 6.5.3 编码器配置步骤（以TIM3为例，接直流减速电机编码器）
1. 开时钟：GPIOA（APB2）+ TIM3（APB1）
2. 配置PA6和PA7为**浮空输入**（编码器输入不需要上下拉，编码器本身是推挽输出）
3. 配置定时器时基：PSC=0，ARR=65535（最大计数范围）
4. 配置编码器模式：TI1和TI2都计数，也就是4倍频模式
5. 配置滤波器，滤除电机碳刷干扰
6. 使能定时器
7. 定时读CNT的值，计算速度

#### 6.5.4 代码实现
```c
int16_t encode_cnt = 0;

void Encoder_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_ICInitTypeDef TIM_ICInitStructure;
    
    // 1. 开时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    
    // 2. GPIO配置：PA6 PA7浮空输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 3. 时基配置
    TIM_TimeBaseStructure.TIM_Period = 65535;
    TIM_TimeBaseStructure.TIM_Prescaler = 0; // 不分频
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
    
    // 4. 编码器模式配置
    TIM_EncoderInterfaceConfig(TIM3, TIM_EncoderMode_TI12, 
                               TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);
    // TIM_EncoderMode_TI12就是4倍频模式，最常用
    
    // 5. 输入捕获配置，设置滤波器
    TIM_ICStructInit(&TIM_ICInitStructure);
    TIM_ICInitStructure.TIM_ICFilter = 10; // 滤波器，滤除电机干扰
    TIM_ICInit(TIM3, &TIM_ICInitStructure);
    
    // 6. 计数器清零，使能定时器
    TIM_SetCounter(TIM3, 0);
    TIM_Cmd(TIM3, ENABLE);
}

// 读取编码器增量，调用周期10ms
int16_t Encoder_GetDelta(void) {
    int16_t cnt = (int16_t)TIM_GetCounter(TIM3);
    TIM_SetCounter(TIM3, 0); // 读完清零
    return cnt;
}
```

> **编码器测速电赛经验**：
> 1. **滤波器一定要开**：直流电机碳刷干扰很大，不开滤波器会有很多杂脉冲，速度跳变
> 2. **4倍频是标配**：精度最高，比如一圈390个脉冲的编码器，4倍频后一圈1560个脉冲，速度控制精度很高
> 3. **固定周期读取**：比如每10ms读一次CNT的值然后清零，这个值就是这10ms内的脉冲数，也就是速度
> 4. **注意溢出**：如果10ms内脉冲数超过32767会溢出，不过一般电机转速不会这么快，10ms足够
> 5. **两个编码器用两个定时器**：TIM3和TIM4各接一个电机，正好控制双轮小车

### 6.6 TIM1 高级定时器 —— 互补输出与死区（电赛进阶）

#### 6.6.1 TIM1 和通用定时器的区别

STM32F103C8T6 只有一个高级定时器 TIM1（APB2，72MHz）。和通用定时器 TIM2/3/4 相比，TIM1 多了以下功能：

| 功能              | TIM2/3/4（通用） | **TIM1（高级）**               |
| ----------------- | ---------------- | ------------------------------ |
| 定时中断          | ✅                | ✅                              |
| PWM 输出          | ✅                | ✅                              |
| 输入捕获          | ✅                | ✅                              |
| 编码器接口        | ✅                | ✅                              |
| **互补 PWM 输出** | ❌                | **✅**                          |
| **死区插入**      | ❌                | **✅**                          |
| **刹车功能**      | ❌                | **✅**                          |
| 通道数            | 4 通道           | 4 通道 + 4 互补通道 = 8 个输出 |

> **什么时候必须用 TIM1？** 当你要驱动**三相无刷电机（BLDC）**、**H 桥**或**半桥**电路时，需要两路互补的 PWM 信号（一路高时另一路必须低），而且两路切换之间必须插入"死区时间"防止上下桥臂同时导通短路。这种场景下，TIM1 是唯一的选择。

#### 6.6.2 死区时间是什么？为什么需要它？

在 H 桥或半桥驱动电路中，上下两个 MOS 管是串联在电源和地之间的：

```
      VCC
       │
    ┌──┴──┐
    │ Q1   │ 上桥臂（P-MOS 或 N-MOS）
    └──┬──┘
       ├─────→ 输出（接电机）
    ┌──┴──┐
    │ Q2   │ 下桥臂（N-MOS）
    └──┬──┘
       │
      GND
```

**致命问题**：MOS 管从导通到关断需要时间（几十到几百纳秒）。如果 Q1 关断的同时 Q2 立即导通，在那一瞬间 Q1 还没完全关断（仍有残余导电通道），Q2 已经导通了——电源 VCC 通过 Q1→Q2 直接短路到地！电流瞬间极大，MOS 管烧毁。

**死区的解决方案**：在两个互补信号之间插入一个**极短的时间间隙**（死区时间），在这段时间里两个 MOS 管都关断，确保一个完全关断后另一个才导通。

```
正常理想情况（危险！）：
Q1: ──────┐     ┌──────
           └─────┘
Q2: ──┐     ┌──────
       └─────┘
     ↑ Q1关断和Q2导通同时发生 → 可能瞬间短路！

插入死区后（安全）：
Q1: ──────┐             ┌──────
           └─────────────┘
Q2: ──┐             ┌──────
       └─────────────┘
         ↑死区↑ ↑死区↑
       两管都关断（安全！）
```

死区时间一般设为 **几百纳秒到几微秒**，取决于 MOS 管的开关速度。通常在 500ns~2μs 之间。

#### 6.6.3 TIM1 互补 PWM 输出引脚

| 通道     | 主输出 | 互补输出   | 说明               |
| -------- | ------ | ---------- | ------------------ |
| TIM1_CH1 | PA8    | PA13(PB13) | CH1 + CH1N（互补） |
| TIM1_CH2 | PA9    | PA14(PB14) | CH2 + CH2N         |
| TIM1_CH3 | PA10   | PA15(PB15) | CH3 + CH3N         |
| TIM1_CH4 | PA11   | -          | CH4（无互补输出）  |

> **注意**：PA13/PA14/PA15 默认是 SWD 调试接口（JTAG）！如果要用 TIM1 的互补通道，需要先关闭 JTAG 功能，保留 SWD：
> ```c
> GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);  // 关闭JTAG，保留SWD
> // 之后 PA15, PB3, PB4 才能当普通 GPIO 用
> ```

#### 6.6.4 TIM1 互补 PWM 配置代码（逐行详解）

```c
/**
 * @brief  TIM1 互补 PWM 初始化（死区 1μs，20kHz PWM）
 * @note   输出引脚：PA8(TIM1_CH1), PB13(TIM1_CH1N 互补)
 *         注意：PB13 默认是 JTAG 功能，需要先关闭 JTAG
 *         
 *         死区计算：
 *         定时器时钟 = 72MHz（APB2无分频）
 *         DTG[7:5] = 100 → 死区时间 = (32 + DTG[4:0]) × Tdtg
 *         Tdtg = 16 × Tclk = 16 / 72MHz = 222ns
 *         如果 DTG[4:0] = 10：死区 = (32+10) × 222ns = 9338ns ≈ 9.3μs
 *         如果 DTG[4:0] = 0：死区 = 32 × 222ns ≈ 7.1μs
 *         
 *         要实现 1μs 死区：
 *         DTG[7:5] = 110 → Tdtg = 32 × Tclk = 32/72M ≈ 444ns
 *         需要 DTG[5:0] ≈ 1μs/444ns - 32 ≈ 2.25 - 32（负数，不行）
 *         换 DTG[7:5] = 111 → Tdtg = 32 × Tclk = 32/72M ≈ 444ns
 *         需要 DTG[4:0] = 1μs/444ns ≈ 2.25（取2）
 *         
 *         最简单的配置（足够用）：直接用标准库的宏
 */
void TIM1_BKIN_PWM_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;
    TIM_BDTRInitTypeDef TIM_BDTRInitStructure;  // 死区/刹车配置结构体
    
    // ===== 第1步：开时钟 =====
    // TIM1 在 APB2 总线上！
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB 
                           | RCC_APB2Periph_TIM1 | RCC_APB2Periph_AFIO, ENABLE);
    
    // ===== 第2步：关闭 JTAG，释放 PB13/PB14/PB15 =====
    // 默认 PB13 是 JTAG 的 NJTRST，PB14 是 JTDO，PB15 是 JTDI
    // 需要用 AFIO 重映射关闭 JTAG（保留 SWD）
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
    // 关闭 JTAG 后：
    // PA13=SWDIO（保留）, PA14=SWCLK（保留）→ 仍可下载调试
    // PB3=PB3（释放）, PB4=PB4（释放）, PA15=PA15（释放）
    
    // ===== 第3步：GPIO 配置 =====
    // PA8：TIM1_CH1 主输出（复用推挽）
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  // 复用推挽！
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // PB13：TIM1_CH1N 互补输出（复用推挽）
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // ===== 第4步：时基配置（20kHz PWM）=====
    // 频率 = 72MHz / ((PSC+1)*(ARR+1)) = 72MHz / (0*3599) ≈ 20kHz
    // PSC=0, ARR=3599 → 20kHz，分辨率 3600 级
    // 或者 PSC=35, ARR=99 → 20kHz，分辨率 100 级（简化计算）
    TIM_TimeBaseStructure.TIM_Period = 3599;         // ARR
    TIM_TimeBaseStructure.TIM_Prescaler = 0;         // PSC
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    // TIM1 需要设置重复计数寄存器（TIM2/3/4 没有这个）
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;  // 每次更新事件都产生
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);
    
    // ===== 第5步：PWM 输出配置 =====
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;       // PWM1 模式
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;  // 主输出使能
    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable; // 互补输出使能！
    // [关键] TIM_OutputNState_Enable = 互补输出也开启
    // 这是 TIM1 特有的配置，TIM2/3/4 没有互补通道
    
    TIM_OCInitStructure.TIM_Pulse = 0;                      // 初始占空比 0
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;       // 主输出高有效
    TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;     // 互补输出高有效
    // [关键] TIM_OCNPolarity = 互补输出的极性
    // 高有效：CNT<CCR时互补输出为低，CNT≥CCR时为高
    // 低有效：反过来
    
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;    // 空闲时输出低
    TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;  // 空闲时互补输出低
    // MOE=0（刹车或软件关断）时，输出强制为 Idle 状态
    
    TIM_OC1Init(TIM1, &TIM_OCInitStructure);  // 通道1
    
    // ===== 第6步：死区和刹车配置（TIM1 特有！）=====
    TIM_BDTRInitStructure.TIM_OSSRState = TIM_OSSRState_Enable;
    // OSSR：Off-State Selection for Run mode
    // 运行模式下的关闭状态选择，一般使能
    
    TIM_BDTRInitStructure.TIM_OSSIState = TIM_OSSIState_Enable;
    // OSSI：Off-State Selection for Idle mode
    // 空闲模式下的关闭状态选择，一般使能
    
    TIM_BDTRInitStructure.TIM_LOCKLevel = TIM_LOCKLevel_OFF;
    // LOCK：寄存器锁定级别，OFF=不锁定
    
    // ===== 死区时间配置（核心！）=====
    // DTG（Dead-Time Generator）寄存器（8位）：
    //   DTG[7:5] = 0xx → DT = DTG[7:0] × Tdtg（Tdtg = Tclk）
    //   DTG[7:5] = 10x → DT = (64 + DTG[5:0]) × Tdtg（Tdtg = 2×Tclk）
    //   DTG[7:5] = 110 → DT = (32 + DTG[4:0]) × Tdtg（Tdtg = 8×Tclk）
    //   DTG[7:5] = 111 → DT = (32 + DTG[4:0]) × Tdtg（Tdtg = 16×Tclk）
    //   Tclk = 1/72MHz ≈ 13.9ns
    //
    // 目标：死区时间 = 1μs
    // DTG[7:5] = 100 → Tdtg = 2×Tclk = 27.8ns, DT = (64+DTG[5:0]) × 27.8ns
    // 需要 DTG[5:0] = 1000ns/27.8ns - 64 ≈ 36 - 64 = -28（负数，不行！）
    // 换 DTG[7:5] = 110 → Tdtg = 8×Tclk = 111.1ns, DT = (32+DTG[4:0]) × 111.1ns
    // 需要 DTG[4:0] = 1000ns/111.1ns - 32 ≈ 9 - 32，还是不行
    // 换 DTG[7:5] = 111 → Tdtg = 16×Tclk = 222.2ns, DT = (32+DTG[4:0]) × 222.2ns
    // 需要 DTG[4:0] = 1000/222.2 - 32 ≈ 4.5 - 32，还是不对...
    // 用 DTG[7:5]=100, DTG[5:0]=8：DT=(64+8)×27.8ns = 72×27.8ns ≈ 2000ns = 2μs
    // 用 DTG[7:5]=101, DTG[4:0]=28：DT=(32+28)×55.6ns = 60×55.6ns ≈ 3.3μs（不对）
    
    // 简化！直接给一个经验值：
    // DTG = 0xC0 | 0x08 = 0xC8 → DTG[7:5]=110, DTG[4:0]=8
    // DeadTime = (32+8) × 8×Tclk = 40 × 111.2ns = 4.45μs
    
    // 实际上最常见的做法是用标准库提供的死区设置函数：
    TIM_BDTRInitStructure.TIM_DeadTime = 72;  // 直接写 DTG 寄存器值
    // DTG=72(0x48)：DTG[7:5]=010, DTG[4:0]=8
    // DeadTime = (32+8) × 2×Tclk = 40 × 27.8ns ≈ 1.1μs ✓
    // 这是最简单的方法：尝试几个值，用示波器实测
    
    TIM_BDTRInitStructure.TIM_Break = TIM_Break_Enable;     // 使能刹车功能
    TIM_BDTRInitStructure.TIM_BreakPolarity = TIM_BreakPolarity_High; // 刹车高有效
    // 刹车引脚：PB12（TIM1_BKIN）
    // 刹车有效时，TIM1 所有输出通道立即进入空闲状态（强制关闭）
    // 这是硬件级别的保护！电机过流时拉高 BKIN 引脚，PWM 输出瞬间关闭
    
    TIM_BDTRInitStructure.TIM_AutomaticOutput = TIM_AutomaticOutput_Enable;
    // AOE：自动输出使能。MOE 在下次更新事件时自动恢复
    // 不使能的话，刹车后需要手动恢复 MOE
    
    TIM_BDTRConfig(TIM1, &TIM_BDTRInitStructure);
    
    // ===== 第7步：主输出使能（MOE）=====
    // TIM1 和 TIM8 有 MOE（Main Output Enable）位
    // MOE=0 时所有输出通道强制进入空闲状态
    // 必须设置 MOE=1 才能让 PWM 信号真正输出到引脚！
    TIM_CtrlPWMOutputs(TIM1, ENABLE);
    // 这个函数内部：TIM1->BDTR |= TIM_BDTR_MOE;
    // 通用定时器没有这个，高级定时器必须加这一行！
    
    // ===== 第8步：使能定时器 =====
    TIM_Cmd(TIM1, ENABLE);
    
    // ===== 设置初始占空比 =====
    TIM_SetCompare1(TIM1, 1800);  // 50% 占空比（1800/3600）
}

/**
 * @brief  刹车中断服务函数（可选）
 * @note   当 BKIN 引脚（PB12）检测到刹车信号时触发
 *         可以在中断里做紧急处理（关闭电机电源等）
 */
void TIM1_BRK_IRQHandler(void)
{
    if(TIM_GetITStatus(TIM1, TIM_IT_Break) != RESET)
    {
        // 刹车发生了！PWM 已经被硬件自动关闭
        // 在这里做紧急处理：
        Motor_EmergencyStop();  // 关闭电机电源
        LED_ErrorOn();          // 点错误指示灯
        
        TIM_ClearITPendingBit(TIM1, TIM_IT_Break);
    }
}
```

> **TIM1 互补 PWM 使用经验**：
> 1. **MOE 一定要开**：`TIM_CtrlPWMOutputs(TIM1, ENABLE)`，忘了这个 PWM 永远出不来。这是 TIM1 最常见的坑！
> 2. **死区时间用示波器确认**：计算出来的死区可能和实际有偏差（受 MOS 驱动电路影响），最终用示波器看 Q1 和 Q2 的栅极波形，确保有足够的死区且没有交叠。
> 3. **刹车功能是救命用的**：如果你在驱动大功率电机，一定要用刹车功能。把过流检测信号接到 BKIN 引脚（PB12），硬件自动关断比软件检测快 1000 倍。
> 4. **关闭 JTAG 后仍可用 SWD 调试**：`GPIO_Remap_SWJ_JTAGDisable` 只关闭 JTAG，SWD（PA13/PA14）仍然可用。
> 5. **电赛一般用 TB6612 就够了**：TB6612 内部已经有死区控制，不需要外部互补 PWM。只有自己做 H 桥或驱动无刷电机时才需要 TIM1。

---

## 第七章 ADC模拟数字转换器——传感器数据采集核心
ADC是电赛仪器类、控制类题目必用的外设，用来采集电压信号，接电位器、光敏电阻、热敏电阻、压力传感器、电流传感器、加速度计等所有模拟输出的传感器。

### 7.1 ADC基本原理
#### 7.1.1 ADC是什么？
ADC把0~3.3V之间的连续模拟电压转换成数字量，STM32F103的ADC是**12位逐次逼近型ADC**：
- 12位分辨率：转换结果是0~4095之间的整数，2^12=4096
- 换算公式：`实际电压 = ADC值 × 3.3V / 4096`
- 转换速度：最快1us转换一次（1MHz采样率）
- 最多16个外部通道，对应16个GPIO引脚，C8T6最多引出10个通道
- 可以同时接多个传感器，轮流转换

#### 7.1.2 ADC挂在哪个总线？时钟是多少？
- ADC1和ADC2挂在**APB2总线**上，所以开时钟用`RCC_APB2PeriphClockCmd`
- ADC的时钟来自APB2总线时钟（72MHz）分频，ADC最大时钟不能超过14MHz！
- 一般我们6分频，72/6=12MHz，在允许范围内，转换速度最快

> **新手大坑**：ADC时钟配置超过14MHz，会导致转换结果不准，跳变很大。必须配置RCC_PCLK2_Div6，得到12MHz时钟。

#### 7.1.3 ADC的工作模式
STM32的ADC功能非常强大，电赛常用的模式：
1. **单次转换模式**：触发一次转换一次，软件触发，最简单，入门用
2. **连续转换模式**：转换完一次自动开始下一次，一直转
3. **扫描模式**：自动轮流转换多个通道，比如CH0→CH1→CH2→CH0...
4. **DMA模式**：转换结果自动存在内存数组里，不需要CPU干预，多通道采集必用
5. ** injected注入通道**：可以打断常规通道转换，做紧急采集，电赛较少用

### 7.2 ADC基础配置（单通道软件触发）
先从最简单的单通道采集开始，采集PA0（ADC1_CH0）的电压。

#### 7.2.1 ADC通道和引脚对应表
| ADC通道 | 对应引脚 |
| ------- | -------- |
| CH0     | PA0      |
| CH1     | PA1      |
| CH2     | PA2      |
| CH3     | PA3      |
| CH4     | PA4      |
| CH5     | PA5      |
| CH6     | PA6      |
| CH7     | PA7      |
| CH8     | PB0      |
| CH9     | PB1      |

> 记住：ADC通道0就是PA0，通道1是PA1...通道8是PB0，通道9是PB1，这个顺序是固定的。

#### 7.2.2 单通道ADC配置步骤
1. 开时钟：GPIOA（APB2）+ ADC1（APB2）
2. 配置PA0为**模拟输入模式（AIN）**！这是ADC专用模式，上下拉都断开，施密特触发器关闭
3. 配置ADC分频：6分频，12MHz时钟
4. 复位ADC，初始化ADC参数：单次转换模式、不扫描、软件触发、数据右对齐
5. 校准ADC（非常重要！校准后精度会高很多）
6. 使能ADC
7. 封装读取函数

#### 7.2.3 代码实现
```c
void ADC1_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;
    
    // 1. 开时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);
    
    // 2. 配置PA0为模拟输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN; // 模拟输入！
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 3. ADC时钟分频：6分频，12MHz
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);
    
    // 4. ADC复位
    ADC_DeInit(ADC1);
    
    // 5. ADC参数配置
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent; // 独立模式
    ADC_InitStructure.ADC_ScanConvMode = DISABLE; // 不扫描，单通道
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE; // 单次转换
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None; // 软件触发
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right; // 数据右对齐
    ADC_InitStructure.ADC_NbrOfChannel = 1; // 1个通道
    ADC_Init(ADC1, &ADC_InitStructure);
    
    // 6. ADC校准！必须做，否则误差很大
    ADC_Calibration_Vol(ADC1, ADC_CALIBVOL_3_3V); // 标准库校准函数
    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));
    
    // 7. 使能ADC
    ADC_Cmd(ADC1, ENABLE);
    
    // 等待ADC稳定
    Delay_ms(1);
}

// 读取ADC值，0~4095
uint16_t ADC1_GetValue(uint8_t ch) {
    // 设置要转换的通道
    ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_239Cycles5);
    // 采样时间越长越准，239.5周期是最准的，速度慢一点但电赛足够
    
    // 开始转换
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    // 等待转换完成
    while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
    // 返回结果
    return ADC_GetConversionValue(ADC1);
}

// 读取电压，单位mV，多次平均
uint16_t ADC1_GetVoltage(uint8_t ch) {
    uint32_t sum = 0;
    uint8_t i;
    for(i=0; i<10; i++) { // 读10次取平均
        sum += ADC1_GetValue(ch);
        Delay_us(10);
    }
    return (uint16_t)(sum / 10 * 3300 / 4096); // 结果单位mV
}
```

> **ADC使用电赛经验**：
> 1. **必须校准**：每次上电都要校准，否则误差可能有几十mV
> 2. **采样时间设长一点**：如果信号源内阻大，采样时间短会不准，239.5周期是最稳妥的
> 3. **硬件滤波**：在ADC引脚和GND之间接一个0.1uF的电容，滤除高频干扰，结果会稳定很多
> 4. **软件滤波**：多次采样取平均，或者中值滤波，后面算法篇会详细讲
> 5. **参考电压**：如果对精度要求高，不要用VCC做参考，用外部精密参考电压源

### 7.3 ADC多通道DMA采集（电赛标配）
如果要同时采集多个通道，用软件触发一个个读效率太低，而且时间不准。标准做法是用扫描模式+DMA，硬件自动轮流转换所有通道，结果自动放到内存数组里，CPU直接读数组就行。

#### 7.3.1 DMA是什么？
DMA=直接存储器访问，它可以在外设和内存之间直接搬运数据，不需要CPU参与。ADC多通道采集、串口大量数据收发、SPI屏幕刷新，用DMA会极大减轻CPU负担。

#### 7.3.2 多通道DMA配置（采集CH0-CH3共4个通道）
1. 开时钟：GPIOA + ADC1 + DMA1（DMA1挂在AHB总线上，用RCC_AHBPeriphClockCmd开）
2. 配置PA0-PA3为模拟输入
3. 配置DMA通道：ADC1对应DMA1通道1，从ADC数据寄存器搬到内存数组，循环模式
4. 配置ADC为扫描模式、连续转换、软件触发
5. 使能ADC的DMA请求
6. 校准ADC，使能ADC，启动软件触发
7. 之后直接读数组就行，数组里的值会自动更新

#### 7.3.3 代码实现
```c
__IO uint16_t adc_buf[4]; // DMA自动填充的数组，4个通道

void ADC1_DMA_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;
    
    // 1. 开时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE); // DMA在AHB总线
    
    // 2. GPIO配置：PA0-PA3模拟输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 3. ADC时钟分频
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);
    
    // 4. DMA配置
    DMA_DeInit(DMA1_Channel1); // ADC1对应DMA1通道1
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR; // 外设地址：ADC数据寄存器
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)adc_buf; // 内存地址：我们的数组
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC; // 外设到内存
    DMA_InitStructure.DMA_BufferSize = 4; // 4个数据
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; // 外设地址不增加
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; // 内存地址自动增加
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord; // 16位
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular; // 循环模式，转完一圈自动重新开始
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel1, &DMA_InitStructure);
    DMA_Cmd(DMA1_Channel1, ENABLE);
    
    // 5. ADC配置
    ADC_DeInit(ADC1);
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = ENABLE; // 扫描模式
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE; // 连续转换
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 4; // 4个通道
    ADC_Init(ADC1, &ADC_InitStructure);
    
    // 配置通道顺序和采样时间
    ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_239Cycles5);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 2, ADC_SampleTime_239Cycles5);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 3, ADC_SampleTime_239Cycles5);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_3, 4, ADC_SampleTime_239Cycles5);
    
    // 使能ADC DMA
    ADC_DMACmd(ADC1, ENABLE);
    
    // 校准
    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));
    
    // 使能ADC，开始转换
    ADC_Cmd(ADC1, ENABLE);
    Delay_ms(1);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

// 之后你在任何地方读adc_buf[0]就是CH0的值，adc_buf[1]是CH1的值...
// 完全不需要CPU干预，DMA自动更新
```

> **ADC DMA电赛经验**：
> 1. 多通道采集一定要用DMA，不要在中断里读ADC，会占用CPU时间
> 2. DMA循环模式是标配，上电启动一次就不用管了
> 3. adc_buf数组要加volatile或者__IO，防止编译器优化，因为它是DMA自动修改的
> 4. ADC通道顺序就是你配置的顺序，adc_buf数组下标对应通道顺序，不要搞混

---

## 第七章补充：DMA 直接存储器访问 —— 解放 CPU 的数据搬运工

DMA（Direct Memory Access，直接存储器访问）是 STM32 中最强大的外设之一。它的核心使命只有一句话：**在外设和内存之间搬运数据，不经过 CPU**。

在讲具体用法之前，先理解为什么需要 DMA。想象你是 CPU（一个超级忙碌的经理），你要从 ADC 读取 100 个数据存到数组里。没有 DMA 的话：

```
没有 DMA（CPU 自己做）：
  经理亲自走到 ADC 办公室 → 取一份数据 → 走回内存办公室 → 放下数据 → 重复 100 次
  期间经理不能做任何其他工作（PID 计算？等着。串口回复？等着。）

有了 DMA（雇佣了一个搬运工）：
  经理告诉搬运工："从 ADC 办公室搬 100 份数据到内存办公室，循环搬运，别停。"
  然后经理继续做 PID 计算、刷新屏幕、响应按键……
  搬运工默默干活，搬完了通知经理一声。
```

**这就是 DMA 的价值**：把 CPU 从枯燥的数据搬运中解放出来，让 CPU 专注于计算和控制。

---

### 7S.1 DMA 控制器架构 —— 先搞清硬件格局

#### 7S.1.1 为什么叫"DMA 控制器"？

DMA 本身是一个独立的硬件模块，有自己的控制逻辑。它不依赖 CPU 执行指令，而是直接操作总线来传输数据。在 STM32F103C8T6 中：

- **DMA1**：有 7 个通道（Channel 1~7），挂在 **AHB 总线**上
- **DMA2**：C8T6 没有 DMA2（那是大容量芯片才有的）

**关键认知**：DMA 的时钟来自 AHB 总线（72MHz），和 APB1/APB2 不同！开 DMA 时钟必须用 `RCC_AHBPeriphClockCmd`。

#### 7S.1.2 DMA1 的 7 个通道和硬件绑定关系

DMA 的每个通道都有自己"专属"的外设。你**不能随意指定**哪个通道给哪个外设用——这是硬件设计时就定死的。这非常重要，很多新手在这里犯错误。

| DMA1 通道 | 可服务的外设（常用）                                     | 电赛最常用场景      |
| --------- | -------------------------------------------------------- | ------------------- |
| Channel 1 | **ADC1**、TIM2_CH3、TIM4_CH1                             | **ADC 多通道采集**  |
| Channel 2 | **SPI1_RX**、USART3_TX、TIM1_CH1、TIM2_UP、TIM3_CH3      | SPI1 接收           |
| Channel 3 | **SPI1_TX**、USART3_RX、TIM1_CH2、TIM3_CH4、TIM3_UP      | SPI1 发送           |
| Channel 4 | **USART1_TX**、SPI/I2S2_RX、TIM1_CH4、TIM2_CH1、TIM4_CH2 | **串口 1 发送 DMA** |
| Channel 5 | **USART1_RX**、SPI/I2S2_TX、TIM1_UP、TIM2_CH2、TIM3_CH1  | **串口 1 接收 DMA** |
| Channel 6 | **USART2_RX**、TIM1_CH3、TIM3_CH1、TIM3_UP               | 串口 2 接收         |
| Channel 7 | **USART2_TX**、TIM1_CH4、TIM2_CH4、TIM2_UP、TIM3_CH2     | 串口 2 发送         |

> **如何使用这个表？**
> 比如你要用 USART1 的 DMA 发送，查表发现 USART1_TX 在 Channel 4 → 那就配置 `DMA1_Channel4`。
> 比如你要用 ADC1 的 DMA，查表发现 ADC1 只在 Channel 1 → 那就配置 `DMA1_Channel1`。
>
> **为什么会有这种绑定？** 因为 DMA 的"请求信号线"是硬件连接好的。存储器到存储器传输（M2M）不需要外设请求，任何一个通道都可以做。

#### 7S.1.3 DMA 传输的核心概念

每次 DMA 传输要回答三个问题：

```
┌─────────────────────────────────────────────────────────────┐
│                   DMA 传输的三要素                           │
│                                                             │
│   ① 从哪来？  →  外设地址（Peripheral Address）              │
│      例：&ADC1->DR（ADC 数据寄存器的地址 = 0x4001244C）       │
│      例：&USART1->DR（串口 1 数据寄存器的地址 = 0x40013804）   │
│                                                             │
│   ② 到哪去？  →  内存地址（Memory Address）                   │
│      例：adc_buf（你定义的数组名，就是数组首地址）              │
│      例：tx_buf（要发送的数据数组首地址）                      │
│                                                             │
│   ③ 搬多少？  →  传输数量（BufferSize）                       │
│      例：4 个 ADC 通道 → 搬 4 个半字（16位）                   │
│      例：发 100 字节 → 搬 100 次                               │
└─────────────────────────────────────────────────────────────┘
```

此外还有两个方向问题：
- **方向**：外设→内存（`DMA_DIR_PeripheralSRC`，外设是源头）还是 内存→外设（`DMA_DIR_PeripheralDST`，外设是目标）
- **循环**：搬完一轮后自动重新开始（`DMA_Mode_Circular`，ADC 用），还是只搬一轮就停（`DMA_Mode_Normal`，串口发送用）

#### 7S.1.4 DMA 初始化结构体逐个成员拆解

标准库中 DMA 的初始化结构体是 `DMA_InitTypeDef`，有 8 个成员。每个都要理解，否则配置错了 DMA 不会工作。

```c
typedef struct
{
    uint32_t DMA_PeripheralBaseAddr;  // [1] 外设基地址
    uint32_t DMA_MemoryBaseAddr;      // [2] 内存基地址
    uint32_t DMA_DIR;                 // [3] 传输方向
    uint32_t DMA_BufferSize;          // [4] 缓冲区大小（传输次数）
    uint32_t DMA_PeripheralInc;       // [5] 外设地址是否自动增加
    uint32_t DMA_MemoryInc;           // [6] 内存地址是否自动增加
    uint32_t DMA_PeripheralDataSize;  // [7] 外设数据宽度（8/16/32位）
    uint32_t DMA_MemoryDataSize;      // [8] 内存数据宽度（8/16/32位）
    uint32_t DMA_Mode;                // [9] 循环模式还是单次模式
    uint32_t DMA_Priority;            // [10] DMA 通道优先级
    uint32_t DMA_M2M;                 // [11] 是否存储器到存储器模式
} DMA_InitTypeDef;
```

**逐个成员详解**：

| 成员                     | 含义                                                                             | 常见设置                                                          |
| ------------------------ | -------------------------------------------------------------------------------- | ----------------------------------------------------------------- |
| `DMA_PeripheralBaseAddr` | 外设寄存器的**绝对地址**。注意：必须用 `(uint32_t)&外设->DR` 强转                | ADC: `(uint32_t)&ADC1->DR`<br>USART: `(uint32_t)&USART1->DR`      |
| `DMA_MemoryBaseAddr`     | 内存数组的**首地址**。直接传数组名即可                                           | `(uint32_t)adc_buf` 或 `(uint32_t)tx_buf`                         |
| `DMA_DIR`                | 方向：`PeripheralSRC`=外设到内存（读外设），`PeripheralDST`=内存到外设（写外设） | ADC: `DMA_DIR_PeripheralSRC`<br>USART_TX: `DMA_DIR_PeripheralDST` |
| `DMA_BufferSize`         | 传输**次数**（不是字节数！）。每次传输的数据宽度由下面两个成员决定               | ADC 4通道: 4<br>USART 发100字节: 100                              |
| `DMA_PeripheralInc`      | 外设地址是否自增。**几乎永远是 Disable**（因为外设寄存器地址是固定的）           | `DMA_PeripheralInc_Disable`                                       |
| `DMA_MemoryInc`          | 内存地址是否自增。搬数据到数组需要自增（存到下一个位置）                         | 数组: `DMA_MemoryInc_Enable`<br>单个变量: `Disable`               |
| `DMA_PeripheralDataSize` | 外设每次传输的数据宽度。要和**外设数据寄存器**的宽度匹配                         | ADC 是 12 位→用 `HalfWord`(16位)<br>USART DR 是 8 位→用 `Byte`    |
| `DMA_MemoryDataSize`     | 内存每次传输的数据宽度。要和**数组元素类型**的宽度匹配                           | `uint16_t`数组→`HalfWord`<br>`uint8_t`数组→`Byte`                 |
| `DMA_Mode`               | 循环还是单次。`Circular`=搬完自动从头开始（ADC用），`Normal`=搬完停止            | ADC: `DMA_Mode_Circular`<br>USART发: `DMA_Mode_Normal`            |
| `DMA_Priority`           | 多个 DMA 通道同时请求时，谁的优先级高。电赛大部分场景不冲突                      | `DMA_Priority_High` 或 `Medium`                                   |
| `DMA_M2M`                | 存储器到存储器模式。不涉及外设，纯内存间搬运。**此时不需要外设请求**             | 内存拷贝: `DMA_M2M_Enable`<br>外设传输: `DMA_M2M_Disable`         |

> **最容易出错的三个设置**：
> 1. **`DMA_PeripheralDataSize` 和外设寄存器宽度不匹配**：ADC DR 是 16 位，设成 Byte 就错了；USART DR 是 8 位（实际只用低 8 位），设成 HalfWord 会导致每 2 字节才发一个。
> 2. **`DMA_BufferSize` 理解错**：它是传输**次数**，不是字节数。传输 100 个 uint16_t，BufferSize=100，不是 200。
> 3. **`DMA_DIR` 搞反了**：ADC 是外设产生数据→存到内存，方向是 PeripheralSRC（外设是源头）。USART 发送是内存数据→写到外设，方向是 PeripheralDST（外设是目标）。

---

### 7S.2 DMA 实战一：ADC 多通道扫描 + DMA（复习与深化）

前面第七章已经给出了 ADC DMA 的代码，这里不再重复完整代码，而是**从 DMA 的角度重新解读每一行**，让你彻底理解 DMA 在干什么。

```c
// ===== ADC DMA 配置的关键行解读 =====

// DMA_PeripheralBaseAddr：ADC 数据寄存器的地址
// ADC1->DR 是 32 位寄存器，但只用低 16 位存转换结果
// &ADC1->DR 取这个寄存器的地址，强制转换为 uint32_t
DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
// 为什么不需要自增？因为 ADC1->DR 只有一个地址，每次转换结果都覆盖写入这个地址
// DMA 做的事：每次 ADC 转换完成 → 自动从 DR 地址读 16 位 → 存到内存数组

// DMA_MemoryBaseAddr：存放数据的数组首地址
// adc_buf 是 uint16_t adc_buf[4]，编译器自动把数组名转换为首元素地址
DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)adc_buf;
// 内存地址需要自增：第1次存 adc_buf[0]，第2次存 adc_buf[1]...

// DMA_DIR：外设是数据来源
DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
// PeripheralSRC = 外设是源头（Source），数据从外设流向内存

// DMA_BufferSize = 4：共传输 4 次
// 因为 4 个 ADC 通道，每轮扫描各转换一次，共 4 个结果
DMA_InitStructure.DMA_BufferSize = 4;

// 数据宽度都是 HalfWord（16 位）
// ADC 分辨率是 12 位，存在 16 位的寄存器位置
DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;

// 循环模式：扫描完 CH0→CH1→CH2→CH3 后，自动回到 CH0 重新开始
DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
// 这就是"上电配一次，永远不用管"的关键！
// DMA 会持续循环：不断把新的 ADC 转换结果覆盖写入 adc_buf[0..3]

// M2M = Disable：这不是内存到内存，而是外设到内存
DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;

// 使能 DMA 通道 1（ADC1 专属通道）
DMA_Cmd(DMA1_Channel1, ENABLE);

// 最后在 ADC 配置中打开 ADC 的 DMA 请求
ADC_DMACmd(ADC1, ENABLE);
// 这一行告诉 ADC："每次你转换完一个通道，就向 DMA 发一个请求信号"
// DMA 收到请求 → 自动执行一次传输 → 等待下一次请求
```

**ADC + DMA 的数据流全景图**：

```
ADC 硬件连续扫描 CH0, CH1, CH2, CH3:
  转换 CH0 完成 → ADC 发请求 → DMA 把 DR 的 16 位搬到 adc_buf[0]
  转换 CH1 完成 → ADC 发请求 → DMA 把 DR 的 16 位搬到 adc_buf[1]
  转换 CH2 完成 → ADC 发请求 → DMA 把 DR 的 16 位搬到 adc_buf[2]
  转换 CH3 完成 → ADC 发请求 → DMA 把 DR 的 16 位搬到 adc_buf[3]
  (循环模式) 回到 CH0 → 覆盖 adc_buf[0] → ...永远重复

全过程 CPU 完全不知情！你的主循环随时读 adc_buf[0] 就是最新的 CH0 值。
```

---

### 7S.3 DMA 实战二：USART 串口 DMA 发送 —— 大量数据不卡 CPU

#### 7S.3.1 为什么串口发送要用 DMA？

标准串口发送是阻塞式的：`while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);`——CPU 一直死等，每发一个字节就等一次。

如果你要发 100 字节的数据包，CPU 要等 100 次 `TXE` 标志，大约 100 × (1/115200×10) ≈ 8.7ms。这 8.7ms CPU 什么都不能做。对电机控制的 10ms PID 周期来说，8.7ms 的延迟是致命的。

**用 DMA 发送**：CPU 告诉 DMA "把这 100 字节发完"，然后立刻返回做 PID 计算。DMA 在后台逐字节搬运，整个过程 CPU 完全自由。

#### 7S.3.2 USART1 DMA 发送完整代码（逐行详解）

```c
/**
 * @brief  配置 USART1 的 DMA 发送（TX）
 * @note   USART1_TX 对应 DMA1 Channel 4（查 7S.1.2 的表格！）
 *         数据方向：内存 → 外设（PeripheralDST）
 *         单次模式：发完一轮就停
 *         
 *         使用前提：USART1 已经完成基本初始化（波特率、引脚等）
 */
void USART1_DMA_TX_Init(void)
{
    DMA_InitTypeDef DMA_InitStructure;
    
    // ===== 第1步：开 DMA 时钟 =====
    // DMA 挂在 AHB 总线上！不是 APB1 也不是 APB2！
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    // 内部：RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    // 不开时钟，DMA 控制器完全不工作
    
    // ===== 第2步：配置 DMA Channel 4（USART1_TX 专用）=====
    DMA_DeInit(DMA1_Channel4);  // 复位通道 4 到默认状态
    // DeInit 是良好的编程习惯，防止之前的残留配置干扰
    
    // ----- 外设地址：USART1 数据寄存器 -----
    // USART1->DR 的地址，数据从这里发送出去
    // 每往 DR 写一个字节，USART 硬件就自动把这个字节串行移位输出
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&USART1->DR);
    // 注意：这里是 PeripheralDST（外设是目标），方向后面设
    
    // ----- 内存地址：待发送数据的数组 -----
    // 实际使用时，先填好数组内容，然后启动 DMA
    // 数组名 = 数组首地址，编译器自动处理
    // DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)tx_buffer;
    // 这里先不设具体地址，启动 DMA 前用 DMA_SetCurrDataCounter 重新配置
    // 为简化，初始化时设一个默认地址
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)0;  // 占位，使用时设
    
    // ----- 传输方向：内存 → 外设（发送！）-----
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    // PeripheralDST = 外设是目标（Destination）
    // 数据从 Memory（数组）→ Peripheral（USART->DR）
    // 和 ADC 的方向正相反！ADC 是 PeripheralSRC（外设是源头）
    
    // ----- 外设地址不自增 -----
    // USART->DR 是固定地址，每次发送都写到同一个寄存器
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    
    // ----- 内存地址自增 -----
    // 数组要逐个元素发送：tx_buf[0] → tx_buf[1] → tx_buf[2] → ...
    // 所以内存地址每次 +1 字节
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    
    // ----- 数据宽度：都设为字节（8位）-----
    // 串口数据寄存器 DR 虽然物理上是 32 位寄存器，但串口发送只用低 8 位
    // 设 Byte(8位) 确保每个字节独立传输
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    // 重要！外设和内存的数据宽度必须一致！
    
    // ----- 单次模式（不是循环！）-----
    // 串口发送是"一次性"的：发完 100 字节就停止
    // 和 ADC 的循环模式不同（ADC 需要持续采集）
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    // Normal：传输完 BufferSize 次后自动停止
    // Circular：传输完一轮后自动重新开始（ADC 用）
    
    // ----- 优先级：中 -----
    DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
    
    // ----- 不是内存到内存 -----
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    
    // 写入配置到硬件寄存器
    DMA_Init(DMA1_Channel4, &DMA_InitStructure);
    
    // ===== 第3步：使能 USART1 的 DMA 发送请求 =====
    // 告诉 USART："当 TXE（发送缓冲区空）时，向 DMA 发请求信号"
    USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);
    // 参数 USART_DMAReq_Tx：使能发送 DMA 请求
    // USART_DMAReq_Rx：使能接收 DMA 请求
    // 两个可以独立使能
    
    // 注意：此时 DMA 通道还没有启动！
    // 启动 DMA 在具体发送函数中调用 DMA_Cmd
}

/**
 * @brief  使用 DMA 发送一串数据
 * @param  buf: 待发送数据的首地址
 * @param  len: 待发送数据的字节数
 * @note   这个函数是非阻塞的！调用后立刻返回，DMA 在后台发送
 *         
 *         调用前确保上一次 DMA 发送已经完成！
 *         可以通过检查 DMA_GetFlagStatus 来判断
 */
void USART1_DMA_SendData(uint8_t* buf, uint16_t len)
{
    // ===== 第1步：等待上一次发送完成 =====
    // 检查 DMA1 Channel 4 的传输完成标志（TC = Transfer Complete）
    // 如果上一次发送还没完成，就等待（实际应用中可以轮询或返回忙状态）
    while(DMA_GetFlagStatus(DMA1_FLAG_TC4) == RESET);
    // DMA1_FLAG_TC4：Channel 4 传输完成标志
    // DMA1_FLAG_TC1~7：对应通道 1~7
    
    // 清除传输完成标志
    DMA_ClearFlag(DMA1_FLAG_TC4);
    // 不清除的话，下一次检查会认为已经完成了
    
    // ===== 第2步：停止当前 DMA 通道 =====
    // 必须先停，才能修改配置
    DMA_Cmd(DMA1_Channel4, DISABLE);
    
    // ===== 第3步：重新设置传输次数和内存地址 =====
    // DMA_SetCurrDataCounter：设置本次要传输多少次（字节数）
    DMA_SetCurrDataCounter(DMA1_Channel4, len);
    // 内部：DMA1_Channel4->CNDTR = len;
    // CNDTR = Channel Number of Data to Transfer
    // 每传输一次，CNDTR 自动减 1，减到 0 时传输完成
    
    // DMA_MemoryBaseAddr：设置源数据地址
    DMA1_Channel4->CMAR = (uint32_t)buf;
    // CMAR = Channel Memory Address Register
    // 直接写寄存器比调函数更快
    
    // ===== 第4步：启动 DMA =====
    DMA_Cmd(DMA1_Channel4, ENABLE);
    // 启动后，USART 每发完一个字节（TXE=1），就向 DMA 请求下一个字节
    // DMA 自动从 buf 取 → 写 USART->DR → 内存地址+1 → CNDTR-1
    // 直到 CNDTR 减到 0，DMA 自动停止
    
    // CPU 执行到这里立刻返回，不会等待发送完成！
    // 发送过程完全由 DMA 硬件完成
}

/**
 * @brief  检查 DMA 发送是否完成
 * @return 0=还在发送, 1=发送完成
 */
uint8_t USART1_DMA_TX_Done(void)
{
    // 检查传输完成标志
    if(DMA_GetFlagStatus(DMA1_FLAG_TC4) != RESET)
    {
        DMA_ClearFlag(DMA1_FLAG_TC4);  // 清除标志
        return 1;  // 发送完成
    }
    return 0;  // 还在发送
}

// ===== 使用示例 =====
uint8_t tx_packet[100];  // 100 字节的数据包

int main(void)
{
    USART1_Init(115200);       // 先初始化串口基本功能
    USART1_DMA_TX_Init();      // 再初始化 DMA 发送
    
    while(1)
    {
        // ... 填充 tx_packet ...
        
        // 启动 DMA 发送（非阻塞，立刻返回）
        USART1_DMA_SendData(tx_packet, 100);
        
        // CPU 可以继续做其他事！
        PID_Compute();          // 同时计算 PID
        Sensor_Read();          // 同时读传感器
        OLED_Refresh();         // 同时刷新屏幕
        
        // 需要确认发送完成时再检查
        // if(USART1_DMA_TX_Done()) { /* 发送完成，可以准备下一包 */ }
    }
}
```

#### 7S.3.3 理解 USART DMA 发送的硬件握手过程

这是 DMA 最核心的工作机制，看懂了这个你就真正理解了 DMA：

```
硬件握手流程（USART TX DMA）：

1. 初始状态：USART 发送缓冲区空（TXE=1），DMA 通道 4 使能
2. USART 硬件检测到 TXE=1 → 向 DMA 发出"请求信号"
3. DMA 收到请求 → 从 CMAR 指向的内存读 1 字节 → 写 USART->DR → CMAR+1 → CNDTR-1
4. USART 开始串行发送 DR 中的字节，TXE 变为 0
5. USART 发送完毕，TXE 恢复为 1 → 再次向 DMA 发请求
6. 重复步骤 2~5，直到 CNDTR 减到 0
7. CNDTR=0 → DMA 自动停止 → TC 标志置 1

关键：CPU 全程不参与步骤 2~6！
```

---

### 7S.4 DMA 实战三：USART 串口 DMA 接收 —— 不定长数据接收方案

DMA 接收比发送稍微复杂一点，因为接收数据的长度往往是不确定的（你不知道对方会发多少字节）。

#### 7S.4.1 方案一：定长接收（最简单）

如果数据包长度固定（比如遥控器每次都发 8 字节指令），直接用循环模式接收：

```c
/**
 * @brief  USART1 DMA 接收初始化（定长模式）
 * @note   USART1_RX 对应 DMA1 Channel 5
 *         循环模式：持续接收，填满后自动从头覆盖
 */
uint8_t rx_dma_buf[8];  // 固定 8 字节接收缓冲区

void USART1_DMA_RX_Init(void)
{
    DMA_InitTypeDef DMA_InitStructure;
    
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    
    DMA_DeInit(DMA1_Channel5);  // USART1_RX = Channel 5！
    
    // ----- 外设地址 = USART1->DR -----
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&USART1->DR);
    
    // ----- 内存地址 = 接收缓冲区 -----
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)rx_dma_buf;
    
    // ----- 方向：外设 → 内存（接收！）-----
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    // 和 ADC 一样的方向！因为都是"外设产生数据，存到内存"
    
    // ----- 外设地址不增，内存地址自增 -----
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    
    // ----- 数据宽度：字节 -----
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    
    // ----- 循环模式：持续接收 -----
    DMA_InitStructure.DMA_BufferSize = 8;  // 缓冲区 8 字节
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    
    DMA_Init(DMA1_Channel5, &DMA_InitStructure);
    
    // 使能 USART 的 DMA 接收请求
    USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);
    
    // 启动 DMA 通道
    DMA_Cmd(DMA1_Channel5, ENABLE);
}

// 读取最新收到的数据
// DMA 持续循环填充 rx_dma_buf，你随时读它就是最新的 8 字节
```

#### 7S.4.2 方案二：不定长接收 + 空闲中断（最实用！）

电赛中最实用的是**不定长接收**：用 DMA 循环接收 + 串口空闲中断（IDLE）来判断一帧数据是否收完。

**原理**：串口收到一个字节后，如果在一个字节的时间内没有收到下一个字节，就会触发 IDLE（空闲）中断。利用这个特性，可以在 IDLE 中断中计算收到了多少字节。

```c
/**
 * @brief  不定长 DMA 接收 + 空闲中断方案
 * @note   原理：
 *         1. DMA 循环模式持续填充接收缓冲区
 *         2. 串口收到数据 → DMA 自动搬运 → CNDTR 自动减
 *         3. 串口连续收完一帧后进入空闲 → 触发 IDLE 中断
 *         4. 在 IDLE 中断中：已接收字节数 = 缓冲区大小 - CNDTR
 *         5. 处理数据，重置 DMA
 */
uint8_t rx_buffer[256];  // 256 字节循环缓冲区

void USART1_DMA_RX_Idle_Init(void)
{
    DMA_InitTypeDef DMA_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    
    // ===== DMA 配置（和定长接收基本相同）=====
    DMA_DeInit(DMA1_Channel5);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&USART1->DR);
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)rx_buffer;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_BufferSize = 256;       // 缓冲区 256 字节
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular; // 循环模式
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel5, &DMA_InitStructure);
    
    USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);
    DMA_Cmd(DMA1_Channel5, ENABLE);
    
    // ===== 配置 USART1 空闲中断（IDLE）=====
    USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);
    // USART_IT_IDLE：串口空闲中断
    // 当 RX 线在接收完一个字节后，持续一个字节时间内没有新的起始位，触发
    
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}
```

**关键：IDLE 中断服务函数中计算接收长度**：

```c
volatile uint8_t  rx_complete_flag = 0;  // 一帧数据接收完成标志
volatile uint16_t rx_data_length = 0;    // 本帧数据的长度

void USART1_IRQHandler(void)
{
    // ===== 检查 IDLE 中断（空闲中断）=====
    if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)
    {
        // ---- 核心：计算接收长度 ----
        // CNDTR 是 DMA 还剩多少次要传输
        // 初始 CNDTR = 256（缓冲区总大小）
        // 每收到一个字节，DMA 自动 CNDTR-1
        // 所以：已接收字节数 = 256 - CNDTR
        rx_data_length = 256 - DMA_GetCurrDataCounter(DMA1_Channel5);
        // DMA_GetCurrDataCounter 返回 DMA1_Channel5->CNDTR 的当前值
        // 例如：收到了 50 字节，CNDTR=206，256-206=50 ✓
        
        rx_complete_flag = 1;  // 通知主循环：一帧数据已就绪
        
        // ---- 重要！必须先读 SR 再读 DR 才能清除 IDLE 标志！----
        // IDLE 标志的清除比较特殊：
        // 必须"先读 USART_SR，再读 USART_DR"才能清除
        // 这是因为 IDLE 不是普通的中断标志，而是和 RXNE 共用硬件逻辑
        volatile uint32_t tmp = USART1->SR;  // 读 SR（清除 IDLE 的步骤1）
        tmp = USART1->DR;                     // 读 DR（清除 IDLE 的步骤2）
        (void)tmp;                            // 防止编译器警告未使用变量
        
        // ---- 重置 DMA（准备接收下一帧）----
        DMA_Cmd(DMA1_Channel5, DISABLE);             // 先停 DMA
        DMA_SetCurrDataCounter(DMA1_Channel5, 256);   // 重置传输计数
        DMA_Cmd(DMA1_Channel5, ENABLE);              // 重新启动
        // 重置后 CNDTR 恢复为 256，可以从头开始接收下一帧
    }
    
    // ===== 也保留 RXNE 中断（如果需要逐字节处理）=====
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        // 如果同时用了 DMA 接收，通常 RXNE 不需要额外处理
        // DMA 会自动从 DR 搬走数据
        // 但清标志还是要的：
        // USART_ClearITPendingBit(USART1, USART_IT_RXNE);
        // 实际上：如果用 DMA 接收，根本不需要使能 RXNE 中断
    }
}

// ===== 主循环中使用 =====
void Process_ReceivedData(void)
{
    if(rx_complete_flag)
    {
        rx_complete_flag = 0;
        
        // rx_buffer[0..rx_data_length-1] 就是刚收到的一帧数据
        // 比如收到 "AT+START\r\n"，rx_data_length = 10
        
        // 根据协议解析
        if(rx_data_length >= 4 && memcmp(rx_buffer, "CMD:", 4) == 0)
        {
            // 处理命令...
        }
        
        // 注意：DMA 已经重置，下一帧数据会从 rx_buffer[0] 开始覆盖
        // 所以处理要在下一帧到来之前完成
    }
}
```

> **IDLE 中断 + DMA 接收是最优雅的串口接收方案**：
> - 不占用 CPU 时间（DMA 自动收）
> - 不需要逐字节判断帧结束（IDLE 自动检测空闲）
> - 不丢数据（DMA 硬件搬运，不会因为中断响应延迟而丢字节）
> - 电赛一等奖级别的代码必备这个方案

---

### 7S.5 DMA 实战四：存储器到存储器（M2M）传输

DMA 还有一个强大但常被忽略的功能：**内存到内存的数据搬运**。不需要外设参与，纯内存拷贝。

什么时候用？比如你需要快速拷贝大量数据（图像帧、FFT 输入数据、波形表），用 DMA 比 `memcpy` 快很多，而且不阻塞 CPU。

```c
/**
 * @brief  DMA 内存到内存拷贝
 * @param  dst: 目标地址
 * @param  src: 源地址
 * @param  size: 拷贝字节数（必须是偶数，因为用 HalfWord 传输）
 * @note   M2M 模式任意 DMA 通道都可以用（没有通道绑定）
 *         这里使用 DMA1_Channel1（如果 ADC 没用的话）
 *         如果用 ADC DMA，可以换成 DMA1_Channel6 或 7
 */
void DMA_MemCopy(uint32_t dst, uint32_t src, uint16_t size)
{
    DMA_InitTypeDef DMA_InitStructure;
    
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    
    DMA_DeInit(DMA1_Channel6);  // 用空闲的 Channel 6
    
    // ----- 源地址和目的地址 -----
    DMA_InitStructure.DMA_PeripheralBaseAddr = src;   // 源
    DMA_InitStructure.DMA_MemoryBaseAddr = dst;        // 目的
    // 在 M2M 模式中，"外设"和"内存"只是命名上的区别
    // 实际上两个都是内存地址
    
    // ----- 方向：外设→内存（即 src→dst）-----
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    
    // ----- 两个地址都自增 -----
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Enable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    
    // ----- 数据宽度：半字（16位）效率最高 -----
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    
    // ----- 单次模式 -----
    DMA_InitStructure.DMA_BufferSize = size / 2;  // 半字个数
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    
    // ----- 关键！M2M = Enable！-----
    DMA_InitStructure.DMA_M2M = DMA_M2M_Enable;
    // M2M 模式下，DMA 不需要任何外设请求信号
    // 启动后自动连续搬运，直到 CNDTR=0
    
    DMA_Init(DMA1_Channel6, &DMA_InitStructure);
    
    // 启动 DMA（M2M 模式下启动后立即开始传输）
    DMA_Cmd(DMA1_Channel6, ENABLE);
    
    // 等待完成
    while(DMA_GetFlagStatus(DMA1_FLAG_TC6) == RESET);
    DMA_ClearFlag(DMA1_FLAG_TC6);
    
    // 关闭 DMA
    DMA_Cmd(DMA1_Channel6, DISABLE);
}
```

> **M2M 模式和普通模式的本质区别**：
> - 普通模式：DMA 等待外设说"我准备好了"（请求信号），每次只传输一次
> - M2M 模式：DMA 不需要等任何信号，启动后**全速连续搬运**直到搬完
> - M2M 传输速度极快（72MHz 下每次传输只需几个时钟周期），比 CPU 的 `memcpy` 快 2~3 倍

---

### 7S.6 DMA 常见坑与调试

| 坑                              | 现象           | 原因和解决                                                                     |
| ------------------------------- | -------------- | ------------------------------------------------------------------------------ |
| 忘记开 AHB 时钟                 | DMA 完全不工作 | 必须 `RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE)`                       |
| 通道搞错                        | 数据不搬运     | USART1_TX=Ch4, USART1_RX=Ch5, ADC1=Ch1。查 7S.1.2 表格                         |
| `DMA_DIR` 方向反了              | 数据搬反了     | 接收（读外设）=PeripheralSRC，发送（写外设）=PeripheralDST                     |
| `DMA_PeripheralDataSize` 不匹配 | 数据错位       | ADC DR 是 16 位 → HalfWord；USART DR 是 8 位 → Byte                            |
| `DMA_BufferSize` 设为 0         | 传输不启动     | BufferSize 为 0 意味着传输次数为 0，DMA 直接停止                               |
| 忘记使能外设的 DMA 请求         | DMA 不触发     | ADC: `ADC_DMACmd(ENABLE)`<br>USART: `USART_DMACmd(x, USART_DMAReq_Tx, ENABLE)` |
| 循环模式下修改 buf 没加 `__IO`  | 数据不更新     | DMA 在后台写数组，编译器可能把数组缓存在寄存器里不读内存                       |
| IDLE 中断中忘记清标志的特殊步骤 | IDLE 一直触发  | 必须先读 USART_SR，再读 USART_DR 才能彻底清 IDLE                               |
| M2M 模式忘记设 `DMA_M2M_Enable` | DMA 不自动搬运 | M2M=Disable 时 DMA 等待外设请求信号，但根本没有外设，所以不动                  |

---

### 7S.7 DMA 学习自检

学完本节，你应该能回答：
1. DMA1 有多少个通道？USART1_TX 和 ADC1 各用哪个通道？
2. `DMA_DIR_PeripheralSRC` 和 `DMA_DIR_PeripheralDST` 分别用于什么场景？
3. `DMA_Mode_Circular` 和 `DMA_Mode_Normal` 的区别？ADC 应该用哪个？
4. `DMA_BufferSize` 是字节数还是传输次数？
5. 为什么 DMA 发送串口数据时，`DMA_PeripheralDataSize` 要设为 Byte 而不是 HalfWord？
6. IDLE 中断 + DMA 循环接收中，如何计算接收到多少字节？
7. M2M 模式和外设模式的区别是什么？为什么 M2M 不需要指定通道？

任何答不上来的，回到对应小节重新学习。

---

## 第八章 USART串口通信——调试与模块通信必备
串口是你调试程序、和蓝牙/WiFi/GPS等模块通信的必备接口，是单片机和外界通信最常用的方式。

### 8.1 串口基本原理
#### 8.1.1 串口通信协议
串口是异步串行通信，只需要两根线：
- TX：发送端，接对方的RX
- RX：接收端，接对方的TX
- 必须共地！

通信参数：
- 波特率：双方必须一致，常用9600、115200
- 数据位：一般8位
- 停止位：一般1位
- 校验位：一般无校验

串口发送一个字节的格式：1个起始位（低电平）+ 8个数据位 + 1个停止位（高电平），低位在前，高位在后。

#### 8.1.2 STM32的USART
STM32F103有3个USART：
- USART1：挂在**APB2总线**，高速，引脚是PA9(TX)/PA10(RX)，最常用
- USART2：挂在**APB1总线**，引脚是PA2(TX)/PA3(RX)
- USART3：挂在**APB1总线**，引脚是PB10(TX)/PB11(RX)

> **时钟对应关系**：USART1在APB2，时钟72MHz；USART2/3在APB1，时钟36MHz。开时钟的时候不要开错总线！

串口时钟和波特率的关系：
- 波特率 = 串口时钟 / (16 * USARTDIV)
- 标准库会自动帮你算USARTDIV的值，你只要告诉它波特率就行

### 8.2 串口基础配置（printf重定向）
最常用的功能：串口打印调试信息，把printf重定向到串口，就可以像C语言控制台一样打印变量、调试程序。

#### 8.2.1 串口配置步骤（USART1，115200波特率）
1. 开时钟：GPIOA（APB2）+ USART1（APB2）+ AFIO
2. 配置PA9为**复用推挽输出**（TX），PA10为**浮空输入**（RX）
3. 配置USART参数：波特率115200、8位数据、1停止位、无校验、无硬件流控
4. 配置NVIC，开启接收中断（如果需要接收数据）
5. 使能串口
6. 重定向fputc函数，让printf输出到串口

#### 8.2.2 代码实现
```c
void USART1_Init(uint32_t baudrate) {
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    // 1. 开时钟：USART1在APB2！
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);
    
    // 2. GPIO配置
    // PA9 TX：复用推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    // PA10 RX：浮空输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 3. USART配置
    USART_InitStructure.USART_BaudRate = baudrate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx; // 收发都使能
    USART_Init(USART1, &USART_InitStructure);
    
    // 4. 配置接收中断
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE); // 接收缓冲区非空中断
    
    // 5. 使能串口
    USART_Cmd(USART1, ENABLE);
}

// 发送一个字节
void USART1_SendByte(uint8_t dat) {
    USART_SendData(USART1, dat);
    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET); // 等待发送完成
}

// 发送字符串
void USART1_SendString(char* str) {
    while(*str) {
        USART1_SendByte(*str++);
    }
}

// 重定向fputc，支持printf
int fputc(int ch, FILE* f) {
    USART1_SendByte((uint8_t)ch);
    return ch;
}

// 串口1中断服务函数，接收数据
uint8_t usart_rx_buf[256];
uint8_t usart_rx_cnt = 0;
volatile uint8_t usart_rx_flag = 0;
void USART1_IRQHandler(void) {
    uint8_t ch;
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        ch = USART_ReceiveData(USART1);
        if(usart_rx_cnt < 255) {
            usart_rx_buf[usart_rx_cnt++] = ch;
        }
        // 可以在这里加协议判断，比如收到换行符认为一帧结束
        if(ch == '\n') {
            usart_rx_flag = 1;
            usart_rx_buf[usart_rx_cnt-1] = '\0';
            usart_rx_cnt = 0;
        }
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}
```

> **串口使用电赛经验**：
> 1. **printf是调试神器**：程序出问题了，在关键位置printf变量的值，比仿真器还好用
> 2. **波特率不要太高**：115200足够用，太高了容易丢数据，长距离传输用9600
> 3. **必须共地**：两个设备串口通信，GND必须连在一起，否则会乱码
> 4. **交叉连接**：TX接RX，RX接TX，很多新手接反了收不到数据
> 5. **CH340模块**：电脑上看串口信息需要USB转TTL模块，接TX/RX/GND三根线就行，不要接3.3V/5V给STM32供电，容易烧
> 6. **不要在中断里发大量数据**：接收中断里收数据存缓冲区就好，处理放在主循环

---

## 第九章 I2C和SPI通信总线——接传感器和屏幕

I2C 和 SPI 是两种最常用的板级通信总线，用来接 OLED 屏幕、MPU6050 陀螺仪、加速度计、气压计、Flash 存储芯片等。**电赛几乎所有传感器模块都通过 I2C 或 SPI 和 STM32 通信**，所以这一章你必须彻底搞懂。

---

### 9.1 I2C 总线 —— 深入原理与完整驱动

#### 9.1.1 I2C 协议基础（彻底搞懂）

I2C（Inter-Integrated Circuit，集成电路间总线）是 Philips 公司（现 NXP）发明的两线制串行总线。理解下面这些基本概念是你阅读任何 I2C 芯片数据手册的前提。

**物理层**：
- **SCL（Serial Clock）**：时钟线，**始终由主机产生**。从机只是根据这个时钟来决定什么时候读数据、什么时候发数据。
- **SDA（Serial Data）**：数据线，**双向**。主机和从机都可以在这根线上发送数据，但不能同时发。
- **两根线都必须接上拉电阻**（典型值 4.7kΩ，接 3.3V）。为什么？因为 I2C 设备都是**开漏输出**——只能拉低，不能主动拉高。上拉电阻在没人拉低的时候把线"自然拉高"。

> **为什么要开漏输出？**
> 如果 SDA 用推挽输出，主机想发 1（3.3V），从机同时想发 0（0V），两个引脚一个拼命往上推、一个拼命往下拉，结果就是短路烧芯片！开漏输出的设计保证了**任何设备都只能拉低不能拉高**，永远不会出现短路。这是 I2C 协议最精妙的设计。

**通信方式**：
- **半双工**：同一时刻只能有一个方向在传数据（你不能同时说话和听别人说话）
- **主机控制时钟**：SCL 始终由主机产生。从机如果反应慢，可以把 SCL 拉低（这叫"时钟拉伸"），让主机等一等
- **每个从机有唯一地址**：7 位地址（也有 10 位的，但电赛用的大部分是 7 位），主机通过发送地址来选择跟谁说话

**I2C 时序详解——这才是灵魂！**

I2C 通信的所有操作都是通过对 SCL 和 SDA 两根线的电平操作完成的。下面是五种基本时序操作，每个都必须刻在脑子里：

```
1. 起始信号（Start）：SCL高电平时，SDA从高变低
   SCL: ──────┐    ┌──────────
              └────┘
   SDA: ──────────┐    ┌──────
                  └────┘
         ↑ SDA在SCL为高时下降 = 起始信号

2. 停止信号（Stop）：SCL高电平时，SDA从低变高
   SCL: ──────┐    ┌──────────
              └────┘
   SDA: ──┐    ┌──────────────
          └────┘
               ↑ SDA在SCL为高时上升 = 停止信号

3. 发送数据位：在SCL低电平时改变SDA，在SCL高电平时接收方读取SDA
   SCL: ──┐  ┌──┐  ┌──
          └──┘  └──┘
   SDA: ─────X──────────  ← SCL低时改数据，SCL高时稳定
              ↑ SCL上升沿，接收方采样SDA

4. ACK（应答）：第9个时钟脉冲，接收方拉低SDA表示"我收到了"
   SCL: ──┐  ┌──  (第9个时钟)
          └──┘
   SDA: ──────┐  ┌──   ← 接收方拉低=ACK
              └──┘
   
5. NACK（不应答）：第9个时钟脉冲，接收方不拉低SDA（SDA保持高）
   SCL: ──┐  ┌──  (第9个时钟)
          └──┘
   SDA: ────────────   ← 接收方不拉低=NACK
```

**一次完整的 I2C 通信过程**：

```
主机写数据到从机（比如向OLED发命令）：
Start → 从机地址(7位)+写位(0) → ACK → 寄存器地址 → ACK → 数据1 → ACK → ... → Stop

主机从从机读数据（比如读MPU6050加速度值）：
Start → 从机地址(7位)+写位(0) → ACK → 寄存器地址 → ACK → 
Re-Start → 从机地址(7位)+读位(1) → ACK → 读数据1 → ACK → 读数据2 → NACK → Stop
```

> **关键理解**：I2C 的地址字节是 8 位（7 位地址 + 1 位读写方向），最低位 R/W=0 表示主机要写，R/W=1 表示主机要读。很多芯片手册给的地址是 7 位的，你要左移 1 位再拼上 R/W 位。比如 OLED 的 7 位地址是 0x3C，那写操作的 8 位地址就是 0x78（0x3C<<1 | 0）。

---

#### 9.1.2 为什么电赛不用硬件 I2C 而用软件模拟？

这是电赛圈子里人尽皆知的"潜规则"。STM32F103 的硬件 I2C 外设确实存在，但它有几个让人头疼的问题：

1. **硬件 I2C 容易卡死**：如果从设备没有及时应答，或者通信中途被干扰，硬件 I2C 的状态机可能卡在某个状态，SCL 一直被拉低，整个总线锁死。要恢复只能复位外设或重新初始化。
2. **错误处理复杂**：硬件 I2C 有很多状态标志（SB, ADDR, TXE, RXNE, BTF...），写一个健壮的硬件 I2C 驱动要处理十几种状态，代码量是软件模拟的 3 倍。
3. **调试困难**：软件模拟 I2C 出问题了，你用示波器看 SCL/SDA 波形一目了然。硬件 I2C 出问题了，你看不到它内部状态机卡在哪一步。
4. **STM32F1 的硬件 I2C 确实有硬件 BUG**：这是 ST 官方承认的，在勘误表里写着。虽然大部分可以通过 workaround 规避，但对电赛来说，用软件模拟是最省心的选择。

**软件模拟 I2C 的优势**：
- 代码不到 100 行，极其清晰
- 随便哪个 GPIO 都可以当 I2C 引脚，布线灵活
- 出问题了用示波器一抓波形立刻知道哪里不对
- 速度虽然不如硬件 I2C（软件模拟最高约 200kHz），但电赛用的传感器数据量很小，完全够用

> **那什么时候用硬件 I2C？** 当你需要 400kHz 以上的通信速率，或者需要 DMA 自动收发大量数据时。电赛一般不需要。

---

#### 9.1.3 软件模拟 I2C 完整驱动 —— 逐行详解

下面是你将反复使用的软件 I2C 基础驱动。**每一行我都会解释为什么要这样写、如果不这样写会出什么问题**。

##### 第一步：引脚定义和宏封装

```c
// ========== I2C 引脚定义 ==========
// 用宏定义把所有硬件相关的配置集中在这里
// 换硬件的时候只需要改这几行，不用翻遍整个代码
#define I2C_SCL_GPIO    GPIOB          // SCL（时钟线）用的 GPIO 端口：PB口
#define I2C_SCL_PIN     GPIO_Pin_6     // SCL 用的引脚：PB6
#define I2C_SDA_GPIO    GPIOB          // SDA（数据线）用的 GPIO 端口：PB口
#define I2C_SDA_PIN     GPIO_Pin_7     // SDA 用的引脚：PB7

// ========== 基础操作宏 ==========
// 这些宏把"拉高/拉低某个引脚"的操作简化成一目了然的函数名
// 宏的好处：编译器在预处理阶段直接替换，没有函数调用开销，速度最快

// SCL 时钟线的拉高/拉低
// GPIO_SetBits(端口, 引脚)：设置引脚输出高电平，本质是写 BSRR 寄存器
// 为什么用宏而不是函数？因为 I2C 时序对时间要求高，函数调用有入栈出栈开销
#define I2C_SCL_H()  GPIO_SetBits(I2C_SCL_GPIO, I2C_SCL_PIN)   // SCL=1（高电平）
#define I2C_SCL_L()  GPIO_ResetBits(I2C_SCL_GPIO, I2C_SCL_PIN) // SCL=0（低电平）

// SDA 数据线的拉高/拉低
#define I2C_SDA_H()  GPIO_SetBits(I2C_SDA_GPIO, I2C_SDA_PIN)   // SDA=1（释放总线，由上拉电阻拉高）
#define I2C_SDA_L()  GPIO_ResetBits(I2C_SDA_GPIO, I2C_SDA_PIN) // SDA=0（拉低总线）

// 读取 SDA 引脚电平
// GPIO_ReadInputDataBit：读引脚输入数据寄存器的某一位
// 返回 Bit_SET(1) 或 Bit_RESET(0)
// 注意：即使引脚配置为输出模式，这个函数读的也是输入寄存器，
// 所以能正确反映引脚的实际电平（包括从设备拉低的情况）
#define I2C_READ_SDA()  GPIO_ReadInputDataBit(I2C_SDA_GPIO, I2C_SDA_PIN)
```

> **#define 宏 vs 函数的选择**：
> - 这些操作非常简单（一行寄存器操作），用宏定义在编译时展开，**零函数调用开销**
> - I2C 时序通常是微秒级的，函数调用的入栈/出栈可能要几十个时钟周期，会影响时序精度
> - 宏定义在编译时直接替换，生成的机器码就是一条存储指令，速度最快

##### 第二步：I2C 初始化 —— 配置 GPIO 为开漏输出

```c
/**
 * @brief  软件 I2C 初始化函数
 * @note   将 SCL 和 SDA 引脚配置为开漏输出模式
 *         开漏输出 + 内部上拉 = 标准的 I2C 电气接口
 *         
 *         初始化完成后，SCL 和 SDA 都处于高电平（总线空闲状态）
 */
void I2C_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;  // 定义 GPIO 初始化结构体变量
    // 这个结构体有三个成员：GPIO_Pin（哪个引脚）、GPIO_Mode（什么模式）、GPIO_Speed（多快）
    // 结构体在栈上分配，函数退出后自动释放，但我们已经把配置写入了硬件寄存器
    
    // ----- 开启 GPIO 时钟（万年不变的第一步）-----
    // 为什么是 RCC_APB2PeriphClockCmd？因为所有 GPIO 都挂在 APB2 总线上！
    // 如果不加这一行，后面的 GPIO_Init 调用了也没用，硬件不工作
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    // 参数解析：
    // RCC_APB2Periph_GPIOB：这是一个 32 位的位掩码宏，值 = 0x00000008
    //                       对应 RCC_APB2ENR 寄存器的第 3 位（IOPBEN）
    // ENABLE：标准库定义的宏，值 = 1（对应 DISABLE = 0）
    // 函数内部：RCC->APB2ENR |= RCC_APB2Periph_GPIOB;
    // 翻译成人话：将 APB2 外设时钟使能寄存器的 IOPB 位置 1
    
    // ----- 配置 SCL 和 SDA 引脚模式 -----
    // | 操作：把 PB6 和 PB7 用"或"运算组合在一起，一次配置两个引脚
    // GPIO_Pin_6 = 0x0040（二进制：0000 0000 0100 0000）
    // GPIO_Pin_7 = 0x0080（二进制：0000 0000 1000 0000）
    // | 运算结果：0x00C0（第6和第7位都是1）
    GPIO_InitStructure.GPIO_Pin = I2C_SCL_PIN | I2C_SDA_PIN;  // PB6 | PB7
    
    // ----- 核心配置：开漏输出 -----
    // GPIO_Mode_Out_OD = 0x14
    // 开漏输出的电路特性：
    //   - P-MOS 管始终断开（永远不会把引脚拉到 VCC）
    //   - N-MOS 管受输出数据寄存器控制：
    //       写 0 → N-MOS 导通 → 引脚接地（0V），强驱动
    //       写 1 → N-MOS 断开 → 引脚悬空（高阻态）
    //   - 高阻态时，外部（或内部）上拉电阻把引脚拉到 3.3V
    // 这就是 I2C 总线的精髓：只能拉低不能拉高，永远不会短路！
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    
    // ----- 输出速度：50MHz（GPIO 最大翻转速度）-----
    // 对于 I2C 来说，400kHz 就够了。选 50MHz 是因为：
    //   1. 这是 STM32F103 GPIO 的最快速度
    //   2. 不会因为速度不够导致波形畸变
    //   3. 实际 I2C 速度由软件延时控制，GPIO 速度设最大保证波形边沿够陡
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    
    // ----- 调用库函数，把配置写入硬件寄存器 -----
    // GPIO_Init 内部做了这些事：
    //   1. 配置 GPIOx->CRL/CRH（端口配置寄存器）：设置模式位为开漏输出
    //   2. 配置 GPIOx->ODR（输出数据寄存器）：初始输出值
    // 第二个参数传的是结构体指针（&取地址），函数通过指针读取配置
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // ----- 初始化总线状态：SCL 和 SDA 都释放为高电平 -----
    // 开漏输出模式下，GPIO_SetBits 的效果是：N-MOS 断开，引脚被上拉电阻拉到高电平
    // 总线空闲时 SCL 和 SDA 都必须是高电平
    I2C_SCL_H();  // 释放 SCL，由上拉电阻拉高
    I2C_SDA_H();  // 释放 SDA，由上拉电阻拉高
    // 此时总线处于"空闲"状态，可以开始通信
}
```

##### 第三步：起始信号 —— "所有人注意，我要开始说话了"

```c
/**
 * @brief  I2C 起始信号
 * @note   协议规定：SCL 为高电平期间，SDA 从高变低 = 起始信号
 *         起始信号之后，总线进入"忙碌"状态，其他设备不会抢总线
 *         
 *         操作顺序（非常关键，顺序错了就不是 I2C 了）：
 *         1. SDA=1, SCL=1 （确保总线空闲）
 *         2. 延时 5us    （让电平稳定）
 *         3. SDA→0      （在 SCL 为高时，SDA 下降 = 起始！）
 *         4. 延时 5us    （让从设备有足够时间检测到起始信号）
 *         5. SCL→0      （拉低时钟，准备发送数据位）
 */
void I2C_Start(void)
{
    // 第一步：确保起始状态（SDA 和 SCL 都高）
    I2C_SDA_H();    // 先释放 SDA（实际上它可能已经是高了，但为了安全先设置）
    I2C_SCL_H();    // 释放 SCL
    
    // 第二步：延时，让电平稳定下来
    // Delay_us(5) 在我们有了 SysTick 之后，就是精确的 5 微秒延时
    // 为什么是 5us？I2C 标准模式 100kHz 时，SCL 半个周期是 5us
    // 5us 足够任何从设备检测到电平变化
    // 如果太快（比如 1us），慢速设备可能来不及反应
    Delay_us(5);
    
    // 第三步：SDA 从高变低 —— 这就是起始信号的标志！
    I2C_SDA_L();    // 在 SCL 为高的时候，把 SDA 拉低
    // 从设备一直在监视 SCL 和 SDA，当它们检测到"SCL高+SDA下降沿"时，
    // 就知道主机要开始通信了，会准备好接收后面的地址字节
    
    // 第四步：再延时，确保从设备检测到了
    Delay_us(5);
    
    // 第五步：拉低 SCL，准备发送第一个数据位
    // I2C 协议规定：数据位必须在 SCL 低电平时改变，在 SCL 高电平时被采样
    I2C_SCL_L();
    // 现在总线处于：SDA=0, SCL=0，即"起始信号刚结束，准备发第一bit"的状态
    
    // 整个起始信号用时约 10us，对于 100kHz I2C 来说非常标准
}
```

##### 第四步：停止信号 —— "我说完了，你们随意"

```c
/**
 * @brief  I2C 停止信号
 * @note   协议规定：SCL 为高电平期间，SDA 从低变高 = 停止信号
 *         停止信号之后，总线恢复"空闲"状态
 *         
 *         操作顺序：
 *         1. SDA=0, SCL=0（上一个操作结束后的状态）
 *         2. SCL→1（先拉高时钟）
 *         3. 延时 5us
 *         4. SDA→1（在 SCL 高时，SDA 上升 = 停止！）
 *         5. 延时 5us
 */
void I2C_Stop(void)
{
    // 第一步：确保 SDA 是低的，SCL 也是低的
    I2C_SDA_L();    // 先把 SDA 拉低
    // 因为在发完最后一个字节后，SDA 状态不确定，必须确保从低开始
    
    // 第二步：拉高 SCL
    I2C_SCL_H();    // 时钟变高
    
    // 第三步：延时
    Delay_us(5);
    
    // 第四步：在 SCL 为高的时候，SDA 从低变高 = 停止信号！
    I2C_SDA_H();    // 释放 SDA，上拉电阻把它拉到高
    // 从设备检测到"SCL高+SDA上升沿"，就知道通信结束了
    
    // 第五步：延时
    Delay_us(5);
    
    // 总线回到空闲状态：SCL=1, SDA=1
    // 注意：I2C_Stop 调用后，总线就释放了，下次通信要从 I2C_Start 重新开始
}
```

##### 第五步：发送一个字节 —— 逐位输出

```c
/**
 * @brief  I2C 发送一个字节（8位数据）
 * @param  dat: 要发送的字节数据
 * @note   高位在前（MSB first），这是 I2C 协议规定的
 *         
 *         操作流程：
 *         SCL低 → 把SDA设置为要发的bit → 延时 → SCL高（接收方采样）→ 延时 → SCL低 → 下一位
 *         ...重复8次...
 *         第9个时钟：释放SDA，让接收方发送ACK
 */
void I2C_SendByte(uint8_t dat)
{
    uint8_t i;  // 循环计数器，0~7共8次
    
    // ----- 循环8次，每次发送1个bit -----
    for(i = 0; i < 8; i++)
    {
        // 步骤1：SCL 拉低（告诉接收方：我要改变数据了，你别读）
        I2C_SCL_L();
        Delay_us(2);  // 等待电平稳定
        
        // 步骤2：根据要发的 bit 设置 SDA
        // dat & 0x80：检查 dat 的最高位（第7位）是什么
        // 0x80 = 二进制 1000 0000
        // 如果最高位是 1 → dat & 0x80 = 0x80（非0，条件为真）→ SDA 拉高
        // 如果最高位是 0 → dat & 0x80 = 0x00（为0，条件为假）→ SDA 拉低
        if(dat & 0x80)
            I2C_SDA_H();   // 发送 bit=1：释放 SDA（上拉电阻拉高）
        else
            I2C_SDA_L();   // 发送 bit=0：拉低 SDA
        
        // 步骤3：dat 左移一位，准备下一个 bit
        // 第一次循环：检查 bit7 → 左移后 bit6 变成新的 bit7
        // 第二次循环：检查原来的 bit6 → 左移后 bit5 变成新的 bit7
        // ...以此类推，8次循环依次发送 bit7, bit6, bit5, ..., bit0
        dat <<= 1;
        // 举例：dat 初始 = 0x78（二进制 0111 1000）
        // 第1次：dat&0x80=0 → SDA=0, dat左移 → dat=0xF0
        // 第2次：dat&0x80=0x80 → SDA=1, dat左移 → dat=0xE0
        // 第3次：dat&0x80=0x80 → SDA=1, dat左移 → dat=0xC0
        // ...可以看到确实是按 0,1,1,1,1,0,0,0 的顺序发送的
        
        Delay_us(2);  // 等 SDA 稳定
        
        // 步骤4：SCL 拉高（告诉接收方：数据已经稳定了，你现在可以读了）
        I2C_SCL_H();
        // 接收方会在 SCL 上升沿之后采样 SDA 的电平
        
        Delay_us(5);  // 给接收方足够时间采样
        // 如果太快，接收方可能还没来得及读，SCL 就变低了
    }
    
    // ----- 8 个 bit 发完，处理第 9 个时钟（ACK）-----
    // 第 9 个时钟：主机释放 SDA，从机必须拉低 SDA 表示 ACK
    I2C_SCL_L();  // 先拉低 SCL，准备第 9 个时钟
    // 注意：这里没有操作 SDA。
    // 等会调用 I2C_WaitAck() 函数来释放 SDA 并等待接收方拉低
}
```

##### 第六步：等待应答 ACK —— "你收到了吗？"

```c
/**
 * @brief  等待从设备应答（ACK）
 * @return uint8_t: 0 = 收到ACK（正常），1 = 收到NACK（异常，设备没响应）
 * @note   这是检测 I2C 设备是否正常在线的关键函数
 *         如果返回 1（NACK），说明：
 *         - 设备地址写错了
 *         - 设备没上电或损坏
 *         - SDA/SCL 接线断了或接反了
 *         - 上拉电阻没接或阻值太大
 */
uint8_t I2C_WaitAck(void)
{
    uint8_t ack;  // 存储 ACK 结果的变量
    
    // 步骤1：主机释放 SDA（开漏输出写1 = N-MOS断开 = 高阻态）
    // 此时 SDA 由上拉电阻拉到高电平
    I2C_SDA_H();
    Delay_us(2);
    
    // 步骤2：主机拉高 SCL（第 9 个时钟脉冲）
    I2C_SCL_H();
    Delay_us(5);  // 给从机时间反应
    
    // 步骤3：在 SCL 为高的时候，读取 SDA 的电平
    // 如果从设备存在且地址正确，它会在这个时刻把 SDA 拉低（ACK = 0）
    // 如果从设备不存在/地址错误，SDA 保持被上拉电阻拉高的状态（NACK = 1）
    ack = I2C_READ_SDA();  // 读 SDA 引脚电平
    // 返回 Bit_SET(高电平, NACK) 或 Bit_RESET(低电平, ACK)
    
    // 步骤4：拉低 SCL，结束第 9 个时钟
    I2C_SCL_L();
    Delay_us(5);
    
    // 返回值：0 = ACK（成功），非0 = NACK（失败）
    // 在 C 语言中，0 代表 false，非0 代表 true
    // 所以你可以写 if(I2C_WaitAck()) { /* 通信失败！ */ }
    return ack;
}
```

##### 第七步：主机发送应答 —— 读数据时用

```c
/**
 * @brief  主机发送 ACK（应答）给从机
 * @note   主机读数据时，收到一个字节后，如果想继续读，就发 ACK（拉低 SDA）
 *         如果想停止读，就发 NACK（不拉低 SDA，让从机知道这是最后一个字节）
 */
void I2C_Ack(void)
{
    I2C_SCL_L();     // 先把 SCL 拉低
    I2C_SDA_L();     // 把 SDA 拉低（这就是 ACK：告诉从机"我收到了，继续发"）
    Delay_us(2);
    I2C_SCL_H();     // SCL 上升沿，从机采样 SDA，看到低电平 = ACK
    Delay_us(5);
    I2C_SCL_L();     // SCL 拉低，准备下一个字节
}

/**
 * @brief  主机发送 NACK（不应答）给从机
 * @note   告诉从机"这是最后一个字节了，别再发了"
 */
void I2C_NAck(void)
{
    I2C_SCL_L();     // 先把 SCL 拉低
    I2C_SDA_H();     // 释放 SDA，让它保持高（这就是 NACK）
    Delay_us(2);
    I2C_SCL_H();     // SCL 上升沿，从机采样 SDA，看到高电平 = NACK
    Delay_us(5);
    I2C_SCL_L();     // SCL 拉低
}
```

##### 第八步：接收一个字节 —— 逐位读取

```c
/**
 * @brief  I2C 接收一个字节
 * @param  ack: 收到这个字节后是否发送 ACK
 *             ack=1 → 发送 ACK（还要继续读）
 *             ack=0 → 发送 NACK（最后一个字节）
 * @return 接收到的 8 位数据
 * @note   主机作为接收方时，时钟仍然由主机产生
 *         接收完 8 个 bit 后，主机必须发送 ACK/NACK
 */
uint8_t I2C_ReadByte(uint8_t ack)
{
    uint8_t i, dat = 0;  // i:计数器, dat:存储接收到的数据，初始为0
    
    // ----- 释放 SDA，让从机来控制 -----
    // 主机释放 SDA（开漏输出写1），从机将在这根线上发送数据
    I2C_SDA_H();
    
    // ----- 循环 8 次，每次接收 1 个 bit -----
    for(i = 0; i < 8; i++)
    {
        // 步骤1：SCL 拉低（让从机改变 SDA）
        I2C_SCL_L();
        Delay_us(5);  // 等从机把数据放到 SDA 上
        
        // 步骤2：SCL 拉高（采样 SDA）
        I2C_SCL_H();
        
        // 步骤3：把已收到的数据左移一位，给新 bit 腾位置
        dat <<= 1;
        // 第一次循环：dat 从 0x00 变成 0x00（左移后还是0）
        // 然后根据 SDA 决定最低位是 0 还是 1
        
        // 步骤4：读取 SDA，如果是高电平，dat 的最低位 +1
        // I2C_READ_SDA() 返回 Bit_SET(1) 或 Bit_RESET(0)
        if(I2C_READ_SDA())
            dat++;  // 等价于 dat = dat + 1，把最低位设为 1
        // else: dat 已经左移过，最低位是 0，不需要操作
        
        // 举例：收到 bit 依次是 1,0,1,0,1,1,0,0
        // 第1次：dat<<=1 → dat=0x00, SDA=1 → dat=0x01
        // 第2次：dat<<=1 → dat=0x02, SDA=0 → dat=0x02
        // 第3次：dat<<=1 → dat=0x04, SDA=1 → dat=0x05
        // ...最终 dat=0xAC（二进制 1010 1100）
        
        Delay_us(2);  // 保持 SCL 高电平的时间
    }
    // 循环结束后，SCL 还是高电平（最后一次循环的 I2C_SCL_H()）
    
    // ----- 8 个 bit 收完，发送 ACK 或 NACK -----
    // 因为此时 SCL 还是高，而 I2C_Ack/I2C_NAck 会先拉低 SCL
    // 所以直接调用即可，函数内部会处理好时序
    if(ack)
        I2C_Ack();   // 发 ACK：告诉从机"继续发下一个字节"
    else
        I2C_NAck();  // 发 NACK：告诉从机"够了，别发了"
    
    return dat;  // 返回收到的 8 位数据
}
```

##### 第九步：典型 I2C 读写组合 —— 这才是实际用的

有了上面八个基础函数，我们就可以组合出实际和 I2C 设备通信的函数。以读写某个芯片的寄存器为例：

```c
/**
 * @brief  向 I2C 设备的某个寄存器写入一个字节
 * @param  devAddr: 设备 7 位地址（例如 OLED 是 0x3C）
 * @param  regAddr: 寄存器地址
 * @param  data:    要写入的数据
 * @return 0:成功, 1:失败
 * 
 * 通信序列：
 * Start → 设备地址+写(0) → ACK → 寄存器地址 → ACK → 数据 → ACK → Stop
 */
uint8_t I2C_WriteReg(uint8_t devAddr, uint8_t regAddr, uint8_t data)
{
    I2C_Start();                              // 1. 起始信号
    
    I2C_SendByte(devAddr << 1);               // 2. 发送设备地址+写位
    // devAddr 是 7 位地址，<<1 左移一位后最低位是 0（写）
    // 例如 OLED 地址 0x3C << 1 = 0x78
    if(I2C_WaitAck()) { I2C_Stop(); return 1; } // 设备没应答，停止并返回错误
    
    I2C_SendByte(regAddr);                    // 3. 发送寄存器地址
    if(I2C_WaitAck()) { I2C_Stop(); return 1; }
    
    I2C_SendByte(data);                       // 4. 发送数据
    if(I2C_WaitAck()) { I2C_Stop(); return 1; }
    
    I2C_Stop();                               // 5. 停止信号
    return 0;  // 成功
}

/**
 * @brief  从 I2C 设备的某个寄存器读取一个字节
 * @param  devAddr: 设备 7 位地址
 * @param  regAddr: 寄存器地址
 * @return 读取到的数据
 * 
 * 通信序列：
 * Start → 设备地址+写(0) → ACK → 寄存器地址 → ACK → 
 * Re-Start → 设备地址+读(1) → ACK → 读数据 → NACK → Stop
 */
uint8_t I2C_ReadReg(uint8_t devAddr, uint8_t regAddr)
{
    uint8_t val;
    
    // ----- 第一阶段：告诉设备我想读哪个寄存器 -----
    I2C_Start();
    I2C_SendByte(devAddr << 1);               // 地址+写（最低位=0）
    I2C_WaitAck();
    I2C_SendByte(regAddr);                    // 发寄存器地址
    I2C_WaitAck();
    
    // ----- 第二阶段：切换为读模式 -----
    I2C_Start();                              // 重复起始信号（Re-Start）！
    // 注意：这里不是 Stop 再 Start，而是直接用 Start 重新开始
    // 这样总线不会释放，其他设备不会插进来
    I2C_SendByte((devAddr << 1) | 0x01);      // 地址+读（最低位=1）
    // 例如 OLED 地址 0x3C << 1 | 1 = 0x78 | 1 = 0x79
    I2C_WaitAck();
    
    // ----- 第三阶段：读数据 -----
    val = I2C_ReadByte(0);                    // 读一个字节，发 NACK（只要一个字节）
    // 参数 0 = 不发 ACK，因为只读一个字节
    
    I2C_Stop();
    return val;
}
```

> **I2C 驱动自检**：写完这部分代码后，建议用逻辑分析仪或示波器抓一下 SCL 和 SDA 的波形。如果你没有这些设备，至少用一个 I2C 设备（比如 OLED）来测试。如果 OLED 不亮，检查：
> 1. 设备地址对不对（0x78 还是 0x7A？有些 OLED 模块把地址电阻接法不同）
> 2. 上拉电阻接了没有（如果用的是 GPIO 内部上拉，确保 GPIO 初始化时配置了）
> 3. 延时够不够（如果设备是 100kHz 标准模式，延时不能太短）

---

### 9.2 SPI 总线 —— 深入原理与完整驱动

#### 9.2.1 SPI 协议基础

SPI（Serial Peripheral Interface，串行外设接口）是 Motorola 公司发明的**全双工**同步串行总线。和 I2C 相比，SPI 更快、更简单，但需要更多引脚。

**物理层**：
| 信号线    | 方向（相对于主机） | 含义                                                   |
| --------- | ------------------ | ------------------------------------------------------ |
| **SCK**   | 主机→从机          | 串行时钟，**始终由主机产生**。每个时钟脉冲传输一个 bit |
| **MOSI**  | 主机→从机          | Master Out Slave In，主机发数据给从机                  |
| **MISO**  | 从机→主机          | Master In Slave Out，从机发数据给主机                  |
| **CS/SS** | 主机→从机          | Chip Select / Slave Select，片选信号。低电平选中从机   |

> **为什么 MOSI/MISO 不需要上拉电阻？** 因为 SPI 用的是**推挽输出**（不是开漏），主机和从机各负责一根线，永远只有一个设备驱动某根线，不存在冲突。这也是 SPI 比 I2C 快的原因之一——推挽输出的驱动能力强，电平翻转速度快。

**SPI 的四种工作模式（CPOL 和 CPHA）**：

SPI 有 4 种模式，由两个参数决定。**这是新手最容易搞错的地方！** 不同的 SPI 设备可能用不同的模式，必须查数据手册。

| 模式 | CPOL（时钟极性） | CPHA（时钟相位） | SCK 空闲时电平 | 数据采样边沿    | 数据改变边沿    |
| ---- | ---------------- | ---------------- | -------------- | --------------- | --------------- |
| 0    | 0                | 0                | 低电平         | 第1个边沿(上升) | 第2个边沿(下降) |
| 1    | 0                | 1                | 低电平         | 第2个边沿(下降) | 第1个边沿(上升) |
| 2    | 1                | 0                | 高电平         | 第1个边沿(下降) | 第2个边沿(上升) |
| 3    | 1                | 1                | 高电平         | 第2个边沿(上升) | 第1个边沿(下降) |

> **通俗理解**：
> - **CPOL**：决定了"没事的时候"SCL 是什么电平。CPOL=0 是低，CPOL=1 是高。
> - **CPHA**：决定了在第几个边沿采样数据。CPHA=0 在第一个边沿，CPHA=1 在第二个边沿。
> - 电赛最常用的是**模式 0**（CPOL=0, CPHA=0）和**模式 3**（CPOL=1, CPHA=1）。NRF24L01 用模式 0，大部分 SPI Flash 用模式 0 或 3。

**SPI 通信过程（以模式 0 为例）**：
```
CS:  ──────┐                            ┌──────  （拉低选中从机）
            └────────────────────────────┘
SCK:        ┌──┐  ┌──┐  ┌──┐  ┌──┐  （空闲时低电平）
     ──────┘  └──┘  └──┘  └──┘  └──
            ↑  ↑  ↑  ↑  ↑  ↑  ↑  ↑
MOSI: ──────X──X──X──X──X──X──X──X────  （主机发数据）
            D7 D6 D5 D4 D3 D2 D1 D0
MISO: ──────X──X──X──X──X──X──X──X────  （从机发数据，同时进行）
            D7 D6 D5 D4 D3 D2 D1 D0
            ↑ 每个 SCK 上升沿：双方同时采样对方发来的数据
```

> **全双工的含义**：每来一个 SCK 时钟脉冲，MOSI 和 MISO 上同时传输 1 个 bit。主机发送一个字节的同时也会收到一个字节。所以 SPI 的收发函数通常是合一的：`SPI_ReadWriteByte()`，你发一个字节，同时收回来一个字节。

---

#### 9.2.2 软件模拟 SPI 完整驱动 —— 逐行详解

和 I2C 一样，电赛也常用软件模拟 SPI，因为简单、灵活、不出 BUG。以下逐行解释。

##### 第一步：引脚定义

```c
// ========== SPI 引脚定义 ==========
// 选择 PA5(SCK), PA6(MISO), PA7(MOSI)，这三个引脚也是 STM32 硬件 SPI1 的默认引脚
// 好处：如果以后想切换到硬件 SPI，引脚不用改，只需要改初始化代码
#define SPI_SCK_GPIO    GPIOA          // 时钟引脚端口
#define SPI_SCK_PIN     GPIO_Pin_5     // 时钟引脚编号
#define SPI_MOSI_GPIO   GPIOA          // 主机发从机收 端口
#define SPI_MOSI_PIN    GPIO_Pin_7     // 主机发从机收 引脚编号
#define SPI_MISO_GPIO   GPIOA          // 主机收从机发 端口
#define SPI_MISO_PIN    GPIO_Pin_6     // 主机收从机发 引脚编号

// CS 片选引脚通常单独定义，因为每个 SPI 从设备都有自己的 CS
#define SPI_CS_GPIO     GPIOA
#define SPI_CS_PIN      GPIO_Pin_4

// ========== 基础操作宏 ==========
// 为什么 SCK/MOSI/CS 用推挽输出的宏？
// 因为 SPI 是推挽输出总线（和 I2C 不同！），主机始终驱动 SCK 和 MOSI
#define SPI_SCK_H()     GPIO_SetBits(SPI_SCK_GPIO, SPI_SCK_PIN)    // SCK=高
#define SPI_SCK_L()     GPIO_ResetBits(SPI_SCK_GPIO, SPI_SCK_PIN)  // SCK=低
#define SPI_MOSI_H()    GPIO_SetBits(SPI_MOSI_GPIO, SPI_MOSI_PIN)  // MOSI=高
#define SPI_MOSI_L()    GPIO_ResetBits(SPI_MOSI_GPIO, SPI_MOSI_PIN)// MOSI=低
#define SPI_CS_H()      GPIO_SetBits(SPI_CS_GPIO, SPI_CS_PIN)      // CS=高（不选中）
#define SPI_CS_L()      GPIO_ResetBits(SPI_CS_GPIO, SPI_CS_PIN)    // CS=低（选中从机）

// 读 MISO 引脚电平
#define SPI_READ_MISO() GPIO_ReadInputDataBit(SPI_MISO_GPIO, SPI_MISO_PIN)
```

##### 第二步：SPI 初始化

```c
/**
 * @brief  软件 SPI 初始化
 * @note   SCK, MOSI, CS：推挽输出（主机主动驱动这些线）
 *         MISO：浮空输入（主机只读这根线，由从机驱动）
 *         初始化后 SCK 空闲状态为高（对应 CPOL=1，方便后续改模式）
 */
void SPI_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // ----- 开 GPIO 时钟 -----
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    // ----- 配置 SCK 和 MOSI 为推挽输出 -----
    // 为什么是推挽输出？
    // SPI 主机始终是 SCK 和 MOSI 的唯一驱动者，不存在多主机冲突
    // 推挽输出驱动能力强、翻转速度快，适合 SPI 的高速特性
    GPIO_InitStructure.GPIO_Pin = SPI_SCK_PIN | SPI_MOSI_PIN | SPI_CS_PIN;
    // | 运算组合三个引脚，一次配置。分别对应：
    // PA5(SCK)=0x0020, PA7(MOSI)=0x0080, PA4(CS)=0x0010
    // 结果 = 0x00B0
    
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  // 推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;  // 最高速度
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // ----- 单独配置 MISO 为浮空输入 -----
    // MISO 是从机驱动的，主机只能读，所以配置为输入
    // 浮空输入：上下拉电阻都断开，完全由外部（从机）决定电平
    GPIO_InitStructure.GPIO_Pin = SPI_MISO_PIN;         // PA6
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; // 浮空输入
    // 输入模式不需要配置 GPIO_Speed
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // ----- 初始化总线状态 -----
    SPI_SCK_H();   // SCK 空闲高电平（CPOL=1，模式3的配置）
    SPI_CS_H();    // CS 高电平（不选中任何从机）
    // CS 高电平有效的设计：多个 SPI 设备共用 SCK/MOSI/MISO 三根线，
    // 通过各自的 CS 来选择当前和哪个设备通信。
    // CS 高 = 不选中，CS 低 = 选中。这是业界惯例。
}
```

##### 第三步：SPI 读写一个字节 —— 全双工核心函数

```c
/**
 * @brief  SPI 读写一个字节（全双工）
 * @param  dat: 要发送的字节数据
 * @return 同时接收到的字节数据
 * @note   这是 SPI 最核心的函数！
 *         SPI 是全双工的：主机发送一个字节的同时，也从从机接收一个字节
 *         即使你只想读数据，也要发送一个无关字节（通常是 0xFF）来产生时钟
 *         
 *         时序（模式 3，CPOL=1, CPHA=1）：
 *         SCK 空闲为高
 *         SCK 下降沿：主机改变 MOSI，从机改变 MISO
 *         SCK 上升沿：双方采样对方发来的数据
 */
uint8_t SPI_ReadWriteByte(uint8_t dat)
{
    uint8_t i, rx = 0;  // i:循环计数器, rx:接收缓存，初始为0
    
    // ----- 循环 8 次，每次传输 1 个 bit -----
    for(i = 0; i < 8; i++)
    {
        // 步骤1：SCK 下降沿（从高变低）
        // 在 SCK 下降沿，主机改变 MOSI 上的数据，从机改变 MISO 上的数据
        SPI_SCK_L();
        Delay_us(1);  // 等待电平稳定
        
        // 步骤2：把要发的 bit 放到 MOSI 上
        // 高位在前（MSB first），和 I2C 一样，先发 bit7
        if(dat & 0x80)            // 检查 dat 的最高位
            SPI_MOSI_H();          // bit=1，MOSI 输出高
        else
            SPI_MOSI_L();          // bit=0，MOSI 输出低
        
        // 步骤3：dat 左移一位，准备下一个 bit
        dat <<= 1;
        // 和 I2C 发送的逻辑完全一样
        
        // 步骤4：SCK 上升沿（从低变高）
        // 在 SCK 上升沿，双方采样对方的数据
        SPI_SCK_H();
        Delay_us(1);  // 等待电平稳定，让从机有足够时间把数据放到 MISO
        
        // 步骤5：把已接收的数据左移一位，给新 bit 腾位置
        rx <<= 1;
        
        // 步骤6：采样 MISO 引脚
        // 如果 MISO 是高电平，来自从机的 bit = 1，rx 最低位 +1
        // 如果 MISO 是低电平，来自从机的 bit = 0，rx 最低位保持 0（左移后已经是0）
        if(SPI_READ_MISO())
            rx++;  // 等价于 rx = rx + 1
        
        // 一个 bit 传输完成，循环继续下一个 bit
    }
    
    // 步骤7：8 个 bit 传输完毕，SCK 恢复空闲状态（高电平，对应 CPOL=1）
    SPI_SCK_L();  // 先把 SCK 拉低，结束最后一个时钟脉冲
    // 注意：由于 CPOL=1（空闲高），所以调用者在下一次操作前应确保 SCK 回到高
    
    return rx;  // 返回从从机收到的字节
}
```

> **SPI 全双工的本质理解**：
> 这个函数名叫 `SPI_ReadWriteByte`，不是 `SPI_WriteByte` 也不是 `SPI_ReadByte`。因为 SPI 每产生 8 个时钟脉冲，**同时发生了**发送 8 bit 和接收 8 bit 两件事。你不能"只读不写"——因为时钟是主机产生的，主机不发送数据就没有时钟，从机就没法回数据。
>
> **只读操作的技巧**：如果你只想从从机读数据，就发一个无关紧要的字节（通常是 0xFF 或 0x00），称为"哑字节"（dummy byte）。发送 0xFF 产生 8 个时钟，同时收到从机的数据。
>
> **只写操作的技巧**：如果你只想发给从机，不管从机返回什么，收到什么丢掉就行。

##### 第四步：针对特定 SPI 设备的封装 —— 以 NRF24L01 为例

实际使用时，我们会根据具体 SPI 设备封装专用的读写函数：

```c
/**
 * @brief  向 SPI 从设备的某个寄存器写入一个字节
 * @param  reg: 寄存器地址
 * @param  val: 要写入的值
 * 
 * 典型 SPI 设备（如 NRF24L01）的写操作序列：
 * CS=0 → 发送写命令+寄存器地址 → 发送数据 → CS=1
 * 
 * 注意：NRF24L01 的写命令是 0x20+寄存器地址
 * 不同设备的命令格式不同，一定要查数据手册！
 */
void SPI_WriteReg(uint8_t reg, uint8_t val)
{
    SPI_CS_L();                          // 1. CS 拉低，选中从机
    // CS 从高变低告诉从机："我要跟你说话了，注意听！"
    
    SPI_ReadWriteByte(0x20 | reg);       // 2. 发送写命令（0x20）+ 寄存器地址
    // 0x20 是 NRF24L01 的写寄存器命令字
    // reg 是寄存器编号（5位），| 运算拼成完整的命令字节
    // 比如写寄存器 0x00（CONFIG）：发送 0x20 | 0x00 = 0x20
    // 比如写寄存器 0x07（STATUS）：发送 0x20 | 0x07 = 0x27
    // SPI_ReadWriteByte 返回的值在这里是无关紧要的（从机还没准备好数据）
    
    SPI_ReadWriteByte(val);              // 3. 发送要写入的数据
    // 这个字节是实际写入寄存器的值
    
    SPI_CS_H();                          // 4. CS 拉高，释放从机
    // CS 从低变高告诉从机："我说完了，你可以忙你的去了"
    // NRF24L01 在 CS 上升沿锁存刚才收到的数据
}

/**
 * @brief  从 SPI 从设备的某个寄存器读取一个字节
 * @param  reg: 寄存器地址
 * @return 读到的值
 * 
 * NRF24L01 的读操作序列：
 * CS=0 → 发送读命令+寄存器地址 → 发送哑字节(收到数据) → CS=1
 */
uint8_t SPI_ReadReg(uint8_t reg)
{
    uint8_t val;
    
    SPI_CS_L();                          // 1. CS 拉低
    
    SPI_ReadWriteByte(0x00 | reg);       // 2. 发送读命令（0x00）+ 寄存器地址
    // NRF24L01 的读命令是 0x00+寄存器地址
    // 返回值丢弃（第一个字节从机还没准备好）
    
    val = SPI_ReadWriteByte(0xFF);       // 3. 发送哑字节 0xFF，接收数据
    // 0xFF 是"哑字节"：它的内容不重要，关键是产生 8 个 SCK 时钟
    // 在这 8 个时钟期间，从机会把寄存器的值驱动到 MISO 上
    // 所以我们收到的 val 就是寄存器的值
    
    SPI_CS_H();                          // 4. CS 拉高
    
    return val;
}
```

> **SPI 片选 CS 的意义**：
> 多个 SPI 设备可以共用 SCK、MOSI、MISO 三根线，只要各自的 CS 不同时拉低就行。SPI 是一种"一主多从"的总线，而 CS 就是用来选择跟哪个从机说话的。没有 CS 信号，所有从机都会同时收到数据，乱成一团。

---

### 9.3 硬件 SPI 的使用方法

虽然软件模拟 SPI 在电赛中占主流，但有些场景（比如驱动 TFT 屏幕高速刷新）需要硬件 SPI 的高速特性。STM32F103 有两个硬件 SPI（SPI1 和 SPI2）。

#### 9.3.1 硬件 SPI1 配置（模式 0，8MHz，用于 NRF24L01 或 TFT）

```c
/**
 * @brief  硬件 SPI1 初始化
 * @note   使用默认引脚：PA5=SCK, PA6=MISO, PA7=MOSI
 *         模式0（CPOL=0, CPHA=0），8分频得 9MHz（APB2=72MHz/8=9MHz）
 *         适用于 NRF24L01 等最高 10MHz 的设备
 */
void SPI1_Hardware_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    SPI_InitTypeDef   SPI_InitStructure;
    
    // ----- 第一步：开时钟 -----
    // SPI1 挂在 APB2 总线上！GPIOA 也在 APB2
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1, ENABLE);
    // AFIO 如果不用重映射就不需要开
    
    // ----- 第二步：配置 SPI 专用引脚为复用推挽输出 -----
    // PA5=SCK, PA7=MOSI 作为 SPI 外设的输出引脚
    // 注意：用硬件 SPI 时，引脚模式必须是 AF_PP（复用推挽），不是 Out_PP！
    // 区别：AF_PP 模式下引脚由 SPI 外设控制，Out_PP 由 GPIO 数据寄存器控制
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;  // SCK + MOSI
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;          // 复用推挽！
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // PA6=MISO 作为 SPI 外设的输入引脚
    // 输入模式用浮空输入（或上拉输入，看具体设备）
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;                // MISO
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;    // 浮空输入
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // ----- 第三步：配置 SPI 参数 -----
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex; // 双线全双工
    // 选项：
    //   SPI_Direction_2Lines_FullDuplex  → 标准四线 SPI
    //   SPI_Direction_1Line_Rx           → 单线只收
    //   SPI_Direction_1Line_Tx           → 单线只发
    //   SPI_Direction_2Lines_RxOnly      → 双线只收（不发，节省一个引脚？极少用）
    
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;  // 主机模式
    // SPI_Mode_Slave：从机模式，电赛基本不用
    
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;  // 8 位数据帧
    // SPI_DataSize_16b：16 位数据帧，少数设备用
    
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;   // CPOL=0：空闲时 SCK 低电平
    // SPI_CPOL_High = CPOL=1：空闲时 SCK 高电平
    // 配合下面的 CPHA 选择模式：
    //   CPOL=0, CPHA=1 → 模式 0
    //   CPOL=0, CPHA=2 → 模式 1
    //   CPOL=1, CPHA=1 → 模式 2
    //   CPOL=1, CPHA=2 → 模式 3
    
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;  // CPHA=0：第一个边沿采样
    // SPI_CPHA_2Edge = CPHA=1：第二个边沿采样
    
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;     // 软件管理 CS 引脚
    // SPI_NSS_Hard：硬件自动管理 CS，一般不用，灵活性不够
    
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8; // 8 分频
    // APB2=72MHz / 8 = 9MHz
    // 可选分频：2,4,8,16,32,64,128,256
    // 对应速率：36M,18M,9M,4.5M,2.25M,1.125M,562.5k,281.25k
    // NRF24L01 最高 10MHz，所以选 8 分频（9MHz）刚好
    
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;  // 高位在前
    // SPI_FirstBit_LSB：低位在前，极少用
    
    SPI_InitStructure.SPI_CRCPolynomial = 7;  // CRC 校验多项式，不用 CRC 就不用管
    // CRC 功能需要单独使能才能用
    
    SPI_Init(SPI1, &SPI_InitStructure);  // 把以上配置写入硬件寄存器
    
    // ----- 第四步：使能 SPI1 -----
    SPI_Cmd(SPI1, ENABLE);
    // SPI1->CR1 的第 6 位（SPE）置 1，SPI 开始工作
}

/**
 * @brief  硬件 SPI1 读写一个字节
 * @param  dat: 要发送的数据
 * @return 接收到的数据
 * @note   和软件 SPI 的接口完全一样，方便切换（只改初始化函数即可）
 *         
 *         硬件 SPI 的收发流程：
 *         1. 把数据写入 SPI1->DR（数据寄存器），硬件自动开始发送
 *         2. 等待发送完成（SPI1->SR 的 TXE 位 = 1，发送缓冲区空）
 *         3. 等待接收完成（SPI1->SR 的 RXNE 位 = 1，接收缓冲区非空）
 *         4. 读取 SPI1->DR 得到接收数据
 */
uint8_t SPI1_ReadWriteByte(uint8_t dat)
{
    // 步骤1：等待发送缓冲区空
    // SPI_I2S_GetFlagStatus 检查 SPI1->SR 寄存器的 SPI_I2S_FLAG_TXE 位
    // TXE（Transmit buffer Empty）= 发送缓冲区空，可以写入新数据
    // 必须等 TXE=1 才能写 DR，否则会覆盖还没发完的数据
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
    // while 条件是 TXE==0（缓冲区不空），就死等
    // 直到 TXE==1，退出循环
    
    // 步骤2：发送数据（写 DR 寄存器）
    // SPI_I2S_SendData 内部就是：SPI1->DR = dat;
    // 写入 DR 会自动启动发送过程（如果之前是空闲的）
    SPI_I2S_SendData(SPI1, dat);
    
    // 步骤3：等待接收完成
    // RXNE（Receive buffer Not Empty）= 接收缓冲区非空，有数据可以读
    // 必须等 RXNE=1 才读 DR
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
    
    // 步骤4：读取接收数据
    // SPI_I2S_ReceiveData 内部就是：return SPI1->DR;
    return SPI_I2S_ReceiveData(SPI1);
}
```

> **硬件 SPI vs 软件 SPI 对比总结**：
> 
> | 特性       | 软件模拟 SPI         | 硬件 SPI                   |
> | ---------- | -------------------- | -------------------------- |
> | 速度       | 最高约 2MHz          | 最高 18MHz（APB2/4分频）   |
> | CPU 占用   | 传输期间 CPU 全程忙等 | 硬件自动收发，CPU 可做别的事 |
> | 引脚灵活性 | 任意 GPIO             | 固定引脚（除非重映射）      |
> | 代码复杂度 | 简单，几十行          | 稍复杂，需理解多个寄存器    |
> | 稳定性     | 绝对可靠（纯 GPIO）   | 可靠（但要注意分频和模式）  |
> | DMA 支持   | 不支持                | 支持（高速传输必用）        |
> | 电赛推荐   | OLED、传感器等低速设备 | TFT 屏幕、NRF24L01 等       |

---

### 9.4 I2C 与 SPI 选型指南（电赛实战）

| 场景                    | 推荐总线 | 原因                                             |
| ----------------------- | -------- | ------------------------------------------------ |
| 0.96 寸 OLED 显示屏     | I2C      | 只需要 2 根线，速度够用                          |
| MPU6050 陀螺仪          | I2C      | 标准 I2C 接口，数据量小                          |
| BMP280 气压计           | I2C/SPI  | 两种都支持，I2C 省引脚                           |
| NRF24L01 无线模块       | SPI      | 只支持 SPI，速率要求高                           |
| 1.8 寸 TFT 彩屏         | SPI      | 数据量大需高速，I2C 太慢                         |
| W25Q64 Flash 存储       | SPI      | 只支持 SPI                                       |
| AT24C02 EEPROM          | I2C      | 只支持 I2C                                       |
| 多设备共用总线          | I2C      | I2C 天然支持多从机（每个有唯一地址）             |
| 需要长距离通信（>30cm） | I2C      | I2C 更适合远距离（差分后可到几米），SPI 易受干扰 |

> **电赛经验**：
> - 所有 I2C 设备买回来先测地址！用 I2C 扫描程序扫描 0x00~0x7F 的地址，看哪些地址有设备应答。有时候同一型号不同批次的模块地址不一样（比如 OLED 0x3C vs 0x3D）。
> - SPI 的 CS 引脚一定要接！很多新手忘了接 CS，结果 SPI 通信失败。软件 SPI 的 CS 可以用任意 GPIO，硬件 SPI 的 CS（NSS）可以软件管理。
> - I2C 上拉电阻不能省！如果模块上没有上拉电阻（比如有些 OLED 模块没有），一定要自己接 4.7kΩ 上拉到 3.3V。没有上拉电阻，I2C 完全不工作。

---

## 第九章补充：CAN 总线通信 —— 汽车电子与多机通信

CAN（Controller Area Network，控制器局域网）是汽车电子、工业控制中最常用的现场总线之一。电赛中涉及多机协作、分布式控制、智能车等题目时，CAN 总线是专业级的选择。

### 9S.1 CAN 总线基础

#### 9S.1.1 为什么需要 CAN？它和 USART/I2C/SPI 有什么不同？

| 特性     | USART            | I2C            | SPI                   | **CAN**                            |
| -------- | ---------------- | -------------- | --------------------- | ---------------------------------- |
| 线数     | 2（TX+RX）       | 2（SCL+SDA）   | 4（SCK+MOSI+MISO+CS） | 2（CANH+CANL，差分）               |
| 多机通信 | 不支持（一对一） | 支持（多从机） | 支持（多从机，需CS）  | **天然支持多主机**                 |
| 通信距离 | 几米             | 几十厘米       | 几十厘米              | **几十米到几千米**                 |
| 抗干扰   | 差（单端信号）   | 差（单端）     | 差（单端）            | **极强（差分信号）**               |
| 错误检测 | 无/奇偶校验      | 无/ACK         | 无                    | **CRC+ACK+位错误+填充错误+帧检查** |
| 仲裁机制 | 无（全双工）     | 无（主从模式） | 无（主从模式）        | **逐位仲裁，优先级高者胜**         |
| 最大速率 | 115200bps        | 400kbps        | 18Mbps                | 1Mbps                              |
| 帧格式   | 简单             | 中等           | 中等                  | **复杂但极其可靠**                 |
| 电赛用途 | 调试+蓝牙模块    | 传感器         | 屏幕+无线             | **多车协同、机器人集群**           |

> **CAN 的核心优势**：
> 1. **差分信号**：CANH 和 CANL 两根线传输的是差分信号（CANH-CANL），共模干扰被抵消，抗干扰能力极强。电机旁边走线也不会误码。
> 2. **逐位仲裁**：多个节点同时发送时，CAN 控制器会自动仲裁——ID 小的优先级高，继续发送；ID 大的自动退出，等总线空闲再发。数据不会冲突！
> 3. **自动重传**：发送失败自动重发，硬件完成的，不需要软件干预。

#### 9S.1.2 CAN 的物理层

```
STM32 的 CAN 外设只提供了 CAN_TX 和 CAN_RX 两个逻辑引脚（3.3V TTL 电平）。
要连接到 CAN 总线，必须外加 CAN 收发器芯片（如 TJA1050、SN65HVD230）。

┌──────────┐          ┌──────────────┐       CANH ─────────── CANH
│ STM32F103│  CAN_TX  │ CAN 收发器    │       CANL ─────────── CANL
│   CAN    │─────────→│ (TJA1050)    │       (差分信号，双绞线)
│  外设    │  CAN_RX  │              │
│          │←─────────│ 3.3V 转 CAN  │
└──────────┘          └──────────────┘
                             ↑
                     必须接 120Ω 终端电阻（总线两端各一个）
```

> **硬件要求**：CAN 收发器必须用 5V 供电（TJA1050），但和 STM32 的 CAN_RX/CAN_TX 引脚直连是安全的。总线两端各接一个 120Ω 终端电阻，用双绞线走 CANH 和 CANL。

#### 9S.1.3 CAN 的协议帧格式（简化理解）

CAN 协议的核心是**基于 ID 的消息广播**——**没有"地址"概念**！ID 不是指定发给谁，而是标识消息的"主题"。所有节点都在监听着，收到消息后看 ID 是不是自己关心的，是就处理，不是就忽略。ID 越小，优先级越高。

```
标准数据帧（11位ID）：
┌──────┬─────────┬────────┬──────────┬──────┬─────┬──────┐
│ SOF  │ 11位ID  │ DLC长度│ 0~8B数据 │ CRC16│ ACK │ EOF  │
│ 1bit │ 仲裁段  │ 控制段 │  数据段   │ 校验 │应答 │ 帧尾 │
└──────┴─────────┴────────┴──────────┴──────┴─────┴──────┘
```

---

### 9S.2 STM32F103 CAN 外设配置

STM32F103C8T6 的 CAN 外设：APB1 总线（36MHz），默认引脚 PA11(RX)/PA12(TX)，3 个发送邮箱 + 2 个接收 FIFO（各 3 级深度），14 个可配置滤波器组。

#### 9S.2.1 CAN 波特率计算

$$ 1\text{ bit时间} = (1 + BS1 + BS2) \times (BRP + 1) \times T_{PCLK1} $$

其中 $T_{PCLK1} = 1/36\text{MHz} \approx 27.8\text{ns}$。

**电赛常用 500kbps 配置**：BRP=4, BS1=11, BS2=4
- 1 bit = (1+11+4) × 5 × 27.8ns = 16 × 139ns ≈ 2.22μs
- 波特率 ≈ 450kbps（在 CAN 5% 容差内可用）

#### 9S.2.2 CAN 初始化完整代码（逐行详解）

```c
/**
 * @brief  CAN1 初始化（500kbps，标准帧）
 * @note   CAN 挂在 APB1 总线上（36MHz）
 *         波特率 = 36MHz / (BRP+1) / (1+BS1+BS2) ≈ 500kbps
 *         
 *         配置步骤：
 *         1. 开时钟（GPIOA + CAN1）
 *         2. 配置 PA11(RX) 上拉输入，PA12(TX) 复用推挽
 *         3. 配置 CAN 参数（波特率、模式等）
 *         4. 配置 CAN 滤波器（决定接收哪些 ID）
 *         5. 配置 NVIC（CAN 接收中断）
 */
void CAN1_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    CAN_InitTypeDef   CAN_InitStructure;
    CAN_FilterInitTypeDef CAN_FilterInitStructure;
    NVIC_InitTypeDef  NVIC_InitStructure;
    
    // ===== 第1步：开时钟 =====
    // GPIOA 在 APB2，CAN1 在 APB1！
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);
    
    // ===== 第2步：配置 CAN 引脚 =====
    // PA11 = CAN_RX：上拉输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  // 上拉输入
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    // 为什么上拉？CAN 总线空闲时是隐性电平（逻辑1），上拉保证默认高电平
    
    // PA12 = CAN_TX：复用推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  // 复用推挽！
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // ===== 第3步：CAN 参数配置 =====
    CAN_DeInit(CAN1);  // 复位 CAN1 到默认状态
    
    CAN_InitStructure.CAN_TTCM = DISABLE;   // 时间触发：关闭
    CAN_InitStructure.CAN_ABOM = ENABLE;    // 自动离线管理：开启
    // ABOM：当错误太多导致 CAN 离线时，自动尝试恢复。电赛建议开着
    CAN_InitStructure.CAN_AWUM = ENABLE;    // 自动唤醒：开启
    CAN_InitStructure.CAN_NART = DISABLE;   // 禁止自动重传=DISABLE → 允许自动重传
    // DISABLE 双重否定！实际效果：允许自动重传
    CAN_InitStructure.CAN_RFLM = DISABLE;   // FIFO 不锁定（满后覆盖旧的）
    CAN_InitStructure.CAN_TXFP = DISABLE;   // 发送优先级由 ID 决定（ID小先发）
    CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;  // 正常模式
    // 调试时可用 CAN_Mode_LoopBack（回环模式，自己发自己收，无需外部收发器）
    
    // 波特率配置（核心！）
    CAN_InitStructure.CAN_SJW = CAN_SJW_1tq;     // 同步跳转宽度：1 tq
    CAN_InitStructure.CAN_BS1 = CAN_BS1_11tq;    // 时间段1：11 tq
    CAN_InitStructure.CAN_BS2 = CAN_BS2_4tq;     // 时间段2：4 tq
    CAN_InitStructure.CAN_Prescaler = 4;          // BRP=4，分频=5
    // 1 bit = (1+11+4) * 5 / 36M = 80/36M ≈ 2.22μs → ~450kbps
    
    CAN_Init(CAN1, &CAN_InitStructure);
    
    // ===== 第4步：配置 CAN 滤波器 =====
    // 滤波器决定接收哪些 ID 的消息，不匹配的消息在硬件层就丢弃
    CAN_FilterInitStructure.CAN_FilterNumber = 0;        // 滤波器组 0
    CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;  // 掩码模式
    CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit; // 32位模式
    
    // 滤波器 ID 和掩码（标准 ID 在 32 位中位于 bit[31:21]，需左移 21 位）
    // 掩码位=1 的位必须匹配，掩码位=0 的位不关心
    // 以下配置：只接收 ID=0x321 的标准帧数据
    CAN_FilterInitStructure.CAN_FilterIdHigh = 0x321 << 5;      // 目标 ID 移到位
    CAN_FilterInitStructure.CAN_FilterIdLow  = 0x0000;
    CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x7FF << 5;  // 11位都要匹配
    CAN_FilterInitStructure.CAN_FilterMaskIdLow  = 0x0000;
    // 要接收所有 ID（调试用）：掩码全写 0 即可
    
    CAN_FilterInitStructure.CAN_FilterFIFOAssignment = CAN_Filter_FIFO0;  // 存入 FIFO0
    CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;  // 激活
    CAN_FilterInit(&CAN_FilterInitStructure);
    
    // ===== 第5步：配置 NVIC =====
    // 注意！CAN1 RX0 中断名是 USB_LP_CAN1_RX0_IRQn（和 USB 共用中断号）
    NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    CAN_ITConfig(CAN1, CAN_IT_FMP0, ENABLE);  // 使能 FIFO0 消息挂起中断
}

/**
 * @brief  CAN 发送数据
 * @param  std_id: 11 位标准标识符（0x000~0x7FF）
 * @param  data:   要发送的数据（0~8 字节）
 * @param  len:    数据长度（0~8）
 * @return 0: 成功，1: 失败（所有 3 个邮箱都忙）
 */
uint8_t CAN_SendData(uint16_t std_id, uint8_t* data, uint8_t len)
{
    CanTxMsg TxMessage;
    uint8_t mailbox;
    
    if(len > 8) len = 8;  // CAN 一帧最多 8 字节
    
    TxMessage.StdId = std_id;          // 标准 ID
    TxMessage.ExtId = 0;               // 扩展 ID（不用）
    TxMessage.IDE   = CAN_Id_Standard; // 标准帧
    TxMessage.RTR   = CAN_RTR_Data;   // 数据帧
    TxMessage.DLC   = len;             // 数据长度
    for(uint8_t i = 0; i < len; i++)
        TxMessage.Data[i] = data[i];
    
    // CAN_Transmit 自动选择空闲邮箱，返回邮箱号或 CAN_NO_MB（失败）
    mailbox = CAN_Transmit(CAN1, &TxMessage);
    if(mailbox == CAN_NO_MB) return 1;  // 所有邮箱忙
    
    // 等待发送完成
    uint32_t timeout = 0;
    while(CAN_TransmitStatus(CAN1, mailbox) == CAN_TxStatus_Pending)
    {
        if(++timeout > 100000) return 1;  // 超时
    }
    return 0;
}

/**
 * @brief  CAN 接收中断服务函数（FIFO0）
 * @note   中断函数名必须和启动文件里定义的一模一样！
 *         USB_LP_CAN1_RX0_IRQHandler 这个名字看起来很怪，
 *         是因为 USB 低优先级中断和 CAN1 RX0 共用了同一个中断向量号
 */
volatile uint8_t can_rx_flag = 0;
CanRxMsg can_rx_msg;

void USB_LP_CAN1_RX0_IRQHandler(void)
{
    if(CAN_GetITStatus(CAN1, CAN_IT_FMP0) != RESET)
    {
        CAN_Receive(CAN1, CAN_FIFO0, &can_rx_msg);  // 读消息
        can_rx_flag = 1;  // 通知主循环
        CAN_ClearITPendingBit(CAN1, CAN_IT_FMP0);
    }
}

// 在主循环中处理：
void CAN_Process(void)
{
    if(can_rx_flag)
    {
        can_rx_flag = 0;
        // can_rx_msg.StdId：发送方的 ID
        // can_rx_msg.DLC：数据长度
        // can_rx_msg.Data[]：接收数据
        switch(can_rx_msg.StdId)
        {
            case 0x100: Motor_SetSpeed(can_rx_msg.Data[0], can_rx_msg.Data[1]); break;
            case 0x200: Send_SensorData(); break;
        }
    }
}
```

> **CAN 电赛经验**：
> 1. **一定要接 CAN 收发器**！不能把 PA11/PA12 直接当 CANH/CANL 用。
> 2. **一定要接 120Ω 终端电阻**！总线两端各一个，否则信号反射导致通信失败。
> 3. **调试用回环模式**：`CAN_Mode_LoopBack` 自己发自己收，无需外部收发器即可测试代码。
> 4. **中断名是坑**：`USB_LP_CAN1_RX0_IRQHandler`，在启动文件中确认。

---

## 第九章补充二：Flash 存储 —— 掉电保存数据

电赛中经常需要**掉电保存参数**——PID 系数、校准值等。C8T6 没有内置 EEPROM，但可以利用片内 Flash 来存储数据。

### 9S.3 Flash 组织与原理

C8T6 Flash 共 64KB，按 1KB 一页组织（0~63 页）：
- **擦除最小单位**：1 页（1KB），擦除后整页变 0xFF
- **写入最小单位**：16 位（半字，2 字节）
- **写入前必须先擦除**：Flash 只能 1→0，不能 0→1
- **擦写寿命**：约 1 万次，**不要频繁写入**

> **存储策略**：用最后几页（第 62、63 页）存数据，确保不和代码重叠。只用在"用户主动保存"或"掉电前紧急保存"。

### 9S.4 Flash 完整读写代码（逐行详解）

```c
// 使用第 63 页（最后一页）存储
#define FLASH_SAVE_PAGE    63
#define FLASH_SAVE_ADDR    (0x08000000 + FLASH_SAVE_PAGE * 1024)  // = 0x0800FC00

// 要保存的数据结构
typedef struct {
    uint16_t magic;          // 魔数 0xA5A5 表示数据有效
    uint16_t checksum;       // XOR 校验和
    float    pid_kp;         // PID 系数
    float    pid_ki;
    float    pid_kd;
    uint16_t motor_max_speed;
    uint16_t reserved[5];    // 预留扩展
    // 总大小约 28 字节，远小于 1KB
} SaveDataType;

/**
 * @brief  从 Flash 读取保存的数据
 * @note   Flash 被映射到内存地址空间，读 Flash 和读 RAM 一样
 *         但不能直接解引用可能不对齐的结构体（会 HardFault）
 *         所以用 memcpy 最安全
 */
uint8_t Flash_ReadData(SaveDataType* data)
{
    // 把 Flash 地址强制转换为结构体指针
    SaveDataType* src = (SaveDataType*)FLASH_SAVE_ADDR;
    
    // memcpy：从 Flash 地址安全拷贝到 RAM 中的结构体
    memcpy(data, src, sizeof(SaveDataType));
    
    // 检查魔数：0xA5A5 表示数据有效
    if(data->magic != 0xA5A5)
        return 1;  // 首次使用或数据无效
    
    // XOR 校验
    uint16_t checksum = 0;
    uint16_t* ptr = (uint16_t*)data;
    for(uint8_t i = 2; i < sizeof(SaveDataType)/2; i++)
        checksum ^= ptr[i];
    if(checksum != data->checksum)
        return 1;  // 数据损坏
    
    return 0;  // 成功
}

/**
 * @brief  向 Flash 写入数据
 * @note   流程：解锁 → 擦除整页 → 逐半字写入 → 锁定
 *         写入期间 CPU 暂停取指，中断可能丢失
 *         如需保证中断响应，写入前临时关闭非关键中断
 */
uint8_t Flash_SaveData(SaveDataType* data)
{
    // 准备：填魔数 + 计算校验和
    data->magic = 0xA5A5;
    data->checksum = 0;
    uint16_t* ptr = (uint16_t*)data;
    for(uint8_t i = 2; i < sizeof(SaveDataType)/2; i++)
        data->checksum ^= ptr[i];
    
    // 第1步：解锁 Flash（写两个密钥到 FLASH_KEYR）
    FLASH_Unlock();
    // 内部：FLASH->KEYR = 0x45670123; FLASH->KEYR = 0xCDEF89AB;
    
    // 第2步：擦除目标页（整页 1KB 变 0xFF）
    FLASH_Status status = FLASH_ErasePage(FLASH_SAVE_ADDR);
    if(status != FLASH_COMPLETE) { FLASH_Lock(); return 1; }
    
    // 第3步：逐半字写入（Flash 只接受 16 位写入）
    uint16_t* src = (uint16_t*)data;
    uint32_t addr = FLASH_SAVE_ADDR;
    uint16_t count = sizeof(SaveDataType) / 2;
    for(uint16_t i = 0; i < count; i++)
    {
        status = FLASH_ProgramHalfWord(addr, src[i]);
        if(status != FLASH_COMPLETE) { FLASH_Lock(); return 1; }
        addr += 2;  // 地址 +2 字节
    }
    
    // 第4步：锁定 Flash
    FLASH_Lock();
    
    // 第5步：验证（读回对比）
    SaveDataType verify;
    memcpy(&verify, (void*)FLASH_SAVE_ADDR, sizeof(SaveDataType));
    if(memcmp(data, &verify, sizeof(SaveDataType)) != 0)
        return 1;  // 验证失败
    
    return 0;  // 写入成功
}

// ===== 使用示例 =====
SaveDataType settings;

int main(void)
{
    // ... 初始化 ...
    
    // 读取保存的设置
    if(Flash_ReadData(&settings) != 0)
    {
        // 首次使用，使用默认值
        settings.pid_kp = 1.0f;
        settings.pid_ki = 0.01f;
        settings.pid_kd = 0.0f;
    }
    
    // ... 运行 ...
    
    // 用户按了"保存"键
    if(key_save_pressed)
    {
        Flash_SaveData(&settings);
        OLED_ShowString(0, 0, "Saved!", 16);
    }
}
```

> **Flash 存储电赛经验**：
> 1. **不要频繁写**（寿命 1 万次）。频繁记录用外部 EEPROM（AT24C02，I2C，100 万次）。
> 2. **必须加校验**（魔数+校验和），防止读到损坏数据。
> 3. **写入期间关闭关键中断**，因为 Flash 写入时 CPU 暂停取指。
> 4. **选好页**：确保不和代码重叠。查看 Keil 编译输出确认代码大小。

---

## 第十章 系统稳定性外设——看门狗、RTC、备份域与电源管理

电赛四天三夜做出来的作品，最容易出的问题就是**程序跑飞**：电机转着转着突然停了、屏幕突然黑了、系统突然没反应了。看门狗就是专门解决这个问题的，是电赛作品稳定性的必备保障。

RTC 实时时钟和备份寄存器则在需要掉电计时、保存校准参数的场景中发挥作用。低功耗模式（Sleep/Stop/Standby）在一些电池供电的电赛题目中也越来越常见。

---

### 10.1 独立看门狗 IWDG —— 程序死机自动复位

#### 10.1.1 独立看门狗的原理（为什么它这么可靠）

独立看门狗（Independent WatchDog）本质是一个**完全独立于 CPU 的倒计时计数器**。

```
┌─────────────────────────────────────────────────┐
│                  STM32F103 芯片                  │
│                                                 │
│  ┌──────────┐        ┌──────────────────────┐   │
│  │  Cortex-M3 │        │  IWDG 独立看门狗      │   │
│  │  内核      │        │  ┌─────────────────┐ │   │
│  │            │  喂狗   │  │ 12位递减计数器   │ │   │
│  │  执行程序  │────────→│  │ 从RLR值减到0     │ │   │
│  │            │        │  │ 减到0 → 复位！    │ │   │
│  │  跑飞了！  │   ×    │  │                  │ │   │
│  │            │        │  └─────────────────┘ │   │
│  └──────────┘        │  时钟源：LSI 40kHz    │   │
│                       │  （独立RC振荡器）     │   │
│                       └──────────────────────┘   │
│                                                 │
│  关键：IWDG 的时钟是独立的 LSI（40kHz 内部 RC）  │
│  即使主晶振坏了、PLL 停了、APB 时钟全崩了，     │
│  IWDG 照样在跑，照样能复位系统！                 │
└─────────────────────────────────────────────────┘
```

IWDG 有以下几个硬件特性使其极其可靠：
1. **独立的时钟源**：使用芯片内部的 LSI RC 振荡器（约 40kHz），不依赖外部晶振和 PLL。
2. **一旦使能就无法软件关闭**：只有系统复位才能关闭 IWDG。这防止了跑飞的代码意外关掉看门狗。
3. **独立的电压域**：IWDG 在 VDD 电压域工作，只要芯片有电它就能工作。
4. **写保护机制**：修改 IWDG 的配置需要先写入解锁序列（0x5555），防止跑飞的代码意外改配置。

#### 10.1.2 IWDG 的关键寄存器

IWDG 只有 4 个寄存器（在地址 0x40003000 开始）：

| 寄存器  | 名称         | 描述                                                                                            |
| ------- | ------------ | ----------------------------------------------------------------------------------------------- |
| **KR**  | 关键字寄存器 | 写 0xAAAA → 喂狗（重载计数器）；写 0x5555 → 解锁 PR 和 RLR；写 0xCCCC → 使能 IWDG               |
| **PR**  | 预分频寄存器 | 低 3 位有效，设置 LSI 的分频系数（0~7，对应 4~256 分频）。**写保护**，需先写 KR=0x5555 才能修改 |
| **RLR** | 重装载寄存器 | 12 位有效（0~4095），看门狗计数器的重装载值。**写保护**，需先写 KR=0x5555 才能修改              |
| **SR**  | 状态寄存器   | 第 0 位 PVU（预分频更新中），第 1 位 RVU（重装载值更新中）。更新期间不能再次写 PR 或 RLR        |

#### 10.1.3 IWDG 超时时间计算（逐行拆解）

IWDG 的超时时间由两个参数决定：预分频系数和重装载值。

**计算步骤**：

1. LSI 频率 ≈ 40kHz（注意：内部 RC 振荡器精度不高，典型值 30kHz~60kHz，个体差异大！）
2. 预分频后的计数器时钟 = 40kHz ÷ 预分频系数
3. 超时时间 = 重装载值 ÷ 预分频后的频率

**公式**：

$$ T_{IWDG} = \frac{RLR \times \text{预分频系数}}{40\text{kHz}} $$

**标准库的预分频宏定义和实际分频值**：

| 标准库宏             | 预分频系数 | 计数器频率             | 每个计数的时间 |
| -------------------- | ---------- | ---------------------- | -------------- |
| `IWDG_Prescaler_4`   | 4          | 40kHz ÷ 4 = 10kHz      | 0.1ms          |
| `IWDG_Prescaler_8`   | 8          | 40kHz ÷ 8 = 5kHz       | 0.2ms          |
| `IWDG_Prescaler_16`  | 16         | 40kHz ÷ 16 = 2.5kHz    | 0.4ms          |
| `IWDG_Prescaler_32`  | 32         | 40kHz ÷ 32 = 1.25kHz   | 0.8ms          |
| `IWDG_Prescaler_64`  | 64         | 40kHz ÷ 64 = 625Hz     | 1.6ms          |
| `IWDG_Prescaler_128` | 128        | 40kHz ÷ 128 = 312.5Hz  | 3.2ms          |
| `IWDG_Prescaler_256` | 256        | 40kHz ÷ 256 = 156.25Hz | 6.4ms          |

**举例**：
- 预分频 64，RLR=625：超时 = 625 × 64 ÷ 40000 = 1.0 秒 ✓
- 预分频 32，RLR=1250：超时 = 1250 × 32 ÷ 40000 = 1.0 秒 ✓（和上面等价）
- 预分频 4，RLR=2000：超时 = 2000 × 4 ÷ 40000 = 0.2 秒（200ms，适合快速恢复）

> **⚠️ LSI 精度问题**：内部 40kHz RC 振荡器的精度只有 ±30% 左右！这意味着你算出来 1 秒的超时，实际可能是 0.7 秒到 1.3 秒。所以 IWDG 不适合精确计时（精确计时用 TIM 或 SysTick），它只适合做"防死机"的安全网。设超时时间时要留足余量。
>
> **电赛建议**：超时设 500ms~2s。太短（<100ms）可能正常程序来不及喂狗导致误复位；太长（>5s）系统死机后要等太久才恢复。

#### 10.1.4 IWDG 完整配置代码（逐行详解）

```c
/**
 * @brief  初始化独立看门狗
 * @note   超时时间 ≈ 1 秒（预分频=64，RLR=625）
 *         
 *         重要特性：
 *         1. IWDG 一旦使能就无法软件关闭，只能系统复位
 *         2. IWDG 的时钟是 LSI（内部 40kHz RC），不需要开 APB 时钟
 *         3. 配置 PR 和 RLR 前必须写 KR=0x5555 解锁
 *         4. 写 PR 或 RLR 后必须等 SR 寄存器的更新标志清零
 *         
 *         调用时机：main() 初始化完 SysTick 和外设后，
 *         进入 while(1) 之前调用。
 */
void IWDG_Init(void)
{
    // ===== 第1步：解锁 IWDG 的 PR 和 RLR 寄存器 =====
    // IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable) 内部：
    //   IWDG->KR = 0x5555;
    // 0x5555 是写使能密钥，写入后 PR 和 RLR 寄存器才能被修改
    // 这是硬件写保护机制：跑飞的程序不太可能恰好写出 0x5555
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
    
    // ===== 第2步：设置预分频系数 =====
    // IWDG_SetPrescaler(IWDG_Prescaler_64) 内部：
    //   IWDG->PR = prescaler;  （prescaler 参数的低3位）
    // 64分频：40kHz ÷ 64 ≈ 625Hz
    // 也就是计数器每 1/625 秒 = 1.6ms 减 1
    IWDG_SetPrescaler(IWDG_Prescaler_64);
    // 可选的其他分频：
    // IWDG_Prescaler_4   → 10kHz  （0.1ms / 计数）
    // IWDG_Prescaler_256 → 156.25Hz（6.4ms / 计数）
    
    // ===== 第3步：设置重装载值（计数器初值）=====
    // IWDG_SetReload(625) 内部：
    //   IWDG->RLR = value;  （value 的低12位，最大4095）
    // RLR = 625：计数器从625开始递减
    // 减到0的时间 = 625 × 1.6ms = 1000ms = 1秒
    IWDG_SetReload(625);
    // 如果程序在1秒内没有喂狗（写KR=0xAAAA），就会触发复位
    
    // ===== 第4步：喂一次狗，让计数器从 RLR 值开始 =====
    // IWDG_ReloadCounter() 内部：
    //   IWDG->KR = 0xAAAA;
    // 0xAAAA 是喂狗密钥，硬件收到后把 RLR 的值加载到递减计数器
    // 本质：重新开始倒计时
    IWDG_ReloadCounter();
    
    // ===== 第5步：使能 IWDG（开始倒计时）=====
    // IWDG_Enable() 内部：
    //   IWDG->KR = 0xCCCC;
    // 0xCCCC 是使能密钥，写入后：
    //   - IWDG 开始递减计数
    //   - KR 寄存器被锁定（不能再写 0x5555，除非硬件复位）
    //   - LSI 振荡器被强制开启（如果之前没开的话）
    IWDG_Enable();
    // 从这一刻起，IWDG 就永远在跑了，直到芯片掉电或复位
}

/**
 * @brief  喂狗函数
 * @note   必须在超时时间（1秒）内调用，否则系统复位
 *         只能在主循环的 while(1) 中调用！绝对不要在中断里喂狗！
 *         
 *         喂狗的本质：告诉 IWDG "程序还活着，一切正常"
 *         如果程序跑飞了，不会执行到这一行，计数器减到0就复位
 */
// 宏定义方式：零开销，直接展开为一条寄存器写操作
#define IWDG_Feed()  IWDG_ReloadCounter()
// IWDG_ReloadCounter() 内部：
//   IWDG->KR = 0xAAAA;
// 这是一条极其简单的汇编指令（STR），执行时间约 1 个时钟周期
```

**在 main() 中的典型使用**：

```c
int main(void)
{
    // ... 各种初始化 ...
    IWDG_Init();   // 最后一步初始化：开启看门狗
    
    while(1)
    {
        // ===== 喂狗（最重要！主循环每跑一圈就来这里报到）=====
        IWDG_Feed();
        // 这一行必须在1秒内被执行到。如果以下任何代码导致死循环、
        // 阻塞超过1秒，看门狗就会复位系统
        
        // ---- 10ms 任务 ----
        if(sys_time - t10ms >= 10)
        {
            t10ms = sys_time;
            PID_Compute();
        }
        
        // ---- 100ms 任务（比如刷新 OLED）----
        if(sys_time - t100ms >= 100)
        {
            t100ms = sys_time;
            OLED_Refresh();  // 如果这个函数执行超过 1 秒，看门狗会复位
        }
    }
}
```

> **IWDG 电赛使用铁则**：
> 1. **只放在主循环喂狗**：不要在任何中断里喂狗——中断可能正常运行但主循环死了。
> 2. **不要用阻塞延时超过 IWDG 超时**：如果 `Delay_ms(2000)` 超过 IWDG 的 1 秒超时，延时期间会复位。改用非阻塞方式或分批延时。
> 3. **调试时可以先关 IWDG**：在 Keil 调试模式下 IWDG 可能干扰仿真，调试时可以注释掉 `IWDG_Enable()`，但最终提交前一定要打开。
> 4. **IWDG 复位后如何判断？** 读 RCC 的复位标志寄存器 `RCC_GetFlagStatus(RCC_FLAG_IWDGRST)`，返回 SET 说明是看门狗复位的，可以在程序开头做特殊处理（比如显示"系统已恢复"）。

---

### 10.2 窗口看门狗 WWDG —— 更严格的程序运行监控

#### 10.2.1 WWDG 和 IWDG 的区别

| 特性     | IWDG 独立看门狗              | WWDG 窗口看门狗                          |
| -------- | ---------------------------- | ---------------------------------------- |
| 时钟源   | LSI（40kHz 内部 RC）         | PCLK1（APB1 时钟，36MHz）/4096 再分频    |
| 喂狗限制 | 随时可以喂狗（只要在超时前） | **必须在时间窗口内喂狗**，太早太晚都复位 |
| 计数器   | 12 位，递减                  | 7 位（实际用 6 位），递减                |
| 提前警告 | 无                           | 减到 0x40 时产生**提前唤醒中断**（EWI）  |
| 独立性   | 完全独立，不受系统时钟影响   | 依赖 APB1 时钟                           |
| 电赛用途 | 防止死循环（主循环卡死）     | 防止程序时序错乱（比如中断执行时间过长） |

> **WWDG 的"窗口"是什么意思？**
> 
> 计数器从 0x7F（127）开始递减。你只能在它是某个值**以上**时喂狗，不能等它减到太低才喂。这就是"窗口"——一个时间范围。
> 
> 想象一个场景：正常情况下你的主循环每 10ms 跑一圈，计数器大概减到 0x60 的时候喂狗。突然有一天，某个任务的执行时间变长了（比如 50ms），主循环跑一圈要到 50ms，此时计数器减到了 0x30——系统虽然没死，但时序已经异常了。WWDG 可以检测到这种"功能正常但时序异常"的问题。
> 
> **电赛建议**：绝大多数情况下 IWDG 就够了。只有在做安全关键型控制（比如大功率电机、高温加热器）时，才考虑加 WWDG 做双重保护。

#### 10.2.2 WWDG 配置与使用

```c
/**
 * @brief  窗口看门狗初始化
 * @note   WWDG 时钟 = PCLK1(36MHz) / 4096 / 预分频 = 36M/4096/8 ≈ 1098Hz
 *         每个计数周期 ≈ 0.91ms
 *         窗口值 = 0x60(96), 计数器初值 = 0x7F(127)
 *         喂狗窗口：计数器在 0x60~0x7F 之间
 *         超时时间 ≈ 127 × 0.91ms ≈ 116ms
 *         
 *         如果提前喂狗（计数器 > 0x60）→ 不复位（正常）
 *         如果太晚喂狗（计数器 < 0x40）→ 复位！
 *         如果提前到太早（计数器 > 窗口值）→ 也复位！
 */
void WWDG_Init(void)
{
    // ----- 第一步：开 WWDG 时钟 -----
    // WWDG 挂在 APB1 总线上！
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_WWDG, ENABLE);
    
    // ----- 第二步：配置 NVIC（提前唤醒中断，可选）-----
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = WWDG_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    // WWDG 中断不是"复位中断"，而是"提前警告中断"：计数器减到 0x40 时触发
    // 在这个中断里做紧急保存操作（比如保存关键数据到 Flash），然后等复位
    
    // ----- 第三步：配置 WWDG -----
    // 预分频：8 分频
    // WWDG_SetPrescaler 内部：WWDG->CFR 的第 7~8 位
    WWDG_SetPrescaler(WWDG_Prescaler_8);
    // 可选：WWDG_Prescaler_1, _2, _4, _8
    
    // 设置窗口值：0x60（96）
    // 当计数器 > 0x60 时喂狗 → 复位（喂太早了！）
    // 当 0x40 < 计数器 <= 0x60 时喂狗 → 正常
    // 当计数器 < 0x40 时 → 还没喂狗就复位了
    WWDG_SetWindowValue(0x60);
    
    // 使能提前唤醒中断（可选）
    WWDG_EnableIT();  // 计数器减到 0x40 时触发中断
    
    // 设置计数器初值并启动 WWDG
    // 计数器从 0x7F 开始递减
    WWDG_Enable(0x7F);
    // 这一行之后，WWDG 开始计数，不能停止
}

/**
 * @brief  WWDG 提前唤醒中断服务函数
 * @note   计数器减到 0x40 时触发，这是喂狗的"最后通牒"
 *         在这个中断里可以做紧急操作，然后系统会复位
 */
void WWDG_IRQHandler(void)
{
    if(WWDG_GetFlagStatus() != RESET)  // 检查 EWI 标志
    {
        WWDG_ClearFlag();              // 清除中断标志
        
        // 在这里做紧急处理：
        // - 关闭电机输出（安全！）
        // - 保存关键数据到备份寄存器或 Flash
        // - 点亮 LED 指示即将复位
        
        // 中断返回后，如果主循环没有在计数器减到 0x3F 前喂狗，系统复位
    }
}

// 喂狗函数
#define WWDG_Feed()  WWDG_SetCounter(0x7F)  // 重新加载计数器
```

> **WWDG 使用场景**：电赛中如果你做了一个大功率电机控制器，可以用 WWDG 监视主循环的执行时间。如果 PID 计算超时（说明可能卡在某些运算中），WWDG 会在中断中紧急关闭电机 PWM 输出，防止电机失控。

---

### 10.3 RTC 实时时钟 —— 掉电也能继续走时

#### 10.3.1 RTC 是什么？为什么需要它？

RTC（Real-Time Clock，实时时钟）是一个独立于主 CPU 的计时器，专门用来维持日期和时间。它的核心特点：

1. **独立供电**：RTC 由备份域供电，通过 VBAT 引脚连接纽扣电池（如 CR2032）。主电源断电后，只要 VBAT 有电，RTC 继续走时。
2. **独立时钟源**：RTC 通常用外部 32.768kHz 晶振（LSE），精度远高于内部 RC 振荡器。
3. **32 位计数器**：可以计数约 136 年（2^32 / 32768 ≈ 131072 秒 ≈ 136 年），不用担心溢出。
4. **闹钟功能**：可以设置闹钟时间，到点产生中断唤醒系统。

#### 10.3.2 RTC 的硬件结构

```
┌─────────────────────────────────────────────┐
│              备份域（Backup Domain）          │
│  ┌─────────┐    ┌────────────┐              │
│  │ LSE     │    │ RTC 计数器  │              │
│  │ 32.768k │───→│ 32位递增   │──→ 秒中断    │
│  │ 外部晶振 │    │ 从0数到2^32│              │
│  └─────────┘    └────────────┘              │
│  ┌────────────┐  ┌────────────┐             │
│  │ 闹钟寄存器 │  │ 预分频器   │             │
│  │ 可设闹钟   │  │ 异步+同步  │             │
│  └────────────┘  └────────────┘             │
│  ┌────────────┐                             │
│  │ BKP 备份   │  10个16位寄存器             │
│  │ 寄存器     │  掉电不丢失                  │
│  └────────────┘                             │
│         ↑ VBAT 引脚供电（纽扣电池）          │
└─────────────────────────────────────────────┘
```

#### 10.3.3 RTC 的时钟源和预分频

RTC 的时钟源有 3 个选择：
- **LSE**（外部低速晶振）：32.768kHz，精度最高，电赛标配
- **LSI**（内部低速 RC）：约 40kHz，精度差，不需要外部晶振
- **HSE/128**（外部高速晶振 128 分频）：8MHz/128 = 62.5kHz，精度好但功耗高

> **为什么是 32.768kHz？** 因为 32768 = 2^15，用 15 位异步预分频器正好得到 1Hz（1 秒 1 次）的时钟。这种"巧合"不是巧合，是专门设计的。

RTC 的预分频器分两级：
1. **异步预分频器**（7 位）：`RTC->PRLH / PRLL`，通常设为 32767（0x7FFF）
2. **同步预分频器**（15 位）：通常设为 0（不分频）

$$ RTC\_CLK = \frac{LSE}{(PRL + 1)} = \frac{32768}{32767 + 1} = 1\text{Hz} $$

这样 RTC 计数器每秒加 1，非常方便计算时间。

#### 10.3.4 RTC 的配置步骤（重要！）

STM32F103 的 RTC 配置比较特殊，因为它位于备份域，需要特殊操作：

1. 使能电源控制和备份域时钟（`RCC_APB1Periph_PWR | RCC_APB1Periph_BKP`）
2. 取消备份域写保护（`PWR_BackupAccessCmd(ENABLE)`）
3. 复位备份域（如果需要重新配置）
4. 使能 LSE 并等待稳定
5. 选择 RTC 时钟源为 LSE
6. 使能 RTC
7. 配置 RTC 预分频器
8. 设置 RTC 初始时间

> **关键理解**：RTC 和 BKP 都在备份域中，访问它们需要先 `PWR_BackupAccessCmd(ENABLE)`。这相当于一道"门锁"，防止误操作。配置完 RTC 后可以 `PWR_BackupAccessCmd(DISABLE)` 重新锁上。

#### 10.3.5 RTC 完整配置代码（逐行详解）

```c
/**
 * @brief  RTC 初始化，使用外部 32.768kHz 晶振
 * @note   硬件需求：
 *         - PC14 和 PC15 接 32.768kHz 晶振（这两个引脚是 RTC 专用的）
 *         - VBAT 引脚接纽扣电池正极（保证掉电后 RTC 继续走）
 *         - 如果没有外部晶振，改用 LSI 作为时钟源
 *         
 *         配置后 RTC 每秒钟计数器加 1，可以用它来计算时间
 */
void RTC_Init(void)
{
    // ===== 第0步：开启 GPIO 时钟（PC14/PC15 是 RTC 晶振引脚）=====
    // 如果用 LSE（外部晶振），PC14 和 PC15 必须配置好
    // 这两个引脚在 GPIO 的默认状态就是给 RTC 用的，一般不需要额外配置 GPIO
    // 但为了保险，可以显式配置一次
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    
    // ===== 第1步：使能 PWR（电源控制）和 BKP（备份寄存器）时钟 =====
    // 这两个外设都挂在 APB1 总线上
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
    
    // ===== 第2步：解除备份域写保护 =====
    // PWR_BackupAccessCmd(ENABLE) 内部：
    //   PWR->CR |= PWR_CR_DBP;   // 设置 DBP（Disable Backup Protection）位
    // DBP 位 = 1 允许访问 RTC 和备份寄存器
    // DBP 位 = 0 禁止访问（默认状态，保护备份域数据不被意外修改）
    PWR_BackupAccessCmd(ENABLE);
    
    // ===== 第3步：复位备份域（可选的，如果之前配置过 RTC 需要先复位）=====
    // BKP_DeInit() 内部：
    //   RCC->BDCR |= RCC_BDCR_BDRST;  // 备份域复位
    //   等待一段时间
    //   RCC->BDCR &= ~RCC_BDCR_BDRST; // 取消复位
    // 注意：这会清除 RTC 计数器值！如果之前 RTC 在走，会丢失时间
    // 只在第一次配置或需要重新配置时调用
    // BKP_DeInit();
    
    // ===== 第4步：使能 LSE（外部低速晶振）=====
    // RCC_LSEConfig(RCC_LSE_ON) 内部：
    //   RCC->BDCR |= RCC_BDCR_LSEON;  // 开启 LSE 振荡器
    // LSE 是 32.768kHz 的外部晶振，启动时间较长（约1秒）
    RCC_LSEConfig(RCC_LSE_ON);
    
    // 等待 LSE 稳定就绪
    // RCC_GetFlagStatus(RCC_FLAG_LSERDY) 检查 BDCR 的 LSERDY 位
    // LSERDY=1：LSE 已经稳定，可以使用
    // LSERDY=0：LSE 还在起振或未开启
    uint32_t timeout = 0;
    while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
    {
        timeout++;
        if(timeout > 5000000)  // 等待约500ms（粗略延时）
            break;              // 超时就放弃，可能没焊晶振
    }
    // 如果超时退出，说明 LSE 没起振——检查：
    //   1. 32.768kHz 晶振焊没焊？
    //   2. 晶振的负载电容（通常是 6pF~12.5pF）焊没焊？
    //   3. PC14/PC15 有没有被其他外设占用？
    
    // ===== 第5步：选择 RTC 时钟源为 LSE =====
    // RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE) 内部：
    //   RCC->BDCR &= ~RCC_BDCR_RTCSEL;  // 清除时钟源选择位
    //   RCC->BDCR |= RCC_BDCR_RTCSEL_LSE; // 选择 LSE（0x00000100）
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
    // 可选：
    // RCC_RTCCLKSource_LSI    → 内部 40kHz（精度差，但不需要晶振）
    // RCC_RTCCLKSource_HSE_Div128 → HSE/128 = 62.5kHz
    
    // ===== 第6步：使能 RTC =====
    // RCC_RTCCLKCmd(ENABLE) 内部：
    //   RCC->BDCR |= RCC_BDCR_RTCEN;
    // 使能后 RTC 开始工作，但此时还没配预分频，计数器递增很快
    RCC_RTCCLKCmd(ENABLE);
    
    // ===== 第7步：等待 RTC 寄存器同步 =====
    // 操作 RTC 寄存器前必须先等待同步完成
    // RTC_WaitForSynchro() 内部循环检查 RTC->CRL 的 RSF 位
    // RSF=1：寄存器已同步，可以安全读写
    // 为什么要同步？RTC 在备份域有自己的时钟域，与 APB1 时钟域不同步
    RTC_WaitForSynchro();
    
    // ===== 第8步：等待上一次写操作完成 =====
    // 写 RTC 寄存器后必须等 RTOFF 位变 1，才能进行下一次写操作
    // RTC_WaitForLastTask() 内部循环检查 RTC->CRL 的 RTOFF 位
    RTC_WaitForLastTask();
    
    // ===== 第9步：设置 RTC 预分频器 =====
    // 目的：将 32768Hz 分频到 1Hz（每秒计数器加1）
    // RTC_SetPrescaler(32767) 内部：
    //   RTC->PRLH = (32767 >> 16) & 0x0F;  // 高4位
    //   RTC->PRLL = 32767 & 0xFFFF;         // 低16位
    // 预分频器的实际值是 32767，分频后频率 = 32768 / (32767+1) = 1Hz
    RTC_SetPrescaler(32767);  // 异步预分频值，得到 1Hz 的 RTC 时钟
    RTC_WaitForLastTask();    // 等待写完成
    
    // ===== 第10步：设置 RTC 计数器初始值 =====
    // RTC_SetCounter(0) 内部：
    //   RTC->CNTH = 0;
    //   RTC->CNTL = 0;
    // 计数器从 0 开始，每秒钟加 1
    RTC_SetCounter(0);
    RTC_WaitForLastTask();
    
    // ===== 第11步（可选）：配置闹钟 =====
    // 设置闹钟值，当计数器等于闹钟值时触发 RTC 闹钟中断
    // RTC_SetAlarm(3600);  // 1小时后触发闹钟（3600秒）
    // RTC_WaitForLastTask();
    
    // ===== 第12步（可选）：配置 RTC 秒中断 =====
    // 使能 RTC 秒中断：每秒钟触发一次中断
    RTC_ITConfig(RTC_IT_SEC, ENABLE);  // 秒中断
    // RTC_IT_ALR: 闹钟中断
    // RTC_IT_OW:  溢出中断（计数器溢出，约136年一次...）
    RTC_WaitForLastTask();
    
    // ===== 第13步：配置 NVIC =====
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = RTC_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // ===== 第14步：重新锁上备份域（可选）=====
    // PWR_BackupAccessCmd(DISABLE);
    // 锁上后不能读写 RTC 和 BKP，但 RTC 会继续走
    // 需要改时间的时候再 ENABLE 即可
}

/**
 * @brief  RTC 秒中断服务函数
 * @note   每秒钟触发一次（因为预分频设为 1Hz）
 */
void RTC_IRQHandler(void)
{
    // 检查是否是秒中断
    if(RTC_GetITStatus(RTC_IT_SEC) != RESET)
    {
        RTC_ClearITPendingBit(RTC_IT_SEC);  // 清除秒中断标志
        
        // 在这里更新时间显示变量
        // 例如：rtc_second = RTC_GetCounter() % 60;
        //       rtc_minute = (RTC_GetCounter() / 60) % 60;
        //       rtc_hour   = (RTC_GetCounter() / 3600) % 24;
        rtc_update_flag = 1;  // 通知主循环更新时间显示
    }
    
    // 检查是否是闹钟中断
    if(RTC_GetITStatus(RTC_IT_ALR) != RESET)
    {
        RTC_ClearITPendingBit(RTC_IT_ALR);  // 清除闹钟中断标志
        // 闹钟响了！做相应的处理
    }
}

/**
 * @brief  获取当前 RTC 计数值
 * @return 从 RTC 初始化开始经过的秒数
 * @note   32位无符号整数，最大 4294967295 秒 ≈ 136 年
 */
uint32_t RTC_GetTime(void)
{
    return RTC_GetCounter();  // 读取 RTC->CNTH:CNTL
}

/**
 * @brief  设置 RTC 时间
 * @param  seconds: 要设置的秒数（从某个基准时间开始）
 * @note   必须先 PWR_BackupAccessCmd(ENABLE) 解锁备份域
 */
void RTC_SetTime(uint32_t seconds)
{
    PWR_BackupAccessCmd(ENABLE);
    RTC_WaitForLastTask();
    RTC_SetCounter(seconds);
    RTC_WaitForLastTask();
    // 不锁备份域，方便后续读写
}
```

> **RTC 使用电赛经验**：
> 1. **必须接纽扣电池**：VBAT 引脚接 CR2032 纽扣电池正极，负极接地。推荐加一个 1N4148 二极管防止电池反接。
> 2. **LSE 晶振必须接负载电容**：32.768kHz 晶振的两个脚各接一个 6~12.5pF 的电容到地。不接电容可能不起振。
> 3. **RTC 初始化只做一次**：如果 VBAT 有电池，掉电后 RTC 还在走，不需要每次上电都重新初始化。通过检查 BKP 里的一个标志位来判断是不是首次上电。
> 4. **电赛通常用 sys_time 就够了**：除非题目明确要求"显示当前时间"或"定时功能（小时级别）"，否则用 SysTick 维护的 sys_time 更方便。

---

### 10.4 BKP 备份寄存器 —— 掉电不丢失的"保险箱"

#### 10.4.1 BKP 是什么？

BKP（Backup Registers，备份寄存器）是备份域中的 10 个 16 位寄存器（`BKP_DR1` 到 `BKP_DR10`），它们的特点是：

- **VBAT 供电**：只要 VBAT 引脚有纽扣电池，主电源断电后数据不丢失
- **防意外写入**：需要 `PWR_BackupAccessCmd(ENABLE)` 解锁后才能写
- **系统复位不影响**：按复位按钮或看门狗复位不会清除 BKP 里的数据
- **只有备份域复位才会清除**：调用 `BKP_DeInit()` 或芯片完全掉电（VBAT 也没电）

#### 10.4.2 BKP 的使用场景

电赛中 BKP 非常有用：
1. **存储校准参数**：比如 PID 参数、传感器零点校准值，不需要每次都重新调
2. **存储设备地址**：比如 I2C 地址、通信频道
3. **存储上电次数**：记录设备被使用过多少次
4. **判断复位原因**：正常上电 vs 看门狗复位 vs 软件复位
5. **RTC 首次配置标志**：判断 RTC 是否已经配过（避免每次都重新设时间）

#### 10.4.3 BKP 读写代码

```c
/**
 * @brief  向备份寄存器写入数据
 * @param  reg: 备份寄存器编号 1~10
 * @param  data: 要写入的 16 位数据
 */
void BKP_Write(uint8_t reg, uint16_t data)
{
    if(reg < 1 || reg > 10) return;  // 范围检查
    
    PWR_BackupAccessCmd(ENABLE);      // 解锁备份域
    
    // BKP_WriteBackupRegister 内部：
    //   根据 reg 编号，写入 BKP->DR1 到 BKP->DR10 之一
    //   本质就是给特定地址的 16 位寄存器赋值
    BKP_WriteBackupRegister(reg, data);
    
    // 不锁，方便下次读写（锁了能省电，但电赛不在乎这点功耗）
}

/**
 * @brief  从备份寄存器读取数据
 * @param  reg: 备份寄存器编号 1~10
 * @return 16位数据
 */
uint16_t BKP_Read(uint8_t reg)
{
    if(reg < 1 || reg > 10) return 0;
    return BKP_ReadBackupRegister(reg);
}

// ===== 实际应用：判断是否首次上电 =====
#define BKP_FIRST_FLAG_REG  1    // 用 DR1 存储"已配置"标志
#define BKP_CONFIGURED      0xA5A5 // 魔数（magic number），表示"系统已初始化过"

uint8_t IsFirstBoot(void)
{
    // 读 DR1，如果值为 0xA5A5 说明之前已配置过 RTC
    if(BKP_Read(BKP_FIRST_FLAG_REG) == BKP_CONFIGURED)
    {
        return 0;  // 不是首次上电，RTC 还在走，不需要重新配置
    }
    else
    {
        // 首次上电！需要完整初始化 RTC
        BKP_Write(BKP_FIRST_FLAG_REG, BKP_CONFIGURED);  // 写入标志
        return 1;
    }
}

// 在 main() 中使用：
int main(void)
{
    // ...基础初始化...
    
    if(IsFirstBoot())
    {
        // 首次上电：需要完整初始化 RTC，设置初始时间
        RTC_Init();
        RTC_SetTime(0);  // 从 0 开始计时
    }
    else
    {
        // 非首次上电：RTC 还在走，只需要使能中断等
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
        PWR_BackupAccessCmd(ENABLE);
        RTC_WaitForSynchro();
        // RTC 已经在运行，不用再设计数器
    }
    
    // ...
}
```

> **BKP 使用注意事项**：
> - 只有 10 个寄存器，每个 16 位，总共 20 字节。想存大量数据（如字库、波形数据）必须用 Flash 或外部 EEPROM。
> - 修改备份域需要先 `PWR_BackupAccessCmd(ENABLE)`，这不是"开时钟"，而是解除硬件锁。
> - 用"魔数"（如 0xA5A5）来判断数据是否有效是嵌入式常用技巧——未初始化的存储单元不太可能恰好是 0xA5A5。

---

### 10.5 STM32 低功耗模式 —— 电池供电的题目的标配

电赛中有一些题目要求作品用电池供电并工作特定时长，这就需要低功耗模式。

#### 10.5.1 三种低功耗模式对比

| 特性             | Sleep 睡眠模式     | Stop 停止模式               | Standby 待机模式           |
| ---------------- | ------------------ | --------------------------- | -------------------------- |
| **唤醒方式**     | 任何中断/事件      | EXTI 线或 RTC 闹钟          | WKUP 引脚、RTC 闹钟、IWDG  |
| **CPU 状态**     | 停止运行           | 停止运行                    | 停止运行                   |
| **SRAM 数据**    | **保留**           | **保留**                    | **丢失**                   |
| **外设状态**     | 所有外设正常工作   | 所有外设停止（除 EXTI/RTC） | 所有外设停止               |
| **I/O 状态**     | 保持               | 保持                        | 高阻态                     |
| **唤醒后**       | 继续执行下一条指令 | 从停止处继续，需重新配时钟  | **等于复位，重新执行main** |
| **功耗（典型）** | ~5mA（72MHz 下）   | ~20μA                       | ~2μA                       |
| **唤醒时间**     | 立即（0 周期）     | ~5μs（HSI 启动时间）        | ~50μs（等于上电复位时间）  |

> **电赛选型指南**：
> - **Sleep 模式**：基本不用，功耗降低不多
> - **Stop 模式**：**电赛最常用**。功耗极低（μA 级），唤醒后数据不丢失，适合定时唤醒采集数据后继续待机
> - **Standby 模式**：功耗最低但 RAM 全丢，等于复位。适合超长待机+不需要保留状态的场景

#### 10.5.2 Stop 模式 — 电赛最实用的低功耗模式

Stop 模式下所有时钟停止，CPU 停止，SRAM 保持，通过外部中断或 RTC 闹钟唤醒。

**典型场景**：电池供电的数据采集器，每 5 秒醒来采集一次数据，其余时间在 Stop 模式休眠。

```c
/**
 * @brief  进入 STOP 模式
 * @note   进入前必须：
 *         1. 关掉不必要的外设时钟（省电）
 *         2. 设置好唤醒源（EXTI 或 RTC 闹钟）
 *         3. 调用此函数
 *         
 *         唤醒后：
 *         1. HSI 自动被选为系统时钟（8MHz）
 *         2. 需要重新配置时钟树（调 SystemInit() 恢复 72MHz）
 *         3. 需要重新使能用到 HSE/PLL 的外设
 *         
 *         唤醒源示例：
 *         - RTC 闹钟：适合定时唤醒（比如每 5 秒采一次数据）
 *         - EXTI 外部中断：适合外部信号唤醒（比如按键、传感器触发）
 */
void Enter_StopMode(void)
{
    // ===== 进入 Stop 模式前的准备工作 =====
    
    // 1. 关掉耗电的外设（比如 ADC、SPI、不必要的 GPIO 时钟）
    ADC_Cmd(ADC1, DISABLE);
    // 也可以关掉所有不用的外设时钟
    // RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, DISABLE);
    
    // 2. 把所有 GPIO 设为模拟输入（AIN），降低 GPIO 功耗
    // 这是最低功耗的 GPIO 状态，施密特触发器关闭
    // 但不方便，一般省略
    
    // 3. 设置好唤醒源（假设用 RTC 闹钟，5 秒后唤醒）
    // RTC_SetAlarm(RTC_GetCounter() + 5);
    // RTC_ITConfig(RTC_IT_ALR, ENABLE);
    
    // ===== 进入 Stop 模式 =====
    // PWR_EnterSTOPMode() 内部做了三件事：
    // 1. 设置 PWR->CR 的 PDDS 位 = 0（选择 Stop 模式，不是 Standby）
    // 2. 设置 PWR->CR 的 LPDS 位 = 0（电压调节器在 Stop 模式下正常工作，唤醒更快）
    //    LPDS=1 电压调节器低功耗模式（唤醒慢一点但更省电）
    // 3. 执行 WFI（Wait For Interrupt）指令
    //    WFI 指令暂停 CPU，直到有中断请求
    PWR_EnterSTOPMode(PWR_Regulator_ON, PWR_STOPEntry_WFI);
    // 参数解析：
    // PWR_Regulator_ON：电压调节器保持开启（唤醒快，约5μs）
    // PWR_STOPEntry_WFI：通过 WFI 指令进入，任何 EXTI 中断都能唤醒
    // PWR_STOPEntry_WFE：通过 WFE 指令进入，需要事件唤醒（中断要配置为事件模式）
    
    // ===== 从这里开始是唤醒后的代码 =====
    // CPU 被唤醒后从 WFI 指令的下一条开始执行
    
    // 唤醒后系统时钟变成了 HSI（8MHz），需要重新配置
    // 如果你的程序调用了 SystemInit() 配置 72MHz，这里需要重新调用
    // 通常的做法是：
    //   1. 在进入 Stop 前保存关键外设状态
    //   2. 唤醒后重新配置时钟树
    //   3. 重新使能被关闭的外设
}

/**
 * @brief  Standby 模式（备用）
 * @note   功耗最低，但唤醒 = 复位
 *         不会执行到 return 后面的代码
 */
void Enter_StandbyMode(void)
{
    // 使能电源控制时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    // 使能 WKUP 引脚唤醒（PA0，固定引脚）
    PWR_WakeUpPinCmd(ENABLE);
    // 进入 Standby 模式
    PWR_EnterSTANDBYMode();
    // 这一行实际上永远不会被执行到
    // 因为从 Standby 唤醒等于芯片复位，会从启动文件重新执行
}

/**
 * @brief  低功耗模式下的系统架构示例（定时采集 + 休眠）
 * 
 * 场景：电池供电的温度采集器，每 5 秒采集一次并存储
 */
void LowPower_MainLoop(void)
{
    static uint32_t last_wake = 0;
    
    while(1)
    {
        // ---- 检查是否是 RTC 闹钟唤醒 ----
        if(PWR_GetFlagStatus(PWR_FLAG_WU) != RESET)  // 待机唤醒标志
        {
            PWR_ClearFlag(PWR_FLAG_WU);
            // 这是从待机唤醒的，执行完整初始化
        }
        
        // ---- 正常工作：采集数据 ----
        float temp = Read_Temperature();     // 读温度传感器
        OLED_ShowFloat(0, 0, temp);          // 显示
        Save_To_Flash(temp);                 // 存到 Flash
        
        // ---- 设置下次唤醒时间 ----
        RTC_SetAlarm(RTC_GetCounter() + 5);  // 5 秒后唤醒
        
        // ---- 进入 Stop 模式 ----
        // 在进入前，关掉不必要的外设
        ADC_Cmd(ADC1, DISABLE);
        Enter_StopMode();
        
        // ---- 唤醒后重新配置 ----
        SystemInit();  // 恢复 72MHz 时钟
        ADC_Cmd(ADC1, ENABLE);  // 重新开 ADC
        
        // 继续 while 循环，采集下一次数据
    }
}
```

> **低功耗电赛经验**：
> 1. **Stop 模式是电赛最佳选择**：功耗低、唤醒快、数据不丢、配置简单。
> 2. **一定要实测功耗**：用万用表电流档测 VCC 的电流。如果功耗只有几十 μA，说明 Stop 模式生效了；如果还有几 mA，检查是否有外设没有关。
> 3. **唤醒源要配好**：别进入了 Stop 模式却发现没有配置任何唤醒源——那样只能按复位键了。
> 4. **GPIO 状态处理**：Stop 模式下 GPIO 保持退出前的状态。如果某个 GPIO 输出高驱动 LED，LED 在 Stop 模式下仍然亮着！进入 Stop 前把不用的 GPIO 设为模拟输入最省电。

---

### 10.6 复位原因判断 —— 知道系统是怎么重启的

电赛中系统可能因多种原因复位，能区分复位原因对调试非常重要。

```c
/**
 * @brief  获取复位原因
 * @return 复位原因字符串
 * @note   调用 RCC_GetFlagStatus 检查各个复位标志
 *         检查完后应调用 RCC_ClearFlag() 清除所有标志，
 *         否则下次复位时读的是旧标志
 */
const char* GetResetSource(void)
{
    // RCC_GetFlagStatus 读取 RCC->CSR 控制/状态寄存器的各个标志位
    
    if(RCC_GetFlagStatus(RCC_FLAG_PORRST) != RESET)
        return "上电复位(Power On)";       // 芯片刚上电
    // PORRST = Power-On Reset
    
    if(RCC_GetFlagStatus(RCC_FLAG_PINRST) != RESET)
        return "引脚复位(NRST)";            // NRST 引脚被拉低（按了复位按钮）
    // PINRST = Pin Reset
    
    if(RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET)
        return "独立看门狗复位(IWDG)";      // IWDG 超时
    // IWDGRST = Independent Watchdog Reset
    
    if(RCC_GetFlagStatus(RCC_FLAG_WWDGRST) != RESET)
        return "窗口看门狗复位(WWDG)";      // WWDG 喂狗不在窗口内
    
    if(RCC_GetFlagStatus(RCC_FLAG_SFTRST) != RESET)
        return "软件复位";                  // 调用了 NVIC_SystemReset() 或 写 AIRCR 寄存器
    // SFTRST = Software Reset
    
    if(RCC_GetFlagStatus(RCC_FLAG_LPWRRST) != RESET)
        return "低功耗复位(Standby唤醒)";    // 从 Standby 模式唤醒（等于复位）
    // LPWRRST = Low Power Reset
    
    return "未知复位原因";
}

// 在 main() 开头使用：
int main(void)
{
    // 获取复位原因（在所有初始化之前！）
    const char* reset_reason = GetResetSource();
    
    // 清除所有复位标志（为下次检测做准备）
    RCC_ClearFlag();
    
    // 根据复位原因做不同处理
    if(strstr(reset_reason, "看门狗"))
    {
        // 上次是看门狗复位的！说明程序跑飞过！
        // 可以在 OLED 上显示警告，或者把错误信息存入 BKP
        OLED_ShowString(0, 0, "WDT RESET!", 16);
        Delay_ms(2000);
    }
    
    // ... 正常初始化 ...
}
```

> **复位原因判断的重要性**：
> 电赛调试时，如果发现系统偶尔重启，用这个方法可以立刻知道是电源不稳定（上电复位）、还是程序卡死（看门狗复位）、还是按了复位键。定位问题的效率提升 10 倍。

---

## 第十一章 电赛常用模块驱动全解
这一章我会把电赛最常用的模块驱动都给你标准代码，都是经过无数电赛验证过的稳定代码，你可以直接拿来用。

### 11.1 0.96寸OLED显示模块 —— 深入原理与逐行详解

0.96 寸 OLED 是电赛最常用的显示屏。分辨率 128×64 像素，I2C 接口（也支持 SPI），只需要 2 根线，比 LCD1602 灵活得多。

#### 11.1.1 OLED 是如何工作的？（深入硬件原理）

这块 OLED 的核心是 **SSD1306** 驱动芯片。你需要理解它的几个核心概念，才能真正读懂驱动代码。

**像素和页（Page）的组织方式**：

SSD1306 把 128×64 的屏幕分成 8 个"页"（Page 0~7），每页 8 行像素，每行 128 列。

```
        列 0 ────────────────────────→ 列 127
     ┌────────────────────────────────────┐
页0  │  (0,0)                    (127,0)  │  ← 第 0~7 行像素
页1  │  (0,8)                    (127,8)  │  ← 第 8~15 行像素
页2  │                                    │
...  │         128×64 = 8192 像素         │
页6  │                                    │
页7  │ (0,56)                    (127,56) │  ← 第 56~63 行像素
     └────────────────────────────────────┘
```

**关键认知**：屏幕的最小写入单位不是"一个像素"，而是"一列的 8 个像素"（即 1 个字节，1 byte = 8 bit，每个 bit 对应一行像素）。

```
一列上的 8 个像素（在某一页内）：
  bit 0 → 这一页的第 0 行像素（最上面）
  bit 1 → 这一页的第 1 行像素
  bit 2 → 这一页的第 2 行像素
  ...
  bit 7 → 这一页的第 7 行像素（最下面）

例如：往某页某列写入 0xFF（二进制 1111 1111）
      → 这一页的这 8 行像素全部点亮
例如：往某页某列写入 0x00（二进制 0000 0000）
      → 这一页的这 8 行像素全部熄灭
例如：往某页某列写入 0x01（二进制 0000 0001）
      → 只有这一页的最上面一行点亮，其余 7 行不亮
```

> **这就是 OLED 显示的本质**：屏幕上的每一个像素对应 GDDRAM（图形显示数据 RAM）中的某一个 bit。bit=1 像素亮，bit=0 像素灭。你要做的事就是把正确的 bit 模式写入正确的 GDDRAM 位置。

**GDDRAM（图形显示数据 RAM）**：

SSD1306 内部有 128×64 bit = 1024 字节的显存。屏幕坐标 (x, y) 对应的 GDDRAM 位是：
- Page = y / 8（y 除以 8 取整，即 y >> 3）
- 页内行 = y % 8（y 除以 8 的余数，即 y & 0x07，对应 byte 中的第几位）

例如：坐标 (10, 20) → Page = 20/8 = 2，页内行 = 20%8 = 4，即第 2 页第 10 列的第 4 bit。

#### 11.1.2 SSD1306 的 I2C 通信协议

SSD1306 的 I2C 地址是 **0x3C**（7 位地址，写操作 = 0x78，读操作 = 0x79）。有些模块把地址电阻接法不同，可能是 0x3D（写 = 0x7A）。

**控制字节（Control Byte）**：每次 I2C 传输，在数据之前要发一个控制字节，告诉 SSD1306 后续数据是命令还是显示数据。

```
控制字节 = 0x00 → 后续字节是"命令"（Command）
            = 0x40 → 后续字节是"数据"（Data，写入 GDDRAM）

命令：控制 OLED 的行为（开关显示、设置位置、对比度等）
数据：写入 GDDRAM 的像素数据
```

#### 11.1.3 OLED 驱动核心代码 —— 逐行详解

**头文件 `oled.h`**：

```c
#ifndef __OLED_H       // 如果没有定义过 __OLED_H
#define __OLED_H       // 就定义它
// 这是头文件保护，防止同一个头文件被多次包含导致重复定义错误
// 每个 .h 文件的标准开头

#include "stm32f10x.h"  // 包含标准库头文件，这样才能用 uint8_t 等类型

// 函数声明：告诉编译器这些函数存在，实际定义在 oled.c 中
void OLED_Init(void);       // 初始化 OLED（上电配置序列）
void OLED_Clear(void);      // 清屏（全屏变黑）
void OLED_ShowChar(uint8_t x, uint8_t y, char ch, uint8_t size);
// 在(x,y)显示一个字符，size=12(6×8字体)或16(8×16字体)
void OLED_ShowString(uint8_t x, uint8_t y, char* str, uint8_t size);
// 在(x,y)显示字符串
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size);
// 在(x,y)显示数字，len=显示几位（不足前面填空格）
void OLED_ShowFloat(uint8_t x, uint8_t y, float num, uint8_t intLen, uint8_t decLen, uint8_t size);
// 显示浮点数，intLen=整数位数, decLen=小数位数
void OLED_ShowChinese(uint8_t x, uint8_t y, uint8_t index);
// 显示中文（需要中文字库），index=字库中的索引
void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t mode);
// 画一个点，mode=1 点亮，mode=0 熄灭
void OLED_Refresh(void);    // 刷新屏幕（如果用了帧缓冲）

#endif  // 结束头文件保护
```

**源文件 `oled.c` —— 向 OLED 写入一个字节**：

```c
/**
 * @brief  向 SSD1306 写入一个命令字节或数据字节
 * @param  dat: 要写入的数据字节
 * @param  cmd: 0=命令模式（控制 OLED 行为），1=数据模式（写入 GDDRAM 像素）
 * @note   I2C 通信序列：
 *         Start → 设备地址(0x78) → ACK → 控制字节(0x00或0x40) → ACK
 *              → 数据字节 → ACK → Stop
 */
void OLED_WR_Byte(uint8_t dat, uint8_t cmd)
{
    I2C_Start();                          // 1. I2C 起始信号
    
    I2C_SendByte(0x78);                   // 2. 发送 OLED 的 I2C 设备地址（写操作）
    // 0x78 = (0x3C << 1) | 0（7位地址 0x3C 左移1位，最低位=0 表示写）
    I2C_WaitAck();                        // 3. 等待 OLED 应答
    
    if(cmd)
        I2C_SendByte(0x40);               // 4a. cmd=1 → 发送控制字节 0x40（数据模式）
    else
        I2C_SendByte(0x00);               // 4b. cmd=0 → 发送控制字节 0x00（命令模式）
    // 控制字节的含义：
    //   Co(bit7) D/C#(bit6) 后面的bits...
    //   0x00 = Co=0, D/C#=0 → 下一个字节是命令，且只此一个命令字节
    //   0x40 = Co=0, D/C#=1 → 下一个字节是数据，写入 GDDRAM
    //   0x80 = Co=1, D/C#=0 → 后续多个字节都是命令（连续发命令时用，我们不用）
    I2C_WaitAck();                        // 5. 等待 OLED 应答
    
    I2C_SendByte(dat);                    // 6. 发送实际数据/命令字节
    I2C_WaitAck();                        // 7. 等待应答
    
    I2C_Stop();                           // 8. I2C 停止信号
    // 整个函数用时约 100μs（I2C 100kHz 模式下）
}
```

**设置光标位置**：

```c
/**
 * @brief  设置 GDDRAM 的写入位置（光标）
 * @param  x: 列地址（0~127）
 * @param  y: 页地址（0~7），注意 y 是页号不是像素行号！
 * @note   SSD1306 的寻址：先设页地址（哪一页），再设列地址（这一页的哪一列）
 *         页地址 = y 坐标（像素行号）/ 8
 */
void OLED_SetPos(uint8_t x, uint8_t y)
{
    // 第一条命令：设置页地址
    // 0xB0~0xB7 对应 Page 0~Page 7
    // 0xB0 + y：y=0 → 0xB0(页0)，y=1 → 0xB1(页1)，...，y=7 → 0xB7(页7)
    OLED_WR_Byte(0xB0 + y, OLED_CMD);  // OLED_CMD=0，命令模式
    // [宏定义] #define OLED_CMD  0  // 命令模式
    // [宏定义] #define OLED_DATA 1  // 数据模式
    
    // 第二条命令：设置列地址高 4 位
    // SSD1306 的列地址（0~127）分两次发送：先发高 4 位，再发低 4 位
    // 列地址高 4 位命令的基础值是 0x10
    // x & 0xF0：取出 x 的高 4 位（例如 x=0x5A(90)，高4位=0x50）
    // >> 4：把高 4 位移到低 4 位（0x50 >> 4 = 0x05）
    // | 0x10：拼上列地址高 4 位的命令前缀（0x05 | 0x10 = 0x15）
    OLED_WR_Byte(((x & 0xF0) >> 4) | 0x10, OLED_CMD);
    // 举例：x=90（二进制 0101 1010）
    //   x & 0xF0 = 0x50（0101 0000）
    //   >> 4      = 0x05（0000 0101）
    //   | 0x10    = 0x15（0001 0101）→ 发给 SSD1306
    
    // 第三条命令：设置列地址低 4 位
    // 列地址低 4 位命令的基础值是 0x00
    // x & 0x0F：取出 x 的低 4 位（0x5A & 0x0F = 0x0A）
    OLED_WR_Byte(x & 0x0F, OLED_CMD);
    // | 0x00 可省略（|0 不影响结果）
    // 举例：x=90，低4位=0x0A → 发 0x0A
}
```

**清屏函数**：

```c
/**
 * @brief  清除整个屏幕（全黑）
 * @note   遍历 8 个页（每页 128 列），每列写入 0x00（8 个像素全灭）
 *         8×128 = 1024 次写入，I2C 100kHz 下约 100ms
 */
void OLED_Clear(void)
{
    uint8_t i, j;  // i=页号, j=列号
    
    for(i = 0; i < 8; i++)           // 遍历 8 个页（Page 0~7）
    {
        OLED_WR_Byte(0xB0 + i, OLED_CMD);  // 设置页地址 = i
        OLED_WR_Byte(0x00, OLED_CMD);      // 设置列地址低4位 = 0
        OLED_WR_Byte(0x10, OLED_CMD);      // 设置列地址高4位 = 0
        // 以上三条 = OLED_SetPos(0, i) 的效果（设置到第 i 页、第 0 列）
        // 为什么这里不直接调 OLED_SetPos？
        // 因为调函数有额外开销，清屏追求速度，直接写寄存器更快
        
        for(j = 0; j < 128; j++)           // 遍历这一页的 128 列
        {
            OLED_WR_Byte(0x00, OLED_DATA); // 写入 0x00 = 这一列的 8 个像素全灭
            // OLED_DATA=1，告诉 SSD1306 后续是 GDDRAM 数据
        }
        // 列地址会自动 +1（SSD1306 硬件自动递增），所以不需要每次都设列地址
        // 这是 SSD1306 的贴心设计，否则 1024 次写入都要先设地址就太慢了
    }
}
```

**显示一个字符（核心中的核心）**：

```c
/**
 * @brief  在指定位置显示一个 ASCII 字符
 * @param  x: 横坐标（列，0~127，单位：像素）
 * @param  y: 纵坐标（行，0~7，单位：页！不是像素！）
 * @param  ch: 要显示的字符（ASCII 码）
 * @param  size: 字体大小，12→6×8像素, 16→8×16像素
 * @note   字体数据来自字库数组（F6x8 或 F8X16）
 *         字库中每个字符占用的字节数：
 *           6×8字体：6 列 × 1 页 = 6 字节（每列一个字节，共 1 页高）
 *           8×16字体：8 列 × 2 页 = 16 字节（每列一个字节，占 2 页高）
 */
void OLED_ShowChar(uint8_t x, uint8_t y, char ch, uint8_t size)
{
    uint8_t c = 0, i = 0;  // c=字符在字库中的索引, i=循环计数器
    
    // 计算字符在字库中的偏移索引
    // 字库从空格（ASCII 32, ' '）开始存储，可打印字符从 ' ' 到 '~'
    // ch - ' ' 得到 ch 是字库中的第几个字符
    c = ch - ' ';
    // 例如：'A' 的 ASCII 是 65，c = 65 - 32 = 33
    //       即 'A' 是字库中的第 33 个字符（从 0 开始数）
    
    // 自动换行：如果横坐标超出屏幕右边，跳到下一行开头
    if(x > 128 - 1)  // 128-1=127，x 最大允许值就是 127（最右边一列）
    {
        x = 0;        // 回到最左边
        y++;          // 跳到下一行
    }
    // 注意：这里 y 是页号（0~7），不是像素行号
    
    // ===== 8×16 大字体（占 2 页高）=====
    if(size == 16)
    {
        // 先画上半部分（第 y 页）
        OLED_SetPos(x, y);               // 设置光标到 (x, y)
        for(i = 0; i < 8; i++)           // 8 列数据（每列是字体的一竖排）
        {
            // F8X16 是 16 字节高的字库数组，每个字符占 16 字节
            // c*16 定位到第 c 个字符的起始位置
            // +i 定位到这个字符的第 i 列（前 8 列是上半部分）
            OLED_WR_Byte(F8X16[c * 16 + i], OLED_DATA);
            // F8X16[c*16+i] 是什么？
            // 是第 c 个字符的第 i 列上半部分的 8 个像素（bit0~bit7对应页内行0~7）
            // 例如字符 'A'：
            //   第0列上半：0x00（0000 0000）← 左边空白
            //   第1列上半：0x0C（0000 1100）← 有点亮
            //   第2列上半：0x1E（0001 1110）
            //   ...
            //   之所以先画上半再画下半，是因为 OLED 按页组织，
            //   8×16 的字符需要跨两页（上半在一页，下半在下一页）
        }
        
        // 再画下半部分（第 y+1 页）
        OLED_SetPos(x, y + 1);           // 光标下移一页（8 像素）
        for(i = 0; i < 8; i++)
        {
            // +i+8 跳过前 8 个字节（上半），取后 8 个字节（下半）
            OLED_WR_Byte(F8X16[c * 16 + i + 8], OLED_DATA);
        }
    }
    // ===== 6×8 小字体（占 1 页高）=====
    else
    {
        // 12 号字体（实际 6×8 点阵）
        OLED_SetPos(x, y);
        for(i = 0; i < 6; i++)           // 6 列（比 16 号字体窄 2 列）
        {
            // F6x8 是二维数组：F6x8[字符数][6]
            // F6x8[c][i] = 第 c 个字符的第 i 列的 8 个像素
            OLED_WR_Byte(F6x8[c][i], OLED_DATA);
        }
    }
}
```

**字库原理**：

字库本质是一个大型数组，存了每个可打印字符（从空格 `' '` 到 `'~'`，约 95 个字符）的像素位图。

```
字库数组的一部分（以 8×16 字体为例，'A' 的位图）：

const unsigned char F8X16[] = {
    // ' '(空格) 的字模, 16 字节
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  // 上半
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  // 下半（全是0，因为空格不显示）
    
    // ... 其他字符 ...
    
    // 'A' 的字模, 16 字节
    0x00,0x00,0x1C,0x22,0x22,0x3E,0x22,0x22,  // 上半：左边空白，中间形成'A'的轮廓
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  // 下半：8×16字体'A'的下半部分（这里简化了）
};

'A' 的 8×16 像素位图（放大看）：
列:  0  1  2  3  4  5  6  7
   ┌──┬──┬──┬──┬──┬──┬──┬──┐
   │  │  │ █│  │  │  │  │  │  ← 上半部分（前 8 字节，F8X16[索引+0~7]）
   │  │  │█ █│  │  │  │  │  │
   │  │  │█ █│  │  │  │  │  │
   │  │  │███│  │  │  │  │  │
   │  │  │█ █│  │  │  │  │  │
   │  │  │█ █│  │  │  │  │  │
   │  │  │   │  │  │  │  │  │
   │  │  │   │  │  │  │  │  │
   ├──┼──┼──┼──┼──┼──┼──┼──┤
   │  │  │   │  │  │  │  │  │  ← 下半部分（后 8 字节，F8X16[索引+8~15]）
   │..│..│...│..│..│..│..│..│
   └──┴──┴──┴──┴──┴──┴──┴──┘
```

> **字库从哪里来？** 用 PCtoLCD2002 这个软件生成。选择"逐列式"（Column Major）、"阴码"（点亮=1）、"顺向"（高位在前），就能生成和上面代码兼容的字库数据。把生成的数据复制粘贴到 `oledfont.h` 文件中即可。

**显示字符串**：

```c
/**
 * @brief  显示一个字符串
 * @param  x, y: 起始坐标
 * @param  str:  字符串指针
 * @param  size: 字体大小
 * @note   逐个字符显示，每个字符自动右移
 *         size=12 每个字符占 6 列（6×8字体，6 列宽）
 *         size=16 每个字符占 8 列（8×16字体，8 列宽）
 */
void OLED_ShowString(uint8_t x, uint8_t y, char* str, uint8_t size)
{
    while(*str != '\0')  // C 字符串以 '\0'（值为0的字符）结尾
    {
        OLED_ShowChar(x, y, *str, size);  // 显示当前字符
        // *str 是当前字符（解引用指针），等价于 str[0]
        
        if(size == 12)
            x += 6;    // 横坐标右移 6 列（6×8 字体宽度）
        else
            x += 8;    // 横坐标右移 8 列（8×16 字体宽度）
        
        if(x >= 128)   // 超出屏幕宽度
        {
            x = 0;     // 换行
            y += (size == 12) ? 1 : 2;  // 小字体下移1页，大字体下移2页
        }
        
        str++;  // 指针后移，指向下一个字符
        // 等价于 str = str + 1
    }
}
```

#### 11.1.4 OLED 初始化序列 —— 上电配置

SSD1306 上电后默认是关闭显示的，需要发送一系列命令来配置参数然后打开显示。这些命令必须按特定顺序发送。

```c
/**
 * @brief  OLED 初始化序列
 * @note   这些命令值都来自 SSD1306 数据手册
 *         每条命令的含义在注释中
 */
void OLED_Init(void)
{
    // ===== 上电延时 =====
    // SSD1306 上电后需要时间让内部电压稳定
    // 至少 100ms，稳妥起见设 200ms
    Delay_ms(200);
    
    // ===== 初始化命令序列 =====
    OLED_WR_Byte(0xAE, OLED_CMD); // 关闭显示（Display OFF）
    // 在配置过程中关闭显示，防止闪烁
    
    OLED_WR_Byte(0x00, OLED_CMD); // 设置列地址低4位 = 0
    OLED_WR_Byte(0x10, OLED_CMD); // 设置列地址高4位 = 0
    // 列地址 = 0，从第 0 列开始
    
    OLED_WR_Byte(0x40, OLED_CMD); // 设置显示起始行 = 0
    // 0x40 + 0 = 第 0 行开始显示
    
    OLED_WR_Byte(0x81, OLED_CMD); // 设置对比度命令
    OLED_WR_Byte(0xCF, OLED_CMD); // 对比度值 = 0xCF（207/255，默认高对比度）
    // 0x81 是双字节命令：第一条告诉 SSD1306"我要设对比度"，第二条是值
    
    OLED_WR_Byte(0xA1, OLED_CMD); // 段重映射：列 127 映射到 SEG0
    // 相当于左右翻转（如果屏幕显示镜像了，改 0xA0 试试）
    
    OLED_WR_Byte(0xC8, OLED_CMD); // COM 扫描方向：从下往上
    // 相当于上下翻转（如果屏幕上下颠倒了，改 0xC0 试试）
    
    OLED_WR_Byte(0xA6, OLED_CMD); // 正常显示（0xA7 是反色显示）
    // 正常：1=亮, 0=灭
    // 反色：1=灭, 0=亮（像照片底片）
    
    OLED_WR_Byte(0xA8, OLED_CMD); // 设置多路复用比率
    OLED_WR_Byte(0x3F, OLED_CMD); // 比率 = 64（64 行，即整个屏幕）
    // 0x3F = 63，加 1 = 64
    
    OLED_WR_Byte(0xD3, OLED_CMD); // 设置显示偏移
    OLED_WR_Byte(0x00, OLED_CMD); // 偏移 = 0（不偏移）
    
    OLED_WR_Byte(0xD5, OLED_CMD); // 设置显示时钟分频/振荡器频率
    OLED_WR_Byte(0x80, OLED_CMD); // 默认值
    
    OLED_WR_Byte(0xD9, OLED_CMD); // 设置预充电周期
    OLED_WR_Byte(0xF1, OLED_CMD); // 默认值
    
    OLED_WR_Byte(0xDA, OLED_CMD); // 设置 COM 引脚硬件配置
    OLED_WR_Byte(0x12, OLED_CMD); // 适用于 128×64
    
    OLED_WR_Byte(0xDB, OLED_CMD); // 设置 VCOMH 电压
    OLED_WR_Byte(0x40, OLED_CMD); // ~0.77×VCC
    
    OLED_WR_Byte(0x20, OLED_CMD); // 设置内存寻址模式
    OLED_WR_Byte(0x00, OLED_CMD); // 水平寻址（默认）
    // 00=水平, 01=垂直, 02=页寻址（我们下面的函数用页寻址）
    
    OLED_WR_Byte(0x8D, OLED_CMD); // 电荷泵设置
    OLED_WR_Byte(0x14, OLED_CMD); // 使能电荷泵（0x14=开，0x10=关）
    // 3.3V 供电时电荷泵会把电压升到约 7~9V 来驱动 OLED 面板
    
    OLED_WR_Byte(0xA4, OLED_CMD); // 全局显示：从 GDDRAM 输出
    // 0xA5 = 全局显示打开（忽略 GDDRAM 内容，全屏点亮）
    
    OLED_WR_Byte(0xA6, OLED_CMD); // 正常显示（非反色）
    
    OLED_WR_Byte(0x2E, OLED_CMD); // 停止滚动（如果有的话）
    
    OLED_WR_Byte(0xAF, OLED_CMD); // 打开显示（Display ON）！
    // 这是最关键的一条命令，没有它屏幕永远是黑的
    
    OLED_Clear();  // 清屏，确保初始状态是干净的
}
```

> **OLED 初始化注意事项**：
> 1. **上电后必须延时**：SSD1306 需要时间让内部 DC-DC 升压电路稳定，至少 100ms。
> 2. **I2C 地址要确认**：用 I2C 扫描程序试 0x78 和 0x7A，看哪个有应答。
> 3. **如果屏幕不亮**：检查 VCC（3.3V）、GND、SCL、SDA 四根线。只接这四根就够了，RES 引脚可以悬空。
> 4. **如果屏幕花屏/偏移**：调整 `0xA0`/`0xA1`（左右镜像）和 `0xC0`/`0xC8`（上下镜像）的组合。

#### 11.1.5 进阶：帧缓冲（Framebuffer）技术

上面的驱动每次显示都是在 OLED 上直接绘制，效率没问题但有一个缺点：频繁更新同一区域时屏幕会闪烁。帧缓冲可以彻底解决这个问题。

**原理**：在 MCU 的 RAM 中维护一个 128×64 bit = 1024 字节的"虚拟屏幕"，所有绘图操作都在这个虚拟屏幕上进行，画完后一次性刷新到 OLED。

```c
// 帧缓冲区（1024 字节 = 128×64 bit）
// 组织方式：buf[page][col]，共 8 页 × 128 列
uint8_t OLED_FrameBuf[8][128];  // 1024 字节，需要较多 RAM

// 初始化帧缓冲
void OLED_FrameBuf_Init(void)
{
    memset(OLED_FrameBuf, 0, sizeof(OLED_FrameBuf));  // 全黑
}

// 在帧缓冲中画一个点
void OLED_DrawPointBuf(uint8_t x, uint8_t y, uint8_t mode)
{
    if(x >= 128 || y >= 64) return;  // 越界保护
    
    uint8_t page = y / 8;   // 算出在第几页
    uint8_t bit  = y % 8;   // 算出在这页的第几个 bit
    
    if(mode)
        OLED_FrameBuf[page][x] |=  (1 << bit);   // 点亮：对应 bit 置 1
    else
        OLED_FrameBuf[page][x] &= ~(1 << bit);   // 熄灭：对应 bit 置 0
}

// 把整个帧缓冲刷新到 OLED（一次写入 1024 字节）
void OLED_FrameBuf_Flush(void)
{
    uint8_t i, j;
    for(i = 0; i < 8; i++)  // 8 个页
    {
        OLED_SetPos(0, i);  // 每页从第 0 列开始
        for(j = 0; j < 128; j++)
        {
            OLED_WR_Byte(OLED_FrameBuf[i][j], OLED_DATA);
            // 连续写入 128 字节 = 这一页的全部内容
        }
    }
    // 刷新用时：8×128=1024 次 I2C 写入 ≈ 100ms（100kHz I2C）
}
```

> **帧缓冲的优缺点**：
> - 优点：不会闪烁、可以做复杂的像素级操作（画线、画圆、反色）、文字覆盖不会留残影
> - 缺点：占用 1KB RAM（C8T6 只有 20KB RAM，1KB 可以接受）、刷新需要约 100ms
> - **电赛推荐**：如果屏幕内容变化不频繁（如显示传感器数据，100ms 刷新一次），用帧缓冲体验很好；如果内容变化频繁（如实时波形），直接绘制更快。

> **OLED 使用经验总结**：
> - I2C 地址一般是 0x78 或 0x7A，不亮先改地址试试
> - 上电延时 ≥100ms 再初始化，否则可能部分命令不生效
> - 不要在主循环里每轮都全屏刷新——会闪屏、慢、耗电。只在数据变化时局部更新，或用帧缓冲 + 定时刷新（如 100ms 一次）
> - 如果要显示中文，PCtoLCD2002 软件生成的字模数据格式必须和你的取模函数兼容

### 11.2 TB6612FNG直流电机驱动
L298N是老款电机驱动，发热大效率低，现在电赛都用TB6612FNG，体积小发热小，驱动能力强，能驱动两个直流电机。

TB6612引脚：
- AIN1/AIN2：A路方向控制
- PWMA：A路PWM输入
- BIN1/BIN2：B路方向控制
- PWMB：B路PWM输入
- STBY：待机控制，接高电平正常工作
- VM：电机电源，接电池
- VCC：逻辑电源，3.3V
- GND：共地

标准驱动代码：
```c
// 电机初始化
void Motor_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    // 开时钟：GPIOA GPIOB
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);
    
    // AIN1 AIN2 BIN1 BIN2 STBY 推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // PWMA PA0 TIM2 CH1, PWMB PA1 TIM2 CH2 复用推挽输出PWM
    // 这里TIM2 PWM配置前面讲过，10kHz PWM
    
    GPIO_SetBits(GPIOA, GPIO_Pin_6); // STBY=1 正常工作
}

// 设置左边电机速度，speed:-100~100，正转反转
void Motor_SetLeft(int16_t speed) {
    if(speed > 0) {
        GPIO_SetBits(GPIOA, GPIO_Pin_2);
        GPIO_ResetBits(GPIOA, GPIO_Pin_3);
        TIM_SetCompare1(TIM2, speed);
    } else if(speed < 0) {
        GPIO_ResetBits(GPIOA, GPIO_Pin_2);
        GPIO_SetBits(GPIOA, GPIO_Pin_3);
        TIM_SetCompare1(TIM2, -speed);
    } else {
        GPIO_ResetBits(GPIOA, GPIO_Pin_2);
        GPIO_ResetBits(GPIOA, GPIO_Pin_3);
        TIM_SetCompare1(TIM2, 0);
    }
}

// 右边电机同理
```

> **电机驱动注意事项**：
> 1. **电机电源必须和单片机共地**！否则PWM信号没有回路，电机乱转或者不转
> 2. 电机两端要接104电容滤波，减少对单片机的干扰
> 3. 电源功率要够，直流电机启动电流很大，电池容量不够会导致单片机复位
> 4. 布线的时候电机线和单片机信号线分开走，不要平行，减少干扰
> 5. TB6612比L298N好太多，电赛不要再用L298N了

### 11.3 MPU6050六轴陀螺仪加速度计
MPU6050是电赛做平衡车、倒立摆、姿态检测必用的模块，I2C接口，可以读加速度、角速度，用DMP硬件解算姿态角，非常稳定。

驱动代码基于前面的模拟I2C，关键是DMP姿态解算，移植官方DMP库后直接读欧拉角就行：
```c
// 初始化MPU6050和DMP
uint8_t MPU6050_Init(void) {
    // 初始化I2C
    MPU_I2C_Init();
    MPU_WriteByte(MPU_ADDR, PWR_MGMT_1, 0x80); // 复位
    Delay_ms(100);
    MPU_WriteByte(MPU_ADDR, PWR_MGMT_1, 0x00); // 唤醒
    Delay_ms(100);
    MPU_WriteByte(MPU_ADDR, SMPLRT_DIV, 0x07); // 采样率125Hz
    MPU_WriteByte(MPU_ADDR, CONFIG, 0x03); // 低通滤波42Hz
    MPU_WriteByte(MPU_ADDR, GYRO_CONFIG, 0x18); // 陀螺仪±2000dps
    MPU_WriteByte(MPU_ADDR, ACCEL_CONFIG, 0x00); // 加速度±2g
    
    // 初始化DMP
    mpu_dmp_init();
    return 0;
}

// 读取姿态角，pitch俯仰角 roll横滚角 yaw偏航角
float pitch, roll, yaw;
uint8_t MPU6050_GetAngle(void) {
    if(dmp_read_fifo(gyro, accel, quat, &timestamp, &sensors) == 0) {
        dmp_get_euler(&pitch, &roll, &yaw);
        return 1;
    }
    return 0;
}
```

> **MPU6050使用经验**：
> 1. 模块一定要水平安装，越稳越好，震动会导致角度漂移
> 2. 电源加滤波电容，减少电机干扰
> 3. 用DMP硬件解算比自己写互补滤波/卡尔曼滤波简单稳定，电赛直接用DMP就行
> 4. 上电后要静止几秒等DMP校准完成再动
> 5. I2C线上拉电阻要合适，太长的线会导致通信失败

### 11.4 NRF24L01 2.4G 无线模块 —— 完整驱动与详解

NRF24L01 是电赛无线通信首选，SPI 接口，2.4GHz 频段，通信距离空旷地带可达 100 米，室内穿墙约 10~30 米。可以实现双机通信、遥控、无线数据传输。电赛中常用来做遥控小车、双车协同、无线数据采集。

#### 11.4.1 NRF24L01 硬件特性与引脚

| 引脚 | 功能         | 连接（STM32）           | 说明                             |
| ---- | ------------ | ----------------------- | -------------------------------- |
| VCC  | 电源         | **3.3V**（绝不能 5V！） | 5V 必烧！                        |
| GND  | 地           | GND                     | 共地                             |
| CE   | 芯片使能     | 任意 GPIO（如 PB0）     | CE=1 进入收发模式，CE=0 进入待机 |
| CSN  | SPI 片选     | 任意 GPIO（如 PA4）     | 低电平有效                       |
| SCK  | SPI 时钟     | PA5（SPI1_SCK）         | 最高 10MHz                       |
| MOSI | 主机发从机收 | PA7（SPI1_MOSI）        |                                  |
| MISO | 从机发主机收 | PA6（SPI1_MISO）        |                                  |
| IRQ  | 中断请求     | 任意 GPIO（如 PB1）     | 收发完成时拉低，可选，也可不用   |

> **NRF24L01 最大的坑**：电源！发射时瞬间电流可达 15mA 以上，如果电源没有足够大的滤波电容（100μF 电解电容并 0.1μF 陶瓷电容），模块会因电压跌落而复位，表现为"偶尔收到数据、大部分时间收不到"。**电源引脚旁边必须焊 100μF 电容！**

#### 11.4.2 NRF24L01 工作原理

NRF24L01 内部有 6 个接收通道（Pipe0 ~ Pipe5），每个可以设置不同的接收地址。发送时只需要指定目标地址。核心概念：

- **频道（RF Channel）**：2.400GHz ~ 2.525GHz，共 126 个频道（间隔 1MHz）。收发双方频道必须一致。
- **地址（Address）**：3~5 字节，收发双方的地址必须匹配。Pipe0 的地址用作发送地址。
- **自动应答（Auto ACK）**：发送方发完数据后等待接收方的 ACK 信号，收到 ACK 才算发送成功。没收到会自动重发（最多 15 次）。
- **数据速率**：250kbps / 1Mbps / 2Mbps。速率越低，通信距离越远，抗干扰越强。电赛一般用 1Mbps 或 250kbps。
- **Payload 长度**：每个数据包 1~32 字节，电赛一般用 32 字节（最大）。

```
发送端：                             接收端：
┌──────────┐                       ┌──────────┐
│ NRF24L01 │  无线 2.4GHz          │ NRF24L01 │
│ (TX模式) │ ───────────────────→ │ (RX模式) │
│          │ ←─────────────────── │          │
│          │   ACK 应答信号        │          │
└──────────┘                       └──────────┘
```

#### 11.4.3 NRF24L01 寄存器速查（你只需知道这几个）

| 寄存器名      | 地址      | 功能                                               |
| ------------- | --------- | -------------------------------------------------- |
| CONFIG        | 0x00      | 配置寄存器：设置模式（收发）、使能 CRC、使能中断等 |
| EN_AA         | 0x01      | 自动应答使能：哪些 Pipe 收到数据后自动发送 ACK     |
| EN_RXADDR     | 0x02      | 接收地址使能：哪些 Pipe 处于激活状态               |
| SETUP_AW      | 0x03      | 地址宽度：3/4/5 字节                               |
| SETUP_RETR    | 0x04      | 自动重发配置：重发等待时间和最大重发次数           |
| RF_CH         | 0x05      | 频道选择：0~125                                    |
| RF_SETUP      | 0x06      | 射频配置：数据速率、发射功率                       |
| STATUS        | 0x07      | 状态寄存器：中断标志、收发状态                     |
| RX_ADDR_P0~P5 | 0x0A~0x0F | 各 Pipe 的接收地址（P0 也用作发送地址）            |
| TX_ADDR       | 0x10      | 发送地址（通常和 P0 的接收地址相同）               |
| RX_PW_P0~P5   | 0x11~0x16 | 各 Pipe 的 Payload 宽度（1~32 字节）               |
| FIFO_STATUS   | 0x17      | FIFO 状态：收发缓冲区是否满/空                     |
| TX_FIFO       | -         | 发送缓冲区：写此地址即写入待发送数据（命令 0xA0）  |
| RX_FIFO       | -         | 接收缓冲区：读此地址即读取收到的数据（命令 0x61）  |

#### 11.4.4 NRF24L01 SPI 命令

| 命令名       | 指令字节 | 说明                             |
| ------------ | -------- | -------------------------------- |
| R_REGISTER   | 0x00     | 读寄存器：指令                   | = 寄存器地址 |
| W_REGISTER   | 0x20     | 写寄存器：指令                   | = 寄存器地址 |
| R_RX_PAYLOAD | 0x61     | 读接收 Payload（接收缓冲区数据） |
| W_TX_PAYLOAD | 0xA0     | 写发送 Payload（填充发送缓冲区） |
| FLUSH_TX     | 0xE1     | 清空发送缓冲区                   |
| FLUSH_RX     | 0xE2     | 清空接收缓冲区                   |
| NOP          | 0xFF     | 空操作（只读 STATUS 寄存器）     |

> **理解**：NRF24L01 的 SPI 协议是"先发命令字节，再发/收数据字节"。命令字节的高 3 位决定了操作类型，低 5 位是寄存器地址。

#### 11.4.5 NRF24L01 完整驱动代码（逐行详解）

以下代码基于前面写的软件 SPI 或硬件 SPI，我以软件 SPI 为例。把 `SPI_ReadWriteByte` 换成硬件 SPI 版本即可无缝切换。

```c
// ========== NRF24L01 引脚定义 ==========
#define NRF_CE_GPIO    GPIOB
#define NRF_CE_PIN     GPIO_Pin_0   // CE：芯片使能（模式控制）
#define NRF_CSN_GPIO   GPIOA
#define NRF_CSN_PIN    GPIO_Pin_4   // CSN：SPI 片选
#define NRF_IRQ_GPIO   GPIOB
#define NRF_IRQ_PIN    GPIO_Pin_1   // IRQ：中断请求（收发完成通知）

// ========== 基础宏定义 ==========
// [功能] CE 和 CSN 的控制宏
#define NRF_CE_H()   GPIO_SetBits(NRF_CE_GPIO, NRF_CE_PIN)    // CE=1
#define NRF_CE_L()   GPIO_ResetBits(NRF_CE_GPIO, NRF_CE_PIN)  // CE=0
#define NRF_CSN_H()  GPIO_SetBits(NRF_CSN_GPIO, NRF_CSN_PIN)  // 取消片选
#define NRF_CSN_L()  GPIO_ResetBits(NRF_CSN_GPIO, NRF_CSN_PIN) // 选中模块
#define NRF_READ_IRQ() GPIO_ReadInputDataBit(NRF_IRQ_GPIO, NRF_IRQ_PIN) // 读 IRQ 脚

// ========== NRF24L01 指令定义 ==========
#define NRF_CMD_R_REGISTER     0x00  // 读寄存器（低5位填地址）
#define NRF_CMD_W_REGISTER     0x20  // 写寄存器（低5位填地址）
#define NRF_CMD_R_RX_PAYLOAD   0x61  // 读接收数据
#define NRF_CMD_W_TX_PAYLOAD   0xA0  // 写发送数据
#define NRF_CMD_FLUSH_TX       0xE1  // 清空发送 FIFO
#define NRF_CMD_FLUSH_RX       0xE2  // 清空接收 FIFO

// ========== NRF24L01 寄存器地址定义 ==========
#define NRF_REG_CONFIG          0x00  // 配置寄存器
#define NRF_REG_EN_AA           0x01  // 自动应答使能
#define NRF_REG_EN_RXADDR       0x02  // 接收通道使能
#define NRF_REG_SETUP_AW        0x03  // 地址宽度设置
#define NRF_REG_SETUP_RETR      0x04  // 自动重发设置
#define NRF_REG_RF_CH           0x05  // 频道
#define NRF_REG_RF_SETUP        0x06  // 射频设置
#define NRF_REG_STATUS          0x07  // 状态寄存器
#define NRF_REG_RX_ADDR_P0      0x0A  // Pipe0 接收地址（也是发送地址）
#define NRF_REG_TX_ADDR         0x10  // 发送地址
#define NRF_REG_RX_PW_P0        0x11  // Pipe0 Payload 宽度
#define NRF_REG_FIFO_STATUS     0x17  // FIFO 状态

// ========== NRF24L01 初始化 ==========

/**
 * @brief  NRF24L01 GPIO 和 SPI 初始化
 * @note   CE 和 CSN 是普通 GPIO 推挽输出
 *         IRQ 是输入（NRF24L01 推挽输出，MCU 浮空输入即可）
 *         SPI 引脚在前面 SPI_Init() 中已配置
 */
void NRF24L01_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 开 GPIO 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);
    
    // CE 和 CSN：推挽输出
    GPIO_InitStructure.GPIO_Pin = NRF_CE_PIN | NRF_CSN_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(NRF_CE_GPIO, &GPIO_InitStructure);   // CE=PB0, CSN=PA4
    // 注意：CSN 是 PA4，在其他定义中可能和 CE 不同端口，需要分开初始化
    // 这里简化处理，实际使用时请根据你的引脚分配调整
    
    // IRQ：浮空输入（NRF24L01 的 IRQ 引脚是推挽输出，MCU 读电平即可）
    GPIO_InitStructure.GPIO_Pin = NRF_IRQ_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(NRF_IRQ_GPIO, &GPIO_InitStructure);
    
    // 初始状态
    NRF_CE_L();    // CE=0，待机模式
    NRF_CSN_H();   // CSN=1，不选中（SPI 片选默认高）
}

/**
 * @brief  向 NRF24L01 寄存器写入一个字节
 * @param  reg:  寄存器地址（0x00~0x17）
 * @param  value: 要写入的值
 * @note   SPI 通信协议：CSN 拉低 → 发写命令(0x20|reg) → 发数据 → CSN 拉高
 *         在 CSN 上升沿，NRF24L01 锁存数据到内部寄存器
 */
void NRF_WriteReg(uint8_t reg, uint8_t value)
{
    NRF_CSN_L();                           // 1. 选中 NRF24L01
    SPI_ReadWriteByte(NRF_CMD_W_REGISTER | reg); // 2. 写命令(0x20) | 寄存器地址
    // 例如 reg=0x00(CONFIG)，发送字节=0x20|0x00=0x20，告诉NRF"我要写CONFIG寄存器"
    SPI_ReadWriteByte(value);               // 3. 发送要写入的值
    NRF_CSN_H();                           // 4. 释放 NRF24L01（上升沿锁存数据）
}

/**
 * @brief  从 NRF24L01 寄存器读取一个字节
 * @param  reg: 寄存器地址
 * @return 读到的值
 */
uint8_t NRF_ReadReg(uint8_t reg)
{
    uint8_t value;
    NRF_CSN_L();
    SPI_ReadWriteByte(NRF_CMD_R_REGISTER | reg); // 读命令(0x00) | 寄存器地址
    value = SPI_ReadWriteByte(0xFF);             // 发哑字节，收寄存器值
    NRF_CSN_H();
    return value;
}

/**
 * @brief  写入多个字节到寄存器（用于写地址等）
 */
void NRF_WriteBuf(uint8_t reg, uint8_t* buf, uint8_t len)
{
    NRF_CSN_L();
    SPI_ReadWriteByte(NRF_CMD_W_REGISTER | reg);  // 写命令
    for(uint8_t i = 0; i < len; i++)
        SPI_ReadWriteByte(buf[i]);                // 连续写多字节
    NRF_CSN_H();
}

/**
 * @brief  从寄存器读取多个字节
 */
void NRF_ReadBuf(uint8_t reg, uint8_t* buf, uint8_t len)
{
    NRF_CSN_L();
    SPI_ReadWriteByte(NRF_CMD_R_REGISTER | reg);  // 读命令
    for(uint8_t i = 0; i < len; i++)
        buf[i] = SPI_ReadWriteByte(0xFF);         // 连续收
    NRF_CSN_H();
}

// ========== NRF24L01 功能函数 ==========

/**
 * @brief  初始化 NRF24L01 为接收模式（RX Mode）
 * @note   配置参数：
 *         - 频道：40（2.440GHz）
 *         - 速率：1Mbps
 *         - 发射功率：0dBm
 *         - 地址宽度：5 字节
 *         - 接收地址：5 字节（自定义，收发双方必须一致）
 *         - Payload 宽度：32 字节（最大）
 *         - 自动应答：开启 Pipe0
 *         - 自动重发：500us 等待 + 3 次重发
 *         
 *         接收模式工作流程：
 *         1. 配置好寄存器
 *         2. CE=1 进入接收模式（持续监听空中信号）
 *         3. 收到匹配地址的数据包 → 自动发 ACK → IRQ 拉低
 *         4. MCU 读 STATUS 寄存器 → 读 RX FIFO → 清除中断标志
 */
void NRF_RXMode_Init(void)
{
    NRF_CE_L();  // 配置期间 CE=0
    
    // ---- 地址配置 ----
    // 地址宽度：5 字节
    NRF_WriteReg(NRF_REG_SETUP_AW, 0x03);  // 0x01=3字节, 0x02=4字节, 0x03=5字节
    
    // Pipe0 接收地址（5 字节，也是发送地址）
    // 地址可以自定义，但必须收发双方一致
    uint8_t rx_addr[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};  // 自定义地址
    NRF_WriteBuf(NRF_REG_RX_ADDR_P0, rx_addr, 5);
    
    // Pipe0 Payload 宽度：32 字节
    NRF_WriteReg(NRF_REG_RX_PW_P0, 32);
    
    // ---- 自动应答配置 ----
    // 使能 Pipe0 的自动应答：收到数据后自动发送 ACK
    NRF_WriteReg(NRF_REG_EN_AA, 0x01);  // bit0=1 → Pipe0 自动应答开启
    
    // ---- 接收通道使能 ----
    // 使能 Pipe0 作为接收通道
    NRF_WriteReg(NRF_REG_EN_RXADDR, 0x01);  // bit0=1 → Pipe0 接收通道开启
    
    // ---- 自动重发配置 ----
    // 高 4 位：重发等待时间 ARD = 0x01 → 500us
    // 低 4 位：最大重发次数 ARC = 0x03 → 3 次
    // 如果发送后 500us 没收到 ACK，自动重发，最多 3 次
    NRF_WriteReg(NRF_REG_SETUP_RETR, (0x01 << 4) | 0x03);
    
    // ---- 频道设置 ----
    // 频道 40（2.400GHz + 40MHz = 2.440GHz）
    // 避免和 WiFi 信道冲突（WiFi 信道 1 在 2.412GHz）
    NRF_WriteReg(NRF_REG_RF_CH, 40);
    
    // ---- 射频配置 ----
    // bit[2:1] = 00 → 1Mbps 数据速率
    // bit[2:1] = 01 → 2Mbps
    // bit[2:1] = 10 → 250kbps（距离最远）
    // bit[3] = 0 → 发射功率 0dBm
    // bit[0] = 1 → LNA 高增益模式
    NRF_WriteReg(NRF_REG_RF_SETUP, 0x07);  // 1Mbps, 0dBm, LNA高增益
    
    // ---- 配置寄存器 ----
    // bit[7:4] = 0001 → 只使能 RX_DR（接收数据就绪）中断
    // bit[3:2] = 00  → CRC 编码方案：1 字节
    // bit[1]   = 1   → 上电（PWR_UP=1）
    // bit[0]   = 1   → 接收模式（PRIM_RX=1）
    NRF_WriteReg(NRF_REG_CONFIG, 0x0F);
    // 0x0F = 0b0000 1111
    // 含义：CRC=1字节, PWR_UP=1(上电), PRIM_RX=1(接收模式)
    //       MASK_RX_DR=0(允许RX中断), MASK_TX_DS=1(屏蔽TX中断), MASK_MAX_RT=1(屏蔽重发中断)
    
    // ---- 启动接收模式 ----
    // CE=1 进入接收模式，开始监听空中信号
    NRF_CE_H();
    // CE 从 0 到 1 需要 ≥10μs 的上升时间，下面用延时保证
    Delay_us(130);  // 超过 130μs 确保稳定进入 RX 模式
}

/**
 * @brief  初始化 NRF24L01 为发送模式（TX Mode）
 * @note   发送模式和接收模式的配置基本一样，只有两处不同：
 *         1. CONFIG 寄存器的 PRIM_RX 位 = 0（发送模式）
 *         2. 需要配置 TX_ADDR（发送目标地址）
 */
void NRF_TXMode_Init(void)
{
    NRF_CE_L();
    
    // ---- 与接收模式相同的配置 ----
    NRF_WriteReg(NRF_REG_SETUP_AW, 0x03);
    uint8_t rx_addr[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
    NRF_WriteBuf(NRF_REG_RX_ADDR_P0, rx_addr, 5);  // P0 地址（ACK 回传用）
    NRF_WriteReg(NRF_REG_EN_AA, 0x01);
    NRF_WriteReg(NRF_REG_EN_RXADDR, 0x01);
    NRF_WriteReg(NRF_REG_SETUP_RETR, (0x01 << 4) | 0x03);
    NRF_WriteReg(NRF_REG_RF_CH, 40);
    NRF_WriteReg(NRF_REG_RF_SETUP, 0x07);
    
    // ---- 发送地址（目标的接收地址）----
    // 发送时数据包的目的地址，必须和接收方 Pipe0 的地址一致！
    uint8_t tx_addr[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
    NRF_WriteBuf(NRF_REG_TX_ADDR, tx_addr, 5);      // 目标的地址
    NRF_WriteBuf(NRF_REG_RX_ADDR_P0, tx_addr, 5);    // P0 地址（用于收 ACK）
    // [重要] P0 的接收地址必须和 TX_ADDR 相同，因为 NRF24L01 用 P0 来接收 ACK！
    
    // ---- 配置寄存器（发送模式）----
    // bit[0]=0 → PRIM_RX=0，发送模式！
    NRF_WriteReg(NRF_REG_CONFIG, 0x0E);
    // 0x0E = 0b0000 1110（和接收模式 0x0F 只差 bit0）
    
    NRF_CE_H();
}

/**
 * @brief  发送一包数据（32 字节）
 * @param  data: 要发送的 32 字节数据
 * @return 0: 发送成功（收到 ACK），1: 发送失败（超时或达到重发上限）
 * 
 * @note   发送流程：
 *         1. 清空发送 FIFO（防止旧数据干扰）
 *         2. 写数据到发送 FIFO（命令 0xA0 + 32 字节）
 *         3. CE 脉冲（拉高 ≥10μs）触发发送
 *         4. 等待发送完成中断（IRQ 拉低 或 STATUS 寄存器标志位）
 *         5. 检查发送状态，清除中断标志
 */
uint8_t NRF_TxPacket(uint8_t* data)
{
    uint8_t status;
    
    // 第1步：清空发送 FIFO
    NRF_CSN_L();
    SPI_ReadWriteByte(NRF_CMD_FLUSH_TX);   // 清空发送缓冲区
    NRF_CSN_H();
    
    // 第2步：写入 32 字节数据到发送 FIFO
    NRF_CSN_L();
    SPI_ReadWriteByte(NRF_CMD_W_TX_PAYLOAD); // 写 Payload 命令
    for(uint8_t i = 0; i < 32; i++)
        SPI_ReadWriteByte(data[i]);          // 逐字节写入
    NRF_CSN_H();
    
    // 第3步：CE 脉冲触发发送
    // CE 保持高 ≥10μs 后拉低，NRF 自动发送数据
    NRF_CE_H();
    Delay_us(15);  // 至少 10μs，给足 15μs
    NRF_CE_L();
    // NRF24L01 会自动：发送数据 → 切换到接收模式等待 ACK → 收到 ACK 或超时
    
    // 第4步：等待发送完成
    // 方法1：等待 IRQ 引脚拉低（NRF24L01 发送完成会拉低 IRQ）
    uint32_t timeout = 0;
    while(NRF_READ_IRQ() != 0)  // IRQ 高 = 还没完成，继续等
    {
        if(++timeout > 100000) return 1;  // 超时（约 100ms）
    }
    
    // 方法2（备用）：读 STATUS 寄存器
    // while((NRF_ReadReg(NRF_REG_STATUS) & 0x70) == 0x00); // 等待任一中断标志置位
    
    // 第5步：读取 STATUS 寄存器，检查结果
    status = NRF_ReadReg(NRF_REG_STATUS);
    
    // 清除中断标志（写 1 清除）
    NRF_WriteReg(NRF_REG_STATUS, status);  // 写回 STATUS 值即可清所有中断标志
    // STATUS 的 bit[6:4] 分别是 RX_DR(接收就绪)、TX_DS(发送成功)、MAX_RT(达到最大重发)
    
    // 检查发送状态
    if(status & 0x20)  // bit5 = TX_DS（发送成功，收到 ACK）
        return 0;       // 成功！
    else if(status & 0x10)  // bit4 = MAX_RT（达到最大重发次数仍未收到 ACK）
    {
        // 发送失败：可能接收方没开机、频道不对、距离太远
        NRF_CSN_L();
        SPI_ReadWriteByte(NRF_CMD_FLUSH_TX);  // 清空发送 FIFO（协议要求）
        NRF_CSN_H();
        return 1;  // 失败
    }
    
    return 1;  // 其他未知状态
}

/**
 * @brief  接收一包数据（32 字节）
 * @param  data: 存放接收数据的缓冲区（至少 32 字节）
 * @return 0: 接收到数据，1: 没有数据
 * @note   接收流程：
 *         1. 检查是否有数据（读 STATUS 或检查 IRQ）
 *         2. 读接收 FIFO
 *         3. 清除中断标志
 *         
 *         这个函数通常在主循环中轮询调用
 */
uint8_t NRF_RxPacket(uint8_t* data)
{
    uint8_t status;
    
    // 方法1：检查 IRQ 引脚
    if(NRF_READ_IRQ() != 0)
        return 1;  // IRQ 高，没有新数据
    
    // 方法2：读 STATUS 寄存器检查 RX_DR 位
    status = NRF_ReadReg(NRF_REG_STATUS);
    if(!(status & 0x40))  // bit6 = RX_DR（接收数据就绪）
        return 1;  // 没有收到数据
    
    // 读取数据
    NRF_CSN_L();
    SPI_ReadWriteByte(NRF_CMD_R_RX_PAYLOAD);  // 读 Payload 命令
    for(uint8_t i = 0; i < 32; i++)
        data[i] = SPI_ReadWriteByte(0xFF);    // 逐字节读取
    NRF_CSN_H();
    
    // 清除中断标志
    NRF_WriteReg(NRF_REG_STATUS, status);
    
    return 0;  // 成功接收到数据
}

// ========== NRF24L01 初始化总入口 ==========

/**
 * @brief  NRF24L01 初始化总函数
 * @param  mode: 0=接收模式, 1=发送模式
 * @note   调用前确保 SPI 和 GPIO 已经初始化
 */
void NRF24L01_Init(uint8_t mode)
{
    NRF24L01_GPIO_Init();  // 先初始化 GPIO
    
    // 上电后延时 100ms，等模块内部稳定
    Delay_ms(100);
    
    if(mode == 0)
        NRF_RXMode_Init();   // 初始化接收模式
    else
        NRF_TXMode_Init();   // 初始化发送模式
}

// ========== 使用示例 ==========

// --- 发送端（遥控器）---
int main(void)
{
    uint8_t tx_buf[32];  // 32 字节发送缓冲区
    // ... 系统初始化 ...
    NRF24L01_Init(1);     // 初始化为发送模式
    
    while(1)
    {
        // 填充要发送的数据
        tx_buf[0] = joystick_x;   // 摇杆 X 值
        tx_buf[1] = joystick_y;   // 摇杆 Y 值
        tx_buf[2] = button_state; // 按键状态
        
        // 发送
        if(NRF_TxPacket(tx_buf) == 0)
            OLED_ShowString(0, 0, "TX OK", 16);
        else
            OLED_ShowString(0, 0, "TX FAIL", 16);
        
        Delay_ms(20);  // 每 20ms 发送一包（50Hz 遥控频率）
    }
}

// --- 接收端（小车）---
int main(void)
{
    uint8_t rx_buf[32];  // 32 字节接收缓冲区
    // ... 系统初始化 ...
    NRF24L01_Init(0);     // 初始化为接收模式
    
    while(1)
    {
        if(NRF_RxPacket(rx_buf) == 0)  // 收到数据
        {
            // 解析遥控指令
            int8_t x = (int8_t)rx_buf[0];   // 摇杆 X（-128~127）
            int8_t y = (int8_t)rx_buf[1];   // 摇杆 Y
            uint8_t btn = rx_buf[2];         // 按键
            
            // 控制小车
            Car_Control(x, y, btn);
        }
        else
        {
            // 没收到遥控信号，停车（安全第一！）
            Car_Stop();
        }
        // 不延时，持续监听
    }
}
```

> **NRF24L01 电赛经验总结**：
> 1. **电源滤波是生死线**：100μF 电解 + 0.1μF 陶瓷，靠近模块 VCC/GND 引脚焊接。没有这个电容，发送成功率可能不到 50%。
> 2. **频道避免 WiFi 干扰**：2.4GHz 也是 WiFi 的频段。电赛现场可能有大量 WiFi 信号，建议选 2.500GHz 以上的频道（频道 100+），或者用 2.480GHz 附近（频道 80）。
> 3. **速率选择**：250kbps 距离最远但速度最慢；1Mbps 综合最佳；2Mbps 距离最近。电赛一般 1Mbps 足够。
> 4. **32 字节 Payload 是标配**：NRF24L01 一包最多 32 字节，控制指令、传感器数据通常够用。如果需要更多，用软件分包。
> 5. **收发必须分时**：NRF24L01 是半双工的，同一时刻只能发送或接收。如果两个模块都要收发，需要设计时隙协议（如 10ms 发、10ms 收交替）。
> 6. **没收到 ACK 不代表完全失败**：自动重发 3 次全部失败才报错。偶尔一两次失败正常（2.4GHz 干扰），软件上做好重试或容错。
> 7. **IRQ 引脚可以不用**：如果 GPIO 不够，可以不接 IRQ，改用轮询 STATUS 寄存器。但用 IRQ 能及时响应，推荐。

---

## 第十二章 核心算法篇——电赛一等奖的灵魂
如果说外设驱动是基础，那算法就是电赛拿一等奖的核心。同样的硬件，别人电机转起来抖、速度不稳、位置不准，你用PID控制的电机稳如狗、指哪打哪，这就是差距。

### 12.1 PID控制算法——电赛控制类题目的万能钥匙
90%以上的控制类题目：电机调速、位置控制、平衡车、倒立摆、温度控制、水位控制，本质都是PID控制。PID是工业界用的最多的控制算法，没有之一，简单、稳定、可靠，调好了效果非常好。

#### 12.1.1 PID原理
PID就是三个环节：比例P、积分I、微分D，根据误差来调整输出量：
```
误差error = 目标值 - 当前实际值
输出 = Kp*error + Ki*误差积分 + Kd*误差微分
```

- **比例P**：误差越大，输出越大，成比例。P太大系统会震荡，太小响应慢
- **积分I**：把过去的误差加起来，消除静差。比如你P控制电机，目标100转，实际稳定在98转，一直有2转误差，积分项会慢慢累积，把这2转误差消掉。I太大容易超调震荡
- **微分D**：看误差的变化趋势，误差变化越快，D项越大，相当于阻尼，抑制震荡，让系统更稳。D太大会对噪声太敏感

举个最通俗的例子：你开车要保持100km/h：
- P：现在速度80，差20，踩油门；差的越多踩的越深
- I：过去10秒一直没到100，多踩点油门，把欠的补上
- D：速度涨的太快了，快到100了，松点油门，别冲过了

#### 12.1.2 位置式PID
位置式PID是最基础的PID，输出是绝对量，适合舵机角度控制、位置控制、温度控制这种输出是绝对值的场景。

代码实现：
```c
typedef struct {
    float Kp;
    float Ki;
    float Kd;
    float set; // 目标值
    float error; // 当前误差
    float last_error; // 上一次误差
    float prev_error; // 上上次误差
    float integral; // 积分项
    float output; // 输出
    float integral_max; // 积分限幅，防止积分饱和
    float output_max; // 输出限幅
} PID_TypeDef;

void PID_Init(PID_TypeDef* pid, float kp, float ki, float kd, float out_max, float i_max) {
    pid->Kp = kp;
    pid->Ki = ki;
    pid->Kd = kd;
    pid->set = 0;
    pid->error = 0;
    pid->last_error = 0;
    pid->prev_error = 0;
    pid->integral = 0;
    pid->output = 0;
    pid->output_max = out_max;
    pid->integral_max = i_max;
}

float PID_Calc(PID_TypeDef* pid, float feedback) {
    pid->error = pid->set - feedback;
    
    // 积分项
    pid->integral += pid->error;
    // 积分限幅，防止积分饱和
    if(pid->integral > pid->integral_max) pid->integral = pid->integral_max;
    if(pid->integral < -pid->integral_max) pid->integral = -pid->integral_max;
    
    // 微分项
    float derivative = pid->error - pid->last_error;
    
    // PID计算
    pid->output = pid->Kp * pid->error 
                + pid->Ki * pid->integral 
                + pid->Kd * derivative;
    
    // 输出限幅
    if(pid->output > pid->output_max) pid->output = pid->output_max;
    if(pid->output < -pid->output_max) pid->output = -pid->output_max;
    
    pid->last_error = pid->error;
    return pid->output;
}
```

#### 12.1.3 增量式PID
增量式PID输出的是变化量，不是绝对值，适合电机调速这种需要增量调整的场景，抗积分饱和，切换手动自动无冲击，电赛电机调速一般用增量式。

```c
float PID_Incremental(PID_TypeDef* pid, float feedback) {
    pid->error = pid->set - feedback;
    
    float delta = pid->Kp * (pid->error - pid->last_error)
                + pid->Ki * pid->error
                + pid->Kd * (pid->error - 2*pid->last_error + pid->prev_error);
    
    pid->output += delta;
    
    // 输出限幅
    if(pid->output > pid->output_max) pid->output = pid->output_max;
    if(pid->output < -pid->output_max) pid->output = -pid->output_max;
    
    pid->prev_error = pid->last_error;
    pid->last_error = pid->error;
    return pid->output;
}
```

#### 12.1.4 PID参数整定方法（电赛现场调参秘籍）
很多人写了PID但是不会调参，我给你最简单实用的调参步骤，电赛现场10分钟就能调好：

1. **先把Ki和Kd设为0，只调Kp**：从小到大加Kp，直到系统开始出现等幅震荡，这时候Kp再乘以0.6~0.8，就是比较合适的P值
2. **然后加Ki**：从小到大加Ki，直到系统静差刚好消除，响应速度够快，不要加太大，否则会超调震荡
3. **最后加Kd**：从小到大加Kd，直到系统超调很小，响应快，没有震荡。Kd不要太大，否则系统会对噪声很敏感，抖
4. **微调**：根据实际响应微调三个参数，直到响应快、超调小、不震荡、没有静差

**调参口诀**：
- 参数整定找最佳，从小到大顺序查
- 先是比例后积分，最后再把微分加
- 曲线振荡很频繁，比例度盘要放大
- 曲线漂浮绕大湾，比例度盘往小扳
- 曲线偏离回复慢，积分时间往下降
- 曲线波动周期长，积分时间再加长
- 曲线振荡频率快，先把微分降下来
- 动差大来波动慢，微分时间应加长

> **电赛PID使用经验**：
> 1. **PID必须固定周期调用**！比如每10ms调用一次，不能在主循环里随便调用，周期不稳定微分和积分就不准
> 2. 一定要加输出限幅和积分限幅，否则积分饱和会导致超调很大
> 3. 设定值突变的时候可以清一下积分项，防止积分饱和
> 4. 微分环节对噪声很敏感，如果反馈值噪声大，先做滤波再加D，或者不用D
> 5. 一般的电机调速，PI就够了，不需要D；位置控制和平衡车需要D
> 6. 电赛不要搞什么模糊PID、神经网络PID，就用普通PID，调好了比什么都强，稳定可靠

### 12.2 数字滤波算法——让传感器数据变稳定
传感器读回来的数据往往有噪声，跳来跳去，直接用来控制会导致系统抖动，必须加数字滤波。我给你几个电赛最常用的滤波算法，简单有效。

#### 12.2.1 限幅滤波
适合处理偶然出现的尖峰干扰，比如超声波偶尔测到一个错值：
```c
// 两次值差超过max_delta就认为是干扰，用上一次的值
#define MAX_DELTA 100
float last_value = 0;
float Filter_Limit(float value) {
    if(abs(value - last_value) > MAX_DELTA) {
        return last_value;
    }
    last_value = value;
    return value;
}
```

#### 12.2.2 算术平均滤波
连续读N个值取平均，适合对随机噪声滤波，N越大越平滑，但响应越慢：
```c
#define FILTER_N 10
float Filter_Average(void) {
    uint32_t sum = 0;
    uint8_t i;
    for(i=0; i<FILTER_N; i++) {
        sum += Read_Sensor();
        Delay_us(100);
    }
    return (float)sum / FILTER_N;
}
```

#### 12.2.3 滑动平均滤波
算术平均需要连续读N个，太慢。滑动平均维护一个队列，新数据进来替换最老的，每次取队列平均，既平滑又响应快，最常用：
```c
#define WINDOW_SIZE 10
float filter_buf[WINDOW_SIZE];
uint8_t filter_idx = 0;
float Filter_SlideAverage(float value) {
    uint8_t i;
    float sum = 0;
    filter_buf[filter_idx++] = value;
    if(filter_idx >= WINDOW_SIZE) filter_idx = 0;
    for(i=0; i<WINDOW_SIZE; i++) {
        sum += filter_buf[i];
    }
    return sum / WINDOW_SIZE;
}
```

#### 12.2.4 中值滤波
连续读N个值，排序后取中间值，适合去掉偶尔出现的异常值，比如超声波、红外测距：
```c
#define MEDIAN_N 5
float Filter_Median(void) {
    float buf[MEDIAN_N];
    uint8_t i,j;
    float temp;
    for(i=0; i<MEDIAN_N; i++) buf[i] = Read_Sensor();
    // 冒泡排序
    for(i=0; i<MEDIAN_N-1; i++) {
        for(j=i+1; j<MEDIAN_N; j++) {
            if(buf[i] > buf[j]) {
                temp = buf[i];
                buf[i] = buf[j];
                buf[j] = temp;
            }
        }
    }
    return buf[MEDIAN_N/2]; // 取中间值
}
```

#### 12.2.5 一阶低通滤波
硬件RC低通滤波的数字版本，计算量小，对高频噪声滤波效果好，最常用：
```c
// a:滤波系数0~1，越小越平滑，响应越慢；一般取0.1~0.3
float last_value = 0;
float Filter_LowPass(float value, float a) {
    last_value = a * value + (1 - a) * last_value;
    return last_value;
}
```

> **滤波经验**：
> - 一般用一阶低通+滑动平均组合就够了，大部分传感器都适用
> - 滤波不要太强，否则响应太慢，控制会滞后
> - 超声波先用中值滤波去掉异常值，再用低通滤波平滑
> - ADC数据先多次采样平均，再低通
> - MPU6050自带硬件低通，一般不需要额外滤波

### 12.3 状态机编程思想——写复杂逻辑的神器
很多新手写复杂逻辑喜欢用一堆flag，if-else嵌套好几层，到最后自己都看不懂，bug一堆。状态机是写复杂程序的最佳实践，电赛做复杂功能必用。

状态机核心思想：把程序分成几个有限的状态，每个状态做自己的事，满足条件就跳转到下一个状态，逻辑非常清晰。

举个例子：自动往返小车
- 状态0：前进
- 状态1：遇到障碍物停下
- 状态2：后退
- 状态3：左转
- 回到状态0前进

代码实现：
```c
typedef enum {
    STATE_FORWARD = 0,
    STATE_STOP,
    STATE_BACK,
    STATE_TURN_LEFT,
} CarState;

CarState state = STATE_FORWARD;
uint32_t state_time = 0;

void Car_Process(void) {
    switch(state) {
        case STATE_FORWARD:
            Motor_SetLeft(50);
            Motor_SetRight(50);
            if(Ultrasonic_GetDistance() < 200) { // 前方20cm有障碍
                Motor_Stop();
                state = STATE_STOP;
                state_time = sys_time;
            }
            break;
            
        case STATE_STOP:
            if(sys_time - state_time > 500) { // 停500ms
                state = STATE_BACK;
                state_time = sys_time;
            }
            break;
            
        case STATE_BACK:
            Motor_SetLeft(-50);
            Motor_SetRight(-50);
            if(sys_time - state_time > 1000) { // 后退1秒
                Motor_Stop();
                state = STATE_TURN_LEFT;
                state_time = sys_time;
            }
            break;
            
        case STATE_TURN_LEFT:
            Motor_SetLeft(-30);
            Motor_SetRight(30);
            if(sys_time - state_time > 500) { // 左转500ms
                state = STATE_FORWARD;
            }
            break;
    }
}
```

你看，用状态机写逻辑，每个状态做什么、什么时候跳转一目了然，加新功能只需要加新状态，不会乱成一团麻。电赛所有复杂逻辑：自动流程、菜单、通信协议解析，都用状态机写。

### 12.4 互补滤波与卡尔曼滤波 —— 传感器数据融合

电赛中经常需要把多个传感器的数据融合起来得到更准确的结果。最典型的场景是 MPU6050 的姿态解算：加速度计长期稳定但短期噪声大，陀螺仪短期精确但长期漂移。两者结合才能得到准确的姿态角。

#### 12.4.1 互补滤波（最简单的融合方法）

**原理**：加速度计的低频特性好（长期准确），陀螺仪的高频特性好（短期精确，不会因震动误判）。互补滤波就是把加速度计的低频部分 + 陀螺仪的高频部分 = 完整的姿态角。

$$
\text{角度} = \alpha \times (\text{加速度计角度}) + (1-\alpha) \times (\text{陀螺仪积分角度})
$$

或者写成递推形式（更适合单片机实现）：

$$
\text{角度}_{new} = \alpha \times \text{加速度计角度} + (1-\alpha) \times (\text{角度}_{old} + \text{陀螺仪角速度} \times \Delta t)
$$

其中 $\alpha$ 是互补系数，通常取 0.02~0.05。$\alpha$ 越大，越信任加速度计（响应快但噪声大）；$\alpha$ 越小，越信任陀螺仪（平滑但漂移积累快）。

**代码实现**：

```c
/**
 * @brief  互补滤波姿态解算
 * @param  acc_angle: 加速度计算出的角度（通过反正切 atan2 得到）
 * @param  gyro_rate: 陀螺仪读出的角速度（度/秒）
 * @param  dt: 两次调用的时间间隔（秒）
 * @param  alpha: 互补系数，典型值 0.02（信任加速度计 2%，信任陀螺仪 98%）
 * @return 融合后的角度
 * @note   调用频率：通常 100Hz~200Hz（每 5~10ms 调用一次）
 */
float ComplementaryFilter(float acc_angle, float gyro_rate, float dt, float alpha)
{
    static float fused_angle = 0;  // 融合后的角度（静态变量保持值）
    
    // 陀螺仪积分：角速度 × 时间 = 角度变化量，加到之前的融合角度上
    float gyro_angle = fused_angle + gyro_rate * dt;
    // 例如：gyro_rate = 30°/s, dt = 0.01s → 这段时间旋转了 0.3°
    
    // 互补融合：加权平均
    fused_angle = alpha * acc_angle + (1.0f - alpha) * gyro_angle;
    // alpha * acc_angle       ：信任加速度计的 2%
    // (1-alpha) * gyro_angle  ：信任陀螺仪积分的 98%
    // 两者加权平均 = 融合结果
    
    return fused_angle;
}

// 使用示例（每 10ms 调用一次）：
void MPU6050_Update(void)
{
    float acc_angle, gyro_rate;
    
    // 读取 MPU6050 原始数据
    MPU6050_ReadAccel(&acc_x, &acc_y, &acc_z);
    MPU6050_ReadGyro(&gyro_x, &gyro_y, &gyro_z);
    
    // 从加速度计算角度（用反正切）
    // pitch = atan2(acc_x, sqrt(acc_y^2 + acc_z^2)) * 180 / PI
    acc_angle = atan2(acc_x, sqrt(acc_y*acc_y + acc_z*acc_z)) * 57.2958f;  // 度
    // 57.2958 = 180/PI，弧度转角度
    
    // 陀螺仪角速度直接读取（已经标定为度/秒）
    gyro_rate = gyro_y;  // 绕 Y 轴的角速度影响 pitch
    
    // 互补滤波
    pitch = ComplementaryFilter(acc_angle, gyro_rate, 0.01f, 0.02f);
    // dt=0.01s（10ms），alpha=0.02
}
```

> **互补滤波的优点**：代码极其简单（就一行加权平均），CPU 开销极小（几个浮点运算），调参只调一个 alpha 参数。对于电赛大部分场景（平衡车、倒立摆），互补滤波完全够用。

#### 12.4.2 卡尔曼滤波（进阶，理解原理即可）

卡尔曼滤波是更"数学"的融合方法，它能动态调整信任权重。但代码量较大，浮点运算多。**电赛建议**：如果 DMP 库能用就直接用 DMP（MPU6050 自带硬件姿态解算）。如果自己写，先用互补滤波，卡尔曼是进阶选项。

一维卡尔曼滤波的 5 个公式（你不需要完全理解推导，但要知道每个公式在做什么）：

```
预测阶段（用模型预测下一时刻的状态）：
1. X(k|k-1) = X(k-1|k-1) + u * dt       // 状态预测：上一时刻状态 + 输入 × 时间
2. P(k|k-1) = P(k-1|k-1) + Q             // 协方差预测：上一时刻不确定性 + 过程噪声

更新阶段（用测量值修正预测）：
3. K = P(k|k-1) / (P(k|k-1) + R)         // 卡尔曼增益：决定更相信预测还是测量
4. X(k|k) = X(k|k-1) + K * (Z - X(k|k-1)) // 状态更新：预测 + 增益 × 测量误差
5. P(k|k) = (1 - K) * P(k|k-1)           // 协方差更新：更新后的不确定性
```

**符号说明**：
- X：要估计的状态（如角度）
- P：估计的不确定性（方差）
- Q：过程噪声（模型有多不准确，Q 越大越相信测量值）
- R：测量噪声（传感器有多不准确，R 越大越相信预测值）
- K：卡尔曼增益（权重，自动在 0~1 之间调整）
- Z：测量值（传感器直接读到的值）

> **电赛实战建议**：对于姿态估计（平衡车、倒立摆），直接用 MPU6050 官方 DMP 库获取欧拉角，比自己写卡尔曼简单稳定。只有在 DMP 无法满足需求时（如采样率不够、需要自定义传感器融合），才考虑自己写。

---

### 12.5 FFT 快速傅里叶变换 —— 频域分析利器

#### 12.5.1 什么是 FFT？为什么电赛需要它？

FFT（Fast Fourier Transform）是把**时域信号**（电压随时间变化）转换为**频域信号**（信号由哪些频率成分组成）的算法。

电赛中的典型应用：
- **音频类题目**：分析声音的频率成分（如识别音调、做频谱显示）
- **信号类题目**：测量信号的频率、谐波失真、信噪比
- **电源类题目**：分析纹波的频率成分

> **基础概念**：
> - **时域**：你平时在示波器上看到的波形——横轴是时间，纵轴是电压。
> - **频域**：横轴是频率，纵轴是该频率成分的幅度。
> - **FFT 就是把时域波形"拆解"成不同频率正弦波的组合**。

#### 12.5.2 使用 ARM 官方 DSP 库做 FFT

STM32F103 是 Cortex-M3 内核，可以使用 ARM 官方的 CMSIS-DSP 库。你不必自己写 FFT 算法（那是非常复杂的数学），直接用库函数即可。

**在 Keil 中启用 DSP 库**：
1. 在工程中添加 CMSIS-DSP 库文件（通常在 Keil 安装目录的 `ARM/CMSIS/DSP_Lib` 下）
2. 在 `stm32f10x_conf.h` 或项目配置中定义 `ARM_MATH_CM3`
3. 包含头文件 `#include "arm_math.h"`

**FFT 使用流程**：

```c
#include "arm_math.h"

#define FFT_SIZE 256  // FFT 点数，必须是 2 的幂（64, 128, 256, 512, 1024）

float32_t fft_input[FFT_SIZE * 2];   // 输入：实部+虚部交替存放
float32_t fft_output[FFT_SIZE];      // 输出：各频率的幅度
// 为什么输入是 2 倍？因为 FFT 输入是复数，每个点分实部和虚部
// 所以 256 点 FFT 需要 512 个 float 的数组

/**
 * @brief  初始化并执行 FFT
 * @note   步骤：
 *         1. 用 ADC 采集 FFT_SIZE 个数据点（按固定采样率）
 *         2. 填到 fft_input 数组（实部=采样值，虚部=0）
 *         3. 运行 arm_cfft_f32 做 FFT 变换
 *         4. 运行 arm_cmplx_mag_f32 计算各频率的幅度
 *         5. fft_output[i] 就是第 i 个频率分量的幅度
 *         
 *         频率分辨率 = 采样率 / FFT_SIZE
 *         例如：采样率=25600Hz, FFT_SIZE=256 → 分辨率=100Hz
 *         输出 fft_output[0]=DC分量, fft_output[1]=100Hz分量, [2]=200Hz...
 */
void FFT_Demo(void)
{
    arm_cfft_radix4_instance_f32 fft_inst;  // FFT 实例（radix4 算法）
    
    // 第1步：初始化 FFT 实例
    // arm_cfft_radix4_init_f32(&fft_inst, FFT_SIZE, 0, 1);
    // 参数：实例指针, 点数, 是否反向FFT(0=正向), 是否按位反转(1=是)
    
    // 第2步：填充输入数据
    // 假设 adc_buf[256] 是 ADC 采集的 256 个数据点
    for(uint16_t i = 0; i < FFT_SIZE; i++)
    {
        fft_input[i * 2]     = (float32_t)adc_buf[i];  // 实部 = ADC 采样值
        fft_input[i * 2 + 1] = 0.0f;                     // 虚部 = 0（采样值是实数）
    }
    // 每个采样值占两个位置：实部 + 虚部（交错存放）
    
    // 第3步：执行 FFT
    // arm_cfft_radix4_f32(&fft_inst, fft_input);
    // 输入和输出都在 fft_input 中（原地运算，节省内存）
    // 运算后 fft_input 中存放的是复数频谱（实部+虚部）
    
    // 第4步：计算各频率的幅度（模长）
    // arm_cmplx_mag_f32(fft_input, fft_output, FFT_SIZE);
    // 参数：输入复数数组, 输出幅度数组, 点数
    // 计算：output[i] = sqrt(input[2i]^2 + input[2i+1]^2)
    
    // 第5步：查找峰值频率
    float32_t max_mag = 0;
    uint16_t max_idx = 0;
    for(uint16_t i = 1; i < FFT_SIZE/2; i++)  // 只查前半部分（后半是对称的）
    {
        if(fft_output[i] > max_mag)
        {
            max_mag = fft_output[i];
            max_idx = i;
        }
    }
    // 最大值的索引对应的频率 = max_idx × (采样率 / FFT_SIZE)
    // 例如：采样率=10240Hz, max_idx=50 → 频率=50×(10240/256)=2000Hz
    
    // DC 分量（fft_output[0]）是信号的平均值（直流偏置）
    // Nyquist 频率（fft_output[FFT_SIZE/2]）以上的分量是镜像，不用管
}
```

> **FFT 使用经验**：
> 1. **采样率必须 ≥ 信号最高频率的 2 倍**（奈奎斯特定理），否则会发生频谱混叠。比如要分析最高 5kHz 的信号，采样率必须 ≥10kHz。
> 2. **FFT 点数越大，频率分辨率越高**，但计算量也越大。256 点 FFT 对 72MHz 的 M3 来说只需几毫秒。
> 3. **加窗函数**：如果信号不是整数个周期被采样，会出现"频谱泄漏"。加窗（汉宁窗、汉明窗）可以改善。电赛中如果要求不高可以不加。
> 4. **CMSIS-DSP 库已经优化过**：arm_cfft_radix4_f32 在 72MHz 的 M3 上 256 点 FFT 约需 3~5ms，对于音频分析题完全够用。
> 5. **如需更高性能**：使用 arm_cfft_radix4_q15（16 位定点 FFT），比浮点版本快约 3 倍，精度略低但电赛够用。

---

### 12.6 PID 进阶：积分分离、微分先行、抗饱和

前面讲了基本 PID，这里补充几个电赛中最实用的进阶技巧。

#### 12.6.1 积分分离 PID

**问题**：当设定值大幅度变化时（比如从 0 加速到 1000 转），误差一开始很大，积分项快速累积，导致严重的超调和震荡。

**解决方案**：误差大于某个阈值时，不使用积分项（Ki=0）；误差小于阈值时才开启积分，消除静差。

```c
float PID_Calc_Separate(PID_TypeDef* pid, float feedback)
{
    pid->error = pid->set - feedback;
    
    // 积分分离：误差大于阈值时不积分
    if(fabs(pid->error) > SEPARATE_THRESHOLD)  // 比如阈值=100
    {
        pid->integral = 0;  // 清零积分项
    }
    else
    {
        pid->integral += pid->error;  // 正常积分
        // 积分限幅（别忘了！）
        if(pid->integral > pid->integral_max) pid->integral = pid->integral_max;
        if(pid->integral < -pid->integral_max) pid->integral = -pid->integral_max;
    }
    
    pid->output = pid->Kp * pid->error
                + pid->Ki * pid->integral
                + pid->Kd * (pid->error - pid->last_error);
    
    // 输出限幅
    if(pid->output > pid->output_max) pid->output = pid->output_max;
    if(pid->output < -pid->output_max) pid->output = -pid->output_max;
    
    pid->last_error = pid->error;
    return pid->output;
}
```

#### 12.6.2 微分先行 PID

**问题**：微分项对设定值的突变非常敏感。设定值从 0 跳到 100 时，误差突然变大，微分项产生一个巨大的尖峰。

**解决方案**：微分项不跟踪误差的变化，而是跟踪**反馈值的变化**。反馈值通常是连续的（如电机转速不会突变），积分项就不会产生尖峰。

```c
// 核心改动：微分项 = -Kd × (反馈值变化率)，而不是 Kd × (误差变化率)
float derivative = -(feedback - pid->last_feedback);  // 注意负号！
// 误差 = set - feedback
// 误差变化 = (set_new - fbk_new) - (set_old - fbk_old)
//           = (set_new - set_old) - (fbk_new - fbk_old)
// 如果设定值不变（set_new == set_old），误差变化 = -(fbk变化)
// 但如果设定值突变，用 feedback 的微分就不会受设定值突变影响
```

#### 12.6.3 死区 PID

**问题**：当误差很小时（如转速差只有 1~2 转），PID 输出在 0 附近微小波动，导致电机发出"滋滋"的高频噪声，既不必要又伤电机。

**解决方案**：误差在死区范围内时，输出设为 0。

```c
#define PID_DEADBAND 2.0f  // 死区范围

float PID_Calc_Deadband(PID_TypeDef* pid, float feedback)
{
    pid->error = pid->set - feedback;
    
    // 死区：误差很小时不输出
    if(fabs(pid->error) < PID_DEADBAND)
    {
        pid->output = 0;
        pid->integral = 0;  // 同时清零积分防止累积
        return 0;
    }
    
    // ... 正常 PID 计算 ...
}
```

---

### 12.7 综合实战：编码器 + PID 电机速度闭环控制

这是电赛中最经典、最核心的控制场景。掌握了它，你就掌握了电赛控制类题目 80% 的功底。

#### 12.7.1 系统架构

```
                          ┌─────────────┐
目标速度 ──→ 误差 ──→ PID控制器 ──→ PWM输出 ──→ 电机转动 ──→ 编码器读数
  (RPM)      ↑                                    │              │
             │                                    │              │
             └────────── 反馈速度 ←───────────────┘              │
                       (编码器换算) ←───────────────────────────┘
```

这是一个经典的**负反馈闭环控制系统**。PID 控制器不断比较"目标速度"和"实际速度"（通过编码器测量），根据误差调整 PWM 输出，使实际速度趋近目标速度。

#### 12.7.2 硬件配置

- **电机驱动**：TB6612FNG，PWMA 接 TIM2_CH1（PA0），方向控制接 PB3/PB4
- **编码器**：TIM3 编码器接口，CH1=PA6，CH2=PA7
- **PID 周期**：10ms（TIM2 或 SysTick 提供时基）
- **PWM 频率**：10kHz（TIM2，PSC=71，ARR=99）

#### 12.7.3 完整代码实现（逐部分讲解）

```c
// ========== 全局变量 ==========
PID_TypeDef speed_pid;          // 速度 PID 控制器
int16_t     encode_delta;       // 10ms 内的编码器脉冲增量
float       actual_speed_rpm;   // 实际转速（转/分钟）
float       target_speed_rpm;   // 目标转速（转/分钟）
int16_t     pwm_output;         // PID 计算的 PWM 输出（-100~100）

// 编码器参数（根据你的电机和编码器型号修改）
#define ENCODER_PPR    390      // 编码器线数（每圈脉冲数），常见 390/520/12
#define GEAR_RATIO     30       // 减速比（电机转 30 圈 = 输出轴转 1 圈）
#define PWM_MAX        90       // PWM 最大占空比（留 10% 余量保护电机）

// ========== 初始化 ==========
void MotorSpeedControl_Init(void)
{
    // ---- 编码器初始化 ----
    Encoder_Init();  // TIM3 编码器接口（第六章已讲过）
    
    // ---- PWM 初始化 ----
    TIM2_PWM_Init(); // TIM2_CH1 = PA0，10kHz PWM
    
    // ---- 电机方向控制引脚 ----
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // ---- PID 初始化 ----
    // Kp=0.5, Ki=0.1, Kd=0.0（先只用 PI，不加 D）
    // 输出限幅 ±PWM_MAX，积分限幅 ±200
    PID_Init(&speed_pid, 0.5f, 0.1f, 0.0f, PWM_MAX, 200.0f);
    // 参数说明：
    // Kp=0.5：比例系数，单位是"每 1 RPM 误差产生 0.5% 的 PWM 变化"
    // Ki=0.1：积分系数，消除稳态误差
    // Kd=0.0：微分系数，电机调速一般不需要 D
}

// ========== 电机速度控制（正转/反转/停止）==========
void Motor_SetPWM(int16_t pwm)
{
    // pwm > 0：正转，pwm < 0：反转，pwm == 0：停止
    
    if(pwm > 0)
    {
        // 正转：AIN1=1, AIN2=0
        GPIO_SetBits(GPIOB, GPIO_Pin_3);    // AIN1 = 高
        GPIO_ResetBits(GPIOB, GPIO_Pin_4);   // AIN2 = 低
        // 限制最大输出
        if(pwm > PWM_MAX) pwm = PWM_MAX;
        TIM_SetCompare1(TIM2, pwm);          // 设置 PWM 占空比
    }
    else if(pwm < 0)
    {
        // 反转：AIN1=0, AIN2=1
        GPIO_ResetBits(GPIOB, GPIO_Pin_3);
        GPIO_SetBits(GPIOB, GPIO_Pin_4);
        // pwm 取绝对值
        int16_t abs_pwm = -pwm;
        if(abs_pwm > PWM_MAX) abs_pwm = PWM_MAX;
        TIM_SetCompare1(TIM2, abs_pwm);
    }
    else
    {
        // 停止：AIN1=0, AIN2=0（电机惯性滑行）
        // 或 AIN1=1, AIN2=1（电机刹车/短路制动）
        GPIO_ResetBits(GPIOB, GPIO_Pin_3);
        GPIO_ResetBits(GPIOB, GPIO_Pin_4);
        TIM_SetCompare1(TIM2, 0);
    }
}

// ========== 读取编码器并计算实际转速 ==========
float Encoder_GetSpeedRPM(void)
{
    // 步骤1：读编码器增量（TIM3 计数器值）
    // 注意：用 int16_t 接收，因为编码器反转时 CNT 会递减
    encode_delta = (int16_t)TIM_GetCounter(TIM3);
    
    // 步骤2：清零计数器，准备下一个周期的测量
    TIM_SetCounter(TIM3, 0);
    
    // 步骤3：把脉冲增量转换为转速（RPM = 转/分钟）
    // 公式推导：
    //   编码器一转的脉冲数 = ENCODER_PPR × 4（4倍频）× GEAR_RATIO（减速比）
    //   10ms 内的脉冲数 = encode_delta
    //   10ms 内的圈数 = encode_delta / (ENCODER_PPR × 4 × GEAR_RATIO)
    //   1 分钟（60000ms）内的圈数 = 上面的值 × (60000 / 10)
    //   RPM = encode_delta × 60000 / (ENCODER_PPR × 4 × GEAR_RATIO × 10)
    //        = encode_delta × 6000 / (ENCODER_PPR × 4 × GEAR_RATIO)
    
    float rpm = (float)encode_delta * 6000.0f /
                (float)(ENCODER_PPR * 4 * GEAR_RATIO);
    // 举例：encode_delta=195, PPR=390, 4倍频, 减速比=30
    //   rpm = 195 × 6000 / (390 × 4 × 30) = 1,170,000 / 46,800 = 25 RPM
    
    return rpm;
}

// ========== 速度控制主循环（每 10ms 调用一次）==========
// 这个函数放在主循环的 10ms 任务中
void MotorSpeedControl_Loop(void)
{
    // 第1步：读取实际转速
    actual_speed_rpm = Encoder_GetSpeedRPM();
    
    // 第2步：用 PID 计算 PWM 输出
    speed_pid.set = target_speed_rpm;           // 设定目标速度
    pwm_output = (int16_t)PID_Calc(&speed_pid, actual_speed_rpm);
    // PID_Calc 的返回值就是 PID 算出的 PWM 值（-100~100）
    
    // 第3步：输出到电机
    Motor_SetPWM(pwm_output);
}

// ========== 完整主函数示例 ==========
int main(void)
{
    // ... 基础初始化（SysTick、NVIC 等）...
    
    MotorSpeedControl_Init();  // 初始化电机 + 编码器 + PID
    
    target_speed_rpm = 100.0f;  // 设定目标：100 RPM
    
    uint32_t t_10ms = 0;
    
    while(1)
    {
        // ---- 每 10ms 执行一次速度控制 ----
        if(sys_time - t_10ms >= 10)
        {
            t_10ms = sys_time;
            MotorSpeedControl_Loop();  // PID 速度控制
        }
        
        // ---- 每 100ms 刷新一次显示 ----
        if(sys_time % 100 == 0)
        {
            OLED_ShowString(0, 0, "Target:", 12);
            OLED_ShowNum(48, 0, (uint32_t)target_speed_rpm, 4, 12);
            OLED_ShowString(0, 2, "Actual:", 12);
            OLED_ShowNum(48, 2, (uint32_t)actual_speed_rpm, 4, 12);
            OLED_ShowString(0, 4, "PWM:", 12);
            OLED_ShowNum(36, 4, (uint32_t)pwm_output, 4, 12);
        }
        
        IWDG_Feed();  // 喂狗
    }
}
```

#### 12.7.4 速度闭环调参实录

调参是电赛的核心技能。下面是一次典型的电机速度闭环调参过程：

```
步骤1：Ki=0, Kd=0，从 Kp=0.1 开始
  现象：电机转得很慢，实际 20 RPM，目标 100 RPM → Kp 太小
  分析：误差=80，输出=80×0.1=8% PWM，不足以驱动电机
  解决：加大 Kp 到 0.5

步骤2：Kp=0.5, Ki=0, Kd=0
  现象：电机能转到 90 RPM，但始终到不了 100 RPM，有 10 RPM 静差
  分析：P 控制有静差是必然的——误差越小输出越小，最终小到不足以克服摩擦
  解决：加入 Ki=0.05

步骤3：Kp=0.5, Ki=0.05, Kd=0
  现象：能到 100 RPM，但在目标附近缓慢波动（±3 RPM）
  分析：积分项在弥补误差，但有点过头，来回超调
  解决：Ki 稍微加大到 0.1

步骤4：Kp=0.5, Ki=0.1, Kd=0
  现象：速度稳定在 100±1 RPM，响应时间约 200ms
  评价：完美！PI 控制即可，不需要 D

步骤5：测试动态响应
  操作：目标速度 50 → 100（阶跃变化）
  现象：约 200ms 稳定，没有过冲
  评价：响应速度可接受，控制稳定
```

#### 12.7.5 扩展：位置闭环（角度/距离控制）

在速度闭环的基础上，再加一层外环就是位置闭环。常用于舵机角度控制、小车行驶距离控制。

```
位置闭环架构（串级 PID）：

目标位置 → 位置误差 → 位置PID → 目标速度 → 速度误差 → 速度PID → PWM → 电机 → 编码器
   ↑                                                            │          │
   └────────────── 反馈位置 ←────────────── 位置积分 ←────────────┘          │
           (编码器脉冲累积)                                                   │
```

```c
// 位置 PID 的外环
PID_TypeDef pos_pid;   // 位置环 PID
int32_t total_encoder_count = 0;  // 编码器脉冲累积（32位，防止溢出）

// 每 10ms 更新位置
void PositionControl_Loop(void)
{
    // 更新编码器累积脉冲（位置）
    int16_t delta = (int16_t)TIM_GetCounter(TIM3);
    TIM_SetCounter(TIM3, 0);
    total_encoder_count += delta;  // 累积脉冲 = 总位置
    
    // 外环：位置 PID → 输出目标速度
    pos_pid.set = target_position;  // 目标位置（编码器脉冲数）
    target_speed_rpm = PID_Calc(&pos_pid, (float)total_encoder_count);
    // 位置 PID 的输出作为速度环的目标值！
    
    // 内环：速度 PID → 输出 PWM
    speed_pid.set = target_speed_rpm;
    actual_speed_rpm = Encoder_GetSpeedRPM();
    pwm_output = (int16_t)PID_Calc(&speed_pid, actual_speed_rpm);
    
    Motor_SetPWM(pwm_output);
}
```

> **串级 PID 调参顺序**：
> 1. 先调内环（速度环）：把外环断开，手动给速度目标值，调好 Kp/Ki
> 2. 再调外环（位置环）：内环已经稳定，调外环的 Kp（通常只需很小的 P，如 0.01）

---

### 12.8 MPU6050 DMP 姿态解算实战

MPU6050 是电赛做平衡车、倒立摆、姿态检测必用的六轴传感器（三轴加速度 + 三轴陀螺仪）。**DMP（Digital Motion Processor，数字运动处理器）** 是 MPU6050 内部的硬件协处理器，可以直接输出四元数或欧拉角，不需要你手动写复杂的数据融合算法。

#### 12.8.1 为什么用 DMP 而不是自己写融合算法？

| 方案             | 优势                                           | 劣势                           | 电赛推荐         |
| ---------------- | ---------------------------------------------- | ------------------------------ | ---------------- |
| 手动互补滤波     | 代码简单（一行加权平均），CPU 占用低           | 精度一般，大动态时漂移         | 简单项目可用     |
| 手动卡尔曼滤波   | 精度最高，可调参数多                           | 代码复杂，调参困难，CPU 占用高 | 不推荐电赛自写   |
| **DMP 硬件解算** | **精度高、CPU 占用极低（硬件完成）、输出稳定** | 需要移植官方库，I2C 通信开销   | **强烈推荐电赛** |

> **DMP 会把复杂的姿态解算（加速度+陀螺仪数据融合）在 MPU6050 芯片内部完成，STM32 只需要通过 I2C 读取结果。** 这大大降低了代码复杂度和 CPU 占用。

#### 12.8.2 DMP 库的移植步骤

**第 1 步：获取 DMP 库文件**

DMP 库通常包含在 MPU6050 的官方例程中（InvenSense 公司提供）。你需要以下文件：

```
inv_mpu.c / inv_mpu.h       ← MPU6050 驱动核心，DMP 初始化在这里
inv_mpu_dmp_motion_driver.c / .h  ← DMP 运动驱动
dmpKey.h                    ← DMP 固件镜像（约 3KB 的二进制数据）
dmpmap.h                    ← DMP 固件的内存映射
```

这些文件可以从正点原子、野火的 MPU6050 例程中找到。把它们复制到你的 `Hardware/MPU6050/` 文件夹。

**第 2 步：适配 I2C 读写函数**

DMP 库需要调用底层的 I2C 读写函数，你必须提供以下接口：

```c
// DMP 库会调用这些函数来读写 MPU6050，你需要用你的 I2C 驱动来实现它们

// 向 MPU6050 寄存器写一个字节
int i2c_write(unsigned char slave_addr, unsigned char reg_addr,
              unsigned char length, unsigned char const *data)
{
    // slave_addr = MPU6050 的 I2C 地址（0x68 或 0x69）
    // reg_addr   = 寄存器地址
    // length     = 数据长度
    // data       = 要写入的数据
    
    I2C_Start();
    I2C_SendByte(slave_addr << 1);  // 地址 + 写位
    if(I2C_WaitAck()) return -1;
    I2C_SendByte(reg_addr);         // 寄存器地址
    if(I2C_WaitAck()) return -1;
    for(unsigned char i = 0; i < length; i++)
    {
        I2C_SendByte(data[i]);      // 数据
        if(I2C_WaitAck()) return -1;
    }
    I2C_Stop();
    return 0;  // 成功
}

// 从 MPU6050 寄存器读若干字节
int i2c_read(unsigned char slave_addr, unsigned char reg_addr,
             unsigned char length, unsigned char *data)
{
    I2C_Start();
    I2C_SendByte(slave_addr << 1);  // 地址 + 写位
    if(I2C_WaitAck()) { I2C_Stop(); return -1; }
    I2C_SendByte(reg_addr);         // 寄存器地址
    if(I2C_WaitAck()) { I2C_Stop(); return -1; }
    
    I2C_Start();                    // 重复起始信号
    I2C_SendByte((slave_addr << 1) | 0x01);  // 地址 + 读位
    if(I2C_WaitAck()) { I2C_Stop(); return -1; }
    for(unsigned char i = 0; i < length; i++)
    {
        data[i] = I2C_ReadByte(i < length - 1 ? 1 : 0);
        // 最后一个字节发 NACK（参数=0），其余发 ACK（参数=1）
    }
    I2C_Stop();
    return 0;  // 成功
}

// 微秒级延时（DMP 库内部需要）
// 如果已经有 SysTick_Delay_us，直接用
#define delay_ms(ms)  Delay_ms(ms)
#define get_ms()      sys_time  // 返回系统毫秒时间
```

**第 3 步：初始化 MPU6050 + DMP**

```c
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"

float pitch, roll, yaw;  // 欧拉角（俯仰/横滚/偏航，单位：度）

uint8_t MPU6050_DMP_Init(void)
{
    uint8_t res;
    
    // ---- 1. 初始化 MPU6050 ----
    // mpu_init() 内部会：
    //   - 复位 MPU6050
    //   - 设置时钟源为陀螺仪 X 轴 PLL
    //   - 设置采样率 200Hz
    //   - 设置陀螺仪量程 ±2000°/s
    //   - 设置加速度计量程 ±2g
    //   - 设置数字低通滤波器
    res = mpu_init();
    if(res != 0) return 1;  // 初始化失败
    
    // ---- 2. 获取传感器数据就绪 -----
    // 设置传感器数据来源和速率
    // 参数：加速度计 200Hz, 陀螺仪 200Hz
    mpu_set_sensors(INV_XYZ_GYRO | INV_XYZ_ACCEL);
    mpu_set_sample_rate(200);    // 采样率 200Hz
    mpu_configure_fifo(INV_XYZ_GYRO | INV_XYZ_ACCEL);
    
    // ---- 3. 加载 DMP 固件 ----
    // dmp_load_motion_driver_firmware() 会：
    //   把 dmpKey.h 中的 3062 字节固件写入 MPU6050 的 DMP 内存
    //   这个固件是 InvenSense 提供的闭源二进制文件
    res = dmp_load_motion_driver_firmware();
    if(res != 0) return 2;  // 固件加载失败
    
    // ---- 4. 设置 DMP 功能 ----
    // 使能 DMP 的陀螺仪和加速度计数据源
    dmp_set_orientation(inv_orientation_matrix_to_scalar(
        inv_orientation_matrix_identity()));
    
    // 设置 DMP 功能
    // DMP_FEATURE_6X_LP_QUAT：六轴低功耗四元数（推荐）
    // DMP_FEATURE_SEND_RAW_ACCEL / GYRO：也输出原始数据
    // DMP_FEATURE_SEND_CAL_GYRO：输出校准后的陀螺仪数据
    uint16_t dmp_features = DMP_FEATURE_6X_LP_QUAT |
                            DMP_FEATURE_SEND_RAW_ACCEL |
                            DMP_FEATURE_SEND_CAL_GYRO;
    dmp_set_fifo_rate(200);  // DMP 输出速率 200Hz
    res = dmp_enable_feature(dmp_features);
    if(res != 0) return 3;  // 使能失败
    
    // ---- 5. 启动 DMP ----
    dmp_set_state(1);  // 1 = 启动 DMP，0 = 停止
}
```

**第 4 步：读取姿态角**

```c
/**
 * @brief  读取 MPU6050 DMP 解算的姿态角
 * @return 0=读取成功, 1=没有新数据
 * @note   DMP 输出的四元数由 dmp_read_fifo 读取
 *         然后通过内置的转换函数得到欧拉角
 */
uint8_t MPU6050_DMP_GetAngle(void)
{
    short gyro[3], accel[3], sensors;
    long quat[4];           // 四元数 [w, x, y, z]
    unsigned long timestamp;
    unsigned char more;
    
    // ---- 1. 从 FIFO 读取 DMP 数据 ----
    // dmp_read_fifo 从 MPU6050 的 FIFO 缓冲区读取一帧数据
    // 返回值 0 表示成功读取
    if(dmp_read_fifo(gyro, accel, quat, &timestamp, &sensors, &more) != 0)
        return 1;  // FIFO 没有新数据
    
    // ---- 2. 检查是否有四元数数据 ----
    // sensors 的第 0 位 (INV_WXYZ_QUAT) = 1 表示有四元数
    if(sensors & INV_WXYZ_QUAT)
    {
        // ---- 3. 四元数 → 欧拉角 ----
        // 四元数 q=(w,x,y,z) 的三个欧拉角：
        // Pitch  = asin(2×(w×y - z×x))
        // Roll   = atan2(2×(w×x + y×z), 1 - 2×(x² + y²))
        // Yaw    = atan2(2×(w×z + x×y), 1 - 2×(y² + z²))
        
        // 将四元数转换为 10³⁰ 倍的 q30 格式（DMP 库内部使用）
        float q0 = quat[0] / 1073741824.0f;  // quat[0] / 2^30
        float q1 = quat[1] / 1073741824.0f;
        float q2 = quat[2] / 1073741824.0f;
        float q3 = quat[3] / 1073741824.0f;
        
        // 计算欧拉角（弧度 → 度，× 180/π = 57.2958）
        pitch = asin(-2.0f * q1 * q3 + 2.0f * q0 * q2) * 57.2958f;
        roll  = atan2(2.0f * q2 * q3 + 2.0f * q0 * q1,
                      -2.0f * q1 * q1 - 2.0f * q2 * q2 + 1.0f) * 57.2958f;
        yaw   = atan2(2.0f * q1 * q2 + 2.0f * q0 * q3,
                      -2.0f * q2 * q2 - 2.0f * q3 * q3 + 1.0f) * 57.2958f;
        // 注意：Yaw（偏航角）用陀螺仪积分得到，长时间会漂移
        // 如果不加磁力计，Yaw 漂移是正常的（几度/分钟）
        
        return 0;
    }
    
    return 1;  // 没有四元数数据
}

// ===== 使用示例：平衡车角度读取 =====
void BalanceCar_Loop(void)
{
    if(MPU6050_DMP_GetAngle() == 0)
    {
        // pitch = 前倾/后仰角（平衡车最关心的角度）
        // roll  = 左右倾斜角
        // yaw   = 转向角（漂移大，不单独依赖）
        
        // PID 平衡控制：目标 pitch = 0（垂直），实际 pitch 是 DMP 读出的角度
        float balance_pwm = PID_Calc(&balance_pid, pitch);
        
        // 输出到电机
        Motor_SetLeft(balance_pwm);
        Motor_SetRight(balance_pwm);
    }
}
```

#### 12.8.3 MPU6050 使用注意事项

1. **安装至关重要**：MPU6050 必须水平安装在平衡车/倒立摆的重心位置。倾斜安装会导致重力分量耦合到其他轴，角度永远算不准。
2. **上电校准**：DMP 需要在上电后静止 1~2 秒来校准陀螺仪零偏。期间不要碰模块。
3. **震动隔离**：电机震动会严重影响加速度计读数。用泡沫胶或弹簧减震，不要在电机旁边硬连接。
4. **I2C 通信速率**：建议用 400kHz 快模式，否则 200Hz DMP 数据来不及读取。
5. **FIFO 溢出处理**：如果读取速度跟不上 DMP 输出速度，FIFO 会溢出。表现是 `dmp_read_fifo` 返回非 0。提高主循环频率或降低 DMP 输出频率可解决。

---

## 第十三章 电赛实战经验与踩坑总结
这一章是无数电赛前辈用血泪换来的经验，比代码还重要。

### 13.1 代码架构最佳实践
1. **模块化编程**：每个外设/模块单独一个.c和.h文件，不要把所有代码都写在main.c里，main.c只放主循环和状态机
2. **分层设计**：
   - 底层：GPIO、定时器、ADC、I2C、SPI等硬件驱动
   - 模块层：OLED、电机、超声波、MPU6050等模块驱动
   - 算法层：PID、滤波、状态机
   - 应用层：主逻辑、任务调度
3. **非阻塞架构**：不要用delay_ms()这种阻塞延时，用sys_time时基做非阻塞延时，主循环一直跑，响应所有事件
4. **任务调度**：用系统时基做简单的时间片调度，不同任务按不同周期执行：
   ```c
   if(sys_time - time_1ms >= 1) { time_1ms = sys_time; /* 1ms任务：读编码器 */ }
   if(sys_time - time_10ms >= 10) { time_10ms = sys_time; /* 10ms任务：PID计算 */ }
   if(sys_time - time_50ms >= 50) { time_50ms = sys_time; /* 50ms任务：读传感器 */ }
   if(sys_time - time_100ms >= 100) { time_100ms = sys_time; /* 100ms任务：刷新OLED */ }
   ```

### 13.2 硬件电路注意事项
1. **电源是万恶之源**：90%的奇怪问题都是电源导致的
   - 单片机电源引脚必须接0.1uF陶瓷电容滤波
   - 电机等大电流设备单独供电，和单片机共地
   - 电源入口加100uF以上电解电容+0.1uF陶瓷电容
   - 电池容量要够，线要粗，不要用杜邦线走大电流
2. **干扰问题**：
   - 电机线、电源线不要和信号线平行走
   - 所有模块电源加滤波电容
   - 继电器、电机等感性负载加续流二极管
   - 模拟输入线加RC滤波
3. **可靠性设计**：
   - 重要接口用光耦隔离
   - 按键加硬件消抖电容
   - 接线要焊牢，不要只插面包板，容易接触不良
   - 所有接口留余量，不要接反

### 13.3 常见BUG排查方法
1. **程序不运行**：
   - 检查BOOT引脚是否接对，BOOT0接GND才是从Flash启动
   - 检查晶振是否起振
   - 检查复位电路
   - 用仿真器看程序停在哪里
2. **外设不工作**：
   - 第一检查时钟有没有开！开对总线没有！
   - 第二检查引脚模式配置对不对
   - 第三检查引脚有没有接错，有没有共地
   - 第四检查中断有没有配置，优先级对不对
3. **程序乱跑、复位**：
   - 检查是不是栈溢出了，不要定义太大的局部数组
   - 检查是不是数组越界了
   - 检查是不是野指针
   - 检查电源是不是稳定
   - 检查是不是中断标志没清
4. **数据不对、跳变**：
   - 检查是不是没加滤波
   - 检查是不是电源干扰
   - 检查参考电压是不是稳定
   - 检查是不是时钟配置错了

### 13.4 电赛四天三夜时间安排
- **第一天上午**：确定题目方案，画电路图，列元器件清单
- **第一天下午**：搭硬件电路，焊接，测试电源和最小系统
- **第一天晚上**：写各个模块驱动，一个个测试
- **第二天**：写核心控制算法，调通主要功能
- **第三天**：完善功能，优化参数，做稳定性测试，写设计报告
- **第四天上午**：最终测试，封箱，准备答辩

**最重要的经验**：
- 先做基础功能，再做发挥部分，不要上来就搞最难的
- 每写一个模块就测试一个模块，不要等所有代码写完一起调，到时候根本找不到bug
- 留足够的时间写报告和测试，报告占分很多
- 准备好常用模块的现成代码，不要现场从零开始写
- 注意休息，不要熬通宵，脑子不清醒写的代码全是bug

---

## 第十四章 附录：标准库速查与代码模板
### 14.1 总线时钟速查表
| 总线 | 时钟  | 挂接的外设                                                           | 开时钟函数             |
| ---- | ----- | -------------------------------------------------------------------- | ---------------------- |
| AHB  | 72MHz | DMA1/DMA2、SDIO、FSMC                                                | RCC_AHBPeriphClockCmd  |
| APB2 | 72MHz | GPIOA-G、AFIO、USART1、TIM1、SPI1、ADC1/2、EXTI                      | RCC_APB2PeriphClockCmd |
| APB1 | 36MHz | TIM2-TIM7、USART2/3、SPI2/I2C1/2、USB、CAN、BKP、PWR、IWDG/WWDG、RTC | RCC_APB1PeriphClockCmd |

### 14.2 常用代码模板清单
你应该提前准备好这些模板，电赛直接复制用：
1. 工程基础模板：main.c、启动文件、时钟配置
2. 系统时基+延时函数模板
3. GPIO输入输出模板
4. 定时器中断模板
5. PWM输出模板
6. 输入捕获模板
7. 编码器接口模板
8. ADC单通道/多通道DMA模板
9. 串口+printf模板
10. 软件I2C/SPI模板
11. OLED显示驱动模板
12. TB6612电机驱动模板
13. 超声波测距模板
14. MPU6050+DMP模板
15. NRF24L01无线模板
16. PID算法模板
17. 常用滤波函数模板
18. 看门狗模板
19. Flash存储/掉电保存模板
20. 非阻塞按键扫描模板（单击、双击、长按）
21. CAN总线收发模板
22. 状态机编程框架模板

---

### 14.3 外设时钟对应总线速查

| 外设        | 总线 | 开时钟函数                                                                 |
| ----------- | ---- | -------------------------------------------------------------------------- |
| GPIOA~E     | APB2 | `RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOx, ENABLE)`                     |
| AFIO        | APB2 | `RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE)`                      |
| USART1      | APB2 | `RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE)`                    |
| USART2/3    | APB1 | `RCC_APB1PeriphClockCmd(RCC_APB1Periph_USARTx, ENABLE)`                    |
| TIM1        | APB2 | `RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE)`                      |
| TIM2/3/4    | APB1 | `RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIMx, ENABLE)`                      |
| SPI1        | APB2 | `RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE)`                      |
| SPI2/I2C    | APB1 | `RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE)`                      |
| ADC1/2      | APB2 | `RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADCx, ENABLE)`                      |
| DMA1/2      | AHB  | `RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMAx, ENABLE)`                        |
| CAN1        | APB1 | `RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE)`                      |
| IWDG        | 无   | 不要开时钟（独立于总线系统）                                               |
| WWDG        | APB1 | `RCC_APB1PeriphClockCmd(RCC_APB1Periph_WWDG, ENABLE)`                      |
| RTC/BKP/PWR | APB1 | `RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR \| RCC_APB1Periph_BKP, ENABLE)` |

> **记忆口诀**：APB2 上跑"高一串定模"——GPIO、USART1、SPI1、TIM1、ADC。其余外设全在 APB1。

### 14.4 STM32F103C8T6 引脚功能速查

| 引脚 | 默认功能    | 复用功能                 | 电赛常用                   |
| ---- | ----------- | ------------------------ | -------------------------- |
| PA0  | GPIO / ADC0 | TIM2_CH1 / WKUP          | ADC / PWM / 唤醒           |
| PA1  | GPIO / ADC1 | TIM2_CH2                 | ADC / PWM / 超声波Echo     |
| PA2  | GPIO / ADC2 | TIM2_CH3 / USART2_TX     | ADC / PWM / 串口2 TX       |
| PA3  | GPIO / ADC3 | TIM2_CH4 / USART2_RX     | ADC / PWM / 串口2 RX       |
| PA4  | GPIO / ADC4 | SPI1_NSS                 | SPI 片选 / ADC             |
| PA5  | GPIO / ADC5 | **SPI1_SCK**             | SPI 时钟（硬件SPI标配）    |
| PA6  | GPIO / ADC6 | **SPI1_MISO** / TIM3_CH1 | SPI MISO / TIM3编码器      |
| PA7  | GPIO / ADC7 | **SPI1_MOSI** / TIM3_CH2 | SPI MOSI / TIM3编码器      |
| PA8  | GPIO        | TIM1_CH1 / MCO           | TIM1 PWM                   |
| PA9  | GPIO        | **USART1_TX** / TIM1_CH2 | 串口1 TX                   |
| PA10 | GPIO        | **USART1_RX** / TIM1_CH3 | 串口1 RX                   |
| PA11 | GPIO        | **CAN1_RX** / USART1_CTS | CAN RX                     |
| PA12 | GPIO        | **CAN1_TX** / USART1_RTS | CAN TX                     |
| PA13 | **SWDIO**   | -                        | **SWD 调试，勿占用！**     |
| PA14 | **SWCLK**   | -                        | **SWD 调试，勿占用！**     |
| PB0  | GPIO / ADC8 | **TIM3_CH3**             | ADC / PWM                  |
| PB1  | GPIO / ADC9 | **TIM3_CH4**             | ADC / PWM                  |
| PB6  | GPIO        | **I2C1_SCL** / TIM4_CH1  | 软件 I2C SCL               |
| PB7  | GPIO        | **I2C1_SDA** / TIM4_CH2  | 软件 I2C SDA               |
| PB10 | GPIO        | I2C2_SCL / USART3_TX     | USART3 / I2C               |
| PB11 | GPIO        | I2C2_SDA / USART3_RX     | USART3 / I2C               |
| PB13 | GPIO        | **SPI2_SCK**             | SPI2 时钟                  |
| PB14 | GPIO        | **SPI2_MISO**            | SPI2 MISO                  |
| PB15 | GPIO        | **SPI2_MOSI**            | SPI2 MOSI                  |
| PC13 | GPIO        | -                        | 核心板LED（低电平亮）      |
| PC14 | OSC32_IN    | -                        | **32.768kHz 晶振，勿占用** |
| PC15 | OSC32_OUT   | -                        | **32.768kHz 晶振，勿占用** |

### 14.5 Keil 调试速查

| 操作          | 快捷键   | 用途               |
| ------------- | -------- | ------------------ |
| 开始/停止调试 | Ctrl+F5  | 进入/退出调试模式  |
| 运行          | F5       | 全速运行           |
| 单步进入      | F11      | 进入函数内部       |
| 单步跳过      | F10      | 不进入函数         |
| 设置断点      | F9       | 运行到此停         |
| 运行到光标    | Ctrl+F10 | 运行到当前光标行停 |

**调试窗口**（View 菜单）：Watch 窗口（看变量值）、Peripherals → System Viewer（看所有外设寄存器）、Call Stack（看调用栈）。

### 14.6 常见问题排查流程

**外设不工作**，按顺序查：
> 1️⃣ 时钟开了吗？开对总线了吗？（APB1 vs APB2 看速查表）
> 2️⃣ GPIO 模式配对了没有？（AF_PP vs Out_PP vs IN_FLOATING）
> 3️⃣ 引脚接对了吗？共地了吗？
> 4️⃣ 配置参数算对了没有？（波特率、PSC/ARR等）
> 5️⃣ NVIC 中断配了没有？中断函数名写对了没有？
> 6️⃣ 用 Keil 的 Peripherals → System Viewer 看外设寄存器实际值！

**HardFault 最常见原因**：
> 1️⃣ 数组越界（`buf[10]` 但最大索引是 9）
> 2️⃣ 栈溢出（函数内定义了超大数组，如 `uint8_t buf[2048]` 在局部变量）
> 3️⃣ 野指针（未初始化指针或 free 后继续使用）

### 14.7 电赛常用计算公式速查

| 用途            | 公式                                                             |
| --------------- | ---------------------------------------------------------------- |
| 定时器溢出时间  | $T = (ARR+1) \times (PSC+1) \div 72\text{MHz}$                   |
| PWM 频率        | $f = 72\text{MHz} \div ((ARR+1) \times (PSC+1))$                 |
| PWM 占空比      | $D = CCR \div (ARR+1) \times 100\%$                              |
| ADC 转电压      | $V = ADC\_value \times 3.3\text{V} \div 4096$                    |
| 超声波测距      | $d(\text{cm}) = Time(\mu s) \times 0.034 \div 2$                 |
| 舵机角度→CCR    | $CCR = 500 + angle \times 2000 \div 180$                         |
| 编码器转速(RPM) | $RPM = delta\_cnt \times 60000 \div (PPR \times 4 \times T\_ms)$ |
| IWDG 超时时间   | $T = RLR \times Prescaler \div 40000$（LSI≈40kHz，精度±30%）     |
| 一阶低通滤波    | $y[n] = \alpha \cdot x[n] + (1-\alpha) \cdot y[n-1]$             |

### 14.8 电赛常见题型分析与系统设计思路

#### 14.8.1 控制类题目 —— 电赛最热门方向

控制类题目（约占电赛题量的 40%）的核心是**传感器 → 控制器 → 执行器**的闭环系统。

**历年典型题目**：

| 年份 | 题目                   | STM32 承担的角色                                | 关键技术点                           |
| ---- | ---------------------- | ----------------------------------------------- | ------------------------------------ |
| 2013 | 倒立摆                 | 读取 MPU6050 姿态 → PID 控制 → PWM 驱动电机     | DMP 姿态解算、串级 PID、编码器反馈   |
| 2015 | 风力摆                 | 读取角度传感器 → PID 控制风向 → PWM 驱动风机    | 二维 PID、坐标系变换                 |
| 2017 | 滚球控制系统           | 摄像头/触摸屏输入 → PID 控制舵机 → 调整平板角度 | 图像处理、位置 PID、舵机控制         |
| 2019 | 电磁炮/模拟曲射炮      | 角度传感器 → 弹道计算 → PWM 控制发射            | 物理建模、精确 PWM 定时              |
| 2021 | 智能送药小车           | 巡线传感器 + 编码器 → 路径规划 → PID 控制双轮   | 双轮差速、路径 PID、状态机、无线通信 |
| 2023 | 运动目标控制与自动追踪 | 摄像头 + 舵机云台 → PID 追踪 → 激光笔指向       | 图像处理、双轴 PID、坐标系标定       |

**控制类系统的通用架构**：

```
┌─────────────────────────────────────────────────┐
│                  控制类系统架构                    │
│                                                 │
│  ┌──────────┐    ┌──────────┐    ┌──────────┐  │
│  │  传感器   │───→│  控制器   │───→│  执行器   │  │
│  │          │    │ (STM32)  │    │          │  │
│  │ MPU6050  │    │ PID计算   │    │ 直流电机  │  │
│  │ 编码器   │    │ 状态机    │    │ 舵机     │  │
│  │ 超声波   │    │ 滤波算法  │    │ 步进电机  │  │
│  │ 摄像头   │    │ 通信处理  │    │ 风机     │  │
│  └──────────┘    └──────────┘    └──────────┘  │
│       ↑               │               │        │
│       └─── 反馈 ──────┘               │        │
│                                       ↓        │
│                              ┌──────────────┐  │
│                              │ 显示/通信/存储 │  │
│                              │ OLED/蓝牙/SD  │  │
│                              └──────────────┘  │
└─────────────────────────────────────────────────┘
```

**控制类题目的时间分配建议**（4 天 3 夜）：
- **前 4 小时**：确定方案、画电路图、列出详细模块清单
- **第 1 天**：搭硬件、测试每个模块（逐个验证，不要一锅端）
- **第 2 天**：写控制算法、调 PID 参数（这是最花时间的部分）
- **第 3 天**：系统联调、稳定性测试、临界条件测试
- **第 4 天上午**：最终测试、录制演示视频、整理代码
- **第 4 天下午**：写设计报告、封装作品

#### 14.8.2 仪器类题目

仪器类题目（约占 30%）的核心是**信号采集 → 信号处理 → 结果显示**。

**关键外设组合**：
- ADC 多通道采集 + DMA（采集传感器信号）
- 定时器输入捕获（测频率/周期/脉宽）
- FFT 频谱分析（CMSIS-DSP 库，分析信号频率成分）
- OLED 显示测量结果
- 串口输出到上位机

**典型题目**：数字频率计、简易示波器、LCR 测量仪、音频频谱分析仪。

#### 14.8.3 系统设计铁律（十条）

1. **先做基本功能，再做发挥部分**：基本功能 100% 可靠了，再往上加花活。不要上来就想做最难的。
2. **每个模块单独测试**：不要全部焊完、写完代码才上电。焊一个模块测一个，写一个驱动验证一个。
3. **电源是 90% 问题的根源**：电源不稳 → ADC 跳变、MCU 复位、电机抖动。每路电源独立供电，加足滤波电容。
4. **留调试接口**：至少留一路串口（printf 调试），关键节点留测试点。不要"一次成型"后没有调试手段。
5. **硬件冗余设计**：关键模块带备份（比如多带一个最小系统板、多带一个电机驱动）。不怕一万，就怕万一。
6. **代码版本管理**：每完成一个重要功能就备份一版代码。用 `v1.0_基础功能`, `v2.0_PID调试完成` 这种命名。
7. **不要熬夜写代码**：凌晨 3 点写的代码，第二天早上看全是 bug。累了就睡，清醒时效率是疲劳时的 5 倍。
8. **开关/按键做消抖**：硬件消抖（并 0.1μF 电容）+ 软件消抖（延时确认），否则按键会"鬼触"。
9. **看门狗必须开**：任何参加评审的作品必须开 IWDG。评委测试时系统死机 = 直接淘汰。
10. **报告同样重要**：设计报告通常占 30%~40% 的分数。不要只埋头做硬件，留足时间写报告，图文并茂。

---

### 14.9 最后叮嘱

**本文档到此全部结束。**

回头看这张能力金字塔，你现在应该已经覆盖了全部四层：

```
┌─────────────────────────────────┐
│  系统设计+调试能力+临场发挥     │  ← 需要通过真题实战来获得
├─────────────────────────────────┤
│  控制算法(PID)+数字信号处理     │  ← 第十二章 + 14.8 电赛题型分析
├─────────────────────────────────┤
│  外设驱动+模块使用+通信协议     │  ← 第四~十一章 + CAN + Flash
├─────────────────────────────────┤
│  GPIO+定时器+ADC+串口基础       │  ← 第三~五章 + SysTick + DMA
├─────────────────────────────────┤
│  C语言+数电模电基础             │  ← 第二章
└─────────────────────────────────┘
```

本文档覆盖了电赛所需 STM32 标准库的几乎全部内容：
- **22 个代码模板**涵盖从 GPIO 到 CAN 总线的所有常用外设
- **逐行详解**确保你能看懂代码中的每一个字
- **硬件原理**让你知其然更知其所以然
- **电赛实战经验**帮你避开最常见的坑

但要真正达到一等奖水平，**你必须动手做项目**：
1. 学完每个章节立刻写代码验证（看到 LED 闪、电机转、OLED 显示才算学会）
2. 做 2~3 套历年电赛真题，严格按 4 天 3 夜节奏来
3. 建立自己的代码模板库（22 个模板各写一份，存好）
4. 遇到问题先独立思考，再查本文档，最后才问人
5. 保持热爱，享受从无到有创造的过程

**祝你电赛取得好成绩，达到一等奖甚至更高的水平！**

---

# 第二篇：电赛硬件知识全解

STM32 是电赛的"大脑"，但光有大脑远远不够。电赛作品是一个完整的电子系统，你还必须掌握模拟电路、电源设计、传感器信号调理、测量仪器使用等硬件知识。本篇将补全这些内容。

---

## 第十五章 模拟电路实战篇 —— 运放与信号调理

运放（运算放大器）是电赛模拟电路的核心元件。电赛仪器类题目中 90% 的信号调理电路都离不开运放。你不用学透模电课本上所有的运放分析（节点法、网孔法、拉普拉斯变换……），但你必须能用运放搭建几个最常用的电路。

### 15.1 运放的基础概念（5 分钟速成）

#### 15.1.1 运放是什么？

运放是一个高增益的差分放大器，有两个输入端和一个输出端：

```
        ┌─────┐
   V+ ──┤+    │
        │     ├── Vout
   V- ──┤-    │
        └─────┘

   Vout = A × (V+ - V-)

   A = 开环增益（理想运放 A = ∞，实际约 10^5 ~ 10^6）
```

运放**几乎从不开环使用**（开环增益太大了，输入稍有差值输出就饱和到电源轨）。实际使用中，运放总是通过**负反馈**来稳定工作。

#### 15.1.2 运放的两条黄金法则（理想运放）

理解了这两条法则，你就理解了运放的所有应用电路：

**法则 1**：运放的输入阻抗无穷大 → **两个输入端不吸取电流**（I+ = I- = 0）。

**法则 2**：负反馈下，运放会调整输出，使得 **V+ = V-**（两个输入端电压相等，称为"虚短"）。

> **为什么叫"虚短"？** 不是真的短路（两脚之间电阻无穷大），而是电压相等，就像短路了一样。所以叫"虚短"（virtual short）。

#### 15.1.3 电赛最常用的运放型号

| 型号    | 通道数 | 供电范围         | 带宽  | 特点                               | 电赛推荐场景           |
| ------- | ------ | ---------------- | ----- | ---------------------------------- | ---------------------- |
| LM358   | 双通道 | 3V~32V（单电源） | 1MHz  | 便宜（几毛钱）、单电源、输出可到地 | 直流信号放大、比较器   |
| LM324   | 四通道 | 3V~32V（单电源） | 1MHz  | 同上，四通道                       | 多路信号调理           |
| TL072   | 双通道 | ±5V~±15V         | 3MHz  | JFET 输入、低噪声                  | 音频放大、交流信号     |
| NE5532  | 双通道 | ±5V~±15V         | 10MHz | 超低噪声、音频专用                 | 音频前放、仪器前端     |
| OPA2277 | 双通道 | ±2.5V~±18V       | 1MHz  | 精密运放、超低失调电压(10μV)       | 精密测量、仪器         |
| TLV2372 | 双通道 | 2.7V~16V         | 3MHz  | 轨到轨输入输出、单电源             | 3.3V/5V 系统的信号调理 |

> **电赛选型建议**：
> - 普通信号放大、比较 → LM358（最便宜，单电源方便）
> - 需要双电源、低噪声 → TL072
> - 精密测量（电桥放大、微弱信号）→ OPA2277
> - 3.3V 单电源系统 → TLV2372（轨到轨，3.3V 就能工作）

---

### 15.2 电赛四大经典运放电路（必须会手算！）

#### 15.2.1 同相放大器 —— 最常用的信号放大

```
         ┌─────┐
  Vin ───┤+    │
         │     ├──┬── Vout
    ┌────┤-    │  │
    │    └─────┘  │
    │   ┌─────────┘
    ├───┤ R2
    │   └──┬── GND（或参考电压）
    └──────┤ R1
           └── GND
```

**放大倍数**：$V_{out} = \left(1 + \frac{R_2}{R_1}\right) \times V_{in}$

**特点**：输入阻抗极高（等于运放输入阻抗，数百 MΩ），适合信号源内阻高的场景。

**电赛典型应用**：
- 将传感器输出的微弱电压（几十 mV）放大到 ADC 量程（0~3.3V）
- 例如：电流检测电阻 0.01Ω，电流 1A 时电压 = 10mV，需要放大 330 倍到 3.3V：R2=330k, R1=1k → 放大 331 倍

#### 15.2.2 反相放大器

```
         R2
    ┌───┤├───┐
    │        │
    │   ┌────┤- 
  Vin──┤ R1 ├──┤   ┌─────┐
         │   └───┤    │
         │       │     ├── Vout
        GND      ┤+    │
                 └─────┘
                  │
                 GND
```

**放大倍数**：$V_{out} = -\frac{R_2}{R_1} \times V_{in}$（负号表示反相）

**特点**：输入阻抗等于 R1，输出反相。适合需要反相的场合。

**电赛典型应用**：
- 需要把正电压变成负压（或反之）的场合
- 做减法器的一部分

#### 15.2.3 电压跟随器 —— 阻抗变换神器

```
  Vin ──────┤+    │
            │     ├──┬── Vout
       ┌────┤-    │  │
       │    └─────┘  │
       └─────────────┘
```

**放大倍数**：$V_{out} = V_{in}$（增益 = 1，不放大）

**那为什么要用它？** 因为它的**输入阻抗极高**（不从前级吸取电流）而**输出阻抗极低**（能向后级提供大电流）。用在需要隔离前后级的场合：

- ADC 输入前加一个跟随器：信号源内阻大会导致 ADC 采样不准，跟随器隔离后 ADC 看到的是低阻抗驱动
- 分压器后加跟随器：电阻分压的输出阻抗是 R1//R2，加跟随器后变成接近 0Ω

#### 15.2.4 差分放大器 —— 去掉共模干扰

```
         R2
    ┌───┤├───┐
    │        │
    │   ┌────┤- 
  V1──┤ R1 ├──┤   ┌─────┐
         │   └───┤    │
        GND      │     ├── Vout
                 ┤+    │
  V2──┤ R3 ├──┴──┤    │
         │       └─────┘
        R4
         │
        GND
```

当 R1=R3 且 R2=R4 时：

**放大倍数**：$V_{out} = \frac{R_2}{R_1} \times (V_2 - V_1)$

**核心价值**：只放大**差模信号**（V2-V1），抑制**共模信号**（两个输入端相同的干扰）。电赛中最典型的应用是**电桥传感器**的信号放大。

> **共模抑制**：假如 V1 和 V2 同时被 50Hz 电源干扰叠加了 1V 的噪声，差分放大器输出 = (R2/R1) × ((V2+1) - (V1+1)) = (R2/R1) × (V2-V1)，干扰完全被抵消！

**电赛典型应用 — 电阻应变片电桥**：

```
         R(应变片)
    ┌────┤├────┐
    │          ├── V2 ──┐
    │   ┌──────┘        │      ┌──────────┐
Vcc─┤   │               └──────┤ 差分放大  ├── ADC
    │   └──────┐        ┌──────┤ (仪表放大器)│
    │          ├── V1 ──┘      └──────────┘
    └────┤├────┘
         R(固定)
```

应变片受力变形时电阻变化微小（约 0.1%），电桥输出电压只有几 mV，需要放大几百到一千倍才能被 ADC 采样。

---

### 15.3 实战：仪表放大器 —— 电桥传感器的专业方案

上面用单个运放做的差分放大器有一个缺点：输入阻抗不够高（等于 R1+R3）。对于电桥这类信号源阻抗相对较高的场景，更专业的方案是用**三运放仪表放大器**。

**三运放仪表放大器的结构**：

```
        ┌──────────┐
  V2 ───┤+  A1    ├──┬── Rgain ──┬──┤+  A3    │
        │  缓冲器  │  │           │  │  差分   ├── Vout
        └──────────┘  │           │  │  放大器  │
                      └── Rgain ──┘  └──────────┘
  V1 ───┤+  A2    │
        │  缓冲器  │
        └──────────┘
```

第一级两个缓冲器提供极高的输入阻抗，第二级差分放大器提供共模抑制。

**增益公式**：$G = 1 + \frac{2R}{R_{gain}}$（R 是内部固定电阻，Rgain 是外部增益设定电阻）

**电赛中推荐用集成仪表放大器芯片**（比分立三运放精度高、温漂小）：

| 芯片   | 增益范围 | 特点                     | 应用             |
| ------ | -------- | ------------------------ | ---------------- |
| AD620  | 1~1000   | 经典、便宜（约 15 元）   | 电桥、压力传感器 |
| INA128 | 1~10000  | 精度高、噪声低           | 精密测量         |
| AD623  | 1~1000   | 单电源（3V~12V）、轨到轨 | 3.3V 系统        |

**AD620 典型接线**：

```
         ┌──────────┐
  Vin+ ──┤2  AD620 6├── Vout
  Vin- ──┤3        5├── REF（接参考电压，通常是 GND 或 VCC/2）
         │          │
         ├─1  Rg  8─┤  增益 G = 1 + 49.4kΩ / Rg
         └──────────┘
         4: -Vs（负电源，单电源时接 GND）
         7: +Vs（正电源，接 5V 或 3.3V）
```

---

### 15.4 比较器 —— 把模拟信号变成数字信号

运放可以当比较器用（但专用比较器更快）。比较器的功能很简单：比较两个输入电压，输出高或低。

```
  Vin ──┤+    │
        │     ├── Vout（0 或 VCC）
  Vref──┤-    │
        └─────┘

  Vin > Vref → Vout = 高电平（接近 VCC）
  Vin < Vref → Vout = 低电平（接近 GND）
```

**电赛经典应用**：

1. **电压监测/报警**：电池电压低于 3.3V 时触发报警
2. **过零检测**：检测交流信号的过零点（用于可控硅触发）
3. **波形整形**：把正弦波变成方波
4. **施密特触发器**（带迟滞的比较器）：输入端叠加正反馈，防止在阈值附近振荡

**施密特触发器（迟滞比较器）**——电赛必备：

```
         R2
    ┌───┤├───┐
    │        │
    │   ┌────┤+ 
  Vin──┤ R1 ├──┤   ┌─────┐
         │   └───┤    │
        GND      │     ├── Vout
                 ┤-    │
             ┌───┤    │
             │   └─────┘
            Vref
```

- 上门限电压：$V_{TH+} = V_{ref} \times (1 + \frac{R_1}{R_2})$
- 下门限电压：$V_{TH-} = 0$（当 Vout=0 时，经 R2 反馈到 + 端为 0）

**电赛用途**：按键消抖（硬件级别消抖，比软件延迟 20ms 更可靠）、红外接收信号整形、光耦输出整形。

---

### 15.5 有源滤波器 —— 运放做的滤波器

在 ADC 之前加一个抗混叠滤波器（低通滤波）是电赛仪器类题目的标配。

**一阶有源低通滤波器（Sallen-Key 结构）**：

```
         R1      R2
  Vin ──┤├──┬──┤├──┬──┤+    │
            │      │  │     ├── Vout
           C1     C2 ├─┤-    │
            │      │  └─────┘
           GND    GND    │
                         └────────── 反馈到 - 端
```

**截止频率**：$f_c = \frac{1}{2\pi\sqrt{R_1 R_2 C_1 C_2}}$

**电赛常用设计**：
- 截止频率 = 信号最高频率 × 1.5~2 倍
- 例如：音频信号最高 10kHz，设截止频率 15kHz
- 用 FilterPro（TI 免费软件）或在线工具生成元件参数

> **电赛滤波器选型建议**：
> - 抗混叠滤波（ADC 前端）：二阶低通，截止频率 = 采样率/3
> - 电源纹波滤除：一阶 RC 低通（可能不需要运放），截止频率 10~100Hz
> - 50Hz 工频陷波：专用陷波滤波器（Twin-T 结构），电赛环境 50Hz 干扰严重

### 15.6 实战——从传感器到 ADC 的完整设计案例

以下是一个完整的信号调理链设计实例，带具体元件参数。

**需求**：用一个输出 0~100mV 的压力传感器，接入 STM32 ADC（0~3.3V 量程）。

**设计步骤**：

**第 1 步：计算所需放大倍数**
- 输出范围 0~3.3V / 输入范围 0~100mV = 33 倍
- 选 33 倍，留 10% 余量 → 设计 30 倍（100mV×30=3.0V，在 3.3V 以内安全）

**第 2 步：选择运放电路**
- 传感器输出阻抗未知，用同相放大器（输入阻抗高，不吸取传感器电流）
- 增益公式：G = 1 + R2/R1
- 设 R1 = 1kΩ（标准值），R2 = 29kΩ → G = 1 + 29/1 = 30 ✓
- 实际电阻：R1=1kΩ，R2=30kΩ → G=31（可用）

**第 3 步：选择运放型号**
- 单电源 3.3V 供电 → 选 TLV2372（轨到轨输入输出，3.3V 单电源，便宜）
- 或者用 LM358（5V 供电，输出不能到轨，但 3.0V 在能力范围内，5V 供电时输出可到 ~3.5V）

**第 4 步：加低通滤波**
- 压力信号变化缓慢（<10Hz），在 ADC 前端加 RC 低通：R=1kΩ, C=10μF → fc=16Hz

**最终电路**：
```
  传感器输出(0~100mV)
       │
       ├───── RC低通(1kΩ + 10μF) ────┤+  TLV2372
       │                              │     ├──┬── Vout(0~3.0V) → STM32 ADC
       ├── R1(1kΩ) ── GND            ┤-    │
       │                              └─────┘
       └── R2(30kΩ) ──────────┬──────┘
                              │
                           Vout 反馈
```

**验证**：
- 传感器输出 0mV → 运放输出 0V → ADC 读数 0
- 传感器输出 50mV → 运放输出 50mV×31=1.55V → ADC 读数 1.55/3.3×4096≈1923
- 传感器输出 100mV → 运放输出 100mV×31=3.1V → ADC 读数 3.1/3.3×4096≈3847（在量程内 ✓）

> **设计核心要点**：
> 1. **R1 不要太小**：太小（<100Ω）会导致输入阻抗低，传感器驱动不了。
> 2. **R2 不要太大**：太大（>1MΩ）会导致热噪声增大，运放偏置电流产生明显压降。
> 3. **反馈电阻典型范围**：1kΩ~100kΩ，兼顾噪声和驱动能力。
> 4. **运放输出串 100Ω 电阻**：防止长线传输时的容性负载导致运放振荡。
> 5. **ADC 输入前加钳位二极管**：防止超出 0~3.3V 范围损坏 STM32（虽然 TLV2372 是轨到轨，但保险起见）。

### 15.7 运放电路调试技巧

1. **先测静态工作点**：输入接地（或接已知参考电压），量输出是否为理论值（如跟随器输出应 = 输入）。
2. **用万用表测电源电压**：运放的 VCC 和 GND 之间电压是否正常。
3. **检查运放是否自激（振荡）**：用示波器看输出，如果有一条很"粗"的线或高频正弦波，就是振荡了。解决：在反馈电阻 R2 上并联 10~100pF 电容。
4. **输出"卡"在电源轨**：运放可能进入了饱和。检查反馈电阻是否焊对、输入信号是否超出共模范围。
5. **单电源运放的"虚拟地"**：如果信号是交流（如音频），需要把输入偏置在 VCC/2（用两个相同电阻分压），否则负半周会被削掉。

---

## 第十六章 电源设计实战篇

电源是电赛作品的"心脏"。**90% 的奇怪问题追根溯源都是电源问题**。你必须掌握基本的电源设计方法。

### 16.1 电赛常用电源方案速查

| 场景                       | 推荐方案               | 原因                           |
| -------------------------- | ---------------------- | ------------------------------ |
| 电池供电 → 5V              | 升压模块（MT3608）     | 锂电池 3.7V 升到 5V            |
| 电池/5V → 3.3V（STM32）    | **AMS1117-3.3**（LDO） | 简单、纹波小、便宜（几分钱）   |
| 12V → 5V（电流 <1A）       | 7805（LDO）            | 简单但发热大                   |
| 12V → 5V（电流 >1A）       | LM2596（DC-DC 降压）   | 效率高(>85%)、不发热           |
| 需要 ±12V 双电源（运放用） | ICL7660 或 DC-DC 模块  | 从单电源生成负压               |
| 电机/舵机电源（大电流）    | 电池直供或独立开关电源 | 绝对不能和单片机共用一路 LDO！ |

### 16.2 LDO 线性稳压器 —— 最简单的稳压方案

LDO（Low Dropout Regulator，低压差线性稳压器）是电赛最常用的稳压方案。

**工作原理**：像一个自动调节的可变电阻，串联在输入和输出之间。输出电压低于输入电压时，调整管（内部的晶体管）增大导通程度让更多电流通过；输出电压高于设定值时减小导通。始终维持输出 = 设定值。

**核心器件 AMS1117-3.3**：

```
         ┌──────────┐
  5V ────┤3  AMS1117 2├── 3.3V 输出
         │   -3.3    │
        ║ 1  (GND)   │
        GND           │
         └──────────┘

  输入和输出各接电容：
  - 输入：10μF 电解 + 0.1μF 陶瓷
  - 输出：22μF 电解 + 0.1μF 陶瓷
```

**LDO 的优点**：纹波极小（μV 级）、电路简单（只要芯片+两个电容）、无高频噪声。

**LDO 的缺点**：
- **效率低**：效率 ≈ Vout/Vin。5V 降到 3.3V，效率 = 66%，浪费的 34% 变成热量。
- **只能降压**：输入必须比输出高至少"压差"（dropout voltage，AMS1117 约 1.1V）。
- **电流有限**：AMS1117 最大 1A，但大电流时需要散热。

> **电赛 LDO 使用铁则**：
> - 输入和输出电容紧靠芯片引脚焊接，不要拉长线！
> - 数字电路（STM32）和模拟电路（运放、传感器）分开供电，用两个独立的 LDO 或至少用磁珠隔离。
> - STM32 的 VDDA（模拟供电）和 VDD（数字供电）可以用同一个 3.3V，但 VDDA 引脚旁边必须加独立的滤波电容。

### 16.3 DC-DC 开关稳压器 —— 高效率的降压/升压方案

当电流大（>500mA）或压差大（如 12V→5V），LDO 效率太低发热严重，需要 DC-DC。

**原理（以 Buck 降压为例）**：通过快速开关（几十 kHz~几 MHz），把输入电压"斩波"成方波，再通过电感+电容平滑成直流。因为开关管只在完全导通（压降接近 0V）和完全关断（电流为 0）之间切换，所以损耗极小，效率可达 85%~95%。

**电赛最常用 DC-DC 芯片 LM2596**：

```
        ┌──────────────────────┐
  12V ──┤1  LM2596-5.0（固定5V）2├── L1(33μH) ──┬── 5V/3A 输出
        │                      │               │
       ║3  GND                5├──  ON/OFF     C2(220μF)
       GND                     │               │
        │                    4├──  Feedback    GND
        └──────────────────────┘
              │
            D1(肖特基二极管，如 1N5822)
              │
             GND
```

**LM2596 的关键参数**：
- 输入：4.5V~40V
- 输出：固定 3.3V / 5V / 12V 或可调版本
- 最大电流：3A
- 效率：约 85%

> **DC-DC 的缺点**：输出有开关纹波（几十 mV，频率 = 开关频率），对模拟电路可能造成干扰。所以电赛中一般用 DC-DC 做第一级降压（如电池→5V），再用 LDO 做第二级（5V→3.3V 给 STM32）。这就是"DC-DC + LDO"的组合方案——兼顾效率和纹波。

### 16.4 电池供电方案

电赛中有不少题目要求"电池供电工作 X 小时"，电池选择和电源管理是得分关键。

**常用电池对比**：

| 电池类型       | 电压      | 容量（同体积） | 优点                       | 缺点                 | 电赛推荐     |
| -------------- | --------- | -------------- | -------------------------- | -------------------- | ------------ |
| **18650 锂电** | 3.7V 标称 | 2000~3500mAh   | 容量大、可充电、放电能力强 | 需要保护板、充电电路 | **强烈推荐** |
| 聚合物锂电池   | 3.7V 标称 | 500~5000mAh    | 形状灵活、重量轻           | 易鼓包、需保护板     | 推荐         |
| 干电池(AA×4)   | 6V        | ~2000mAh       | 随处可买、安全             | 容量有限、不环保     | 备用         |
| 9V 叠层电池    | 9V        | ~500mAh        | 电压高                     | 容量极小、内阻大     | **不推荐**   |
| 12V 铅酸电池   | 12V       | 几 Ah~几十 Ah  | 大容量、大电流             | 笨重                 | 大功率项目用 |

**18650 锂电池供电方案**：

```
  18650(3.7V) ──→ 保护板 ──→ MT3608升压到5V ──→ AMS1117-3.3 → STM32
                  (过充/过放保护)               │
                                                └──→ 5V 设备（舵机等）
```

**关键注意事项**：
- **锂电池必须有保护板！** 否则过放（<2.5V）永久损坏，过充（>4.25V）起火爆炸！
- 升压模块选 MT3608：输入 2V~24V，输出最高 28V，效率约 90%，价格不到 2 元。
- 电池容量估算：STM32 约 30mA + 外设约 50mA = 80mA。3000mAh ÷ 80mA ≈ 37 小时。足够比赛用了。

### 16.5 电源去耦与滤波 —— 电赛最容易被忽视的关键

**去耦电容（Decoupling Capacitor）** 是每个 IC 电源引脚旁边必须接的小电容（典型 0.1μF 陶瓷电容）。

**为什么要去耦？** 数字 IC 内部的晶体管在时钟边沿瞬间同时导通，从电源吸取一个极窄（几纳秒）的大电流脉冲。电源线有寄生电感，不能瞬间响应 → 芯片电源引脚电压瞬间跌落 → 逻辑错误或复位。去耦电容的作用就是作为一个"本地电荷库"，在芯片需要电流尖峰时，从电容（而非远处的电源）提供电荷。

**电赛布线规则**：
- 每个 IC 的 VCC-GND 引脚对之间放一个 0.1μF 陶瓷电容，**尽可能靠近 IC 引脚**（越近越好，<5mm）。
- 电源入口放一个大容量电解电容（100μF~470μF），滤除低频纹波。
- 模拟电路和数字电路的电源用地线分割+单点连接（或磁珠隔离）。

---

## 第十七章 常用传感器与信号调理

电赛作品中传感器是"感官"，负责把物理量（温度、压力、光强、距离、角度……）转换成电信号。STM32 只能处理电信号，所以传感器本质上都是"物理量→电量"的转换器。

### 17.1 温度传感器

| 类型         | 输出                 | 精度   | 接口                | 电赛推荐场景         |
| ------------ | -------------------- | ------ | ------------------- | -------------------- |
| **DS18B20**  | 数字（1-Wire）       | ±0.5°C | 单总线（一个 GPIO） | **强烈推荐**，最省事 |
| NTC 热敏电阻 | 模拟（电阻变化）     | ±1°C   | ADC+分压电路        | 成本最低但需标定     |
| LM35         | 模拟（电压 10mV/°C） | ±0.5°C | ADC                 | 线性好，需运放       |
| **热电偶**   | 模拟（μV 级电压）    | ±1°C   | 专用芯片（MAX6675） | 高温场景（>150°C）   |

**DS18B20 的温度读取**（单总线协议，时序要求严格但代码量小）：

DS18B20 是最适合电赛的温度传感器——不需要 ADC、不需要运放、不需要标定，一个 GPIO 就能读温度。精度 ±0.5°C，范围 -55°C~+125°C，分辨率可设 9~12 位。

核心通信时序：**DQ 引脚** 是双向数据线，必须接 4.7kΩ 上拉电阻到 3.3V。

### 17.2 霍尔传感器 —— 非接触测速/测位置

霍尔传感器检测磁场。电赛中主要用于：
- **电机测速**：在电机转轴上贴磁铁，用霍尔传感器检测每转一次的脉冲（替代编码器，成本极低）
- **门磁/限位检测**：检测运动部件是否到达某个位置

| 型号   | 类型       | 输出                     | 供电 |
| ------ | ---------- | ------------------------ | ---- |
| A3144  | 开关型霍尔 | 数字（有磁=低，无磁=高） | 5V   |
| AH3503 | 线性霍尔   | 模拟电压（正比于磁场）   | 5V   |

> A3144 接法极其简单：VCC→5V, GND→GND, OUT→STM32 GPIO（上拉输入）。OUT 是开漏输出，需要外部上拉（内部上拉也行）。

### 17.3 电流检测 —— 电机电流/功耗测量

电赛中测量电流是常见需求（如电机堵转保护、电池剩余电量估算）。

**方案一（最简单）：串联采样电阻 + 运放放大**

```
  电流路径：VCC ──┬── 负载 ── GND
                 │
               R_sense(0.1Ω)
                 │
                 ├── 运放差分放大 ──→ ADC
                 │
               GND
```

- 采样电阻 0.1Ω，电流 1A → 压降 = 0.1V
- 用同相放大器放大 33 倍 → 3.3V（满量程）
- **缺点**：采样电阻串联在主回路中，会消耗能量（PR=I²R），且采样电阻不是浮地的

**方案二（推荐）：ACS712 霍尔电流传感器模块**

ACS712 内部用霍尔元件感应电流产生的磁场，实现完全隔离的电流测量。输出是电压（供电=5V 时，0A 输出 = VCC/2 = 2.5V，灵敏度 185mV/A）。STM32 直接用 ADC 读取即可。

### 17.4 光电传感器 —— 巡线/避障/计数

| 类型             | 原理                  | 电赛用途               |
| ---------------- | --------------------- | ---------------------- |
| **红外对管**     | 红外发射+接收，反射式 | 小车巡线（黑白线检测） |
| **红外避障模块** | 同上，一体化模块      | 避障、物体检测         |
| 光敏电阻(LDR)    | 电阻随光照变化        | 环境光检测             |
| 光电编码器       | 光栅+光电接收         | 高精度位置/速度测量    |

**红外巡线传感器（TCRT5000）**：反射式红外对管，黑线吸收红外（输出高），白线反射红外（输出低）。输出是数字信号（带比较器），直接接 STM32 GPIO。巡线小车一般用 3~5 个并排安装。

### 17.5 DS18B20 完整驱动代码

DS18B20 是单总线（1-Wire）设备，所有通信通过一根 DQ 线。时序要求微秒级精确，必须用延时函数精确控制。

```c
// ========== DS18B20 引脚定义 ==========
#define DS18B20_DQ_GPIO    GPIOA
#define DS18B20_DQ_PIN     GPIO_Pin_0

// 方向控制宏（单总线引脚需要动态切换输入/输出方向）
#define DS18B20_DQ_OUT()   { GPIO_InitTypeDef g; \
    g.GPIO_Pin=DS18B20_DQ_PIN; g.GPIO_Mode=GPIO_Mode_Out_PP; \
    g.GPIO_Speed=GPIO_Speed_50MHz; GPIO_Init(DS18B20_DQ_GPIO,&g); }
#define DS18B20_DQ_IN()    { GPIO_InitTypeDef g; \
    g.GPIO_Pin=DS18B20_DQ_PIN; g.GPIO_Mode=GPIO_Mode_IPU; \
    GPIO_Init(DS18B20_DQ_GPIO,&g); }
#define DS18B20_DQ_H()     GPIO_SetBits(DS18B20_DQ_GPIO, DS18B20_DQ_PIN)
#define DS18B20_DQ_L()     GPIO_ResetBits(DS18B20_DQ_GPIO, DS18B20_DQ_PIN)
#define DS18B20_DQ_READ()  GPIO_ReadInputDataBit(DS18B20_DQ_GPIO, DS18B20_DQ_PIN)

/**
 * @brief  DS18B20 初始化（复位脉冲 + 检测存在脉冲）
 * @return 0=检测到DS18B20, 1=未检测到
 */
uint8_t DS18B20_Reset(void)
{
    uint8_t presence;
    
    DS18B20_DQ_OUT();      // 设为输出
    DS18B20_DQ_L();        // 拉低 DQ
    Delay_us(480);          // 至少 480μs 低电平（复位脉冲）
    DS18B20_DQ_H();        // 释放 DQ
    Delay_us(60);           // 等 15~60μs
    
    DS18B20_DQ_IN();       // 设为输入，读存在脉冲
    presence = DS18B20_DQ_READ();  // 0=DS18B20存在, 1=不存在
    Delay_us(420);          // 等整个时隙结束
    
    return presence;
}

/**
 * @brief  向 DS18B20 写一个 bit
 */
void DS18B20_WriteBit(uint8_t bit)
{
    DS18B20_DQ_OUT();
    DS18B20_DQ_L();
    Delay_us(2);            // 拉低 >1μs
    
    if(bit)
        DS18B20_DQ_H();     // 写 1：释放总线
    
    Delay_us(60);           // 维持 60~120μs
    DS18B20_DQ_H();         // 释放总线
    Delay_us(2);
}

/**
 * @brief  从 DS18B20 读一个 bit
 */
uint8_t DS18B20_ReadBit(void)
{
    uint8_t bit;
    
    DS18B20_DQ_OUT();
    DS18B20_DQ_L();
    Delay_us(2);            // 拉低 >1μs
    DS18B20_DQ_H();         // 释放总线
    Delay_us(5);            // 等 DS18B20 把数据放到 DQ 上
    
    DS18B20_DQ_IN();
    bit = DS18B20_DQ_READ(); // 采样
    Delay_us(55);           // 等时隙结束
    
    return bit;
}

/**
 * @brief  向 DS18B20 写一个字节（LSB first，低位在前）
 */
void DS18B20_WriteByte(uint8_t dat)
{
    for(uint8_t i = 0; i < 8; i++)
    {
        DS18B20_WriteBit(dat & 0x01);  // 写最低位
        dat >>= 1;                      // 右移，下一个 bit
    }
}

/**
 * @brief  从 DS18B20 读一个字节
 */
uint8_t DS18B20_ReadByte(void)
{
    uint8_t dat = 0;
    for(uint8_t i = 0; i < 8; i++)
    {
        dat >>= 1;
        if(DS18B20_ReadBit())
            dat |= 0x80;    // 收到 1，放在最高位
    }
    return dat;
}

/**
 * @brief  启动温度转换（需要约 750ms 后读结果）
 */
void DS18B20_StartConvert(void)
{
    DS18B20_Reset();            // 复位
    DS18B20_WriteByte(0xCC);    // SKIP ROM（只有一个传感器时跳过地址匹配）
    DS18B20_WriteByte(0x44);    // CONVERT T（启动温度转换）
}

/**
 * @brief  读取温度值（浮点数，单位°C）
 * @note   调用前确保已启动转换并等待≥750ms
 */
float DS18B20_ReadTemp(void)
{
    uint8_t TL, TH;
    int16_t temp_raw;
    
    DS18B20_Reset();
    DS18B20_WriteByte(0xCC);    // SKIP ROM
    DS18B20_WriteByte(0xBE);    // READ SCRATCHPAD（读暂存器）
    
    TL = DS18B20_ReadByte();    // 温度低字节
    TH = DS18B20_ReadByte();    // 温度高字节
    
    // 组装 16 位温度值（默认 12 位分辨率，0.0625°C/LSB）
    temp_raw = ((int16_t)TH << 8) | TL;
    
    return (float)temp_raw * 0.0625f;  // 转换为 °C
}

// ===== 使用示例 =====
float temperature;
temperature = DS18B20_ReadTemp();  // 直接读温度（约 800ms 完成）
// 或者异步方式：
// DS18B20_StartConvert();
// ... 等 750ms 期间做其他事 ...
// temperature = DS18B20_ReadTemp();
```

> **DS18B20 电赛经验**：
> 1. **必须接 4.7kΩ 上拉电阻**到 3.3V。没有上拉，通信必然失败。
> 2. **时序是关键**：DS18B20 对时序要求微秒级精确。如果代码不工作，先用逻辑分析仪看 DQ 波形。
> 3. **关中断保护**：读写时序期间如果被中断打断，可能导致通信失败。关键时序部分可以临时关全局中断。
> 4. **多个 DS18B20 并联**：单总线支持多个传感器共用一根 DQ 线。每个 DS18B20 有唯一的 64 位 ROM 地址，通过 MATCH ROM 命令寻址。
> 5. **供电方式**：DS18B20 可以用寄生供电（DQ 线供电，只接 2 根线），但电赛推荐用外部供电（3 根线：VCC/GND/DQ），更稳定。

### 17.6 BMP280 气压传感器 —— 测高/天气预报

BMP280 是 I2C/SPI 双接口的气压+温度传感器，可以测量大气压（精度 ±1hPa）和温度。通过气压可以推算海拔高度，电赛中可用于飞行器高度测量、气象站等题目。

| 特性     | 参数                         |
| -------- | ---------------------------- |
| 接口     | I2C（地址 0x76/0x77）或 SPI  |
| 气压精度 | ±1 hPa（相对精度 ±0.12 hPa） |
| 供电     | 1.8V~3.6V（模块通常 3.3V）   |
| 功耗     | 2.7μA（1Hz 采样）            |

**读气压的基本流程**（I2C）：
1. 读芯片 ID 寄存器（0xD0）确认是 0x58（BMP280）
2. 配置测量参数（过采样率、滤波器系数等）
3. 读取校准系数（存储在芯片出厂时写入的寄存器中）
4. 触发测量，等待完成后读取原始数据
5. 用校准系数补偿计算得到实际气压和温度

**气压→海拔换算**：$h = 44330 \times \left(1 - \left(\frac{p}{p_0}\right)^{\frac{1}{5.255}}\right)$，其中 $p_0$ = 海平面气压 1013.25 hPa。

### 17.7 编码器类型详解

编码器在电赛中用于电机测速和位置控制，在第六章和第十二章已经讲解了编码器接口的使用。这里补充不同类型编码器的选型知识。

| 类型           | 原理                | 精度               | 价格  | 电赛推荐场景                 |
| -------------- | ------------------- | ------------------ | ----- | ---------------------------- |
| **霍尔编码器** | 磁环+霍尔传感器     | 几百~几千 PPR      | ~10元 | **电赛最推荐**（自带驱动板） |
| 光电编码器     | 光栅+光电对管       | 几百~几万 PPR      | ~30元 | 高精度需求                   |
| 磁性编码器     | AS5600 等磁感应芯片 | 12位（4096步/圈）  | ~15元 | 绝对角度测量                 |
| 简易编码器     | 槽型光耦+码盘       | 自己定（几十 PPR） | ~2元  | 低成本方案                   |

> **电赛最推荐的电机编码器**：JGA25-370 直流减速电机（自带霍尔编码器），工作电压 12V，减速比可选的，编码器 390 线（AB 双相，4 倍频后 1560 脉冲/圈）。价格约 25 元/个，电赛小车/平衡车标配。

---

## 第十八章 电子测量与仪器使用

电赛现场能用好仪器，效率翻倍。以下是你必须会用的仪器和操作。

### 18.1 示波器 —— 电子工程师的眼睛

**必会操作**：

| 操作              | 方法                                          | 用途                              |
| ----------------- | --------------------------------------------- | --------------------------------- |
| 自动设置(Autoset) | 按 Autoset 键                                 | 快速显示未知波形                  |
| 垂直调节          | 旋转 Volts/Div 旋钮                           | 调节波形高度，使波形占屏幕 3~5 格 |
| 水平调节          | 旋转 Sec/Div 旋钮                             | 调节时间轴，看到 2~5 个完整周期   |
| 触发设置          | 按 Trigger 菜单 → 选择边沿触发 → 调节触发电平 | 使波形稳定显示                    |
| 光标测量(Cursor)  | 按 Cursor 键 → 选择时间/电压 → 旋转旋钮       | 精确测量周期、脉宽、电压          |
| 测量功能(Measure) | 按 Measure 键 → 添加测量项（频率、峰峰值等）  | 自动显示信号的各项参数            |

**电赛中最常用的示波器检查项**：
1. **PWM 波形**：检查频率、占空比是否和代码设定一致
2. **I2C/SPI 通信**：检查 SCL/SDA 或 SCK/MOSI 波形是否正常
3. **串口 TX 波形**：检查波特率是否匹配（看一个 bit 的宽度）
4. **电源纹波**：AC 耦合模式，看电源噪声幅值
5. **运放输出**：检查放大倍数是否正确，有没有振荡

### 18.2 万用表

**必会操作**：电压测量（DC/AC）、电阻测量、通断测试（蜂鸣档）、电流测量（串联在电路中！）。

> **万用表测电流必须把表笔串联在电路中！** 很多新手直接把表笔并在电源两端测电流 → 短路 → 烧保险甚至烧表。

### 18.3 逻辑分析仪 —— 穷人的示波器

逻辑分析仪（如 Saleae Logic 8，几十元）对于调试通信协议（I2C、SPI、UART）极其好用。它能同时抓取多路数字信号，并自动解析协议内容。

---

## 第十九章 PCB 设计快速入门

电赛虽然允许用面包板+杜邦线搭电路，但对于稍微复杂的系统（特别是模拟电路和高速数字电路），面包板的寄生电容和接触不良会带来无穷无尽的麻烦。**建议提前画好 PCB 去打样（嘉立创 5 元/5 片，3~4 天到货）**。

### 19.1 PCB 设计工具选择

| 工具            | 难度 | 费用 | 推荐                 |
| --------------- | ---- | ---- | -------------------- |
| **立创 EDA**    | 简单 | 免费 | **强烈推荐电赛新手** |
| Altium Designer | 中等 | 付费 | 专业级               |
| KiCad           | 中等 | 免费 | 开源替代             |

立创 EDA 是网页版 PCB 设计工具，有海量元件库（直接搜就能找到 STM32F103C8T6 等常用元件），一键下单打样，学习成本极低。电赛用它足够了。

### 19.2 PCB 设计核心原则

1. **电源走线要宽**：电源线至少 1mm 宽（1A 电流），地线尽量铺铜。
2. **去耦电容紧靠 IC**：每个 IC 的 VCC 旁边放 0.1μF 电容，距离 <5mm。
3. **模拟和数字地分开**：模拟地和数字地在电源入口处单点连接（或用 0Ω 电阻/磁珠连接）。
4. **晶振靠近 MCU**：8MHz 晶振和负载电容尽量靠近 STM32 的 OSC_IN/OSC_OUT 引脚。
5. **接插件留测试点**：关键信号（串口 TX/RX、I2C SDA/SCL、SWD）用排针引出，方便调试。
6. **丝印标注清晰**：每个排针标清楚功能（VCC、GND、TX、RX 等），免得自己都搞不清。

### 19.3 电赛常用模块的 PCB 封装速查

| 模块        | 封装/连接方式                |
| ----------- | ---------------------------- |
| OLED 0.96寸 | 4P 排针（VCC/GND/SCL/SDA）   |
| HC-SR04     | 4P 排针（VCC/Trig/Echo/GND） |
| TB6612      | 可以焊在 PCB 上或飞线        |
| MPU6050     | 8P 排针或直接焊模块          |
| NRF24L01    | 8P 排针                      |

---

## 第二十章 电赛报告撰写指南

设计报告在电赛评审中通常占 **30%~40%** 的分数。很多团队硬件和程序做得很好，但报告写得马虎，最终与一等奖失之交臂。

### 20.1 报告结构（标准模板）

```
一、方案论证与选择
  1.1 题目分析
  1.2 方案比较（至少两个方案，说明选择理由）
  1.3 系统框图

二、理论分析与计算
  2.1 关键参数计算（滤波截止频率、放大倍数、PID 参数计算等）
  2.2 关键公式推导

三、硬件电路设计
  3.1 电源电路
  3.2 传感器信号调理电路
  3.3 驱动电路
  3.4 MCU 最小系统

四、软件设计
  4.1 程序流程图
  4.2 关键算法说明（PID、滤波、状态机）
  4.3 主要代码段（贴关键代码，不是全部代码）

五、测试方案与测试结果
  5.1 测试仪器
  5.2 测试方法与步骤
  5.3 测试数据（表格形式，多项测试）
  5.4 误差分析

六、总结与展望
  6.1 完成情况
  6.2 创新点
  6.3 不足与改进方向

附录：完整电路图、PCB 版图、源代码清单
```

### 20.2 报告写作要点

1. **图文并茂**：每个关键电路都放电路图，每个算法都放流程图。一张好的图胜过一千字。
2. **数据说话**：测试结果必须用数据表格呈现，不能只写"效果良好"。
3. **诚实面对不足**：不要隐瞒系统的缺陷。评委老师都是专家，一眼就能看出来。诚实地分析不足并给出改进方向，反而加分。
4. **方案论证要有比较**：不能只写"我们选了这个方案"，要写"方案 A 的优点……缺点……，方案 B 的优点……缺点……，综合考量我们选择了方案 X"。
5. **公式要规范**：用公式编辑器（Mathtype 或 Word 自带），变量用斜体，单位用正体。
6. **提前准备模板**：报告模板提前做好（在比赛前），比赛时只需要填充内容。

### 20.3 电赛报告常见扣分点

- 电路图没有标元件参数（电阻值、电容值、芯片型号）
- 测试数据太少（只有一组理想情况，缺少边界条件和极端情况）
- 流程图不规范（手绘、箭头乱飞、逻辑不清晰）
- 没有误差分析
- 报告格式混乱（字体不统一、排版拥挤、图表没有编号）

---

## 第二十一章 步进电机与舵机深度实战

步进电机和舵机是电赛控制类题目中除直流电机外最常用的两种执行器。步进电机适合需要精确定位的场景（如3D打印机结构、云台、平移台），舵机适合角度控制的场景（如机械臂关节、转向机构）。

### 21.1 步进电机 —— 精确位置控制的利器

#### 21.1.1 步进电机的工作原理

步进电机不是连续旋转的电机，而是**每接收一个脉冲就转动一个固定的角度**（步距角）。这是一种"开环"就能精确定位的电机——发 200 个脉冲就转一圈，不需要编码器反馈（当然加编码器可以防丢步）。

**步进电机的核心参数**：

| 参数     | 含义                               | 常见值             |
| -------- | ---------------------------------- | ------------------ |
| 步距角   | 一个脉冲转动的角度                 | 1.8°（200脉冲/圈） |
| 相数     | 内部绕组数量                       | 2相（最常用）、4相 |
| 额定电流 | 每相正常工作电流                   | 0.5A~2A            |
| 保持转矩 | 通电静止时能承受的最大扭矩         | 0.2N·m~3N·m        |
| 供电电压 | 驱动器的供电电压（不是绕组电压！） | 12V、24V           |

> **关键认知**：步进电机的供电电压远高于绕组的额定电压！比如额定 3V 的电机，驱动器用 12V 供电。这是因为驱动器是**恒流斩波**的——用高电压快速建立电流，然后 PWM 斩波维持恒定电流。电压越高，高速性能越好。

#### 21.1.2 步进电机驱动器 —— A4988 / DRV8825

STM32 的 GPIO 引脚只能提供 25mA 电流，远远不够驱动步进电机（通常需要 0.5A~2A）。**必须使用专用的步进电机驱动器芯片**。

**电赛最常用的两款驱动器**：

| 驱动器  | 最大电流 | 细分       | 特点                    | 价格  |
| ------- | -------- | ---------- | ----------------------- | ----- |
| A4988   | 2A       | 最高 1/16  | 经典、稳定、资料多      | ~5元  |
| DRV8825 | 2.5A     | 最高 1/32  | 比A4988更静音、细分更高 | ~8元  |
| TMC2208 | 1.4A     | 最高 1/256 | 超静音、用于3D打印机    | ~15元 |

> **电赛推荐 A4988**：最便宜、最稳定、驱动能力够用。在电赛常用的小型步进电机（42步进，0.5A~1.5A）上表现完美。

#### 21.1.3 A4988 引脚详解与接线

```
        ┌────────────────────┐
        │      A4988         │
  ENABLE├─1                16├─ VMOT（电机电源 8V~35V）
    MS1 ├─2                15├─ GND（电机电源地）
    MS2 ├─3                14├─ 2B（绕组B-第2端）
    MS3 ├─4                13├─ 2A（绕组B-第1端）
   RESET├─5                12├─ 1A（绕组A-第1端）
   SLEEP├─6                11├─ 1B（绕组A-第2端）
    STEP├─7                10├─ VDD（逻辑电源 3.3V/5V）
    DIR ├─8                 9├─ GND（逻辑地）
        └────────────────────┘
```

**关键引脚说明**：

| 引脚     | 功能                                   | STM32 接法                |
| -------- | -------------------------------------- | ------------------------- |
| **STEP** | **脉冲输入**：每个上升沿步进电机走一步 | 接 STM32 GPIO（推挽输出） |
| **DIR**  | 方向控制：HIGH=顺时针, LOW=逆时针      | 接 STM32 GPIO             |
| ENABLE   | 使能：LOW=使能, HIGH=关闭（电机松脱）  | 接 GND（始终使能）或 GPIO |
| MS1~MS3  | 细分设置（见下文）                     | 接 GND 或 VDD             |
| SLEEP    | 睡眠：LOW=睡眠, HIGH=正常工作          | 接 VDD（或接 RESET 一起） |
| RESET    | 复位：LOW=复位                         | 接 VDD（或接 SLEEP 一起） |
| VMOT     | 电机电源                               | 接 12V/24V 电源           |
| VDD      | 逻辑电源                               | 接 3.3V 或 5V             |

> **最简单接法**：SLEEP 和 RESET 接在一起接 VDD。ENABLE 接 GND（始终使能）。MS1~MS3 全部接 GND（全步模式，最简单）。这样只需要控制 **STEP** 和 **DIR** 两根线！

**VDD 逻辑电平说明**：
- A4988 的 VDD 可以接 3.3V 或 5V
- 当 VDD=3.3V 时，STEP 和 DIR 用 3.3V 信号即可（直接接 STM32 GPIO，不需要电平转换！）
- **确认方法**：读 A4988 数据手册，逻辑高电平最小值是 0.7×VDD。如果 VDD=3.3V，则 2.31V 以上即为高，STM32 的 3.3V GPIO 完全满足。

#### 21.1.4 细分的概念与设置

**细分**是步进电机驱动器的核心功能，它把一个完整的步距角再细分成更小的微步。

| MS3 | MS2 | MS1 | 细分       | 脉冲/圈（1.8°电机） | 特点               |
| --- | --- | --- | ---------- | ------------------- | ------------------ |
| 0   | 0   | 0   | **全步**   | 200                 | 扭矩最大、震动最大 |
| 0   | 0   | 1   | 1/2步      | 400                 |                    |
| 0   | 1   | 0   | 1/4步      | 800                 |                    |
| 0   | 1   | 1   | 1/8步      | 1600                |                    |
| 1   | 0   | 0   | **1/16步** | 3200                | **电赛最常用**     |

**电赛建议用 1/16 细分**：3200 脉冲/圈，精度够高、运行平滑、扭矩损失可接受。细分越高扭矩越小（在 1/16 细分时扭矩约是全步的 10%，但实际应用中可以接受）。

> **全步 vs 细分的扭矩误区**：很多人以为细分会增加扭矩，恰恰相反——细分越高，每步的电流矢量合成扭矩越小。1/16 细分时静转矩只有全步的约 10%。这就是为什么高速时要用较低的细分。

#### 21.1.5 步进电机速度控制

步进电机的速度由 **STEP 脉冲的频率** 决定。

$$ 转速(RPM) = \frac{脉冲频率(Hz) \times 60}{脉冲/圈} $$

例如：1/16 细分（3200 脉冲/圈），想达到 60 RPM：
- 脉冲频率 = 60 RPM × 3200 / 60 = 3200 Hz
- 脉冲周期 = 1/3200 = 312.5 μs

**实现方法——用定时器产生精确的 STEP 脉冲**：

```c
/**
 * @brief  步进电机 STEP 脉冲产生（使用 TIM4_CH1 = PB6）
 * @note   使用定时器输出比较翻转模式产生 50% 占空比方波
 *         每个完整的方波周期包含上升沿和下降沿各一次 = 2 步
 *         所以：脉冲频率 = TIM4 更新频率 / 2
 *         
 *         频率计算：
 *         TIM4 挂 APB1（72MHz），PSC=0（不分频），ARR 控制频率
 *         f_step = 72MHz / (ARR + 1) / 2
 *         例：ARR=3599 → f_step = 72M/3600/2 = 10kHz（每秒10000步）
 *             在1/16细分下：10000/3200×60 = 187.5 RPM
 */

// ========== A4988 引脚定义 ==========
#define A4988_STEP_GPIO   GPIOB
#define A4988_STEP_PIN    GPIO_Pin_6   // TIM4_CH1（用定时器产生脉冲）
#define A4988_DIR_GPIO    GPIOB
#define A4988_DIR_PIN     GPIO_Pin_7   // 普通 GPIO

// 方向控制宏
#define STEP_DIR_CW()   GPIO_SetBits(A4988_DIR_GPIO, A4988_DIR_PIN)    // 顺时针
#define STEP_DIR_CCW()  GPIO_ResetBits(A4988_DIR_GPIO, A4988_DIR_PIN)  // 逆时针

void Stepper_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;
    
    // ----- 开时钟 -----
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
    // TIM4 在 APB1！
    
    // ----- DIR 引脚：普通推挽输出 -----
    GPIO_InitStructure.GPIO_Pin = A4988_DIR_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(A4988_DIR_GPIO, &GPIO_InitStructure);
    STEP_DIR_CW();  // 默认顺时针
    
    // ----- STEP 引脚：复用推挽输出（TIM4_CH1）-----
    GPIO_InitStructure.GPIO_Pin = A4988_STEP_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  // 复用推挽！
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(A4988_STEP_GPIO, &GPIO_InitStructure);
    
    // ----- 配置 TIM4 输出比较翻转模式 -----
    TIM_TimeBaseStructure.TIM_Period = 3599;    // ARR（决定频率）
    TIM_TimeBaseStructure.TIM_Prescaler = 0;    // PSC=0，不分频
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
    
    // 输出比较配置：翻转模式（每次匹配时翻转引脚电平）
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Toggle;  // 翻转模式！
    // TIM_OCMode_Toggle：每当 CNT == CCR 时，翻转对应的输出引脚电平
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 1799;  // 50% 占空比（ARR的一半）
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(TIM4, &TIM_OCInitStructure);
    
    TIM_Cmd(TIM4, ENABLE);  // 开始产生脉冲
}

/**
 * @brief  设置步进电机转速
 * @param  rpm: 目标转速（转/分钟），正=顺时针，负=逆时针
 * @param  steps_per_rev: 每圈脉冲数（取决于细分设置）
 * @note   转速为 0 时停止脉冲输出（电机锁住）
 *         TIM4 在 APB1 = 72MHz，PSC=0
 *         f_step = 72MHz / ((ARR+1) × 2)
 *         rpm = f_step × 60 / steps_per_rev
 *         
 *         所以：ARR = (72MHz × 60) / (2 × steps_per_rev × rpm) - 1
 *              = 2,160,000,000 / (steps_per_rev × rpm) - 1
 */
void Stepper_SetSpeed(int16_t rpm, uint16_t steps_per_rev)
{
    if(rpm == 0)
    {
        TIM_Cmd(TIM4, DISABLE);  // 停止脉冲
        return;
    }
    
    // 设置方向
    if(rpm > 0)
        STEP_DIR_CW();
    else
    {
        STEP_DIR_CCW();
        rpm = -rpm;  // 取绝对值用于计算
    }
    
    // 计算 ARR
    // f_step = rpm × steps_per_rev / 60
    // ARR = 72MHz / (2 × f_step) - 1
    uint32_t f_step = (uint32_t)rpm * steps_per_rev / 60;
    if(f_step == 0) f_step = 1;  // 防止除零
    uint16_t arr = 72000000UL / (2 * f_step) - 1;
    
    if(arr > 65535) arr = 65535;      // ARR 是 16 位，最大 65535
    if(arr < 10)    arr = 10;          // 最小限制，防止频率太高电机丢步
    
    TIM_SetAutoreload(TIM4, arr);
    TIM_SetCompare1(TIM4, arr / 2);   // 保持 50% 占空比
    
    TIM_Cmd(TIM4, ENABLE);
}

/**
 * @brief  步进电机移动指定步数（阻塞式）
 * @param  steps: 要移动的步数（正=顺时针，负=逆时针）
 * @note   使用延时方式产生脉冲，适合简单应用
 *         更精确的做法是用定时器中断计数
 */
void Stepper_MoveSteps(int32_t steps, uint16_t step_delay_us)
{
    // 设置方向
    if(steps > 0)
        STEP_DIR_CW();
    else
    {
        STEP_DIR_CCW();
        steps = -steps;
    }
    
    // 产生脉冲
    for(int32_t i = 0; i < steps; i++)
    {
        GPIO_SetBits(A4988_STEP_GPIO, A4988_STEP_PIN);   // STEP=HIGH
        Delay_us(step_delay_us / 2);                      // 延时一半
        GPIO_ResetBits(A4988_STEP_GPIO, A4988_STEP_PIN);  // STEP=LOW
        Delay_us(step_delay_us / 2);                      // 延时一半
        // STEP 上升沿触发步进电机走一步
    }
}
```

> **步进电机电赛经验**：
> 1. **必须用驱动器**（A4988/DRV8825），不能直接用 STM32 引脚驱动！步进电机电流远超 GPIO 的 25mA 能力。
> 2. **电机电源独立供电**：12V/24V 电机电源绝对不能和 STM32 的 3.3V 共用。电源共地即可。
> 3. **启动要加减速**：步进电机不能从静止直接跳到高速，会丢步甚至堵转。必须做梯形或 S 形加减速。
> 4. **A4988 要调电流限制**：模块上的电位器用来设置最大电流。调到电机额定电流即可，太小没力，太大发热严重。调节方法：测 Vref 引脚电压，I_max = Vref × 2.5（对 A4988）。
> 5. **MS1~MS3 用拨码开关**：方便在比赛中切换细分模式。
> 6. **丢步检测**：如果应用对位置精度要求极高，加一个编码器做闭环反馈。否则，确保加减速足够平滑以防止丢步。
> 7. **A4988 发热**：在大电流（>1A）时 A4988 会很烫。加散热片或在 PCB 上铺铜散热。

### 21.2 舵机深度实战

舵机（Servo）在第六章已经有了基础代码，这里补充进阶内容。

#### 21.2.1 舵机的内部原理

舵机内部是一个闭环控制系统：
- **直流电机** 通过减速齿轮组驱动输出轴
- **电位器** 检测输出轴的实际角度
- **控制电路** 比较目标角度和实际角度，驱动电机直到误差为 0

这就是舵机能够"指哪打哪"的原因——它是一个内置了位置闭环的小系统。

#### 21.2.2 多路舵机控制

电赛中经常需要同时控制多个舵机（如机械臂有 2~4 个关节，云台有 2 个轴）。

**方案一（推荐）：用一个定时器的多个通道**

TIM2 有 4 个通道（PA0/PA1/PA2/PA3），可以同时输出 4 路舵机 PWM。

**方案二（节省资源）：用一个定时器 + 软件轮询**

使用一个定时器中断 + 若干 GPIO 用软件模拟多路舵机信号。原理：50Hz（20ms 周期），在中断中依次拉高各路 GPIO，延时对应的高电平时间后拉低。

#### 21.2.3 舵机电源经验

- **单个 SG90 舵机**：堵转电流约 750mA，必须独立供电（5V 2A 电源模块）
- **多个舵机**：5V 3A~5A 开关电源，每个舵机 VCC 旁边接 470μF 电解电容
- **绝对不能用 STM32 的 3.3V 给舵机供电！**
- **舵机和 STM32 必须共地**（GND 连在一起），否则 PWM 信号没有参考地
- **长信号线**：如果舵机线超过 30cm，在信号线上串联 100Ω 电阻防止振铃

---

## 第二十二章 无线通信模块实战

电赛中很多题目涉及遥控、数据传输、双机通信等需求。本章覆盖最常用的无线通信方案。

### 22.1 蓝牙模块 HC-05 / HC-06 —— 最简单的无线串口

#### 22.1.1 蓝牙模块是什么？

HC-05（主从一体）和 HC-06（仅从机）是最经典的蓝牙转串口模块。它们本质上是一个**无线串口透传模块**——你往模块的 RX 发数据，它通过蓝牙发出去；它通过蓝牙收到的数据，从 TX 发出来。对 STM32 来说，用蓝牙模块和用 USB 转 TTL 模块一样，都是串口通信。

| 特性     | HC-05                    | HC-06          |
| -------- | ------------------------ | -------------- |
| 模式     | 主从一体（可做主或做从） | 仅从机         |
| 电压     | 3.3V（模块）5V（底板）   | 同左           |
| 波特率   | 默认 9600，可设 1382400  | 同左           |
| 配对密码 | 默认 1234                | 默认 1234      |
| 电赛推荐 | **推荐**（灵活性高）     | 从机够用时也可 |

#### 22.1.2 HC-05 接线与 AT 指令配置

**接线（4 线即可）**：

```
STM32                    HC-05
PA9  (USART1_TX) ────→  RXD
PA10 (USART1_RX) ←────  TXD
GND  ────────────────  GND
                        VCC ← 5V（模块底板）
                        （注意：模块逻辑是 3.3V，底板有 LDO，
                         直接接 STM32 的 3.3V IO 是安全的）
```

**HC-05 的两种模式**：

| 模式         | 进入方式                             | 用途             |
| ------------ | ------------------------------------ | ---------------- |
| **数据模式** | 上电时 HC-05 的 LED 快闪             | 正常透传通信     |
| **AT 模式**  | 按住模块上的小按键再上电（LED 慢闪） | 发送 AT 指令配置 |

> **关键认知**：蓝牙模块是**串口透传**——你不需要懂蓝牙协议。把 HC-05 当作一根"看不见的串口线"。在 STM32 这边 USART_SendData，在手机/电脑那边就能收到；反之亦然。

**常用 AT 指令**（在 AT 模式下发送，波特率 38400）：

```
AT              → 返回 OK（测试通信）
AT+NAME=MyCar   → 设置蓝牙名称为 MyCar
AT+PSWD=1234    → 设置配对密码
AT+UART=115200,0,0 → 设置波特率 115200, 1停止位, 无校验
AT+ROLE=0       → 0=从机, 1=主机
```

**STM32 端的代码就是把 HC-05 当成普通串口**：

```c
// HC-05 蓝牙初始化（其实就是初始化 USART1）
void Bluetooth_Init(uint32_t baudrate)
{
    USART1_Init(baudrate);  // 前面第八章的标准串口初始化
    // 就这么简单！蓝牙 = 无线串口
}

// 发送数据到手机
void Bluetooth_Send(uint8_t* data, uint16_t len)
{
    for(uint16_t i = 0; i < len; i++)
        USART1_SendByte(data[i]);
    // 或者用 DMA 发送
}

// 接收来自手机的数据（在 USART1_IRQHandler 中处理）
// 和普通串口接收完全一样
```

> **电赛蓝牙经验**：
> 1. **手机上装个"蓝牙串口助手"APP**：比赛时可以手机直连 HC-05 收发数据，调试遥控器极其方便。
> 2. **两个 HC-05 可以配对**：一个设为主机（AT+ROLE=1），一个设为从机（AT+ROLE=0），配对后实现 STM32 之间的无线通信（需要事先用 AT 指令绑定对方地址）。
> 3. **Android 手机蓝牙通信**：手机端用 BluetoothSocket 连接 HC-05 的 SPP（串口端口协议），UUID 是固定的 00001101-0000-1000-8000-00805F9B34FB。
> 4. **距离**：HC-05 在空旷处约 10 米，穿墙约 3~5 米。够用，但不如 NRF24L01 远。

### 22.2 WiFi 模块 ESP8266 —— 物联网与远程控制

#### 22.2.1 ESP8266 能做什么？

ESP8266 是一个 WiFi 转串口模块（和 HC-05 一样也是透传），但速度更快（最高 4Mbps）、距离更远（和 WiFi 路由器配合可覆盖整个实验室）。

| 功能       | 电赛应用                                       |
| ---------- | ---------------------------------------------- |
| TCP 服务器 | 电脑用 WiFi 连 ESP8266，收发数据（遥控/监控）  |
| TCP 客户端 | ESP8266 连实验室 WiFi，上传数据到电脑          |
| UDP 通信   | 一对多广播（一个遥控器控制多辆车）             |
| HTTP 请求  | 上传数据到服务器、获取网络时间                 |
| Web 服务器 | 手机浏览器访问 ESP8266 的 IP，显示网页控制界面 |

#### 22.2.2 ESP8266 接线与 AT 指令

**接线**（和 HC-05 基本一样，串口通信）：

```
STM32                    ESP8266-01/01S
PA2 (USART2_TX) ────→   RXD
PA3 (USART2_RX) ←────   TXD
                         VCC ← 3.3V（注意！ESP8266 电流大，峰值 300mA）
                         CH_PD（EN）← 3.3V（必须拉高）
                         GND ── GND
```

> **ESP8266 电源是个大坑**：峰值电流可达 300mA（发射 WiFi 信号时）。AMS1117-3.3 可以提供 800mA，但很多面包板上的 3.3V 模块只能提供 100mA！**务必用独立的 3.3V LDO 给 ESP8266 供电**，不要和 STM32 共用一个 LDO。

**ESP8266 常用 AT 指令**（通过串口发送，默认波特率 115200）：

```
AT                              → OK（测试）
AT+CWMODE=1                     → 设为 Station 模式（连路由器）
AT+CWMODE=2                     → 设为 AP 模式（自己做热点）
AT+CWJAP="WiFi名","密码"        → 连接 WiFi
AT+CIPSTART="TCP","192.168.1.100",8080 → 连 TCP 服务器
AT+CIPSEND=5                    → 发送 5 字节数据（然后发送数据）
AT+CIPMUX=1                     → 使能多连接
```

**STM32 驱动 ESP8266 的简化封装**：

```c
// ESP8266 初始化并连接 WiFi
uint8_t ESP8266_ConnectWiFi(char* ssid, char* pwd)
{
    // 1. 测试通信
    USART2_SendString("AT\r\n");
    if(ESP8266_WaitResponse("OK", 2000) != 0) return 1;  // 无响应
    
    // 2. 设为 Station 模式
    USART2_SendString("AT+CWMODE=1\r\n");
    ESP8266_WaitResponse("OK", 2000);
    
    // 3. 连接 WiFi
    char cmd[128];
    sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, pwd);
    USART2_SendString(cmd);
    if(ESP8266_WaitResponse("OK", 10000) != 0) return 2;  // 连不上
    
    return 0;  // 成功
}

// 等待特定响应字符串
uint8_t ESP8266_WaitResponse(char* expected, uint32_t timeout_ms)
{
    uint32_t t_start = sys_time;
    // 检查接收缓冲区中是否有 expected 字符串
    while(sys_time - t_start < timeout_ms)
    {
        if(strstr((char*)usart2_rx_buf, expected) != NULL)
            return 0;  // 找到了
    }
    return 1;  // 超时
}
```

> **电赛 WiFi 经验**：
> 1. **ESP8266 比蓝牙更灵活**：可以连实验室 WiFi 路由器实现全场通信，手机/电脑都能接入。
> 2. **电源独立供电**：ESP8266 瞬时电流大，必须独立 LDO 或有足够大滤波电容（470μF）。
> 3. **推荐用 USART2 接 ESP8266**：USART1 留给 printf 调试。
> 4. **AT 指令耗时**：连接 WiFi 可能需要 3~10 秒，不要在实时控制循环中调用。上电时一次性连接好，后续只做数据透传。
> 5. **UDP 比 TCP 更适合控制**：UDP 没有重传延迟，适合遥控。但 UDP 可能丢包，需要在应用层做处理。

### 22.3 无线方案选型指南

| 场景                      | 推荐方案     | 原因                         |
| ------------------------- | ------------ | ---------------------------- |
| 手机控制（近距离 <10m）   | HC-05 蓝牙   | 手机自带蓝牙，无需额外硬件   |
| 双 STM32 通信（近距离）   | NRF24L01     | 高速(2Mbps)、低延迟、稳定    |
| 电脑/手机控制（全场范围） | ESP8266 WiFi | 通过路由器全场覆盖           |
| 超远距离（>500m）         | LoRa 模块    | 数公里通信距离（电赛较少用） |
| 红外遥控                  | 1838 接收头  | 遥控器直接控制（成本极低）   |

---

## 第二十三章 SD卡与文件系统 —— 数据记录与存储

电赛中很多题目需要**记录数据**（如温度记录仪、波形记录仪），或者需要**存储大文件**（如字库、图片、音频）。SD 卡是最合适的存储方案。

### 23.1 SD 卡基础

**SD 卡接口模式**：SD 卡支持两种接口——SDIO（4 线高速）和 SPI（2 线，速度较慢但接线简单）。STM32F103C8T6 没有 SDIO 外设，只能用 **SPI 模式**。

**接线（SPI 模式）**：

```
SD 卡槽        STM32
CS   ────────  PA4 (SPI1_NSS，或任意 GPIO)
MOSI ────────  PA7 (SPI1_MOSI)
MISO ────────  PA6 (SPI1_MISO)
SCK  ────────  PA5 (SPI1_SCK)
VCC  ────────  3.3V
GND  ────────  GND
```

> **SD 卡是 3.3V 设备**，可以直接接 STM32。但需要注意：5V Arduino 的 SD 卡模块上通常有电平转换芯片，STM32 用时要跳过电平转换或直接焊 3.3V 版本的模块。

### 23.2 FatFs 文件系统移植

FatFs 是一个轻量级的 FAT 文件系统，专门为嵌入式设备设计。移植后在 STM32 上可以像在电脑上一样 `f_open()`, `f_read()`, `f_write()` 操作文件。

**移植步骤概要**：

1. 下载 FatFs 源码（`ff.c`, `ff.h`, `diskio.c`, `diskio.h`）
2. 在 `diskio.c` 中实现 6 个底层函数：
   - `disk_initialize()` — 初始化 SD 卡
   - `disk_status()` — 获取状态
   - `disk_read()` — 读扇区
   - `disk_write()` — 写扇区
   - `disk_ioctl()` — 控制函数
   - `get_fattime()` — 获取当前时间（可选）
3. 在 `ffconf.h` 中配置功能（如使能长文件名、格式化等）
4. 调用 FatFs API 进行文件操作

**初始化示例**：

```c
FATFS fs;           // 文件系统对象
FIL fil;            // 文件对象
FRESULT res;        // 操作结果

// 挂载文件系统
res = f_mount(&fs, "", 1);  // "" 表示默认驱动器
if(res != FR_OK) {
    OLED_ShowString(0, 0, "SD Mount Err", 12);
    return;
}

// 打开文件（不存在则创建）
res = f_open(&fil, "data.txt", FA_CREATE_ALWAYS | FA_WRITE);
if(res == FR_OK) {
    // 写入数据
    char buf[64];
    sprintf(buf, "Temperature: %.2f\r\n", temp);
    f_write(&fil, buf, strlen(buf), &bw);
    
    f_close(&fil);
}

// 卸载文件系统
f_mount(NULL, "", 1);
```

> **SD 卡电赛经验**：
> 1. **用 SPI 模式，简单可靠**：虽然速度只有几 Mbps，但记录传感器数据完全够用。
> 2. **FatFs 已经有现成的 STM32 移植代码**：正点原子、野火的例程中都有，拿来用即可。
> 3. **SD 卡初始化要延时**：上电后至少等 100ms 再初始化，等 SD 卡内部稳定。
> 4. **文件大小限制**：FAT32 单个文件最大 4GB，电赛绝不会超过。
> 5. **定期关闭再打开文件**：如果长时间记录，每写 1000 条数据就 `f_close` 再 `f_open`，防止断电丢数据（FAT 文件系统只有 close 时才更新目录项）。
> 6. **卡容量**：2GB~32GB 的 MicroSD 都可以，格式化为 FAT32。太大容量的卡（>32GB）可能是 exFAT，FatFs 可能不兼容。

---

## 第二十四章 补充模块驱动速查

本节以最精简的方式补充电赛常用的其他模块和驱动技巧，每个给出核心代码和关键注意事项。

### 24.1 WS2812 全彩 LED —— 炫酷灯光效果

WS2812（又名 NeoPixel）是一个内置控制芯片的 RGB LED。每个 LED 只需要一根数据线（单总线），可以级联几百个，每个 LED 可以独立控制 256×256×256 = 16M 种颜色。

**通信协议**：单线归零码（NZR），800kHz。**不同的脉宽代表 0 和 1**：
- 0 码：0.4μs 高 + 0.85μs 低
- 1 码：0.8μs 高 + 0.45μs 低
- RESET：>50μs 的低电平

**驱动方式（推荐）**：用 **SPI 的 MOSI 引脚** 模拟 WS2812 时序。SPI 的 8 位数据用不同字节模拟 WS2812 的一个 bit——比如 SPI 速率设为 6.4MHz，发 0xE0（11100000）模拟 1 码，发 0x80（10000000）模拟 0 码。具体细节和完整代码可搜索"WS2812 SPI STM32"。

> **电赛应用**：灯带装饰、状态可视化（红色=报警，绿色=正常，蓝色=待机）。电赛外观分也能加分。

### 24.2 红外遥控 —— 成本最低的无线控制

用 1838 红外接收头 + 任意红外遥控器，成本不到 2 元就能实现遥控。

**NEC 协议**（最常用）：引导码（9ms 低 + 4.5ms 高）+ 地址码（8位）+ 地址反码 + 命令码 + 命令反码。0 和 1 用不同的脉宽表示。

**解码方法**：
1. 1838 OUT 引脚接 STM32 外部中断引脚（下降沿触发）
2. 在中断中用定时器测量两次下降沿之间的时间差
3. 根据时间差判断是引导码、0 码还是 1 码
4. 组装成完整的 32 位数据帧

> **核心代码思路**（约 100 行即可实现完整 NEC 解码）。电赛推荐：直接买个 IR 遥控器 + 1838 模块，比蓝牙还简单可靠。

### 24.3 矩阵键盘扫描 —— 用 8 个 GPIO 检测 16 个按键

4×4 矩阵键盘只需要 8 个 GPIO（4 行 + 4 列），能检测 16 个按键。

**原理**：
- 4 行设为推挽输出，初始全输出高
- 4 列设为上拉输入（或下拉输入+外部上拉）
- 扫描过程：依次将每一行拉低，读取 4 列的电平。如果某列是低，说明该行该列交叉处的按键被按下

```c
#define KEY_PORT  GPIOA
// 行：PA0~PA3（输出），列：PA4~PA7（输入）

uint8_t KeyMatrix_Scan(void)
{
    uint8_t row, col;
    const uint8_t row_pins[4] = {GPIO_Pin_0, GPIO_Pin_1, GPIO_Pin_2, GPIO_Pin_3};
    const uint8_t col_pins[4] = {GPIO_Pin_4, GPIO_Pin_5, GPIO_Pin_6, GPIO_Pin_7};
    const char key_map[4][4] = {
        {'1', '2', '3', 'A'},
        {'4', '5', '6', 'B'},
        {'7', '8', '9', 'C'},
        {'*', '0', '#', 'D'}
    };
    
    for(row = 0; row < 4; row++)
    {
        // 当前行拉低，其余行拉高
        for(uint8_t r = 0; r < 4; r++)
            GPIO_WriteBit(KEY_PORT, row_pins[r], (r == row) ? Bit_RESET : Bit_SET);
        
        Delay_us(10);  // 等电平稳定
        
        // 读 4 列
        for(col = 0; col < 4; col++)
        {
            if(GPIO_ReadInputDataBit(KEY_PORT, col_pins[col]) == Bit_RESET)
            {
                Delay_ms(20);  // 消抖
                if(GPIO_ReadInputDataBit(KEY_PORT, col_pins[col]) == Bit_RESET)
                {
                    while(GPIO_ReadInputDataBit(KEY_PORT, col_pins[col]) == Bit_RESET); // 等松开
                    return key_map[row][col];
                }
            }
        }
    }
    return 0;  // 没有按键按下
}
```

### 24.4 AT24C02 EEPROM —— 掉电不丢失的小容量存储

Flash 存储（第九章补充二）适合大块数据，EEPROM 适合频繁写入的小数据（寿命 100 万次 vs Flash 的 1 万次）。AT24C02 是 256 字节的 I2C EEPROM。

**I2C 地址**：0x50（7 位地址，写 = 0xA0，读 = 0xA1）

```c
// 写一个字节到 EEPROM
void AT24C02_WriteByte(uint8_t addr, uint8_t data)
{
    I2C_Start();
    I2C_SendByte(0xA0);       // 设备地址 + 写
    I2C_WaitAck();
    I2C_SendByte(addr);       // EEPROM 内部地址
    I2C_WaitAck();
    I2C_SendByte(data);       // 数据
    I2C_WaitAck();
    I2C_Stop();
    Delay_ms(5);  // EEPROM 写入需要时间（约 5ms），期间不能操作
}

// 读一个字节
uint8_t AT24C02_ReadByte(uint8_t addr)
{
    uint8_t dat;
    I2C_Start();
    I2C_SendByte(0xA0);       // 先写地址
    I2C_WaitAck();
    I2C_SendByte(addr);
    I2C_WaitAck();
    
    I2C_Start();
    I2C_SendByte(0xA1);       // 再读数据
    I2C_WaitAck();
    dat = I2C_ReadByte(0);    // 读 1 字节，发 NACK
    I2C_Stop();
    return dat;
}
```

> **EEPROM vs Flash 的选择**：经常改的参数（如 PID 系数、校准值）→ EEPROM。偶尔保存的数据（如系统配置、日志）→ Flash。

### 24.5 RS485 总线 —— 工业级长距离通信

RS485 是差分信号的串行通信，抗干扰极强，可以在 1200 米距离上稳定通信（低速率下）。电赛中用于长距离传感器网络或多机通信。

**硬件**：需要 RS485 收发器（如 MAX485、SP3485），把 USART 的 TX/RX 转成差分信号 A/B。

**接线**：
```
STM32 USART2_TX ──→ MAX485 DI
STM32 USART2_RX ←── MAX485 RO
STM32 GPIO      ──→ MAX485 DE/RE（发送/接收控制，高=发送，低=接收）
MAX485 A ──── 双绞线 ──── 另一端 MAX485 A
MAX485 B ──── 双绞线 ──── 另一端 MAX485 B
两端各接 120Ω 终端电阻（A 和 B 之间）
```

> **RS485 和 CAN 的区别**：RS485 只定义了物理层（电平），协议层需要自己实现（如 Modbus）；CAN 是物理层+协议层一体的。RS485 更灵活但需要自己处理仲裁和错误检测；CAN 更"开箱即用"但成本高一点。

---

## 第二十五章 电赛备赛策略与临场应对

这一章是比代码更重要的"元知识"——如何在 4 天 3 夜里高效协作、避免踩坑、最大化得分。

### 25.1 赛前准备清单（比赛前一周必须完成）

**硬件准备**：
- [ ] STM32F103C8T6 最小系统板 ×3（烧一片还有备用的）
- [ ] ST-Link V2 ×2（万一坏了一个）
- [ ] 常用模块各 ×2（OLED、MPU6050、NRF24L01、TB6612、HC-SR04、舵机、编码器电机……）
- [ ] 面包板 ×2 + 杜邦线（公母各 30 根）
- [ ] 基本元器件包：电阻（100Ω~100kΩ 各 10 个）、电容（0.1μF×20、10μF×10、100μF×5）、LED×10、按键×5、电位器 10kΩ×3
- [ ] 电源模块：AMS1117-3.3 模块 ×3、LM2596 模块 ×2、18650 电池 ×4 + 充电器
- [ ] 排针、排母、热缩管、焊锡丝、万用板
- [ ] 32GB MicroSD 卡 + 读卡器（备用存储）
- [ ] USB 转 TTL 模块（CH340）×2
- [ ] 逻辑分析仪（如 Saleae Logic 8）

**软件准备**：
- [ ] Keil MDK 工程模板（标准库，已验证可编译烧录的）
- [ ] **22 个代码模板**全部写好、测试通过、分文件整理好
- [ ] 每个 `.c/.h` 文件都有清楚的文件头注释（功能、引脚、使用说明）
- [ ] 常用上位机软件装好：串口助手、蓝牙调试 APP、网络调试助手
- [ ] PCtoLCD2002（字库生成）、FilterPro（滤波器设计）等工具软件

**文档准备**：
- [ ] 本文档 PDF 版存手机（现场查阅）
- [ ] STM32F103 参考手册 PDF
- [ ] 常用芯片数据手册（AMS1117、LM2596、A4988、TB6612、NRF24L01、MPU6050……）

### 25.2 题目分析与方案选择（比赛第 1~4 小时）

**拿到题目后不要立刻动手！** 花 2~4 小时分析题目。

**分析步骤**：
1. **通读所有题目**（控制类、仪器类、电源类、信号类等），找到自己最擅长的方向
2. **精读选中的题目**，逐字逐句——**题目的每一个字都是得分点！**
3. **列出功能清单**：基本部分（必做）+ 发挥部分（选做，尽量多做）
4. **评估技术可行性**：每个功能你们团队能不能做出来？哪里是技术难点？
5. **列出所需硬件清单**：缺什么立刻让采购的同学去买（比赛期间淘宝来不及，要提前备好！）
6. **画系统框图**：传感器→MCU→执行器的完整链路
7. **分工**：硬件（电路搭建+焊接）→ 一人，软件（编程+调试）→ 一人，文档（报告撰写+辅助测试）→ 一人

> **选题铁律**：选最稳妥的，不选最酷的。能做出来的题目才是好题目。控制类题目通常最稳妥（因为 STM32 方案成熟）。

### 25.3 4 天 3 夜时间分配

```
第1天（方案+硬件）：
  08:00-12:00  分析题目、确定方案、画电路图、列元器件清单
  12:00-18:00  搭硬件（面包板焊接/PCB组装）、测试电源
  18:00-24:00  逐个模块写驱动、测试（LED亮了吗？OLED显示了吗？电机转了吗？）
  
第2天（核心功能）：
  08:00-12:00  写核心控制算法（PID、状态机）
  12:00-18:00  调参、联调、基本功能跑通
  18:00-24:00  完善基本功能的稳定性和边界条件
  ⚠️ 睡前：基本功能必须全部跑通！这是死线！

第3天（发挥+完善）：
  08:00-12:00  做发挥部分（先做最容易得分的发挥项）
  12:00-18:00  系统联调、稳定性测试
  18:00-24:00  极端条件测试、Bug 修复、代码整理
  ⚠️ 睡前：全部功能（基本+发挥）必须稳定运行！

第4天（报告+封箱）：
  08:00-12:00  最终测试、录制演示视频（如果有要求）
  12:00-15:00  写/完善设计报告
  15:00-17:00  封箱、准备答辩 PPT
  17:00 前    封箱！
```

### 25.4 临场应对：常见紧急情况的处理

| 问题               | 紧急处理                                                             |
| ------------------ | -------------------------------------------------------------------- |
| STM32 烧了         | 换备用板（所以必须带 3 块！）。检查烧毁原因（接 5V 了？短路了？）    |
| 电机驱动烧了       | 换备用模块。检查是否堵转过流                                         |
| 程序烧不进去       | 检查 BOOT0 是否接 GND、检查 SWD 接线（PA13/PA14）、检查 ST-Link 驱动 |
| OLED 不显示        | 检查 I2C 地址（0x78 vs 0x7A）、上拉电阻、延时够不够                  |
| 电机不转           | 检查共地、PWM 输出波形（示波器）、方向引脚电平（万用表）             |
| 传感器数据乱跳     | 加滤波电容、查电源、软件滤波                                         |
| 无线通信失败       | 查频道是否一致、地址是否匹配、电源滤波电容是否焊了                   |
| 代码跑飞/死机      | 开启 IWDG、检查栈溢出（大数组不能在函数内部定义！）、检查数组越界    |
| ADC 读数偏差大     | 校准 ADC、检查 VDDA 滤波电容、查参考电压是否稳定                     |
| 答辩时问题答不上来 | 诚实回答"这是我们考虑不足的地方"，外加一句"我们会在后续改进中……"     |

### 25.5 答辩与现场测试注意事项

1. **演示要流畅**：提前演练演示流程 10 遍！评委老师看重的是"稳定可靠地完成功能"。
2. **准备后备方案**：如果关键模块在演示时失效，有没有备用方案？
3. **着装整洁**：第一印象很重要。不需要正装，但不能穿拖鞋短裤。
4. **术业有专攻**：硬件问题由硬件同学回答，软件问题由软件同学回答。不要抢答。
5. **会就是会，不会就是不会**：评委都是教授/专家，你编造答案他们一眼就能看穿。诚实不会扣分，但撒谎肯定扣分。
6. **设计报告打印 3 份**：一份提交、一份评委看、一份自己备用。
7. **代码要规范**：评委可能会要求看代码。注释清晰、命名规范、结构合理的代码会加分。

### 25.6 电赛一等奖的核心竞争力（最终总结）

回顾整篇文档，电赛一等奖的能力结构已经全部覆盖：

```
┌────────────────────────────────────────────┐
│ 层次五：竞赛策略与临场发挥                   │← 第二十五章
│  - 题目分析、时间管理、答辩准备              │
├────────────────────────────────────────────┤
│ 层次四：系统集成与实战经验                   │← 第11~14章 + 第22~24章
│  - 模块驱动、通信模块、SD卡、传感器          │
├────────────────────────────────────────────┤
│ 层次三：控制算法与信号处理                   │← 第12章 + 第15章
│  - PID、滤波、状态机、FFT、运放电路          │
├────────────────────────────────────────────┤
│ 层次二：STM32外设与总线编程                  │← 第3~11章 + 第21章
│  - GPIO/定时器/ADC/USART/I2C/SPI/DMA/CAN    │
│  - 步进电机、舵机                            │
├────────────────────────────────────────────┤
│ 层次一：基础理论                             │← 第2章 + 第16~20章
│  - C语言、数电模电、电源、PCB、仪器、报告    │
└────────────────────────────────────────────┘
```

**最后一句忠告**：此文近万字，但文字不能替代实践。**关上电脑，拿起烙铁，打开 Keil——这才是通往一等奖的唯一道路。**

---

## 附录一：电赛常用元件速查卡

### 电阻色环速读
| 颜色 | 黑  | 棕  | 红   | 橙  | 黄   | 绿    | 蓝     | 紫  | 灰  | 白  |
| ---- | --- | --- | ---- | --- | ---- | ----- | ------ | --- | --- | --- |
| 数字 | 0   | 1   | 2    | 3   | 4    | 5     | 6      | 7   | 8   | 9   |
| 乘数 | ×1  | ×10 | ×100 | ×1K | ×10K | ×100K | ×1M    |     |     |     |
| 误差 | -   | ±1% | ±2%  | -   | -    | ±0.5% | ±0.25% | -   | -   | -   |

四环电阻：①环=第一位，②环=第二位，③环=乘数，④环=误差（金=±5%，棕=±1%）
五环电阻：①②③=三位有效数字，④=乘数，⑤=误差
例如：棕黑红金 = 1→0→×100→±5% = 1000Ω = 1kΩ ±5%

### 电容标称值速读
- 贴片电容无标记（需要用万用表或LCR表测量）
- 电解电容直接标电压和容量（如 "16V 100μF"）
- 104 = 10 × 10^4 pF = 100,000 pF = 100nF = 0.1μF
- 103 = 10 × 10^3 pF = 10,000 pF = 10nF = 0.01μF
- 221 = 22 × 10^1 pF = 220pF

### 常用二极管
| 型号   | 类型             | 参数            | 电赛用途           |
| ------ | ---------------- | --------------- | ------------------ |
| 1N4007 | 普通整流         | 1000V/1A        | 电源整流、反接保护 |
| 1N4148 | 小信号开关       | 100V/200mA      | 信号电路           |
| 1N5822 | 肖特基(Schottky) | 40V/3A, Vf≈0.5V | DC-DC续流二极管    |
| SS34   | 贴片肖特基       | 40V/3A, Vf≈0.5V | PCB DC-DC          |

### 常用三极管/MOS管
| 型号    | 类型               | 参数                       | 电赛用途             |
| ------- | ------------------ | -------------------------- | -------------------- |
| S8050   | NPN BJT            | 40V/1.5A/1W                | 驱动小继电器、LED    |
| S8550   | PNP BJT            | 40V/1.5A/1W（与S8050互补） | 推挽输出、H桥        |
| 2N2222  | NPN BJT            | 40V/0.8A/0.5W              | 通用开关             |
| IRLZ44N | N-MOSFET(逻辑电平) | 55V/47A, Vgs(th)≈1~2V      | 大电流开关、电机驱动 |
| IRF540  | N-MOSFET           | 100V/33A, Vgs(th)≈3~4V     | 需要10V栅极驱动      |

> **MOS 管选择要点**：用 STM32 的 3.3V GPIO 直接驱动 MOS 管，必须选"逻辑电平"型的（Vgs(th) < 2.5V），如 IRLZ44N。普通 IRF 系列需要 5V 以上才能完全导通。

---

## 附录二：电赛常见芯片引脚速查

### AMS1117-3.3
```
    ┌──────┐
GND─┤1  3 ├─ VIN (4.5V~12V)
VOUT┤2    │
    └──────┘
    SOT-223封装（贴片）
```

### TB6612FNG 电机驱动
```
        ┌────────────────────────┐
    GND─┤1   TB6612FNG       16├─ GND
   VM1─┤2                    15├─ VM2
   A01─┤3（A路输出1）        14├─ B02（B路输出2）
   A02─┤4（A路输出2）        13├─ B01（B路输出1）
  PWMA─┤5（A路PWM输入）      12├─ PWMB
  AIN1─┤6                    11├─ BIN1
  AIN2─┤7                    10├─ BIN2
  STBY─┤8                     9├─ VCC(3.3V)
        └────────────────────────┘
```

### A4988 步进电机驱动
```
        ┌──────────────┐
 ENABLE─┤1           16├─ VMOT(8~35V)
    MS1─┤2           15├─ GND
    MS2─┤3   A4988   14├─ 2B（绕组B）
    MS3─┤4           13├─ 2A（绕组B）
  RESET─┤5           12├─ 1A（绕组A）
  SLEEP─┤6           11├─ 1B（绕组A）
   STEP─┤7           10├─ VDD(3.3V/5V)
    DIR─┤8            9├─ GND
        └──────────────┘
```

### NRF24L01 无线模块
```
        ┌──────────────┐
   GND─┤1   NRF24L01 8├─ VCC(3.3V!)
    CE─┤2            7├─ MISO
   CSN─┤3            6├─ MOSI
   SCK─┤4            5├─ IRQ
        └──────────────┘
```

---

# 第二十六章 电赛代码模板全集

本章是你参加电赛的**核心武器**。每一个模板都经过精心编写和验证，你可以直接复制到工程中编译使用。

每个模板包含：
- 📁 **文件位置**：在工程文件夹中的路径及需要包含的头文件
- 🔧 **完整代码**：`.h` 头文件和 `.c` 源文件，带详尽注释
- 🏆 **电赛修改指南**：标注了比赛中需要修改的位置、如何改、为什么改

**使用方式**：按照本章目录将各模板文件复制到你的工程对应位置，在 `stm32f10x_conf.h` 中取消注释用到的外设头文件，在 Keil 工程中添加入对应的 Group 即可编译。

---

## 26.0 工程基础模板 —— 一切代码的起点

### 26.0.1 工程目录结构

电赛代码的标准工程目录结构如下：

```
Template/
├── Libraries/                    ← 库文件（从标准固件库复制）
│   ├── CMSIS/
│   │   ├── core_cm3.c
│   │   ├── core_cm3.h
│   │   ├── system_stm32f10x.c
│   │   ├── system_stm32f10x.h
│   │   └── startup_stm32f10x_md.s
│   └── FWlib/
│       ├── inc/                  ← 外设头文件（全部复制进去）
│       └── src/                  ← 外设源文件（全部复制进去）
├── User/                         ← 用户代码，以下模板放这里
│   ├── main.c                    ← 主函数（见 26.0.2）
│   ├── stm32f10x_conf.h          ← 库配置文件（见 26.0.3）
│   ├── stm32f10x_it.c            ← 中断服务函数集中定义（见 26.0.4）
│   ├── stm32f10x_it.h
│   ├── sys_tick.c / sys_tick.h   ← 系统时基（见 26.0.5）
│   └── delay.c / delay.h         ← 精确延时（见 26.0.6）
├── Hardware/                     ← 你写的所有模块驱动放这里
│   ├── led/          led.c, led.h
│   ├── key/          key.c, key.h
│   ├── oled/         oled_i2c.c, oled_i2c.h, oledfont.h
│   ├── usart/        usart.c, usart.h
│   ├── i2c/          i2c_soft.c, i2c_soft.h
│   ├── spi/          spi_soft.c, spi_soft.h
│   ├── motor_dc/     motor_dc.c, motor_dc.h
│   ├── motor_stepper/ motor_stepper.c, motor_stepper.h
│   ├── servo/        servo.c, servo.h
│   ├── encoder/      encoder.c, encoder.h
│   ├── mpu6050/      mpu6050.c, mpu6050.h
│   ├── hc_sr04/      hc_sr04.c, hc_sr04.h
│   ├── ds18b20/      ds18b20.c, ds18b20.h
│   ├── nrf24l01/     nrf24l01.c, nrf24l01.h
│   ├── bluetooth/    bluetooth.c, bluetooth.h
│   ├── flash_store/  flash_store.c, flash_store.h
│   ├── at24c02/      at24c02.c, at24c02.h
│   ├── pid/          pid.c, pid.h
│   ├── filter/       filter.c, filter.h
│   └── state_machine/ state_machine.h
└── Output/                       ← 编译输出 .hex 文件
```

### 26.0.2 主函数模板 `User/main.c`

```c
/**
 * @file    main.c
 * @brief   TI 杯电赛主程序模板
 * @note    本模板使用了以下基础模块：
 *          - sys_tick：系统时基（1ms）
 *          - delay：精确延时
 *          - led：状态指示
 *          - usart：调试串口
 *          其他模块按需在初始化段中取消注释并添加。
 *          
 *          【电赛修改指南】
 *          1. 在 "添加你的头文件" 区域 #include 你需要的模块头文件
 *          2. 在 "模块初始化" 区域调用各模块的 Init 函数
 *          3. 在 while(1) 中添加你的任务
 *          4. 修改任务周期（TASK_PERIOD_xxx_ms）即可调整各任务频率
 */

/* ===== 标准库头文件 ===== */
#include "stm32f10x.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* ===== 系统基础头文件 ===== */
#include "sys_tick.h"
#include "delay.h"

/* ===== 添加你的头文件 ===== */
// #include "led.h"
// #include "key.h"
// #include "usart.h"
// #include "oled_i2c.h"
// #include "i2c_soft.h"
// #include "spi_soft.h"
// #include "mpu6050.h"
// #include "hc_sr04.h"
// #include "ds18b20.h"
// #include "motor_dc.h"
// #include "motor_stepper.h"
// #include "servo.h"
// #include "encoder.h"
// #include "nrf24l01.h"
// #include "bluetooth.h"
// #include "flash_store.h"
// #include "at24c02.h"
// #include "pid.h"
// #include "filter.h"

/* ===== 任务周期定义（单位：毫秒）===== */
//【电赛修改】根据你的控制需求调整这些周期值
#define TASK_PERIOD_1MS       1     // 1ms 任务（编码器读取等高频操作）
#define TASK_PERIOD_5MS       5     // 5ms 任务（传感器快速读取）
#define TASK_PERIOD_10MS      10    // 10ms 任务（PID 控制计算）
#define TASK_PERIOD_20MS      20    // 20ms 任务（NRF 遥控接收）
#define TASK_PERIOD_50MS      50    // 50ms 任务（传感器更新）
#define TASK_PERIOD_100MS     100   // 100ms 任务（OLED 刷新）
#define TASK_PERIOD_500MS     500   // 500ms 任务（LED 心跳灯）
#define TASK_PERIOD_1000MS    1000  // 1000ms 任务（系统状态输出）

/* ===== 任务时间基准变量 ===== */
static uint32_t t_1ms   = 0;
static uint32_t t_5ms   = 0;
static uint32_t t_10ms  = 0;
static uint32_t t_20ms  = 0;
static uint32_t t_50ms  = 0;
static uint32_t t_100ms = 0;
static uint32_t t_500ms = 0;
static uint32_t t_1000ms = 0;

/* ===== 系统初始化 ===== */
/**
 * @brief  系统初始化
 * @note   初始化顺序很重要！
 *         1. SysTick 最先初始化（提供系统时基，Delay 依赖它）
 *         2. 然后是调试串口（方便查问题）
 *         3. 然后是其他模块（按依赖关系排序）
 */
static void System_Init(void)
{
    /* ----- 第1步：系统时基（最先！）----- */
    SysTick_Init();             // 启动 1ms 系统时基
    
    /* ----- 第2步：调试串口 ----- */
    //【电赛修改】如果不需要 printf 调试可注释掉，但建议保留（比赛现场调试救命用）
    // USART1_Init(115200);        // 初始化串口1，波特率 115200
    // printf("\r\n===== TI Cup System Start =====\r\n");
    // printf("System Clock: 72MHz\r\n");
    // printf("SysTick: 1ms\r\n");
    
    /* ----- 第3步：LED 状态指示 ----- */
    // LED_Init();                 // 初始化核心板 LED（PC13），低电平亮
    // LED_On();                   // 亮灯表示系统已启动
    
    /* ----- 第4步：按键 ----- */
    // KEY_Init();                 // 初始化按键（PA0，上拉输入）
    
    /* ----- 第5步：I2C（如果使用 OLED/MPU6050/AT24C02）----- */
    // I2C_Soft_Init();            // 软件 I2C 初始化（PB6=SCL, PB7=SDA）
    //【电赛修改】如果用其他引脚做 I2C，在 i2c_soft.h 中改宏定义
    
    /* ----- 第6步：SPI（如果使用 NRF24L01）----- */
    // SPI_Soft_Init();            // 软件 SPI 初始化
    
    /* ----- 第7步：显示模块 ----- */
    // OLED_Init();                // OLED 初始化（上电后至少延时 100ms）
    // OLED_Clear();
    // OLED_ShowString(0, 0, "System Ready!", 12);
    
    /* ----- 第8步：传感器 ----- */
    // MPU6050_Init();             // 陀螺仪初始化（需先初始化 I2C）
    // HCSR04_Init();              // 超声波初始化
    // DS18B20_Init();             // 温度传感器初始化
    
    /* ----- 第9步：执行器 ----- */
    // Encoder_Init();             // 编码器初始化（TIM3 CH1=PA6, CH2=PA7）
    // Motor_DC_Init();            // 直流电机初始化（需先初始化编码器和 PWM 定时器）
    // Servo_Init();               // 舵机初始化（使用 TIM2_CH1=PA0, 50Hz）
    // Motor_Stepper_Init();       // 步进电机初始化（使用 TIM4_CH1=PB6）
    
    /* ----- 第10步：通信模块 ----- */
    // NRF24L01_Init(NRF_MODE_RX); // NRF 初始化（0=接收, 1=发送）
    // Bluetooth_Init(9600);       // 蓝牙初始化（默认 9600，HC-05 AT 模式 38400）
    
    /* ----- 第11步：存储 ----- */
    // AT24C02_Init();             // EEPROM 初始化（需先初始化 I2C）
    //【电赛注意】Flash 读取在 PID 初始化之前
    // Flash_LoadConfig();         // 从 Flash 加载上次保存的参数
    
    /* ----- 第12步：算法初始化 ----- */
    // PID_Init(&speed_pid, 0.5f, 0.1f, 0.0f, 90.0f, 200.0f);
    //【电赛修改】PID 参数需要现场调试！上面的是示例值，必须根据你的系统调整
    
    printf("System Init Complete.\r\n");
}

/* ===== 主函数 ===== */
int main(void)
{
    System_Init();              // 系统初始化
    
    /* ===== 主循环 ===== */
    while(1)
    {
        /* ----- 每 1ms 执行的超高频任务 ----- */
        //【电赛修改】1ms 任务只放最紧急的事情（编码器、PWM 保护等）
        if(sys_time - t_1ms >= TASK_PERIOD_1MS)
        {
            t_1ms = sys_time;   // 更新基准时间（用 = 不用 +=，防止累积误差）

            // 示例：编码器读取（如果放在定时器中断里就不需要这里）
            // Encoder_Update();
        }

        /* ----- 每 5ms 执行的高频任务 ----- */
        if(sys_time - t_5ms >= TASK_PERIOD_5MS)
        {
            t_5ms = sys_time;

            // 示例：MPU6050 快速读取
            // MPU6050_ReadData();
        }

        /* ----- 每 10ms 执行的中频任务（PID 控制）----- */
        if(sys_time - t_10ms >= TASK_PERIOD_10MS)
        {
            t_10ms = sys_time;

            // 示例：PID 速度/位置/平衡控制
            // MotorSpeedControl_Loop();    // 速度闭环
            // BalanceControl_Loop();       // 平衡控制
        }

        /* ----- 每 20ms 执行（NRF 遥控接收等）----- */
        if(sys_time - t_20ms >= TASK_PERIOD_20MS)
        {
            t_20ms = sys_time;

            // 示例：NRF 数据发送/接收
            // NRF_Process();
        }

        /* ----- 每 50ms 执行（传感器更新）----- */
        if(sys_time - t_50ms >= TASK_PERIOD_50MS)
        {
            t_50ms = sys_time;

            // 示例：超声波测距、温度读取
            // distance = HCSR04_GetDistance();
            // temperature = DS18B20_ReadTemp();
        }

        /* ----- 每 100ms 执行（显示刷新）----- */
        if(sys_time - t_100ms >= TASK_PERIOD_100MS)
        {
            t_100ms = sys_time;

            // 示例：OLED 显示更新
            // OLED_ShowString(0, 0, "Speed:    RPM", 12);
            // OLED_ShowNum(48, 0, (uint32_t)actual_speed_rpm, 4, 12);
        }

        /* ----- 每 500ms 执行（心跳灯）----- */
        if(sys_time - t_500ms >= TASK_PERIOD_500MS)
        {
            t_500ms = sys_time;

            // 心跳灯翻转（表示系统在正常运行）
            // LED_Toggle();
            
            // IWDG_Feed();  // 喂狗（看门狗）
        }

        /* ----- 每 1000ms 执行（串口状态输出）----- */
        if(sys_time - t_1000ms >= TASK_PERIOD_1000MS)
        {
            t_1000ms = sys_time;

            // 示例：通过串口打印系统状态
            // printf("Time: %lus, CPU: OK\r\n", sys_time / 1000);
        }

        /* ----- 事件驱动任务（非周期性）----- */
        // 按键处理
        // uint8_t key_val = KEY_Scan();
        // if(key_val) Key_Process(key_val);
        
        // 串口接收处理
        // if(usart_rx_flag) { usart_rx_flag = 0; USART_Process(); }
        
        // NRF 接收处理
        // if(nrf_rx_flag) { nrf_rx_flag = 0; NRF_ProcessRX(); }
    }
}
```

### 26.0.3 库配置文件 `User/stm32f10x_conf.h`

```c
/**
 * @file    stm32f10x_conf.h
 * @brief   STM32F10x 标准库配置文件
 * @note    【电赛修改指南】
 *          把你需要的外设头文件取消注释（去掉 //），不需要的保持注释。
 *          这样可以减小编译后的代码体积。
 *          
 *          常用组合：
 *          - 最小系统：gpio + rcc + misc + tim + exti
 *          - 加串口：  + usart
 *          - 加 PWM：  + tim（已包含）
 *          - 加 ADC：  + adc + dma
 *          - 加 I2C：  如果用软件模拟，不需要加 i2c（用 gpio 即可）
 *          - 加 SPI：  如果用软件模拟，不需要加 spi（用 gpio 即可）
 *          - 加 CAN：  + can
 *          - 加看门狗：+ iwdg（不需要 wwdg）
 *          - 加 Flash：+ flash
 */
#ifndef __STM32F10x_CONF_H
#define __STM32F10x_CONF_H

/* 取消注释你需要的外设头文件 */
#include "stm32f10x_gpio.h"     /* GPIO：必选 */
#include "stm32f10x_rcc.h"      /* 时钟：必选 */
#include "stm32f10x_misc.h"     /* NVIC：必选 */
#include "stm32f10x_tim.h"      /* 定时器：必选（PWM/编码器/输入捕获都用它）*/
#include "stm32f10x_exti.h"     /* 外部中断：键盘/传感器中断 */
// #include "stm32f10x_usart.h"   /* 串口：调试/蓝牙/ESP8266 */
// #include "stm32f10x_adc.h"     /* ADC：模拟信号采集 */
// #include "stm32f10x_dma.h"     /* DMA：ADC 多通道/USART 大数据收发 */
// #include "stm32f10x_i2c.h"     /* 硬件 I2C：一般不用（用软件模拟）*/
// #include "stm32f10x_spi.h"     /* 硬件 SPI：高速场景使用 */
// #include "stm32f10x_can.h"     /* CAN：多机通信 */
// #include "stm32f10x_flash.h"   /* 片内 Flash：掉电保存参数 */
// #include "stm32f10x_iwdg.h"    /* 独立看门狗：防死机 */
// #include "stm32f10x_wwdg.h"    /* 窗口看门狗：一般不用 */
// #include "stm32f10x_pwr.h"     /* 电源管理：低功耗模式 */
// #include "stm32f10x_bkp.h"     /* 备份寄存器：存储关键参数 */
// #include "stm32f10x_rtc.h"     /* RTC：实时时钟 */
// #include "stm32f10x_fsmc.h"    /* FSMC：C8T6 没有，不用 */
// #include "stm32f10x_sdio.h"    /* SDIO：C8T6 没有，不用 */

/* 如果定义了 USE_FULL_ASSERT，断言失败时会调用 assert_failed() */
// #define USE_FULL_ASSERT

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line);
#endif

#endif /* __STM32F10x_CONF_H */
```

### 26.0.4 中断服务函数集中定义 `User/stm32f10x_it.c`

```c
/**
 * @file    stm32f10x_it.c
 * @brief   中断服务函数（ISR）集中定义文件
 * @note    【极其重要】中断函数名必须和启动文件 startup_stm32f10x_md.s 中定义一致！
 *          写错一个字母，中断就永远不会触发，程序也不会报任何错误。
 *          
 *          【电赛修改指南】
 *          1. 在要用到的中断服务函数里添加你的处理代码
 *          2. 不用的中断函数可以保留空函数体（或者删除，但保留更安全）
 *          3. 中断里只做最紧急的事（置标志位、存数据），复杂处理放主循环
 *          4. 中断里修改的全局变量必须加 volatile 修饰
 *          5. 进入中断第一时间检查中断标志，退出前必须清除中断标志
 */

#include "stm32f10x_it.h"
#include "sys_tick.h"

/* 外部变量声明（如果中断里需要访问主循环的变量）*/
// extern volatile uint8_t key_flag;
// extern volatile uint8_t usart_rx_flag;

/**
 * @brief  SysTick 中断服务函数（1ms）
 *         系统心跳，已在 sys_tick.c 实现，这里只需声明引用
 */
extern void SysTick_Handler(void);

/**
 * @brief  HardFault 异常处理 —— 程序崩溃时进入这里
 * @note   如果程序进入 HardFault，说明出现了严重错误
 *         （数组越界、野指针、栈溢出等）
 *         调试方法：在 Keil 调试模式下，看 Call Stack 窗口可以定位到崩溃位置
 */
void HardFault_Handler(void)
{
    /* 死循环 + LED 闪烁表示进入了 HardFault */
    while(1)
    {
        /* 如果你的工程里有 LED，可以加闪烁代码帮助判断 */
        // LED_On();
        // for(volatile uint32_t i=0; i<1000000; i++);
        // LED_Off();
        // for(volatile uint32_t i=0; i<1000000; i++);
    }
}

/* ===== 以下中断服务函数按需使用 ===== */

/**
 * @brief  EXTI0 外部中断（PA0 引脚）
 */
// void EXTI0_IRQHandler(void)
// {
//     if(EXTI_GetITStatus(EXTI_Line0) != RESET)
//     {
//         /* 在这里添加你的处理代码 */
//         // key_flag = 1;   // 例如：按键标志位置位
//
//         EXTI_ClearITPendingBit(EXTI_Line0);  // 必须清除中断标志！
//     }
// }

/**
 * @brief  EXTI9_5 外部中断（PA5~PA9 引脚共用）
 */
// void EXTI9_5_IRQHandler(void)
// {
//     /* 注意：多个引脚共用这个中断，需要逐一检查 */
//     if(EXTI_GetITStatus(EXTI_Line5) != RESET)
//     { EXTI_ClearITPendingBit(EXTI_Line5); }
//     if(EXTI_GetITStatus(EXTI_Line6) != RESET)
//     { EXTI_ClearITPendingBit(EXTI_Line6); }
//     /* ... */
// }

/**
 * @brief  TIM2 中断服务函数（多模块共用注意！）
 * @note    
 * 【极其重要】TIM2 可能被以下模块同时使用：
 *   - Motor_DC：PWM 输出（CH1=PA0, CH2=PA1）→ PWM 不需中断
 *   - Servo：PWM 输出（CH3=PA2）→ PWM 不需中断  
 *   - HC-SR04：输入捕获（CH2=PA1）+ 更新中断 → 需要中断！
 * 
 * 如果同时使用 HC-SR04 和其他模块：
 * 必须在同一个 TIM2_IRQHandler 中合并处理。
 * PWM 本身不需要中断（硬件自动输出），
 * 只有 HC-SR04 的捕获/溢出需要中断处理。
 * 
 * 如果只用 Motor_DC 或 Servo（不需要中断）：
 * 保持此函数注释掉，不要取消注释，
 * 也不要使能 TIM2 的更新/捕获中断。
 */
// void TIM2_IRQHandler(void)
// {
//     /* ==== HC-SR04 超声波处理（如果使用）==== */
//     /* 声明 HC-SR04 的外部变量（hc_sr04.c 中定义）*/
//     // extern volatile uint8_t  echo_state;
//     // extern volatile uint32_t echo_time;
//     // extern volatile uint8_t  echo_done;
//     // extern volatile uint16_t echo_ovf_cnt;
//
//     /* 溢出中断 */
//     // if(TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
//     // {
//     //     if(echo_state == 1) {
//     //         echo_ovf_cnt++;
//     //         if(echo_ovf_cnt >= 5)
//     //             { echo_done = 2; echo_state = 0; echo_ovf_cnt = 0; }
//     //     }
//     //     TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
//     // }
//     /* 捕获中断 */
//     // if(TIM_GetITStatus(TIM2, TIM_IT_CC2) != RESET)
//     // {
//     //     if(echo_state == 0) {
//     //         TIM_SetCounter(TIM2, 0); echo_ovf_cnt = 0; echo_state = 1;
//     //         TIM_OC2PolarityConfig(TIM2, TIM_ICPolarity_Falling);
//     //     } else if(echo_state == 1) {
//     //         echo_time = TIM_GetCapture2(TIM2) + echo_ovf_cnt * 65536;
//     //         echo_state = 0; echo_done = 1;
//     //         TIM_OC2PolarityConfig(TIM2, TIM_ICPolarity_Rising);
//     //     }
//     //     TIM_ClearITPendingBit(TIM2, TIM_IT_CC2);
//     // }
//     /* ==== 其他 TIM2 处理加在下面 ==== */
// }

/**
 * @brief  TIM3 定时器中断（更新中断）
 */
// void TIM3_IRQHandler(void)
// {
//     if(TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)
//     {
//         TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
//     }
// }

/**
 * @brief  USART1 串口中断
 */
// void USART1_IRQHandler(void)
// {
//     /* RXNE：接收数据寄存器非空 */
//     if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
//     {
//         uint8_t ch = USART_ReceiveData(USART1);
//         /* 把接收到的字节存入缓冲区 */
//         // if(rx_cnt < RX_BUF_SIZE - 1)
//         //     rx_buf[rx_cnt++] = ch;
//         /* 帧结束判断（例如收到换行符）*/
//         // if(ch == '\n')
//         // {
//         //     rx_buf[rx_cnt] = '\0';
//         //     rx_flag = 1;
//         //     rx_cnt = 0;
//         // }
//         USART_ClearITPendingBit(USART1, USART_IT_RXNE);
//     }
//
//     /* IDLE：空闲中断（一帧数据结束）*/
//     if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)
//     {
//         /* 清除 IDLE：先读 SR，再读 DR */
//         volatile uint32_t tmp = USART1->SR;
//         tmp = USART1->DR;
//         (void)tmp;
//         /* 处理接收完成 */
//         // rx_flag = 1;
//     }
// }

/**
 * @brief  DMA1 Channel 1 中断（ADC DMA 传输完成）
 */
// void DMA1_Channel1_IRQHandler(void)
// {
//     if(DMA_GetITStatus(DMA1_IT_TC1) != RESET)
//     {
//         DMA_ClearITPendingBit(DMA1_IT_TC1);
//         /* ADC DMA 一轮传输完成 */
//     }
// }
```

### 26.0.5 系统时基模板 `User/sys_tick.h` 和 `User/sys_tick.c`

**`User/sys_tick.h`**：

```c
/**
 * @file    sys_tick.h
 * @brief   系统时基模块（SysTick 1ms 中断）
 * @note    提供 uint32_t sys_time 变量，上电后每 1ms 自动加 1
 *          所有非阻塞延时和任务调度都依赖此模块
 *          
 *          【依赖】无（纯 CMSIS 内核外设，不需要开 APB 时钟）
 *          【被依赖】delay.c / main.c / 几乎所有模板
 *          
 *          【电赛修改指南】
 *          通常不需要修改此文件。如需更改时基周期：
 *          改 TICK_PERIOD_MS 宏（1=1ms, 2=2ms...），
 *          Load 值会自动计算。
 */
#ifndef __SYS_TICK_H
#define __SYS_TICK_H

#include "stm32f10x.h"

/* 系统时间（全局变量，每 1ms +1，不要手动修改它！）*/
extern volatile uint32_t sys_time;

/* 时基周期（毫秒），修改此处即可调整时基频率 */
#define TICK_PERIOD_MS  1

void SysTick_Init(void);

#endif
```

**`User/sys_tick.c`**：

```c
/**
 * @file    sys_tick.c
 * @brief   系统时基模块实现
 * @note    使用 Cortex-M3 内核的 SysTick 定时器产生 1ms 中断
 *          SysTick 时钟 = AHB = 72MHz
 *          Load 值 = 72MHz * 0.001s - 1 = 72000 - 1 = 71999
 *          
 *          【电赛修改指南】
 *          通常不需要修改此文件。唯一可能需要改的是中断优先级。
 *          NVIC_SetPriority 的数值越小优先级越高（0=最高，15=最低）。
 */
#include "sys_tick.h"

/* 系统运行毫秒计数（volatile 修饰：中断中修改，主循环读取）*/
volatile uint32_t sys_time = 0;

/**
 * @brief  初始化 SysTick 为 1ms 中断模式
 */
void SysTick_Init(void)
{
    /* ----- 选择时钟源：AHB（72MHz）----- */
    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);

    /* ----- 设置重装载值 ----- */
    /* 公式：Load = (72000000 * TICK_PERIOD_MS / 1000) - 1 */
    /* 1ms 时：72000 * 1 - 1 = 71999 */
    SysTick_SetReload(SystemCoreClock / 1000 * TICK_PERIOD_MS - 1);
    /* SystemCoreClock 定义在 system_stm32f10x.c 中，值为 72000000 */
    /* 这样写的好处：如果改了系统时钟或时基周期，Load 值自动调整 */

    /* ----- 清空计数器 ----- */
    SysTick_CounterCmd(SysTick_Counter_Clear);

    /* ----- 配置中断优先级 ----- */
    /* SysTick_IRQn = -1（内核中断，不是外设中断）*/
    /* 优先级设为 1（较高，因为它提供整个系统时基）*/
    NVIC_SetPriority(SysTick_IRQn, 1);

    /* ----- 使能 SysTick 并开启中断 ----- */
    SysTick_ITConfig(ENABLE);           /* 使能中断 */
    SysTick_CounterCmd(SysTick_Counter_Enable);  /* 启动计数器 */
}

/**
 * @brief  SysTick 中断服务函数（1ms）
 * @note   函数名必须和启动文件中的定义一致！
 *         这个函数在 startup_stm32f10x_md.s 中被声明为 SysTick_Handler
 */
void SysTick_Handler(void)
{
    sys_time++;  /* 每 1ms 加 1 */
}
```

### 26.0.6 精确延时模板 `User/delay.h` 和 `User/delay.c`

**`User/delay.h`**：

```c
/**
 * @file    delay.h
 * @brief   精确延时模块（微秒级 + 毫秒级）
 * @note    使用 SysTick 硬件计数器实现精确延时
 *          不依赖中断，在轮询模式下使用 COUNTFLAG 标志
 *          
 *          【依赖】无（不需要 sys_tick，直接操作 SysTick 寄存器）
 *          
 *          【注意】Delay_us 是阻塞式的！只适合初始化阶段或短延时。
 *                 主循环中不要用 Delay_us(100000) 这样的大延时。
 *                 用 sys_time 做非阻塞延时代替。
 */
#ifndef __DELAY_H
#define __DELAY_H

#include "stm32f10x.h"

void Delay_us(uint32_t us);
void Delay_ms(uint32_t ms);

#endif
```

**`User/delay.c`**：

```c
/**
 * @file    delay.c
 * @brief   精确延时实现
 * @note    工作原理：
 *          - 关闭 SysTick → 设 LOAD 值 → 清 VAL → 开启 → 等 COUNTFLAG → 关闭
 *          - 每个时钟周期 = 1/72MHz ≈ 13.9ns
 *          - LOAD 是 24 位的，最大值 16,777,215
 *          - 最大延时 = 16,777,215 / 72 = 233,016 μs ≈ 233ms
 *          - 超过 233ms 用 Delay_ms（内部循环调用 Delay_us(1000)）
 *          
 *          【电赛注意】
 *          此实现和 sys_tick 的 SysTick 中断可能冲突！
 *          因为 Delay_us 会关闭并重新配置 SysTick。
 *          解决方案：调用 Delay_us 期间 sys_time 不更新（丢失几个 ms）。
 *          对于初始化阶段的延时来说，丢失几个 ms 完全不影响。
 *          如果需要在主循环中同时使用 Delay_us 和 sys_time，
 *          建议改用 TIM 定时器实现延时（不占用 SysTick）。
 */
#include "delay.h"

static uint8_t delay_initialized = 0;  /* 首次调用标志 */

/**
 * @brief  微秒级延时
 * @param  us: 微秒数（最大约 233,000us = 233ms）
 */
void Delay_us(uint32_t us)
{
    if(!delay_initialized)
    {
        /* 首次调用时初始化 SysTick 时钟源 */
        SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);
        delay_initialized = 1;
    }

    /* 关闭计数器 */
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;

    /* 设置重装载值 */
    /* us * 72 是因为 72MHz 下每个计数 = 1/72 μs */
    /* -1 是因为计数过程从 LOAD 减到 0 共 LOAD+1 个周期 */
    SysTick->LOAD = us * (SystemCoreClock / 1000000) - 1;
    /* SystemCoreClock/1e6 = 72，所以等价于 us * 72 - 1 */

    /* 清空当前值 */
    SysTick->VAL = 0UL;

    /* 选择时钟源 + 使能计数器（不使能中断）*/
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;

    /* 等待 COUNTFLAG 置位（计数到 0）*/
    while(!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk));

    /* 关闭计数器 */
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
}

/**
 * @brief  毫秒级延时
 * @param  ms: 毫秒数
 */
void Delay_ms(uint32_t ms)
{
    while(ms--)
    {
        Delay_us(1000);  /* 每次延时 1ms，循环 ms 次 */
    }
}
```

---

## 26.1 GPIO 基础模块 —— LED / 按键 / 蜂鸣器

### 26.1.1 LED `Hardware/led/led.h` 和 `Hardware/led/led.c`

**`Hardware/led/led.h`**：

```c
/**
 * @file    led.h
 * @brief   LED 状态指示灯模块
 * @note    使用核心板上的 PC13（低电平点亮）
 *          常用作：系统心跳灯、状态指示、错误报警
 *          
 *          【电赛修改指南】
 *          如果你的核心板 LED 不在 PC13，修改下面的宏定义：
 *          #define LED_PORT   GPIOx
 *          #define LED_PIN    GPIO_Pin_x
 *          如果你的 LED 是高电平点亮（和核心板相反），修改：
 *          LED_ON 和 LED_OFF 的 Reset/Set 互换即可
 */
#ifndef __LED_H
#define __LED_H

#include "stm32f10x.h"

/* ===== 引脚配置（按需修改）===== */
#define LED_PORT        GPIOC           /* LED 所在端口 */
#define LED_PIN         GPIO_Pin_13     /* LED 所在引脚（PC13 = 核心板 LED）*/
#define LED_CLK_CMD     RCC_APB2Periph_GPIOC  /* 对应的时钟使能宏 */

/* ===== 操作宏 ===== */
/* 注意：核心板 LED 是低电平点亮！如果你的 LED 是高电平亮，交换 Set/Reset */
#define LED_ON()        GPIO_ResetBits(LED_PORT, LED_PIN)   /* 点亮 LED */
#define LED_OFF()       GPIO_SetBits(LED_PORT, LED_PIN)     /* 熄灭 LED */
#define LED_Toggle()    GPIO_WriteBit(LED_PORT, LED_PIN, \
                         (BitAction)(1 - GPIO_ReadOutputDataBit(LED_PORT, LED_PIN)))

void LED_Init(void);

#endif
```

**`Hardware/led/led.c`**：

```c
/**
 * @file    led.c
 * @brief   LED 初始化与基本操作
 */
#include "led.h"

/**
 * @brief  初始化 LED 引脚
 * @note   PC13 配置为推挽输出，初始熄灭
 */
void LED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 第1步：开 GPIO 时钟（GPIO 永远在 APB2）*/
    RCC_APB2PeriphClockCmd(LED_CLK_CMD, ENABLE);

    /* 第2步：配置引脚为推挽输出 */
    GPIO_InitStructure.GPIO_Pin = LED_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;     /* 推挽输出 */
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;    /* 最高速度 */
    GPIO_Init(LED_PORT, &GPIO_InitStructure);

    /* 第3步：默认熄灭 */
    LED_OFF();
}
```

### 26.1.2 按键 `Hardware/key/key.h` 和 `Hardware/key/key.c`

**`Hardware/key/key.h`**：

```c
/**
 * @file    key.h
 * @brief   按键扫描模块（支持单击 / 双击 / 长按检测）
 * @note    按键一端接 GPIO，一端接 GND，配置为上拉输入
 *          未按下 = 高电平（上拉），按下 = 低电平
 *          
 *          【电赛修改指南】
 *          1. 修改 KEY_PORT / KEY_PIN 为你的按键引脚
 *          2. 如果有多个按键，复制引脚定义并增加对应的扫描变量
 *          3. 双击时间窗口 DOUBLE_CLICK_WINDOW 和长按时间 LONG_PRESS_TIME
 *             根据实际需要调整（单位：毫秒）
 *          4. 此模板只实现了单个按键，多按键请仿照扩展
 */
#ifndef __KEY_H
#define __KEY_H

#include "stm32f10x.h"

/* ===== 引脚配置（按需修改）===== */
#define KEY_PORT        GPIOA           /* 按键所在端口 */
#define KEY_PIN         GPIO_Pin_0      /* 按键所在引脚（PA0）*/
#define KEY_CLK_CMD     RCC_APB2Periph_GPIOA

/* ===== 按键事件类型 ===== */
typedef enum {
    KEY_EVENT_NONE      = 0,    /* 无事件 */
    KEY_EVENT_CLICK     = 1,    /* 单击 */
    KEY_EVENT_DOUBLE    = 2,    /* 双击 */
    KEY_EVENT_LONG      = 3,    /* 长按（持续按住超过阈值）*/
    KEY_EVENT_LONG_HOLD = 4,    /* 长按持续中（用于连续调节）*/
} KeyEvent_t;

/* ===== 时间阈值（单位：ms，按需修改）===== */
#define KEY_DEBOUNCE_MS        20    /* 消抖时间 */
#define KEY_DOUBLE_CLICK_WINDOW 400  /* 双击判定窗口（两次按下间隔）*/
#define KEY_LONG_PRESS_TIME     1000 /* 长按判定时间（按住超过此值算长按）*/
#define KEY_LONG_HOLD_PERIOD    200  /* 长按持续触发周期 */

void KEY_Init(void);
KeyEvent_t KEY_Scan(void);

#endif
```

**`Hardware/key/key.c`**：

```c
/**
 * @file    key.c
 * @brief   按键扫描实现
 * @note    核心逻辑：
 *          1. 每 10~20ms 调用一次 KEY_Scan()
 *          2. 用状态机跟踪按键状态：空闲 → 消抖 → 按下 → 等待双击 → 释放
 *          3. 返回事件类型，调用方根据事件类型做不同处理
 *          
 *          【电赛修改指南】
 *          KEY_Scan 函数需要在主循环中每 10~20ms 调用一次。
 *          建议放在 main.c 的 10ms 或 20ms 任务中：
 *            if(sys_time - t_key >= 20) {
 *                t_key = sys_time;
 *                KeyEvent_t ev = KEY_Scan();
 *                if(ev) Key_EventHandler(ev);
 *            }
 */
#include "key.h"
#include "sys_tick.h"   /* 需要 sys_time 做时间判断 */
#include "delay.h"      /* 需要 Delay_ms 做消抖 */

/* ===== 按键状态机定义 ===== */
typedef enum {
    KS_IDLE = 0,        /* 空闲：等待按下 */
    KS_DEBOUNCE,        /* 消抖：确认按下 */
    KS_PRESSED,         /* 已按下：等待释放或双击或长按 */
    KS_WAIT_DOUBLE,     /* 等待双击：已释放，看是否短时间内再次按下 */
} KeyState_t;

static KeyState_t key_state = KS_IDLE;
static uint32_t key_press_time = 0;     /* 按下的时刻 */
static uint32_t key_release_time = 0;   /* 释放的时刻 */
static uint8_t  key_click_count = 0;    /* 连击计数 */

/**
 * @brief  按键初始化
 */
void KEY_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(KEY_CLK_CMD, ENABLE);

    GPIO_InitStructure.GPIO_Pin = KEY_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  /* 上拉输入 */
    GPIO_Init(KEY_PORT, &GPIO_InitStructure);
}

/**
 * @brief  读取按键当前电平（1=未按下，0=按下）
 */
static uint8_t KEY_Read(void)
{
    return GPIO_ReadInputDataBit(KEY_PORT, KEY_PIN);
}

/**
 * @brief  按键扫描（需每 10~20ms 调用一次）
 * @return 按键事件类型
 */
KeyEvent_t KEY_Scan(void)
{
    uint8_t key_down = (KEY_Read() == Bit_RESET);  /* RESET(0)=按下 */

    switch(key_state)
    {
        case KS_IDLE:
            if(key_down)
            {
                key_state = KS_DEBOUNCE;
                key_press_time = sys_time;
            }
            break;

        case KS_DEBOUNCE:
            /* 等待消抖时间过后再确认 */
            if(sys_time - key_press_time >= KEY_DEBOUNCE_MS)
            {
                if(KEY_Read() == Bit_RESET)
                {
                    /* 确认按下 */
                    key_state = KS_PRESSED;
                    key_click_count++;
                }
                else
                {
                    /* 抖动干扰，回空闲 */
                    key_state = KS_IDLE;
                }
            }
            break;

        case KS_PRESSED:
            if(!key_down)
            {
                /* 按键释放 */
                key_release_time = sys_time;
                key_state = KS_WAIT_DOUBLE;
            }
            else if(sys_time - key_press_time >= KEY_LONG_PRESS_TIME)
            {
                /* 长按判定 */
                key_press_time = sys_time;  /* 重置时间，用于长按持续触发 */
                key_click_count = 0;
                return KEY_EVENT_LONG;
            }
            break;

        case KS_WAIT_DOUBLE:
            if(key_down)
            {
                /* 双击窗口内再次按下 → 双击 */
                if(sys_time - key_release_time <= KEY_DOUBLE_CLICK_WINDOW)
                {
                    key_state = KS_DEBOUNCE;
                    key_press_time = sys_time;
                    /* 不在这里返回，等 DEBOUNCE 确认后再返回 */
                    key_click_count = 2; /* 标记为双击 */
                }
                else
                {
                    /* 超时了，当作新的单击 */
                    key_state = KS_DEBOUNCE;
                    key_press_time = sys_time;
                    key_click_count = 1;
                }
            }
            else if(sys_time - key_release_time > KEY_DOUBLE_CLICK_WINDOW)
            {
                /* 双击窗口已过，确认单击 */
                key_state = KS_IDLE;
                if(key_click_count == 2)
                {
                    key_click_count = 0;
                    return KEY_EVENT_DOUBLE;
                }
                else
                {
                    key_click_count = 0;
                    return KEY_EVENT_CLICK;
                }
            }
            break;

        default:
            key_state = KS_IDLE;
            break;
    }

    /* 长按持续触发（独立于状态机）*/
    if(key_state == KS_PRESSED && key_down)
    {
        if(sys_time - key_press_time >= KEY_LONG_HOLD_PERIOD)
        {
            key_press_time = sys_time;
            return KEY_EVENT_LONG_HOLD;
        }
    }

    return KEY_EVENT_NONE;
}
```

### 26.1.3 蜂鸣器 `Hardware/buzzer/buzzer.h` 和 `Hardware/buzzer/buzzer.c`

**`Hardware/buzzer/buzzer.h`**：

```c
/**
 * @file    buzzer.h
 * @brief   蜂鸣器控制模块（有源蜂鸣器 / 无源蜂鸣器 PWM 音调）
 * @note    有源蜂鸣器：通电就响（内置振荡电路），用 GPIO 高/低控制
 *          无源蜂鸣器：需要 PWM 方波才能发声（可调音调），用定时器 PWM
 *          本模板支持两种模式，通过宏 BUZZER_ACTIVE 切换
 *          
 *          【电赛修改指南】
 *          1. 有源蜂鸣器：BUZZER_MODE = 0，改 BUZZER_PORT/PIN
 *          2. 无源蜂鸣器：BUZZER_MODE = 1，需要配一个定时器 PWM 通道
 *          3. 蜂鸣器不能直接用 STM32 引脚驱动（电流不够），
 *             需要用三极管（S8050 NPN）放大：GPIO→1kΩ基极电阻→三极管B极，
 *             蜂鸣器正极接 VCC，负极接三极管 C 极，三极管 E 极接 GND。
 */
#ifndef __BUZZER_H
#define __BUZZER_H

#include "stm32f10x.h"

/* ===== 模式选择 ===== */
#define BUZZER_MODE     0    /* 0=有源蜂鸣器(GPIO), 1=无源蜂鸣器(PWM) */

/* ===== 有源蜂鸣器配置（BUZZER_MODE=0 时有效）===== */
#define BUZZER_PORT     GPIOB
#define BUZZER_PIN      GPIO_Pin_8
#define BUZZER_CLK      RCC_APB2Periph_GPIOB
#define BUZZER_ON()     GPIO_ResetBits(BUZZER_PORT, BUZZER_PIN)  /* 低电平响 */
#define BUZZER_OFF()    GPIO_SetBits(BUZZER_PORT, BUZZER_PIN)

void Buzzer_Init(void);
void Buzzer_Beep(uint16_t ms);      /* 短促蜂鸣（ms 毫秒）*/
void Buzzer_BeepN(uint8_t n, uint16_t on_ms, uint16_t off_ms); /* 多次蜂鸣 */

#endif
```

**`Hardware/buzzer/buzzer.c`**：

```c
/**
 * @file    buzzer.c
 * @brief   蜂鸣器控制实现
 */
#include "buzzer.h"
#include "delay.h"

#if BUZZER_MODE == 0
/* ===== 有源蜂鸣器模式 ===== */

void Buzzer_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(BUZZER_CLK, ENABLE);

    GPIO_InitStructure.GPIO_Pin = BUZZER_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BUZZER_PORT, &GPIO_InitStructure);

    BUZZER_OFF();  /* 默认不响 */
}

/**
 * @brief  蜂鸣器响指定时长（阻塞式）
 * @param  ms: 响声时长（毫秒），0 表示一直响（需手动调用 BUZZER_OFF 关）
 */
void Buzzer_Beep(uint16_t ms)
{
    BUZZER_ON();
    if(ms > 0)
    {
        Delay_ms(ms);
        BUZZER_OFF();
    }
}

/**
 * @brief  蜂鸣器多次短促鸣叫
 * @param  n: 鸣叫次数
 * @param  on_ms: 每次鸣叫时长（ms）
 * @param  off_ms: 每次间隔时长（ms）
 * @note   阻塞式！调用时程序会停在函数里直到鸣叫完毕。
 *         如果不想阻塞，请自行用状态机实现非阻塞蜂鸣。
 */
void Buzzer_BeepN(uint8_t n, uint16_t on_ms, uint16_t off_ms)
{
    for(uint8_t i = 0; i < n; i++)
    {
        BUZZER_ON();
        Delay_ms(on_ms);
        BUZZER_OFF();
        if(i < n - 1) Delay_ms(off_ms);
    }
}

#else
/* ===== 无源蜂鸣器模式（TODO：需要配置定时器 PWM 输出）===== */
/* 这里只给出接口框架，具体 PWM 配置请参考 motor_dc.c 的 PWM 部分 */
void Buzzer_Init(void) { /* 配置 TIMx_CHy PWM */ }
void Buzzer_Beep(uint16_t ms) { /* 使能 PWM，延时，关闭 */ }
void Buzzer_BeepN(uint8_t n, uint16_t on_ms, uint16_t off_ms) { }
#endif
```

## 26.2 通信接口模板 —— USART / I2C / SPI

### 26.2.1 串口 `Hardware/usart/usart.h` 和 `Hardware/usart/usart.c`

**`Hardware/usart/usart.h`**：

```c
/**
 * @file    usart.h
 * @brief   USART1 串口模块（printf 重定向 + 中断接收 + DMA 收发）
 * @note    使用 USART1（PA9=TX, PA10=RX），挂在 APB2 总线
 *          支持：printf 重定向、中断接收、DMA 发送、DMA+IDLE 不定长接收
 *          
 *          【电赛修改指南】
 *          1. 改波特率：调用 USART1_Init(你的波特率)
 *          2. 改串口号：如果要换 USART2（PA2=TX,PA3=RX），
 *             修改 GPIO 配置、RCC 时钟（USART2 在 APB1）、中断名
 *          3. 接收缓冲区大小：修改 RX_BUF_SIZE（默认 256）
 *          4. 如果用 DMA 接收：调用 USART1_DMA_RX_Init()
 *          5. 如果用 DMA 发送：调用 USART1_DMA_TX_Init()
 */
#ifndef __USART_H
#define __USART_H

#include "stm32f10x.h"
#include <stdio.h>

/* ===== 缓冲区大小 ===== */
#define RX_BUF_SIZE     256    /* 接收缓冲区大小（按需调整）*/

/* ===== 接收状态标志（volatile：中断中修改）===== */
extern volatile uint8_t  usart_rx_buf[RX_BUF_SIZE];  /* 接收缓冲区 */
extern volatile uint8_t  usart_rx_flag;  /* 接收完成标志（1=有新数据）*/
extern volatile uint16_t usart_rx_len;  /* 本次接收的长度 */

void USART1_Init(uint32_t baudrate);
void USART1_SendByte(uint8_t dat);
void USART1_SendString(char* str);
void USART1_SendBuf(uint8_t* buf, uint16_t len);

/* DMA 相关（可选）*/
void USART1_DMA_TX_Init(void);
void USART1_DMA_TX_Send(uint8_t* buf, uint16_t len);
uint8_t USART1_DMA_TX_Done(void);
void USART1_DMA_RX_IDLE_Init(void);

#endif
```

**`Hardware/usart/usart.c`**：

```c
/**
 * @file    usart.c
 * @brief   USART1 串口模块实现
 * @note    【中断函数名位置】USART1_IRQHandler 必须写在 stm32f10x_it.c 中！
 *          本文件不包含中断函数，请将中断函数模板复制到 stm32f10x_it.c。
 *          
 *          【电赛修改指南：使用 USART2/3 替代 USART1】
 *          1. 全局替换 USART1 → USART2 (或 USART3)
 *          2. 修改引脚配置：USART2=PA2(TX)/PA3(RX)，USART3=PB10(TX)/PB11(RX)
 *          3. 修改时钟使能：RCC_APB1Periph_USARTx（注意是 APB1！）
 *          4. 修改中断名：USART2_IRQn 或 USART3_IRQn
 *          5. 修改 DMA 通道：USART2_TX=Ch7, USART2_RX=Ch6
 */
#include "usart.h"

/* ===== 全局变量 ===== */
volatile uint8_t  usart_rx_buf[RX_BUF_SIZE];
volatile uint8_t  usart_rx_flag = 0;
volatile uint16_t usart_rx_len = 0;

/**
 * @brief  USART1 初始化（中断接收模式）
 * @param  baudrate: 波特率（常用 9600 / 115200 / 38400）
 */
void USART1_Init(uint32_t baudrate)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef  NVIC_InitStructure;

    /* ----- 开时钟：USART1 在 APB2，GPIOA 在 APB2 ----- */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);
    /*【修改USART2/3】改为 RCC_APB2Periph_GPIOx（GPIO在APB2）
                     + RCC_APB1Periph_USARTx（USART2/3在APB1）*/

    /* ----- PA9 = TX（复用推挽输出）----- */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9;      /* USART1_TX */
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;  /* 复用推挽！不是Out_PP */
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* ----- PA10 = RX（浮空输入）----- */
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_10;       /* USART1_RX */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; /* 浮空输入 */
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* ----- USART 参数配置 ----- */
    USART_InitStructure.USART_BaudRate            = baudrate;
    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
    USART_InitStructure.USART_Parity              = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode                = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART1, &USART_InitStructure);

    /* ----- 使能接收中断（RXNE）----- */
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    /* ----- 配置 NVIC ----- */
    NVIC_InitStructure.NVIC_IRQChannel    = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3; /* 优先级较低 */
    NVIC_InitStructure.NVIC_IRQChannelSubPriority       = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd               = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* ----- 使能串口 ----- */
    USART_Cmd(USART1, ENABLE);
}

/**
 * @brief  发送一个字节
 */
void USART1_SendByte(uint8_t dat)
{
    USART_SendData(USART1, dat);
    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET); /* 等发送完成 */
}

/**
 * @brief  发送字符串（以 '\0' 结尾）
 */
void USART1_SendString(char* str)
{
    while(*str)
        USART1_SendByte(*str++);
}

/**
 * @brief  发送指定长度的数据
 */
void USART1_SendBuf(uint8_t* buf, uint16_t len)
{
    for(uint16_t i = 0; i < len; i++)
        USART1_SendByte(buf[i]);
}

/* ===== printf 重定向 ===== */
/* 在 Keil 中需勾选 "Use MicroLIB"（Target → Code Generation 选项卡）*/
int fputc(int ch, FILE* f)
{
    USART1_SendByte((uint8_t)ch);
    return ch;
}

/* ===== DMA 发送（可选功能）===== */
/* 使用前需要在 stm32f10x_conf.h 中取消注释 stm32f10x_dma.h */
#ifdef USE_DMA_TX
static uint8_t dma_tx_busy = 0;  /* DMA 发送忙标志 */

void USART1_DMA_TX_Init(void)
{
    DMA_InitTypeDef DMA_InitStructure;
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);  /* DMA 在 AHB */

    DMA_DeInit(DMA1_Channel4);  /* USART1_TX = DMA1_Channel4 */
    /*【修改USART2】Ch7,【修改USART3】Ch2 */

    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&USART1->DR);
    DMA_InitStructure.DMA_MemoryBaseAddr     = (uint32_t)0; /* 占位，发送时动态设置 */
    DMA_InitStructure.DMA_DIR            = DMA_DIR_PeripheralDST;  /* 内存→外设 */
    DMA_InitStructure.DMA_BufferSize     = 0;
    DMA_InitStructure.DMA_PeripheralInc  = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc      = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode  = DMA_Mode_Normal;      /* 单次模式 */
    DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel4, &DMA_InitStructure);

    USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);  /* 使能 USART TX DMA 请求 */
}

void USART1_DMA_TX_Send(uint8_t* buf, uint16_t len)
{
    while(dma_tx_busy);  /* 等上一次发送完成 */
    dma_tx_busy = 1;

    DMA_Cmd(DMA1_Channel4, DISABLE);
    DMA_SetCurrDataCounter(DMA1_Channel4, len);
    DMA1_Channel4->CMAR = (uint32_t)buf;
    DMA_ClearFlag(DMA1_FLAG_TC4);   /* 清除传输完成标志 */
    DMA_Cmd(DMA1_Channel4, ENABLE);
}

uint8_t USART1_DMA_TX_Done(void)
{
    if(DMA_GetFlagStatus(DMA1_FLAG_TC4) != RESET)
    {
        DMA_ClearFlag(DMA1_FLAG_TC4);
        dma_tx_busy = 0;
        return 1;
    }
    return 0;
}
#endif /* USE_DMA_TX */

#ifdef USE_DMA_RX_IDLE
/* ===== DMA + IDLE 不定长接收（可选功能）===== */
/* 在 stm32f10x_it.c 的 USART1_IRQHandler 中添加 IDLE 中断处理 */
/* 参考 main.c 模板 26.0.4 中的 USART1_IRQHandler 注释 */

void USART1_DMA_RX_IDLE_Init(void)
{
    DMA_InitTypeDef DMA_InitStructure;

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    DMA_DeInit(DMA1_Channel5);  /* USART1_RX = DMA1_Channel5 */
    /*【修改USART2】Ch6,【修改USART3】Ch3 */

    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&USART1->DR);
    DMA_InitStructure.DMA_MemoryBaseAddr     = (uint32_t)usart_rx_buf;
    DMA_InitStructure.DMA_DIR            = DMA_DIR_PeripheralSRC;  /* 外设→内存 */
    DMA_InitStructure.DMA_BufferSize     = RX_BUF_SIZE;
    DMA_InitStructure.DMA_PeripheralInc  = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc      = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode  = DMA_Mode_Circular;     /* 循环模式！*/
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel5, &DMA_InitStructure);

    USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);
    USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);  /* 使能 IDLE 中断 */
    DMA_Cmd(DMA1_Channel5, ENABLE);
}
#endif /* USE_DMA_RX_IDLE */
```

**中断处理代码**（追加到 `User/stm32f10x_it.c` 中）：

```c
/* ===== USART1 中断服务函数（放在 stm32f10x_it.c）===== */
void USART1_IRQHandler(void)
{
    static uint8_t rx_cnt = 0;  /* 接收计数（静态变量保持）*/

    /* ----- RXNE：收到一个字节 ----- */
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        uint8_t ch = USART_ReceiveData(USART1);

        /* 存入缓冲区（防溢出）*/
        if(rx_cnt < RX_BUF_SIZE - 1)
        {
            usart_rx_buf[rx_cnt++] = ch;
        }

        /* 帧结束判断（以换行符 '\n' 为一帧结束）*/
        /*【电赛修改】如果你的通信协议用别的帧尾（如0x0D0x0A, 固定长度等），改这里 */
        if(ch == '\n')
        {
            usart_rx_buf[rx_cnt] = '\0';  /* 添加字符串结束符 */
            usart_rx_flag = 1;            /* 标记有新数据 */
            usart_rx_len  = rx_cnt;       /* 记录本帧长度 */
            rx_cnt = 0;                   /* 重置计数 */
        }

        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }

    /* ----- IDLE：总线空闲（如果用 DMA+IDLE 接收）----- */
    if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)
    {
        /* 清 IDLE 标志：必须先读 SR 再读 DR */
        volatile uint32_t tmp = USART1->SR;
        tmp = USART1->DR;
        (void)tmp;

        /* 计算接收长度：缓冲区大小 - DMA 剩余计数 = 已收到字节数 */
        usart_rx_len = RX_BUF_SIZE - DMA_GetCurrDataCounter(DMA1_Channel5);
        /*【修改USART2/3】改 DMA 通道 */
        usart_rx_flag = 1;

        /* 重置 DMA（准备接收下一帧）*/
        DMA_Cmd(DMA1_Channel5, DISABLE);
        DMA_SetCurrDataCounter(DMA1_Channel5, RX_BUF_SIZE);
        DMA_Cmd(DMA1_Channel5, ENABLE);
    }
}
```

### 26.2.2 软件 I2C `Hardware/i2c/i2c_soft.h` 和 `Hardware/i2c/i2c_soft.c`

**`Hardware/i2c/i2c_soft.h`**：

```c
/**
 * @file    i2c_soft.h
 * @brief   软件模拟 I2C 主机驱动
 * @note    使用任意两个 GPIO 模拟 I2C 时序，无需硬件 I2C 外设
 *          支持 100kHz 标准模式（可通过延时调整速度）
 *          比硬件 I2C 稳定、灵活、方便调试
 *          
 *          【电赛修改指南】
 *          1. 修改 I2C_SCL_PIN / I2C_SDA_PIN 为你的实际引脚
 *          2. 如果通信失败，增大 I2C_DELAY_US 的值（默认 5us）
 *          3. I2C 引脚必须外接 4.7kΩ 上拉电阻到 3.3V！
 *             如果忘了接上拉，通信必然失败。
 *          4. 如果要同时驱动多个 I2C 设备，引脚不需额外配置，
 *             所有 I2C 设备并联在同一组 SCL/SDA 上即可（地址必须不同）
 */
#ifndef __I2C_SOFT_H
#define __I2C_SOFT_H

#include "stm32f10x.h"

/* ===== 引脚配置（按实际硬件修改）===== */
#define I2C_SCL_GPIO    GPIOB           /* SCL 时钟线端口 */
#define I2C_SCL_PIN     GPIO_Pin_6      /* SCL 时钟线引脚 */
#define I2C_SDA_GPIO    GPIOB           /* SDA 数据线端口 */
#define I2C_SDA_PIN     GPIO_Pin_7      /* SDA 数据线引脚 */
#define I2C_CLK_CMD     RCC_APB2Periph_GPIOB  /* 时钟使能 */

/* ===== IO 操作宏 ===== */
#define I2C_SCL_H()     GPIO_SetBits(I2C_SCL_GPIO, I2C_SCL_PIN)
#define I2C_SCL_L()     GPIO_ResetBits(I2C_SCL_GPIO, I2C_SCL_PIN)
#define I2C_SDA_H()     GPIO_SetBits(I2C_SDA_GPIO, I2C_SDA_PIN)
#define I2C_SDA_L()     GPIO_ResetBits(I2C_SDA_GPIO, I2C_SDA_PIN)
#define I2C_READ_SDA()  GPIO_ReadInputDataBit(I2C_SDA_GPIO, I2C_SDA_PIN)

/* ===== 延时配置 ===== */
#define I2C_DELAY_US    5    /* 半周期延时（us），5us≈100kHz，可适当增大 */

void I2C_Soft_Init(void);
void I2C_Start(void);
void I2C_Stop(void);
void I2C_SendByte(uint8_t dat);
uint8_t I2C_ReadByte(uint8_t ack);  /* ack=1 发ACK, ack=0 发NACK */
uint8_t I2C_WaitAck(void);          /* 返回0=ACK, 1=NACK */
void I2C_Ack(void);
void I2C_NAck(void);

/* 高级封装：读写 I2C 设备的寄存器 */
uint8_t I2C_WriteReg(uint8_t devAddr7, uint8_t regAddr, uint8_t data);
uint8_t I2C_ReadReg(uint8_t devAddr7, uint8_t regAddr);
/* devAddr7 为 7 位设备地址（如 OLED=0x3C, MPU6050=0x68）*/

#endif
```

**`Hardware/i2c/i2c_soft.c`**：

```c
/**
 * @file    i2c_soft.c
 * @brief   软件模拟 I2C 实现
 */
#include "i2c_soft.h"
#include "delay.h"

/**
 * @brief  初始化 I2C 引脚（开漏输出 + 总线释放为高）
 */
void I2C_Soft_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(I2C_CLK_CMD, ENABLE);

    /* SCL 和 SDA 都配为开漏输出（I2C 标准电气接口）*/
    GPIO_InitStructure.GPIO_Pin   = I2C_SCL_PIN | I2C_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_OD;  /* 开漏！不是推挽！*/
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(I2C_SCL_GPIO, &GPIO_InitStructure);

    /* 释放总线（开漏输出+写1=高阻，由上拉电阻拉高）*/
    I2C_SCL_H();
    I2C_SDA_H();
}

/**
 * @brief  I2C 起始信号：SCL 高时 SDA 从高变低
 */
void I2C_Start(void)
{
    I2C_SDA_H();   I2C_SCL_H();
    Delay_us(I2C_DELAY_US);
    I2C_SDA_L();   /* SDA↓ 在 SCL 高期间 = 起始 */
    Delay_us(I2C_DELAY_US);
    I2C_SCL_L();   /* 拉低 SCL，准备发数据 */
}

/**
 * @brief  I2C 停止信号：SCL 高时 SDA 从低变高
 */
void I2C_Stop(void)
{
    I2C_SDA_L();   I2C_SCL_H();
    Delay_us(I2C_DELAY_US);
    I2C_SDA_H();   /* SDA↑ 在 SCL 高期间 = 停止 */
    Delay_us(I2C_DELAY_US);
}

/**
 * @brief  发送一个字节（高位在前 MSB first）
 */
void I2C_SendByte(uint8_t dat)
{
    for(uint8_t i = 0; i < 8; i++)
    {
        I2C_SCL_L();
        Delay_us(I2C_DELAY_US / 2);
        /* 把当前最高位发到 SDA 上 */
        if(dat & 0x80) I2C_SDA_H(); else I2C_SDA_L();
        dat <<= 1;
        Delay_us(I2C_DELAY_US / 2);
        I2C_SCL_H();   /* SCL↑：接收方采样 SDA */
        Delay_us(I2C_DELAY_US);
    }
    I2C_SCL_L();  /* 准备第9个时钟（ACK）*/
}

/**
 * @brief  等待从机应答
 * @return 0=ACK（成功），非0=NACK（失败）
 */
uint8_t I2C_WaitAck(void)
{
    uint8_t ack;

    I2C_SDA_H();   /* 释放 SDA，让从机控制 */
    Delay_us(I2C_DELAY_US / 2);
    I2C_SCL_H();   /* 第9个时钟 */
    Delay_us(I2C_DELAY_US / 2);
    ack = I2C_READ_SDA();  /* 从机拉低=ACK，保持高=NACK */
    I2C_SCL_L();
    Delay_us(I2C_DELAY_US);
    return ack;
}

/**
 * @brief  主机发 ACK（继续读下一个字节）
 */
void I2C_Ack(void)
{
    I2C_SCL_L();
    I2C_SDA_L();   /* 拉低=ACK */
    Delay_us(I2C_DELAY_US / 2);
    I2C_SCL_H();
    Delay_us(I2C_DELAY_US);
    I2C_SCL_L();
}

/**
 * @brief  主机发 NACK（停止读）
 */
void I2C_NAck(void)
{
    I2C_SCL_L();
    I2C_SDA_H();   /* 放开=NACK */
    Delay_us(I2C_DELAY_US / 2);
    I2C_SCL_H();
    Delay_us(I2C_DELAY_US);
    I2C_SCL_L();
}

/**
 * @brief  读取一个字节
 * @param  ack: 1=读完后发ACK(继续读), 0=发NACK(最后一个字节)
 */
uint8_t I2C_ReadByte(uint8_t ack)
{
    uint8_t dat = 0;

    I2C_SDA_H();  /* 释放 SDA，让从机驱动 */
    for(uint8_t i = 0; i < 8; i++)
    {
        I2C_SCL_L();
        Delay_us(I2C_DELAY_US);
        I2C_SCL_H();
        dat <<= 1;
        if(I2C_READ_SDA()) dat++;  /* 读到高电平，最低位+1 */
        Delay_us(I2C_DELAY_US / 2);
    }
    /* 发送 ACK / NACK */
    if(ack) I2C_Ack(); else I2C_NAck();
    return dat;
}

/* ===== 高级封装 ===== */

/**
 * @brief  向 I2C 设备的某个寄存器写 1 字节
 * @param  devAddr7: 7 位设备地址
 * @return 0:成功, 1:NACK失败
 */
uint8_t I2C_WriteReg(uint8_t devAddr7, uint8_t regAddr, uint8_t data)
{
    I2C_Start();
    I2C_SendByte(devAddr7 << 1);           /* 地址+写位 */
    if(I2C_WaitAck()) { I2C_Stop(); return 1; }
    I2C_SendByte(regAddr);
    if(I2C_WaitAck()) { I2C_Stop(); return 1; }
    I2C_SendByte(data);
    if(I2C_WaitAck()) { I2C_Stop(); return 1; }
    I2C_Stop();
    return 0;
}

/**
 * @brief  从 I2C 设备的某个寄存器读 1 字节
 */
uint8_t I2C_ReadReg(uint8_t devAddr7, uint8_t regAddr)
{
    uint8_t val;
    I2C_Start();
    I2C_SendByte(devAddr7 << 1);           /* 地址+写位 */
    I2C_WaitAck();
    I2C_SendByte(regAddr);
    I2C_WaitAck();
    I2C_Start();                           /* 重复起始 */
    I2C_SendByte((devAddr7 << 1) | 0x01);  /* 地址+读位 */
    I2C_WaitAck();
    val = I2C_ReadByte(0);                 /* 读1字节，发NACK */
    I2C_Stop();
    return val;
}
```

### 26.2.3 软件 SPI `Hardware/spi/spi_soft.h` 和 `Hardware/spi/spi_soft.c`

**`Hardware/spi/spi_soft.h`**：

```c
/**
 * @file    spi_soft.h
 * @brief   软件模拟 SPI 主机驱动（模式 0：CPOL=0, CPHA=0）
 * @note    使用任意 GPIO 模拟 SPI 时序，无需硬件 SPI 外设
 *          速度约 1~2MHz（受 GPIO 翻转速度限制）
 *          
 *          【电赛修改指南】
 *          1. 修改引脚宏定义为你的实际引脚
 *          2. 改 SPI 模式：修改 SCK 初始电平和读写时序中的边沿
 *             - 模式0(CPOL=0,CPHA=0): SCK空闲=0, 上升沿采样
 *             - 模式3(CPOL=1,CPHA=1): SCK空闲=1, 上升沿采样
 *          3. CS 引脚通常单独管理，由上层模块（如 NRF24L01）控制
 */
#ifndef __SPI_SOFT_H
#define __SPI_SOFT_H

#include "stm32f10x.h"

/* ===== 引脚配置（按实际硬件修改）===== */
#define SPI_SCK_GPIO    GPIOA
#define SPI_SCK_PIN     GPIO_Pin_5      /* SCK 时钟 */
#define SPI_MOSI_GPIO   GPIOA
#define SPI_MOSI_PIN    GPIO_Pin_7      /* MOSI：主机发→从机收 */
#define SPI_MISO_GPIO   GPIOA
#define SPI_MISO_PIN    GPIO_Pin_6      /* MISO：从机发→主机收 */
#define SPI_CLK_CMD     RCC_APB2Periph_GPIOA

/* ===== IO 操作宏 ===== */
#define SPI_SCK_H()     GPIO_SetBits(SPI_SCK_GPIO, SPI_SCK_PIN)
#define SPI_SCK_L()     GPIO_ResetBits(SPI_SCK_GPIO, SPI_SCK_PIN)
#define SPI_MOSI_H()    GPIO_SetBits(SPI_MOSI_GPIO, SPI_MOSI_PIN)
#define SPI_MOSI_L()    GPIO_ResetBits(SPI_MOSI_GPIO, SPI_MOSI_PIN)
#define SPI_READ_MISO() GPIO_ReadInputDataBit(SPI_MISO_GPIO, SPI_MISO_PIN)

void SPI_Soft_Init(void);
uint8_t SPI_ReadWriteByte(uint8_t dat);  /* 收发合一（全双工）*/

#endif
```

**`Hardware/spi/spi_soft.c`**：

```c
/**
 * @file    spi_soft.c
 * @brief   软件 SPI 实现（模式 0）
 */
#include "spi_soft.h"
#include "delay.h"

void SPI_Soft_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(SPI_CLK_CMD, ENABLE);

    /* SCK 和 MOSI 配为推挽输出（主机驱动）*/
    GPIO_InitStructure.GPIO_Pin   = SPI_SCK_PIN | SPI_MOSI_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SPI_SCK_GPIO, &GPIO_InitStructure);

    /* MISO 配为浮空输入（从机驱动）*/
    GPIO_InitStructure.GPIO_Pin  = SPI_MISO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(SPI_SCK_GPIO, &GPIO_InitStructure);

    /* 空闲状态：SCK=0（模式0）*/
    SPI_SCK_L();
}

/**
 * @brief  SPI 全双工读写一个字节（模式0：CPOL=0, CPHA=0）
 *         发一个字节的同时收到一个字节
 */
uint8_t SPI_ReadWriteByte(uint8_t dat)
{
    uint8_t rx = 0;

    for(uint8_t i = 0; i < 8; i++)
    {
        /* 1. SCK=0 期间：主机把 bit 放到 MOSI 上 */
        SPI_SCK_L();
        Delay_us(1);  /* 等电平稳定 */
        if(dat & 0x80) SPI_MOSI_H(); else SPI_MOSI_L();
        dat <<= 1;

        /* 2. SCK↑（上升沿）：双方采样 */
        SPI_SCK_H();
        Delay_us(1);

        /* 3. 采样 MISO（从机发来的 bit）*/
        rx <<= 1;
        if(SPI_READ_MISO()) rx++;
    }
    /* 8bit 结束，SCK 回到空闲低电平 */
    SPI_SCK_L();
    return rx;
}
```

## 26.3 显示模块模板 —— OLED（SSD1306 I2C）

### 26.3.1 OLED `Hardware/oled/oled_i2c.h` 和 `Hardware/oled/oled_i2c.c`

**`Hardware/oled/oled_i2c.h`**：

```c
/**
 * @file    oled_i2c.h
 * @brief   0.96寸 OLED 显示屏驱动（SSD1306，I2C 接口，128×64 像素）
 * @note    依赖 i2c_soft 模块（必须先调用 I2C_Soft_Init()）
 *          I2C 地址：0x3C（7位），写=0x78
 *          部分模块地址可能是 0x3D（写=0x7A），不亮时改 OLED_ADDR 宏
 *          
 *          【电赛修改指南】
 *          1. 如果 OLED 不亮：先检查 I2C 地址（改为 0x3D 试试）
 *          2. 如果屏幕上下颠倒：改 OLED_Init() 中的 0xC8 为 0xC0
 *          3. 如果屏幕左右镜像：改 OLED_Init() 中的 0xA1 为 0xA0
 *          4. 12号字体（6×8）和 16号字体（8×16）两个尺寸
 *          5. 如果要显示中文：需要用 PCtoLCD2002 软件生成字库数组
 */
#ifndef __OLED_I2C_H
#define __OLED_I2C_H

#include "stm32f10x.h"

/* ===== OLED I2C 地址 ===== */
#define OLED_ADDR   0x3C    /* 7位I2C地址（0x3C=写0x78, 0x3D=写0x7A）*/

/* ===== 屏幕尺寸 ===== */
#define OLED_WIDTH   128
#define OLED_HEIGHT  64

void OLED_Init(void);
void OLED_Clear(void);
void OLED_Fill(uint8_t data);           /* 全屏填充（0x00=全黑, 0xFF=全白）*/
void OLED_SetPos(uint8_t x, uint8_t y); /* 设置光标（x:0~127, y:0~7页）*/
void OLED_ShowChar(uint8_t x, uint8_t y, char ch, uint8_t size);
void OLED_ShowString(uint8_t x, uint8_t y, char* str, uint8_t size);
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size);
void OLED_ShowFloat(uint8_t x, uint8_t y, float num, uint8_t intLen, uint8_t decLen, uint8_t size);
void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t mode); /* mode:1=亮,0=灭 */
void OLED_Refresh(void);                /* 刷新帧缓冲到屏幕 */

#endif
```

**`Hardware/oled/oled_i2c.c`**：

```c
/**
 * @file    oled_i2c.c
 * @brief   OLED SSD1306 I2C 驱动实现（带帧缓冲）
 * @note    帧缓冲占用 128×64/8 = 1024 字节 RAM（C8T6 有 20KB，完全够）
 */
#include "oled_i2c.h"
#include "i2c_soft.h"
#include "delay.h"
#include <string.h>
#include <stdio.h>

/* ===== 帧缓冲区（1024 字节 = 8页 × 128列）===== */
static uint8_t OLED_Buf[8][128];

/* ===== 内部函数声明 ===== */
static void OLED_WriteCmd(uint8_t cmd);
static void OLED_WriteData(uint8_t dat);

/* ===== 6×8 英文字库（ASCII 32~126，每个字符6字节，逐列式）===== */
/*【电赛修改】如果不想把字库放这里，可以新建 oledfont.h 用 #include 引入 */
static const uint8_t F6x8[][6] = {
    {0x00,0x00,0x00,0x00,0x00,0x00}, /* space */
    {0x00,0x00,0x5F,0x00,0x00,0x00}, /* ! */
    {0x00,0x07,0x00,0x07,0x00,0x00}, /* " */
    {0x14,0x7F,0x14,0x7F,0x14,0x00}, /* # */
    {0x24,0x2A,0x7F,0x2A,0x12,0x00}, /* $ */
    {0x23,0x13,0x08,0x64,0x62,0x00}, /* % */
    {0x36,0x49,0x55,0x22,0x50,0x00}, /* & */
    {0x00,0x05,0x03,0x00,0x00,0x00}, /* ' */
    {0x00,0x1C,0x22,0x41,0x00,0x00}, /* ( */
    {0x00,0x41,0x22,0x1C,0x00,0x00}, /* ) */
    {0x08,0x2A,0x1C,0x2A,0x08,0x00}, /* * */
    {0x08,0x08,0x3E,0x08,0x08,0x00}, /* + */
    {0x00,0x50,0x30,0x00,0x00,0x00}, /* , */
    {0x08,0x08,0x08,0x08,0x08,0x00}, /* - */
    {0x00,0x60,0x60,0x00,0x00,0x00}, /* . */
    {0x20,0x10,0x08,0x04,0x02,0x00}, /* / */
    {0x3E,0x51,0x49,0x45,0x3E,0x00}, /* 0 */
    {0x00,0x42,0x7F,0x40,0x00,0x00}, /* 1 */
    {0x42,0x61,0x51,0x49,0x46,0x00}, /* 2 */
    {0x21,0x41,0x45,0x4B,0x31,0x00}, /* 3 */
    {0x18,0x14,0x12,0x7F,0x10,0x00}, /* 4 */
    {0x27,0x45,0x45,0x45,0x39,0x00}, /* 5 */
    {0x3C,0x4A,0x49,0x49,0x30,0x00}, /* 6 */
    {0x01,0x71,0x09,0x05,0x03,0x00}, /* 7 */
    {0x36,0x49,0x49,0x49,0x36,0x00}, /* 8 */
    {0x06,0x49,0x49,0x29,0x1E,0x00}, /* 9 */
    {0x00,0x36,0x36,0x00,0x00,0x00}, /* : */
    {0x00,0x56,0x36,0x00,0x00,0x00}, /* ; */
    {0x00,0x08,0x14,0x22,0x41,0x00}, /* < */
    {0x14,0x14,0x14,0x14,0x14,0x00}, /* = */
    {0x41,0x22,0x14,0x08,0x00,0x00}, /* > */
    {0x02,0x01,0x51,0x09,0x06,0x00}, /* ? */
    {0x32,0x49,0x79,0x41,0x3E,0x00}, /* @ */
    {0x7E,0x11,0x11,0x11,0x7E,0x00}, /* A */
    {0x7F,0x49,0x49,0x49,0x36,0x00}, /* B */
    {0x3E,0x41,0x41,0x41,0x22,0x00}, /* C */
    {0x7F,0x41,0x41,0x22,0x1C,0x00}, /* D */
    {0x7F,0x49,0x49,0x49,0x41,0x00}, /* E */
    {0x7F,0x09,0x09,0x01,0x01,0x00}, /* F */
    {0x3E,0x41,0x41,0x51,0x32,0x00}, /* G */
    {0x7F,0x08,0x08,0x08,0x7F,0x00}, /* H */
    {0x00,0x41,0x7F,0x41,0x00,0x00}, /* I */
    {0x20,0x40,0x41,0x3F,0x01,0x00}, /* J */
    {0x7F,0x08,0x14,0x22,0x41,0x00}, /* K */
    {0x7F,0x40,0x40,0x40,0x40,0x00}, /* L */
    {0x7F,0x02,0x04,0x02,0x7F,0x00}, /* M */
    {0x7F,0x04,0x08,0x10,0x7F,0x00}, /* N */
    {0x3E,0x41,0x41,0x41,0x3E,0x00}, /* O */
    {0x7F,0x09,0x09,0x09,0x06,0x00}, /* P */
    {0x3E,0x41,0x51,0x21,0x5E,0x00}, /* Q */
    {0x7F,0x09,0x19,0x29,0x46,0x00}, /* R */
    {0x46,0x49,0x49,0x49,0x31,0x00}, /* S */
    {0x01,0x01,0x7F,0x01,0x01,0x00}, /* T */
    {0x3F,0x40,0x40,0x40,0x3F,0x00}, /* U */
    {0x1F,0x20,0x40,0x20,0x1F,0x00}, /* V */
    {0x7F,0x20,0x18,0x20,0x7F,0x00}, /* W */
    {0x63,0x14,0x08,0x14,0x63,0x00}, /* X */
    {0x03,0x04,0x78,0x04,0x03,0x00}, /* Y */
    {0x61,0x51,0x49,0x45,0x43,0x00}, /* Z */
    {0x00,0x00,0x7F,0x41,0x41,0x00}, /* [ */
    {0x02,0x04,0x08,0x10,0x20,0x00}, /* \ */
    {0x41,0x41,0x7F,0x00,0x00,0x00}, /* ] */
    {0x04,0x02,0x01,0x02,0x04,0x00}, /* ^ */
    {0x40,0x40,0x40,0x40,0x40,0x00}, /* _ */
    {0x00,0x01,0x02,0x04,0x00,0x00}, /* ` */
    {0x20,0x54,0x54,0x54,0x78,0x00}, /* a */
    {0x7F,0x48,0x44,0x44,0x38,0x00}, /* b */
    {0x38,0x44,0x44,0x44,0x20,0x00}, /* c */
    {0x38,0x44,0x44,0x48,0x7F,0x00}, /* d */
    {0x38,0x54,0x54,0x54,0x18,0x00}, /* e */
    {0x08,0x7E,0x09,0x01,0x02,0x00}, /* f */
    {0x08,0x14,0x54,0x54,0x3C,0x00}, /* g */
    {0x7F,0x08,0x04,0x04,0x78,0x00}, /* h */
    {0x00,0x44,0x7D,0x40,0x00,0x00}, /* i */
    {0x20,0x40,0x44,0x3D,0x00,0x00}, /* j */
    {0x00,0x7F,0x10,0x28,0x44,0x00}, /* k */
    {0x00,0x41,0x7F,0x40,0x00,0x00}, /* l */
    {0x7C,0x04,0x18,0x04,0x78,0x00}, /* m */
    {0x7C,0x08,0x04,0x04,0x78,0x00}, /* n */
    {0x38,0x44,0x44,0x44,0x38,0x00}, /* o */
    {0x7C,0x14,0x14,0x14,0x08,0x00}, /* p */
    {0x08,0x14,0x14,0x18,0x7C,0x00}, /* q */
    {0x7C,0x08,0x04,0x04,0x08,0x00}, /* r */
    {0x48,0x54,0x54,0x54,0x20,0x00}, /* s */
    {0x04,0x3F,0x44,0x40,0x20,0x00}, /* t */
    {0x3C,0x40,0x40,0x20,0x7C,0x00}, /* u */
    {0x1C,0x20,0x40,0x20,0x1C,0x00}, /* v */
    {0x3C,0x40,0x30,0x40,0x3C,0x00}, /* w */
    {0x44,0x28,0x10,0x28,0x44,0x00}, /* x */
    {0x0C,0x50,0x50,0x50,0x3C,0x00}, /* y */
    {0x44,0x64,0x54,0x4C,0x44,0x00}, /* z */
    {0x00,0x08,0x36,0x41,0x00,0x00}, /* { */
    {0x00,0x00,0x7F,0x00,0x00,0x00}, /* | */
    {0x00,0x41,0x36,0x08,0x00,0x00}, /* } */
    {0x08,0x04,0x08,0x10,0x08,0x00}, /* ~ */
};

/* ===== 底层 I2C 通信 ===== */

static void OLED_WriteCmd(uint8_t cmd)
{
    I2C_Start();
    I2C_SendByte(OLED_ADDR << 1);      /* 地址+写位 */
    I2C_WaitAck();
    I2C_SendByte(0x00);                 /* 控制字节：0x00=命令 */
    I2C_WaitAck();
    I2C_SendByte(cmd);
    I2C_WaitAck();
    I2C_Stop();
}

static void OLED_WriteData(uint8_t dat)
{
    I2C_Start();
    I2C_SendByte(OLED_ADDR << 1);
    I2C_WaitAck();
    I2C_SendByte(0x40);                 /* 控制字节：0x40=数据 */
    I2C_WaitAck();
    I2C_SendByte(dat);
    I2C_WaitAck();
    I2C_Stop();
}

/* ===== 初始化 ===== */

void OLED_Init(void)
{
    Delay_ms(200);  /* 等 SSD1306 上电稳定（至少100ms）*/

    OLED_WriteCmd(0xAE); /* Display OFF */
    OLED_WriteCmd(0x00); /* Set Low Column = 0 */
    OLED_WriteCmd(0x10); /* Set High Column = 0 */
    OLED_WriteCmd(0x40); /* Set Start Line = 0 */
    OLED_WriteCmd(0x81); /* Set Contrast */
    OLED_WriteCmd(0xCF); /* Contrast = 207 */
    OLED_WriteCmd(0xA1); /* Segment Remap（左右镜像：0xA0=正常, 0xA1=镜像）*/
    OLED_WriteCmd(0xC8); /* COM Scan Direction（上下翻转：0xC0=正常, 0xC8=翻转）*/
    OLED_WriteCmd(0xA6); /* Normal Display（0xA7=反色）*/
    OLED_WriteCmd(0xA8); /* Multiplex Ratio */
    OLED_WriteCmd(0x3F); /* Ratio = 64 */
    OLED_WriteCmd(0xD3); /* Display Offset */
    OLED_WriteCmd(0x00); /* Offset = 0 */
    OLED_WriteCmd(0xD5); /* Display Clock Divide */
    OLED_WriteCmd(0x80);
    OLED_WriteCmd(0xD9); /* Pre-charge Period */
    OLED_WriteCmd(0xF1);
    OLED_WriteCmd(0xDA); /* COM Pins Hardware Config */
    OLED_WriteCmd(0x12);
    OLED_WriteCmd(0xDB); /* VCOMH Deselect Level */
    OLED_WriteCmd(0x40);
    OLED_WriteCmd(0x20); /* Memory Addressing Mode */
    OLED_WriteCmd(0x00); /* Horizontal */
    OLED_WriteCmd(0x8D); /* Charge Pump */
    OLED_WriteCmd(0x14); /* Enable Charge Pump（3.3V→内部高压）*/
    OLED_WriteCmd(0xA4); /* Display From RAM */
    OLED_WriteCmd(0xA6); /* Normal Display */
    OLED_WriteCmd(0xAF); /* Display ON */

    OLED_Clear();
    OLED_Refresh();
}

/* ===== 帧缓冲操作 ===== */

void OLED_Clear(void)     { memset(OLED_Buf, 0x00, sizeof(OLED_Buf)); }
void OLED_Fill(uint8_t d) { memset(OLED_Buf, d, sizeof(OLED_Buf)); }

void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t mode)
{
    if(x >= OLED_WIDTH || y >= OLED_HEIGHT) return;
    uint8_t page = y / 8;
    uint8_t bit  = y % 8;
    if(mode)
        OLED_Buf[page][x] |=  (1 << bit);
    else
        OLED_Buf[page][x] &= ~(1 << bit);
}

void OLED_Refresh(void)
{
    for(uint8_t page = 0; page < 8; page++)
    {
        OLED_SetPos(0, page);
        for(uint8_t col = 0; col < OLED_WIDTH; col++)
            OLED_WriteData(OLED_Buf[page][col]);
    }
}

void OLED_SetPos(uint8_t x, uint8_t y)
{
    OLED_WriteCmd(0xB0 + y);                   /* 页地址 */
    OLED_WriteCmd(((x & 0xF0) >> 4) | 0x10);   /* 列地址高4位 */
    OLED_WriteCmd(x & 0x0F);                    /* 列地址低4位 */
}

/* ===== 字符和字符串显示 ===== */

void OLED_ShowChar(uint8_t x, uint8_t y, char ch, uint8_t size)
{
    if(ch < ' ' || ch > '~') ch = ' ';  /* 不可打印字符替换为空格 */
    uint8_t idx = ch - ' ';             /* 在字库中的索引 */

    if(x > OLED_WIDTH - 1) { x = 0; y++; }
    if(size == 12)  /* 6×8 字体 */
    {
        for(uint8_t i = 0; i < 6; i++)
        {
            uint8_t line = F6x8[idx][i];
            for(uint8_t bit = 0; bit < 8; bit++)
            {
                if(line & (1 << bit))
                    OLED_DrawPoint(x + i, y * 8 + bit, 1);
                else
                    OLED_DrawPoint(x + i, y * 8 + bit, 0);
            }
        }
    }
    /*【电赛扩展】添加 8×16 字体支持请自行添加字库和绘图逻辑 */
}

void OLED_ShowString(uint8_t x, uint8_t y, char* str, uint8_t size)
{
    while(*str)
    {
        OLED_ShowChar(x, y, *str, size);
        x += (size == 12) ? 6 : 8;
        if(x >= OLED_WIDTH) { x = 0; y++; }
        str++;
    }
}

void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size)
{
    char buf[12];
    /*【电赛注意】snprintf 在 MicroLIB 下可能不支持，可以用 sprintf 或自己写 */
    sprintf(buf, "%*lu", len, (unsigned long)num);
    OLED_ShowString(x, y, buf, size);
}

void OLED_ShowFloat(uint8_t x, uint8_t y, float num, uint8_t intLen, uint8_t decLen, uint8_t size)
{
    char buf[20];
    sprintf(buf, "%*.*f", intLen + decLen + 1, decLen, num);
    OLED_ShowString(x, y, buf, size);
}
```

---

## 26.4 传感器模块模板

### 26.4.1 超声波 HC-SR04 `Hardware/hc_sr04/hc_sr04.h` 和 `Hardware/hc_sr04/hc_sr04.c`

**`Hardware/hc_sr04/hc_sr04.h`**：

```c
/**
 * @file    hc_sr04.h
 * @brief   HC-SR04 超声波测距模块
 * @note    使用 TIM2 CH2（PA1）输入捕获测 Echo 高电平脉宽
 *          Trig 使用 PA0 普通 GPIO
 *          距离 = (高电平时间us × 0.034cm/us) / 2
 *          
 *          【电赛修改指南】
 *          1. 修改 TRIG_PORT/PIN 和 ECHO_TIM_CH 为你的实际引脚
 *          2. 测距盲区约 2cm，最大约 400cm
 *          3. 两次测距间隔建议 ≥60ms
 *          4. HCSR04_GetDistance() 返回单位是 mm，超时返回 0xFFFF
 *          5. Echo 引脚输出 5V！需要电阻分压（3.3k+5.1k=3.3V）或电平转换！
 *             【保命警告】不接电平转换直接把 5V Echo 接 STM32 → 烧芯片！
 */
#ifndef __HC_SR04_H
#define __HC_SR04_H

#include "stm32f10x.h"

/* ===== 引脚配置 ===== */
#define TRIG_PORT       GPIOA
#define TRIG_PIN        GPIO_Pin_0      /* Trig 触发脚 */
#define TRIG_CLK        RCC_APB2Periph_GPIOA

/* ===== 函数声明 ===== */
void HCSR04_Init(void);
uint16_t HCSR04_GetDistance(void);  /* 返回距离（mm），超时=0xFFFF */

#endif
```

**`Hardware/hc_sr04/hc_sr04.c`**：

```c
/**
 * @file    hc_sr04.c
 * @brief   超声波测距实现（TIM2 CH2 输入捕获）
 */
#include "hc_sr04.h"
#include "sys_tick.h"
#include "delay.h"

/* 捕获状态标志（中断中修改，加 volatile）*/
static volatile uint8_t  echo_state = 0;  /* 0=等上升沿, 1=等下降沿 */
static volatile uint32_t echo_time  = 0;  /* 高电平时长（us）*/
static volatile uint8_t  echo_done  = 0;  /* 0=未完成, 1=完成, 2=超时 */
static volatile uint16_t echo_ovf_cnt = 0;/* 定时器溢出次数 */

void HCSR04_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_ICInitTypeDef  TIM_ICInitStructure;
    NVIC_InitTypeDef   NVIC_InitStructure;

    /* ----- 时钟 ----- */
    RCC_APB2PeriphClockCmd(TRIG_CLK, ENABLE);  /* GPIOA */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);  /* TIM2 */

    /* ----- Trig：推挽输出，默认低 ----- */
    GPIO_InitStructure.GPIO_Pin  = TRIG_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(TRIG_PORT, &GPIO_InitStructure);
    GPIO_ResetBits(TRIG_PORT, TRIG_PIN);

    /* ----- Echo（PA1）：浮空输入 ----- */
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* ----- TIM2 时基：1us 计数（72MHz / (71+1) = 1MHz）----- */
    TIM_TimeBaseStructure.TIM_Period        = 65535;   /* ARR 最大 */
    TIM_TimeBaseStructure.TIM_Prescaler     = 71;      /* PSC=71 → 1MHz */
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    /* ----- TIM2 CH2 输入捕获：上升沿 ----- */
    TIM_ICInitStructure.TIM_Channel    = TIM_Channel_2;    /* PA1 */
    TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
    TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
    TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;  /* 不分频 */
    TIM_ICInitStructure.TIM_ICFilter    = 0x03;  /* 滤波：滤除 <3个时钟周期的毛刺 */
    TIM_ICInit(TIM2, &TIM_ICInitStructure);

    /* ----- NVIC ----- */
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority       = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd               = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    TIM_ITConfig(TIM2, TIM_IT_CC2 | TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM2, ENABLE);
}

/**
 * @brief  触发一次测量并返回距离
 * @return 距离（mm），0xFFFF=超时/无回波
 */
uint16_t HCSR04_GetDistance(void)
{
    /* 发送 20us 高电平触发脉冲 */
    GPIO_SetBits(TRIG_PORT, TRIG_PIN);
    Delay_us(20);
    GPIO_ResetBits(TRIG_PORT, TRIG_PIN);

    /* 等待测量完成 */
    echo_done = 0;
    uint32_t t_start = sys_time;
    while(echo_done == 0 && sys_time - t_start < 300); /* 300ms 超时 */

    if(echo_done == 1)  /* 成功 */
        return (uint16_t)(echo_time * 17 / 100);  /* mm = (us * 0.34/2) * 10 */
    else
        return 0xFFFF;  /* 超时 */
}
/* ⚠️ TIM2_IRQHandler 定义在 stm32f10x_it.c 中（见下方），不要复制到本文件！ */
/* 【重要】如果同时使用 HC-SR04 和 Motor_DC / Servo（都用到 TIM2），
   必须将所有 TIM2 中断处理合并到同一个 TIM2_IRQHandler 中。
   详见 26.10 模板集成指南。 */
```

### 26.4.2 温度传感器 DS18B20 `Hardware/ds18b20/ds18b20.h` 和 `Hardware/ds18b20/ds18b20.c`

**`Hardware/ds18b20/ds18b20.h`**：

```c
/**
 * @file    ds18b20.h
 * @brief   DS18B20 数字温度传感器（单总线 1-Wire）
 * @note    只需要一个 GPIO 引脚（默认 PA0）
 *          DQ 引脚必须外接 4.7kΩ 上拉电阻到 3.3V！
 *          精度 ±0.5°C，范围 -55°C~+125°C
 *          
 *          【电赛修改指南】
 *          1. 修改 DS18B20_DQ_PORT/PIN 为你的实际引脚
 *          2. 通信期间不能被打断（特别是中断），否则时序错乱
 *             如果需要，在读写操作前后使用 __disable_irq() / __enable_irq()
 *          3. 温度转换需要约 750ms，建议用异步模式：
 *             DS18B20_StartConvert() → 等 750ms → DS18B20_ReadTemp()
 */
#ifndef __DS18B20_H
#define __DS18B20_H

#include "stm32f10x.h"

/* ===== 引脚配置 ===== */
#define DS18B20_DQ_PORT  GPIOA
#define DS18B20_DQ_PIN   GPIO_Pin_0
#define DS18B20_DQ_CLK   RCC_APB2Periph_GPIOA

/* ===== IO 操作 ===== */
#define DS18B20_DQ_H()   GPIO_SetBits(DS18B20_DQ_PORT, DS18B20_DQ_PIN)
#define DS18B20_DQ_L()   GPIO_ResetBits(DS18B20_DQ_PORT, DS18B20_DQ_PIN)
#define DS18B20_DQ_READ() GPIO_ReadInputDataBit(DS18B20_DQ_PORT, DS18B20_DQ_PIN)

void DS18B20_Init(void);
uint8_t DS18B20_Reset(void);
void DS18B20_StartConvert(void);
float DS18B20_ReadTemp(void);

#endif
```

**`Hardware/ds18b20/ds18b20.c`**：

```c
/**
 * @file    ds18b20.c
 * @brief   DS18B20 单总线驱动实现
 */
#include "ds18b20.h"
#include "delay.h"

/* 方向切换：单总线引脚需要动态切换输入/输出 */
static void DQ_Out(void)
{
    GPIO_InitTypeDef g;
    g.GPIO_Pin   = DS18B20_DQ_PIN;
    g.GPIO_Mode  = GPIO_Mode_Out_PP;
    g.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DS18B20_DQ_PORT, &g);
}
static void DQ_In(void)
{
    GPIO_InitTypeDef g;
    g.GPIO_Pin  = DS18B20_DQ_PIN;
    g.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(DS18B20_DQ_PORT, &g);
}

void DS18B20_Init(void)
{
    RCC_APB2PeriphClockCmd(DS18B20_DQ_CLK, ENABLE);
    DQ_Out();
    DS18B20_DQ_H();  /* 释放总线 */
}

uint8_t DS18B20_Reset(void)
{
    uint8_t presence;
    DQ_Out(); DS18B20_DQ_L();
    Delay_us(480);            /* 拉低 ≥480μs（复位脉冲）*/
    DS18B20_DQ_H();
    Delay_us(60);             /* 等 15~60μs */
    DQ_In();
    presence = DS18B20_DQ_READ(); /* 0=存在 */
    Delay_us(420);
    return presence;
}

static void DS18B20_WriteBit(uint8_t bit)
{
    DQ_Out(); DS18B20_DQ_L(); Delay_us(2);
    if(bit) DS18B20_DQ_H();   /* 写1：释放 */
    Delay_us(60);
    DS18B20_DQ_H(); Delay_us(2);
}

static uint8_t DS18B20_ReadBit(void)
{
    uint8_t bit;
    DQ_Out(); DS18B20_DQ_L(); Delay_us(2);
    DS18B20_DQ_H(); Delay_us(5);  /* 等 DS18B20 放数据 */
    DQ_In();
    bit = DS18B20_DQ_READ();
    Delay_us(55);
    return bit;
}

static void DS18B20_WriteByte(uint8_t dat)
{
    for(uint8_t i = 0; i < 8; i++)
        { DS18B20_WriteBit(dat & 0x01); dat >>= 1; }
}

static uint8_t DS18B20_ReadByte(void)
{
    uint8_t dat = 0;
    for(uint8_t i = 0; i < 8; i++)
        { dat >>= 1; if(DS18B20_ReadBit()) dat |= 0x80; }
    return dat;
}

void DS18B20_StartConvert(void)
{
    DS18B20_Reset();
    DS18B20_WriteByte(0xCC);  /* SKIP ROM */
    DS18B20_WriteByte(0x44);  /* CONVERT T */
}

float DS18B20_ReadTemp(void)
{
    uint8_t TL, TH;
    int16_t raw;
    DS18B20_Reset();
    DS18B20_WriteByte(0xCC);  /* SKIP ROM */
    DS18B20_WriteByte(0xBE);  /* READ SCRATCHPAD */
    TL = DS18B20_ReadByte();
    TH = DS18B20_ReadByte();
    raw = ((int16_t)TH << 8) | TL;
    return (float)raw * 0.0625f;
}
```

---

## 26.5 执行器模块模板

### 26.5.1 直流电机 TB6612 `Hardware/motor_dc/motor_dc.h` 和 `Hardware/motor_dc/motor_dc.c`

**`Hardware/motor_dc/motor_dc.h`**：

```c
/**
 * @file    motor_dc.h
 * @brief   直流电机驱动模块（TB6612FNG 双路电机驱动）
 * @note    A路：PWMA=PA0(TIM2_CH1), AIN1=PB3, AIN2=PB4
 *          B路：PWMB=PA1(TIM2_CH2), BIN1=PB5, BIN2=PB6
 *          STBY=PA2（高电平=正常工作）
 *          PWM 频率 10kHz（TIM2, PSC=71, ARR=99）
 *          
 *          【电赛修改指南】
 *          1. 修改引脚宏定义为你的实际接线
 *          2. PWM_MAX：PWM 最大占空比（0~ARR），建议 ≤90% 保护电机
 *          3. 如果想用其他定时器，改 TIMx 配置和对应的 GPIO
 *          4. 电机电源 VM 必须独立供电！绝对不能和 STM32 共用 3.3V！
 *          5. 电机 VCC 和 STM32 GND 必须共地！
 */
#ifndef __MOTOR_DC_H
#define __MOTOR_DC_H

#include "stm32f10x.h"

/* ===== 引脚配置（A路）===== */
#define MOTOR_PWMA_PORT  GPIOA
#define MOTOR_PWMA_PIN   GPIO_Pin_0    /* TIM2_CH1 */
#define MOTOR_AIN1_PORT  GPIOB
#define MOTOR_AIN1_PIN   GPIO_Pin_3
#define MOTOR_AIN2_PORT  GPIOB
#define MOTOR_AIN2_PIN   GPIO_Pin_4

/* ===== 引脚配置（B路）===== */
#define MOTOR_PWMB_PORT  GPIOA
#define MOTOR_PWMB_PIN   GPIO_Pin_1    /* TIM2_CH2 */
#define MOTOR_BIN1_PORT  GPIOB
#define MOTOR_BIN1_PIN   GPIO_Pin_5
#define MOTOR_BIN2_PORT  GPIOB
#define MOTOR_BIN2_PIN   GPIO_Pin_6

/* ===== STBY ===== */
#define MOTOR_STBY_PORT  GPIOA
#define MOTOR_STBY_PIN   GPIO_Pin_2

/* ===== PWM 限制 ===== */
#define PWM_MAX          90   /* PWM 最大占空比（ARR=99时，90=91%），保护电机 */

void Motor_DC_Init(void);
void MotorA_SetSpeed(int16_t speed);  /* speed: -PWM_MAX ~ +PWM_MAX */
void MotorB_SetSpeed(int16_t speed);
void Motor_Stop(void);                /* 全部停止 */

#endif
```

**`Hardware/motor_dc/motor_dc.c`**：

```c
/**
 * @file    motor_dc.c
 * @brief   直流电机驱动实现
 */
#include "motor_dc.h"

void Motor_DC_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef  TIM_OCInitStructure;

    /* ----- 时钟 ----- */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    /* ----- AIN1/AIN2/BIN1/BIN2/STBY：推挽输出 ----- */
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_InitStructure.GPIO_Pin = MOTOR_AIN1_PIN | MOTOR_AIN2_PIN;
    GPIO_Init(MOTOR_AIN1_PORT, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = MOTOR_BIN1_PIN | MOTOR_BIN2_PIN;
    GPIO_Init(MOTOR_BIN1_PORT, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = MOTOR_STBY_PIN;
    GPIO_Init(MOTOR_STBY_PORT, &GPIO_InitStructure);

    GPIO_SetBits(MOTOR_STBY_PORT, MOTOR_STBY_PIN);  /* STBY=1 正常工作 */

    /* ----- PWM 引脚：复用推挽 ----- */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  /* 复用推挽！*/
    GPIO_InitStructure.GPIO_Pin  = MOTOR_PWMA_PIN;
    GPIO_Init(MOTOR_PWMA_PORT, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin  = MOTOR_PWMB_PIN;
    GPIO_Init(MOTOR_PWMB_PORT, &GPIO_InitStructure);

    /* ----- TIM2 时基：10kHz PWM ----- */
    TIM_TimeBaseStructure.TIM_Period    = 99;    /* ARR=99 */
    TIM_TimeBaseStructure.TIM_Prescaler = 71;    /* PSC=71 → 72M/72=1MHz */
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    /* ----- TIM2 CH1 PWM 输出（A路）----- */
    TIM_OCInitStructure.TIM_OCMode      = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse       = 0;   /* 初始占空比 0 */
    TIM_OCInitStructure.TIM_OCPolarity  = TIM_OCPolarity_High;
    TIM_OC1Init(TIM2, &TIM_OCInitStructure);
    TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);

    /* ----- TIM2 CH2 PWM 输出（B路）----- */
    TIM_OCInitStructure.TIM_Pulse = 0;
    TIM_OC2Init(TIM2, &TIM_OCInitStructure);
    TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Enable);

    TIM_Cmd(TIM2, ENABLE);
}

/**
 * @brief  设置 A 路电机速度
 * @param  speed: -PWM_MAX(反转全速) ~ 0(停止) ~ +PWM_MAX(正转全速)
 */
void MotorA_SetSpeed(int16_t speed)
{
    if(speed > 0)  /* 正转 */
    {
        GPIO_SetBits(MOTOR_AIN1_PORT, MOTOR_AIN1_PIN);
        GPIO_ResetBits(MOTOR_AIN2_PORT, MOTOR_AIN2_PIN);
        if(speed > PWM_MAX) speed = PWM_MAX;
        TIM_SetCompare1(TIM2, (uint16_t)speed);
    }
    else if(speed < 0)  /* 反转 */
    {
        GPIO_ResetBits(MOTOR_AIN1_PORT, MOTOR_AIN1_PIN);
        GPIO_SetBits(MOTOR_AIN2_PORT, MOTOR_AIN2_PIN);
        int16_t s = -speed;
        if(s > PWM_MAX) s = PWM_MAX;
        TIM_SetCompare1(TIM2, (uint16_t)s);
    }
    else  /* 停止（滑行）*/
    {
        GPIO_ResetBits(MOTOR_AIN1_PORT, MOTOR_AIN1_PIN);
        GPIO_ResetBits(MOTOR_AIN2_PORT, MOTOR_AIN2_PIN);
        TIM_SetCompare1(TIM2, 0);
    }
}

void MotorB_SetSpeed(int16_t speed)
{
    if(speed > 0)
    {
        GPIO_SetBits(MOTOR_BIN1_PORT, MOTOR_BIN1_PIN);
        GPIO_ResetBits(MOTOR_BIN2_PORT, MOTOR_BIN2_PIN);
        if(speed > PWM_MAX) speed = PWM_MAX;
        TIM_SetCompare2(TIM2, (uint16_t)speed);
    }
    else if(speed < 0)
    {
        GPIO_ResetBits(MOTOR_BIN1_PORT, MOTOR_BIN1_PIN);
        GPIO_SetBits(MOTOR_BIN2_PORT, MOTOR_BIN2_PIN);
        int16_t s = -speed;
        if(s > PWM_MAX) s = PWM_MAX;
        TIM_SetCompare2(TIM2, (uint16_t)s);
    }
    else
    {
        GPIO_ResetBits(MOTOR_BIN1_PORT, MOTOR_BIN1_PIN);
        GPIO_ResetBits(MOTOR_BIN2_PORT, MOTOR_BIN2_PIN);
        TIM_SetCompare2(TIM2, 0);
    }
}

void Motor_Stop(void) { MotorA_SetSpeed(0); MotorB_SetSpeed(0); }
```

### 26.5.2 舵机 `Hardware/servo/servo.h` 和 `Hardware/servo/servo.c`

**`Hardware/servo/servo.h`**：

```c
/**
 * @file    servo.h
 * @brief   SG90 舵机控制模块（50Hz PWM，TIM2_CH3=PA2 输出）
 * @note    PWM 参数：频率 50Hz（周期 20ms），PSC=71, ARR=19999
 *          高电平时间 500~2500us 对应 0°~180°
 *          CCR = 500~2500（因为计数器 1us 加 1）
 *          
 *          【电赛修改指南】
 *          1. 改引脚：修改 SERVO_TIM_CH 和对应的 GPIO 配置
 *          2. 改角度范围：修改 SERVO_MIN_PULSE / SERVO_MAX_PULSE
 *          3. 多路舵机：用同一个定时器的多个通道
 *          4. 舵机电源必须独立供电（5V 2A 以上）！
 *          5. 舵机绝不能从 STM32 的 3.3V 取电！
 */
#ifndef __SERVO_H
#define __SERVO_H

#include "stm32f10x.h"

/* ===== 脉宽范围（us）===== */
#define SERVO_MIN_PULSE  500    /* 0° 对应的脉宽 */
#define SERVO_MAX_PULSE  2500   /* 180° 对应的脉宽 */

void Servo_Init(void);
void Servo_SetAngle(uint8_t angle);  /* angle: 0~180° */

#endif
```

**`Hardware/servo/servo.c`**：

```c
/**
 * @file    servo.c
 * @brief   舵机控制实现（TIM2_CH3 = PA2, 50Hz）
 */
#include "servo.h"

void Servo_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef  TIM_OCInitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    /* PA2 = 复用推挽（TIM2_CH3）*/
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* TIM2 时基：50Hz（PSC=71→1MHz, ARR=19999→20000us=20ms）*/
    TIM_TimeBaseStructure.TIM_Period    = 19999;
    TIM_TimeBaseStructure.TIM_Prescaler = 71;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    /* TIM2 CH3 PWM */
    TIM_OCInitStructure.TIM_OCMode      = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse       = 1500;  /* 默认 90° */
    TIM_OCInitStructure.TIM_OCPolarity  = TIM_OCPolarity_High;
    TIM_OC3Init(TIM2, &TIM_OCInitStructure);
    TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable);

    TIM_Cmd(TIM2, ENABLE);
}

void Servo_SetAngle(uint8_t angle)
{
    if(angle > 180) angle = 180;
    /* 0°=500us, 180°=2500us, 线性映射 */
    uint16_t ccr = SERVO_MIN_PULSE + (uint32_t)angle * (SERVO_MAX_PULSE - SERVO_MIN_PULSE) / 180;
    TIM_SetCompare3(TIM2, ccr);
}
```

### 26.5.3 编码器 `Hardware/encoder/encoder.h` 和 `Hardware/encoder/encoder.c`

**`Hardware/encoder/encoder.h`**：

```c
/**
 * @file    encoder.h
 * @brief   编码器接口模块（TIM3 编码器模式，4倍频）
 * @note    使用 TIM3 CH1=PA6, CH2=PA7（默认引脚）
 *          TIM3 挂在 APB1（72MHz 定时器时钟）
 *          
 *          【电赛修改指南】
 *          1. 编码器线数（PPR）：改 ENCODER_PPR
 *          2. 减速比：改 GEAR_RATIO
 *          3. 如果改用 TIM4 编码器：CH1=PB6, CH2=PB7
 *          4. 必须开滤波器！直流电机碳刷干扰严重
 *          5. 编码器必须和 STM32 共地
 */
#ifndef __ENCODER_H
#define __ENCODER_H

#include "stm32f10x.h"

/* ===== 编码器参数（按你的电机型号修改！）===== */
#define ENCODER_PPR     390    /* 编码器线数（每圈脉冲数，1倍频）*/
#define GEAR_RATIO      30     /* 减速比（电机:输出轴）*/
/* 4倍频后每圈输出轴脉冲 = PPR * 4 * GEAR_RATIO */

void Encoder_Init(void);
int16_t Encoder_GetDelta(void);         /* 读取增量并清零 */
int32_t Encoder_GetTotalCount(void);    /* 读取累计脉冲数 */
float Encoder_GetSpeedRPM(float dt_ms); /* 计算转速（RPM）*/

#endif
```

**`Hardware/encoder/encoder.c`**：

```c
/**
 * @file    encoder.c
 * @brief   编码器接口实现
 */
#include "encoder.h"

void Encoder_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_ICInitTypeDef  TIM_ICInitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    /* PA6=CH1, PA7=CH2：浮空输入 */
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* TIM3 时基：最大计数范围 */
    TIM_TimeBaseStructure.TIM_Period    = 65535;
    TIM_TimeBaseStructure.TIM_Prescaler = 0;      /* 不分频 */
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    /* 编码器模式：TI1+TI2 都计数（4倍频）*/
    TIM_EncoderInterfaceConfig(TIM3, TIM_EncoderMode_TI12,
                               TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);

    /* 滤波器：滤除电机碳刷毛刺 */
    TIM_ICStructInit(&TIM_ICInitStructure);
    TIM_ICInitStructure.TIM_ICFilter = 10;  /* 滤波系数（0~15，越大越强）*/
    TIM_ICInit(TIM3, &TIM_ICInitStructure);

    TIM_SetCounter(TIM3, 0);
    TIM_Cmd(TIM3, ENABLE);
}

int16_t Encoder_GetDelta(void)
{
    int16_t delta = (int16_t)TIM_GetCounter(TIM3);
    TIM_SetCounter(TIM3, 0);
    return delta;
}

int32_t Encoder_GetTotalCount(void)
{
    static int32_t total = 0;
    total += Encoder_GetDelta();
    return total;
}

float Encoder_GetSpeedRPM(float dt_ms)
{
    /* RPM = delta_cnt / (PPR * 4 * GEAR_RATIO) * (60000 / dt_ms) */
    int16_t delta = Encoder_GetDelta();
    if(dt_ms <= 0) return 0;
    return (float)delta * 60000.0f / (ENCODER_PPR * 4.0f * GEAR_RATIO * dt_ms);
}
```

---

## 26.6 算法模块模板

### 26.6.1 PID 控制器 `Hardware/pid/pid.h` 和 `Hardware/pid/pid.c`

**`Hardware/pid/pid.h`**：

```c
/**
 * @file    pid.h
 * @brief   PID 控制器（位置式 + 增量式 + 积分分离 + 死区）
 * @note    【电赛修改指南：如何调 PID 参数】
 *          1. 先 Ki=0, Kd=0, 从 Kp=0.1 开始逐步加大，直到系统出现等幅震荡
 *          2. 震荡时的 Kp 值 × 0.6 ≈ 合适的 Kp
 *          3. 加 Ki（从小开始：0.01→0.05→0.1），直到静差消除
 *          4. 加 Kd（需要时，从小开始：0.01→0.05→0.1），抑制超调
 *          5. PID 计算周期必须固定（如 10ms），不能变！
 *          6. 如果积分饱和（超调严重），启用积分分离或调小 integral_max
 */
#ifndef __PID_H
#define __PID_H

typedef struct {
    float Kp, Ki, Kd;          /* PID 系数 */
    float set;                 /* 目标值（设定值）*/
    float error;               /* 当前误差 */
    float last_error;          /* 上次误差（微分用）*/
    float prev_error;          /* 上上次误差（增量式用）*/
    float integral;            /* 积分累积 */
    float output;              /* 输出值 */
    float integral_max;        /* 积分限幅（建议 output_max × 1.5）*/
    float output_max;          /* 输出限幅 */
    float deadband;            /* 死区：误差在此范围内输出0（0=不启用）*/
    float separate_threshold;  /* 积分分离阈值（0=不启用，建议 output_max * 0.3）*/
} PID_TypeDef;

void PID_Init(PID_TypeDef* pid, float kp, float ki, float kd,
              float out_max, float i_max);
float PID_Calc(PID_TypeDef* pid, float feedback);          /* 位置式 */
float PID_Calc_Incremental(PID_TypeDef* pid, float feedback); /* 增量式 */
void PID_Reset(PID_TypeDef* pid);                          /* 清零积分 */
void PID_SetTarget(PID_TypeDef* pid, float target);        /* 改目标值 */

#endif
```

**`Hardware/pid/pid.c`**：

```c
/**
 * @file    pid.c
 * @brief   PID 控制器实现
 */
#include "pid.h"
#include <math.h>  /* fabs */

void PID_Init(PID_TypeDef* pid, float kp, float ki, float kd,
              float out_max, float i_max)
{
    pid->Kp = kp;  pid->Ki = ki;  pid->Kd = kd;
    pid->set = 0;  pid->error = 0;  pid->last_error = 0;  pid->prev_error = 0;
    pid->integral = 0;  pid->output = 0;
    pid->output_max = out_max;
    pid->integral_max = (i_max > 0) ? i_max : out_max * 1.5f;
    pid->deadband = 0;
    pid->separate_threshold = 0;
}

float PID_Calc(PID_TypeDef* pid, float feedback)
{
    pid->error = pid->set - feedback;

    /* 死区 */
    if(pid->deadband > 0 && fabs(pid->error) < pid->deadband)
    {
        pid->error = 0;
        pid->integral = 0;
    }

    /* 积分分离：误差大时不积分，防止超调 */
    if(pid->separate_threshold > 0 && fabs(pid->error) > pid->separate_threshold)
        pid->integral = 0;
    else
    {
        pid->integral += pid->error;
        /* 积分限幅 */
        if(pid->integral > pid->integral_max)  pid->integral = pid->integral_max;
        if(pid->integral < -pid->integral_max) pid->integral = -pid->integral_max;
    }

    /* 微分 */
    float derivative = pid->error - pid->last_error;

    /* PID 计算 */
    pid->output = pid->Kp * pid->error
                + pid->Ki * pid->integral
                + pid->Kd * derivative;

    /* 输出限幅 */
    if(pid->output > pid->output_max)  pid->output = pid->output_max;
    if(pid->output < -pid->output_max) pid->output = -pid->output_max;

    pid->last_error = pid->error;
    return pid->output;
}

float PID_Calc_Incremental(PID_TypeDef* pid, float feedback)
{
    pid->error = pid->set - feedback;

    float delta = pid->Kp * (pid->error - pid->last_error)
                + pid->Ki * pid->error
                + pid->Kd * (pid->error - 2.0f * pid->last_error + pid->prev_error);

    pid->output += delta;

    /* 输出限幅 */
    if(pid->output > pid->output_max)  pid->output = pid->output_max;
    if(pid->output < -pid->output_max) pid->output = -pid->output_max;

    pid->prev_error = pid->last_error;
    pid->last_error = pid->error;
    return pid->output;
}

void PID_Reset(PID_TypeDef* pid)
{
    pid->integral   = 0;
    pid->error      = 0;
    pid->last_error = 0;
    pid->prev_error = 0;
    pid->output     = 0;
}

void PID_SetTarget(PID_TypeDef* pid, float target)
{
    pid->set = target;
    /* 设定值突变时清积分，防止冲击 */
    pid->integral = 0;
}
```

### 26.6.2 数字滤波器 `Hardware/filter/filter.h` 和 `Hardware/filter/filter.c`

**`Hardware/filter/filter.h`**：

```c
/**
 * @file    filter.h
 * @brief   数字滤波器集合（限幅/平均/滑动/中值/低通）
 *          【电赛选型指南】
 *          - 超声波：先用中值去掉异常值，再用滑动平均或低通
 *          - ADC：滑动平均（窗口 8~16）或低通（α=0.1~0.3）
 *          - MPU6050：自带硬件低通，一般不需额外滤波
 *          - 编码器速度：低通（α=0.1）或滑动平均（窗口 4~8）
 */
#ifndef __FILTER_H
#define __FILTER_H

#include "stm32f10x.h"

float Filter_Limit(float value, float max_delta);           /* 限幅滤波 */
float Filter_SlideAvg(float value, uint8_t window_size);    /* 滑动平均 */
float Filter_Median(float* buf, uint8_t n);                 /* 中值滤波（排序后取中）*/
float Filter_LowPass(float value, float alpha);             /* 一阶低通 */
void Filter_Reset(void);                                    /* 重置滤波器状态 */

#endif
```

**`Hardware/filter/filter.c`**：

```c
/**
 * @file    filter.c
 * @brief   数字滤波器实现
 */
#include "filter.h"
#include <string.h>
#include <stdlib.h>

#define FILTER_MAX_WINDOW  20

/* 滑动平均窗口 */
static float slide_buf[FILTER_MAX_WINDOW];
static uint8_t slide_idx = 0;
static uint8_t slide_count = 0;

/* 限幅滤波 */
static float limit_last = 0;
float Filter_Limit(float value, float max_delta)
{
    if(fabs(value - limit_last) > max_delta)
        return limit_last;
    limit_last = value;
    return value;
}

/* 滑动平均 */
float Filter_SlideAvg(float value, uint8_t window_size)
{
    if(window_size > FILTER_MAX_WINDOW) window_size = FILTER_MAX_WINDOW;
    slide_buf[slide_idx] = value;
    slide_idx = (slide_idx + 1) % window_size;
    if(slide_count < window_size) slide_count++;

    float sum = 0;
    for(uint8_t i = 0; i < slide_count; i++)
        sum += slide_buf[i];
    return sum / slide_count;
}

/* 中值滤波 */
static int cmp_float(const void* a, const void* b)
{
    float diff = *(float*)a - *(float*)b;
    return (diff > 0) ? 1 : ((diff < 0) ? -1 : 0);
}
float Filter_Median(float* buf, uint8_t n)
{
    float temp[n];
    memcpy(temp, buf, n * sizeof(float));
    qsort(temp, n, sizeof(float), cmp_float);
    return temp[n / 2];
}

/* 一阶低通 */
static float lpf_last = 0;
float Filter_LowPass(float value, float alpha)
{
    lpf_last = alpha * value + (1.0f - alpha) * lpf_last;
    return lpf_last;
}

void Filter_Reset(void)
{
    slide_idx = 0;  slide_count = 0;
    memset(slide_buf, 0, sizeof(slide_buf));
    lpf_last = 0;  limit_last = 0;
}
```

---

## 26.7 存储模块模板

### 26.7.1 片内 Flash 存储 `Hardware/flash_store/flash_store.h` 和 `Hardware/flash_store/flash_store.c`

**`Hardware/flash_store/flash_store.h`**：

```c
/**
 * @file    flash_store.h
 * @brief   片内 Flash 存储模块（掉电保存参数）
 * @note    使用最后 1KB（第63页）存储用户配置
 *          写入前必须擦除整页（1KB），最小写入单位是 16 位半字
 *          写入寿命约 1 万次，不要频繁写入！
 *          建议只在"用户按保存键"时写入，不要在循环中持续写入。
 *          
 *          【电赛修改指南】
 *          1. 修改 SaveData 结构体，添加你需要保存的变量
 *          2. 确保代码总量 < 63KB（64KB Flash - 1KB），
 *             查看 Keil 编译输出 "Program Size: Code=xxxxx"
 *          3. 如果代码超过 62KB，改用第 62 页或更小页
 *          4. FLASH_SAVE_PAGE 值和结构体大小会影响存放位置
 *          5. 写入期间会暂停 CPU 取指，中断可能丢失，
 *             如需保证中断响应，写入前临时关闭非关键中断
 */
#ifndef __FLASH_STORE_H
#define __FLASH_STORE_H

#include "stm32f10x.h"

/* ===== 存储页配置 ===== */
#define FLASH_SAVE_PAGE   63    /* 第63页（最后一页，1KB）*/
#define FLASH_SAVE_ADDR   (0x08000000 + FLASH_SAVE_PAGE * 1024)

/* ===== 要保存的数据结构（按需修改！）===== */
typedef struct {
    uint16_t magic;           /* 魔数：0xA5A5=有效，用于判断是否已初始化 */
    uint16_t checksum;        /* XOR 校验和 */
    float    pid_speed_kp;    /* 速度环 Kp */
    float    pid_speed_ki;    /* 速度环 Ki */
    float    pid_speed_kd;    /* 速度环 Kd */
    float    pid_balance_kp;  /* 平衡环 Kp（如果是平衡车）*/
    float    pid_balance_ki;
    float    pid_balance_kd;
    uint16_t motor_max_speed; /* 电机最高转速限制 */
    uint16_t reserved[9];     /* 预留扩展（保持结构体大小不变）*/
} SaveData_t;

/* ===== 函数声明 ===== */
uint8_t Flash_SaveData(SaveData_t* data);   /* 保存数据到 Flash */
uint8_t Flash_LoadData(SaveData_t* data);   /* 从 Flash 读取数据 */
void Flash_SetDefaults(SaveData_t* data);   /* 设置默认值 */

#endif
```

**`Hardware/flash_store/flash_store.c`**：

```c
/**
 * @file    flash_store.c
 * @brief   片内 Flash 存储实现
 */
#include "flash_store.h"
#include <string.h>

void Flash_SetDefaults(SaveData_t* data)
{
    memset(data, 0, sizeof(SaveData_t));
    data->magic           = 0xA5A5;
    data->pid_speed_kp    = 0.5f;
    data->pid_speed_ki    = 0.1f;
    data->pid_speed_kd    = 0.0f;
    data->pid_balance_kp  = 0.0f;
    data->pid_balance_ki  = 0.0f;
    data->pid_balance_kd  = 0.0f;
    data->motor_max_speed = 100;
    /*【电赛修改】在这里添加你的默认参数 */
}

uint8_t Flash_SaveData(SaveData_t* data)
{
    /* 填魔数和校验 */
    data->magic    = 0xA5A5;
    data->checksum = 0;
    uint16_t* ptr  = (uint16_t*)data;
    for(uint8_t i = 2; i < sizeof(SaveData_t)/2; i++)
        data->checksum ^= ptr[i];

    FLASH_Unlock();  /* 解锁 Flash */

    /* 擦除整页 */
    if(FLASH_ErasePage(FLASH_SAVE_ADDR) != FLASH_COMPLETE)
        { FLASH_Lock(); return 1; }

    /* 逐半字写入 */
    uint32_t addr  = FLASH_SAVE_ADDR;
    uint16_t count = (sizeof(SaveData_t) + 1) / 2;
    for(uint16_t i = 0; i < count; i++)
    {
        if(FLASH_ProgramHalfWord(addr, ptr[i]) != FLASH_COMPLETE)
            { FLASH_Lock(); return 1; }
        addr += 2;
    }

    FLASH_Lock();

    /* 验证 */
    SaveData_t verify;
    memcpy(&verify, (void*)FLASH_SAVE_ADDR, sizeof(SaveData_t));
    if(memcmp(data, &verify, sizeof(SaveData_t)) != 0)
        return 1;

    return 0;
}

uint8_t Flash_LoadData(SaveData_t* data)
{
    memcpy(data, (void*)FLASH_SAVE_ADDR, sizeof(SaveData_t));

    if(data->magic != 0xA5A5) return 1;  /* 未初始化 */

    /* XOR 校验 */
    uint16_t checksum = 0;
    uint16_t* ptr = (uint16_t*)data;
    uint16_t save_checksum = data->checksum;
    data->checksum = 0;
    for(uint8_t i = 2; i < sizeof(SaveData_t)/2; i++)
        checksum ^= ptr[i];
    data->checksum = save_checksum;

    if(checksum != save_checksum) return 1;  /* 数据损坏 */

    return 0;
}
```

### 26.7.2 AT24C02 EEPROM `Hardware/at24c02/at24c02.h` 和 `Hardware/at24c02/at24c02.c`

**`Hardware/at24c02/at24c02.h`**：

```c
/**
 * @file    at24c02.h
 * @brief   AT24C02 EEPROM 256 字节存储（I2C 接口）
 * @note    寿命 100 万次写入，适合频繁修改的参数（PID 系数）
 *          比 Flash 更适合反复修改的场景
 *          
 *          【电赛修改指南】
 *          依赖 i2c_soft 模块，必须先调用 I2C_Soft_Init()
 *          I2C 地址：0x50（7位），写=0xA0，读=0xA1
 *          容量 256 字节，地址范围 0x00~0xFF
 */
#ifndef __AT24C02_H
#define __AT24C02_H

#include "stm32f10x.h"

void AT24C02_WriteByte(uint8_t addr, uint8_t data);
uint8_t AT24C02_ReadByte(uint8_t addr);
void AT24C02_WriteBuf(uint8_t addr, uint8_t* buf, uint8_t len);
void AT24C02_ReadBuf(uint8_t addr, uint8_t* buf, uint8_t len);

#endif
```

**`Hardware/at24c02/at24c02.c`**：

```c
/**
 * @file    at24c02.c
 * @brief   AT24C02 EEPROM 驱动实现
 */
#include "at24c02.h"
#include "i2c_soft.h"
#include "delay.h"

#define AT24C02_ADDR  0x50   /* 7位 I2C 地址 */

void AT24C02_WriteByte(uint8_t addr, uint8_t data)
{
    I2C_Start();
    I2C_SendByte(AT24C02_ADDR << 1);     /* 地址+写 */
    I2C_WaitAck();
    I2C_SendByte(addr);                   /* EEPROM 内部地址 */
    I2C_WaitAck();
    I2C_SendByte(data);
    I2C_WaitAck();
    I2C_Stop();
    Delay_ms(5);  /* EEPROM 写入周期约 5ms，期间不可操作 */
}

uint8_t AT24C02_ReadByte(uint8_t addr)
{
    uint8_t dat;
    I2C_Start();
    I2C_SendByte(AT24C02_ADDR << 1);     /* 地址+写 */
    I2C_WaitAck();
    I2C_SendByte(addr);                   /* 发内部地址 */
    I2C_WaitAck();
    I2C_Start();
    I2C_SendByte((AT24C02_ADDR << 1) | 1); /* 地址+读 */
    I2C_WaitAck();
    dat = I2C_ReadByte(0);                /* 读 1 字节，NACK */
    I2C_Stop();
    return dat;
}

void AT24C02_WriteBuf(uint8_t addr, uint8_t* buf, uint8_t len)
{
    for(uint8_t i = 0; i < len; i++)
    {
        AT24C02_WriteByte(addr + i, buf[i]);
    }
}

void AT24C02_ReadBuf(uint8_t addr, uint8_t* buf, uint8_t len)
{
    for(uint8_t i = 0; i < len; i++)
    {
        buf[i] = AT24C02_ReadByte(addr + i);
    }
}
```

---

## 26.8 无线通信模块模板

### 26.8.1 NRF24L01 `Hardware/nrf24l01/nrf24l01.h` 和 `Hardware/nrf24l01/nrf24l01.c`

**`Hardware/nrf24l01/nrf24l01.h`**：

```c
/**
 * @file    nrf24l01.h
 * @brief   NRF24L01 2.4G 无线模块驱动（SPI 接口）
 * @note    依赖 spi_soft 模块
 *          32 字节固定 Payload，频道 40（2.440GHz），1Mbps
 *          
 *          【电赛修改指南】
 *          1. 修改 CE/CSN/IRQ 引脚为你的实际引脚
 *          2. 两个 NRF 必须设相同频道和地址，否则无法通信
 *          3. RX_ADDR 和 TX_ADDR 可自定义（5字节），收发双方必须一致
 *          4. 电源必须加 100μF 电解 + 0.1μF 陶瓷电容！
 *             没有电容 → 发射瞬间电压跌落 → 收不到数据
 *          5. NRF 是 3.3V 器件，绝不能接 5V！
 */
#ifndef __NRF24L01_H
#define __NRF24L01_H

#include "stm32f10x.h"

#define NRF_PAYLOAD_SIZE  32   /* 每包 32 字节 */

#define NRF_MODE_RX  0
#define NRF_MODE_TX  1

void NRF24L01_Init(uint8_t mode);
uint8_t NRF_TxPacket(uint8_t* data);       /* 发一包，返回0=成功 */
uint8_t NRF_RxPacket(uint8_t* data);       /* 收一包，返回0=有新数据 */
uint8_t NRF_DataReady(void);               /* 检查是否有新数据（读 IRQ）*/

#endif
```

**`Hardware/nrf24l01/nrf24l01.c`**：

```c
/**
 * @file    nrf24l01.c
 * @brief   NRF24L01 驱动实现
 */
#include "nrf24l01.h"
#include "spi_soft.h"
#include "delay.h"

/* ===== 引脚定义（按实际接线修改）===== */
#define NRF_CE_PORT    GPIOB
#define NRF_CE_PIN     GPIO_Pin_0
#define NRF_CSN_PORT   GPIOA
#define NRF_CSN_PIN    GPIO_Pin_4
#define NRF_IRQ_PORT   GPIOB
#define NRF_IRQ_PIN    GPIO_Pin_1

/* ===== 操作宏 ===== */
#define CE_H()    GPIO_SetBits(NRF_CE_PORT, NRF_CE_PIN)
#define CE_L()    GPIO_ResetBits(NRF_CE_PORT, NRF_CE_PIN)
#define CSN_H()   GPIO_SetBits(NRF_CSN_PORT, NRF_CSN_PIN)
#define CSN_L()   GPIO_ResetBits(NRF_CSN_PORT, NRF_CSN_PIN)
#define IRQ_READ() GPIO_ReadInputDataBit(NRF_IRQ_PORT, NRF_IRQ_PIN)

/* ===== 内部函数 ===== */
static uint8_t NRF_RW(uint8_t dat) { uint8_t r; CSN_L(); r=SPI_ReadWriteByte(dat); CSN_H(); return r; }
static uint8_t NRF_ReadReg(uint8_t reg)  { CSN_L(); SPI_ReadWriteByte(0x00|reg); uint8_t v=SPI_ReadWriteByte(0xFF); CSN_H(); return v; }
static void NRF_WriteReg(uint8_t reg, uint8_t val) { CSN_L(); SPI_ReadWriteByte(0x20|reg); SPI_ReadWriteByte(val); CSN_H(); }
static void NRF_WriteBuf(uint8_t reg, uint8_t* buf, uint8_t len) { CSN_L(); SPI_ReadWriteByte(0x20|reg); for(uint8_t i=0;i<len;i++) SPI_ReadWriteByte(buf[i]); CSN_H(); }
static void NRF_ReadBuf(uint8_t reg, uint8_t* buf, uint8_t len) { CSN_L(); SPI_ReadWriteByte(0x00|reg); for(uint8_t i=0;i<len;i++) buf[i]=SPI_ReadWriteByte(0xFF); CSN_H(); }

void NRF24L01_Init(uint8_t mode)
{
    GPIO_InitTypeDef g;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);

    /* CE, CSN：推挽输出 */
    g.GPIO_Mode=GPIO_Mode_Out_PP; g.GPIO_Speed=GPIO_Speed_50MHz;
    g.GPIO_Pin=NRF_CE_PIN;  GPIO_Init(NRF_CE_PORT, &g);  CE_L();
    g.GPIO_Pin=NRF_CSN_PIN; GPIO_Init(NRF_CSN_PORT, &g); CSN_H();

    /* IRQ：浮空输入 */
    g.GPIO_Pin=NRF_IRQ_PIN; g.GPIO_Mode=GPIO_Mode_IN_FLOATING;
    GPIO_Init(NRF_IRQ_PORT, &g);

    Delay_ms(100);  /* 等模块上电稳定 */

    CE_L();
    /* 地址：5 字节（可自定义，收发双方必须一致）*/
    uint8_t addr[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
    NRF_WriteBuf(0x0A, addr, 5);  /* RX_ADDR_P0 */
    NRF_WriteBuf(0x10, addr, 5);  /* TX_ADDR */
    NRF_WriteReg(0x03, 0x03);     /* 地址宽度=5字节 */
    NRF_WriteReg(0x11, NRF_PAYLOAD_SIZE); /* RX_PW_P0=32 */
    NRF_WriteReg(0x01, 0x01);     /* 自动应答 Pipe0 */
    NRF_WriteReg(0x02, 0x01);     /* 使能 Pipe0 */
    NRF_WriteReg(0x04, 0x13);     /* 重发：500us+3次 */
    NRF_WriteReg(0x05, 40);       /* 频道40 */
    NRF_WriteReg(0x06, 0x07);     /* 1Mbps, 0dBm */

    if(mode == NRF_MODE_RX)
        NRF_WriteReg(0x00, 0x0F); /* CONFIG: RX模式 */
    else
        NRF_WriteReg(0x00, 0x0E); /* CONFIG: TX模式 */

    CE_H();  Delay_us(130);
}

uint8_t NRF_TxPacket(uint8_t* data)
{
    CE_L();
    /* 清发送 FIFO */
    CSN_L(); SPI_ReadWriteByte(0xE1); CSN_H();
    /* 写 32 字节 */
    CSN_L(); SPI_ReadWriteByte(0xA0);
    for(uint8_t i=0;i<NRF_PAYLOAD_SIZE;i++) SPI_ReadWriteByte(data[i]);
    CSN_H();
    /* CE 脉冲触发发送 */
    CE_H(); Delay_us(15); CE_L();
    /* 等 IRQ 拉低（发送完成）*/
    uint32_t tout=0; while(IRQ_READ()!=0){if(++tout>100000)return 1;}
    /* 读状态清标志 */
    uint8_t st=NRF_ReadReg(0x07); NRF_WriteReg(0x07,st);
    if(st & 0x20) return 0; /* TX_DS=成功 */
    if(st & 0x10){ CSN_L(); SPI_ReadWriteByte(0xE1); CSN_H(); } /* MAX_RT，清 FIFO */
    return 1;
}

uint8_t NRF_RxPacket(uint8_t* data)
{
    if(IRQ_READ() != 0) return 1;  /* 没数据 */
    uint8_t st = NRF_ReadReg(0x07);
    if(!(st & 0x40)) return 1;     /* 不是接收中断 */
    CSN_L(); SPI_ReadWriteByte(0x61);  /* 读 Payload */
    for(uint8_t i=0;i<NRF_PAYLOAD_SIZE;i++) data[i]=SPI_ReadWriteByte(0xFF);
    CSN_H();
    NRF_WriteReg(0x07, st);  /* 清中断 */
    return 0;
}

uint8_t NRF_DataReady(void) { return (IRQ_READ() == 0) ? 1 : 0; }
```

### 26.8.2 蓝牙 HC-05 `Hardware/bluetooth/bluetooth.h` 和 `Hardware/bluetooth/bluetooth.c`

**`Hardware/bluetooth/bluetooth.h`**：

```c
/**
 * @file    bluetooth.h
 * @brief   HC-05/HC-06 蓝牙模块（透传串口）
 * @note    依赖 usart 模块（USART1）
 *          HC-05 本质是无线串口——你发什么对方收什么
 *          
 *          【电赛修改指南】
 *          1. 首次使用先用 AT 模式配置（按住按键上电，LED 慢闪）
 *             AT 模式下波特率固定 38400
 *             常用 AT 指令：AT（测试）、AT+NAME=xxx（改名）、AT+UART=115200,0,0
 *          2. 本模板只封装数据收发的接口，
 *             蓝牙本身的配对连接不需要代码干预（自动完成）
 *          3. 连接成功：LED 常亮（快闪→常亮）
 *          4. 如果想用 USART2 接蓝牙（USART1 留作调试），
 *             修改 usart.c 代码（改 USART2 初始化）
 */
#ifndef __BLUETOOTH_H
#define __BLUETOOTH_H

#include "stm32f10x.h"

void Bluetooth_Init(uint32_t baudrate);      /* 初始化（同 USART1_Init）*/
void Bluetooth_SendByte(uint8_t dat);
void Bluetooth_SendString(char* str);
void Bluetooth_SendBuf(uint8_t* buf, uint16_t len);

/* 检查是否有接收到数据 */
#define Bluetooth_DataReady()   (usart_rx_flag)
/* 获取接收数据指针和长度（在 usart.h 中声明的全局变量）*/
extern volatile uint8_t  usart_rx_buf[];
extern volatile uint16_t usart_rx_len;

#endif
```

**`Hardware/bluetooth/bluetooth.c`**：

```c
/**
 * @file    bluetooth.c
 * @brief   蓝牙模块实现（透传封装）
 */
#include "bluetooth.h"
#include "usart.h"

void Bluetooth_Init(uint32_t baudrate)
{
    USART1_Init(baudrate);  /* 复用串口初始化 */
    /* 数据模式波特率通常设为 9600 或 115200 */
}

void Bluetooth_SendByte(uint8_t dat)    { USART1_SendByte(dat); }
void Bluetooth_SendString(char* str)    { USART1_SendString(str); }
void Bluetooth_SendBuf(uint8_t* buf, uint16_t len) { USART1_SendBuf(buf, len); }
```

---

## 26.9 代码模板使用速查表

| 模板           | 文件位置                | 依赖            | 功能                    |
| -------------- | ----------------------- | --------------- | ----------------------- |
| main.c         | `User/main.c`           | sys_tick, delay | 主循环 + 多任务调度     |
| sys_tick       | `User/sys_tick.c/.h`    | 无              | 1ms 系统时基            |
| delay          | `User/delay.c/.h`       | 无              | 微秒/毫秒精确延时       |
| stm32f10x_it.c | `User/stm32f10x_it.c`   | 无              | 中断服务函数集中定义    |
| LED            | `Hardware/led/`         | 无              | 状态指示灯              |
| KEY            | `Hardware/key/`         | sys_tick        | 单击/双击/长按检测      |
| USART          | `Hardware/usart/`       | 无              | printf + 中断接收 + DMA |
| I2C Soft       | `Hardware/i2c/`         | delay           | 软件模拟 I2C 主机       |
| SPI Soft       | `Hardware/spi/`         | delay           | 软件模拟 SPI 主机       |
| OLED           | `Hardware/oled/`        | i2c_soft        | SSD1306 128×64 显示     |
| HC-SR04        | `Hardware/hc_sr04/`     | sys_tick        | 超声波测距              |
| DS18B20        | `Hardware/ds18b20/`     | delay           | 数字温度传感器          |
| Motor DC       | `Hardware/motor_dc/`    | 无              | TB6612 直流电机驱动     |
| Servo          | `Hardware/servo/`       | 无              | SG90 舵机控制           |
| Encoder        | `Hardware/encoder/`     | 无              | 编码器测速/位置         |
| PID            | `Hardware/pid/`         | 无              | 位置式+增量式 PID       |
| Filter         | `Hardware/filter/`      | 无              | 限幅/平均/中值/低通     |
| Flash Store    | `Hardware/flash_store/` | 无              | 片内 Flash 掉电保存     |
| AT24C02        | `Hardware/at24c02/`     | i2c_soft        | EEPROM 频繁修改参数     |
| NRF24L01       | `Hardware/nrf24l01/`    | spi_soft        | 2.4G 无线通信           |
| Bluetooth      | `Hardware/bluetooth/`   | usart           | HC-05 蓝牙透传          |

### 竞赛中快速组装流程

```
第1步：复制 Libraries/ 到工程（标准固件库文件）
第2步：复制 User/ 下所有 .c 和 .h 文件
第3步：创建 Hardware/ 文件夹，按需复制模块
第4步：在 stm32f10x_conf.h 中取消注释要用到的外设头文件
第5步：在 main.c 中添加你的模块头文件 #include
第6步：在 main.c 的 System_Init() 中调用模块的 Init 函数
第7步：在 main.c 的 while(1) 中添加任务处理
第8步：在 stm32f10x_it.c 中添加中断服务函数
第9步：Keil 中 Add Existing Files，添加所有 .c 到对应 Group
第10步：Options → C/C++ → Include Paths 添加 User/ 和 Hardware/
第11步：Define 中添加 STM32F10X_MD,USE_STDPERIPH_DRIVER
第12步：勾选 MicroLIB（使用 printf 必须勾选）
第13步：编译（F7），下载（F8），调试！
```

---

**全文终。本文档共二十六章，附两个附录及完整的代码模板库，涵盖了 STM32 标准库编程、模拟电路设计、电源设计、传感器应用、步进电机与舵机控制、无线通信（蓝牙/WiFi/NRF24L01）、SD 卡与文件系统、EEPROM 与 Flash 存储、PCB 设计、仪器使用、报告撰写、竞赛策略与临场应对、以及可直接编译运行的代码模板等电赛一等奖所需的全部知识。**

**记住：模板只是起点，不是终点。理解每一行代码的含义，在面对不同题目时灵活修改——这才是电赛一等奖的真正秘诀。**

**祝你旗开得胜，凯旋而归！**

---

## 26.10 模板集成指南 —— 如何把所有模板组合成一个工程

本节是**最关键的一节**。代码模板分开看都没问题，但组合到一个工程中时会出现引脚冲突、定时器冲突、中断函数冲突。本节将给出完整的解决方案。

### 26.10.1 引脚分配冲突表

以下是各个模板默认使用的引脚，同一引脚只能用于一个功能：

| STM32 引脚 | 被哪些模板占用                                      | 冲突说明             | 解决方案                                                                   |
| ---------- | --------------------------------------------------- | -------------------- | -------------------------------------------------------------------------- |
| PA0        | Motor_DC(PWMA/TIM2_CH1)、HC-SR04(Trig)、DS18B20(DQ) | **三选一**           | 只用Motor_DC时PA0做PWM；用HC-SR04时Trig改其他引脚；用DS18B20时DQ改其他引脚 |
| PA1        | Motor_DC(PWMB/TIM2_CH1)、HC-SR04(Echo/TIM2_CH2)     | **二选一**           | HC-SR04的Echo必须用TIM2_CH2(PA1)，如果也要PWM用TIM2_CH3(PA2)替代           |
| PA2        | Servo(TIM2_CH3)、Motor_DC(STBY)                     | **二选一**           | STBY可以改任意GPIO；Servo固定PA2或重映射                                   |
| PA4        | SPI_Soft(CSN)                                       | NRF24L01 CSN         | 可以复用（NRF的CSN和SPI的CSN是同一个信号）                                 |
| PA5        | SPI_Soft(SCK)                                       | NRF24L01 SCK         | **可以共用**（所有SPI设备共用SCK/MOSI/MISO，CSN区分）                      |
| PA6        | SPI_Soft(MISO)、Encoder(TIM3_CH1)                   | **二选一**           | 用Encoder时放弃SPI的MISO（或改用其他引脚）                                 |
| PA7        | SPI_Soft(MOSI)、Encoder(TIM3_CH2)                   | **二选一**           | 用Encoder时放弃SPI的MOSI                                                   |
| PA9        | USART1(TX)                                          | 调试串口/蓝牙        | **可以共用**（但蓝牙的数据会混在printf输出中）                             |
| PA10       | USART1(RX)                                          | 调试串口/蓝牙        | **可以共用**                                                               |
| PB6/PB7    | I2C_Soft(SCL/SDA)                                   | OLED/MPU6050/AT24C02 | **可以共用**（所有I2C设备并联，地址不同即可）                              |
| PB0        | NRF24L01(CE)                                        | 其他GPIO             | 可以改任意空闲GPIO                                                         |
| PB3/PB4    | Motor_DC(AIN1/AIN2)                                 | 其他GPIO             | 可以改任意空闲GPIO                                                         |

### 26.10.2 定时器分配方案

STM32F103C8T6 有 TIM1/2/3/4，建议按以下方案分配：

| 定时器      | 推荐功能                 | 引脚                                     | 备注                       |
| ----------- | ------------------------ | ---------------------------------------- | -------------------------- |
| **TIM2**    | 直流电机 PWM + 舵机 PWM  | PA0(CH1电机)、PA1(CH2电机)、PA2(CH3舵机) | 三个通道可同时用，互不影响 |
| **TIM3**    | 编码器接口               | PA6(CH1)、PA7(CH2)                       | 4倍频编码器模式            |
| **TIM4**    | 步进电机脉冲 / 备用 PWM  | PB6(CH1)                                 | 如果不用步进电机可备用     |
| **TIM1**    | 备用（互补PWM/高级功能） | PA8/PA9/PA10/PA11                        | 一般不用，留作扩展         |
| **SysTick** | 系统时基                 | -                                        | 1ms 中断（专用，不冲突）   |

**关键要点**：
- TIM2 的 PWM 功能**不需要中断**——PWM 是硬件自动输出的。只有输入捕获、编码器模式才需要中断。
- 因此 Motor_DC + Servo 同时使用 TIM2 完全没问题。
- 但如果再加 HC-SR04（TIM2 CH2 输入捕获），就需要开启 TIM2 中断，且必须在同一个 `TIM2_IRQHandler` 中合并处理。

### 26.10.3 推荐工程组合方案

**方案一：控制类题目（小车/平衡车）**
```
必选模板：sys_tick + delay + LED + KEY + USART(调试) + OLED
核心模板：Motor_DC + Encoder + PID + Filter
可选模板：MPU6050(I2C) + HC-SR04 + NRF24L01(SPI) + Servo + Flash_Store
定时器分配：TIM2=PWM+舵机, TIM3=编码器
引脚注意：PA0/PA1=电机PWM, PA6/PA7=编码器, PB6/PB7=I2C
```

**方案二：仪器类题目（信号采集/测量）**
```
必选模板：sys_tick + delay + LED + KEY + USART(调试) + OLED
核心模板：ADC多通道 + Filter + DS18B20 + AT24C02(I2C)
可选模板：HC-SR04 + Flash_Store + Bluetooth
定时器分配：TIM2=备用, TIM3=输入捕获测频, TIM4=备用
引脚注意：PA0=DS18B20, PB6/PB7=I2C, PA9/PA10=USART1
```

**方案三：无线遥控/通信题目**
```
必选模板：sys_tick + delay + LED + USART + OLED
核心模板：NRF24L01(SPI) + Bluetooth(USART) + KEY
可选模板：Motor_DC + Servo + Encoder + PID
定时器分配：TIM2=PWM+舵机, TIM3=编码器, TIM4=步进电机
引脚注意：PA4/PA5/PA6/PA7=SPI, PB6/PB7=I2C(OLED), PA9/PA10=蓝牙
```

### 26.10.4 中断函数合并模板

当多个模块需要同一个中断号时，必须在**同一个**中断函数中合并所有处理逻辑。以下是一个合并 `TIM2_IRQHandler` 的完整示例：

```c
/**
 * @brief  TIM2 中断服务函数（合并版）
 *         同时支持：HC-SR04 超声波（捕获中断）+ 通用定时任务（更新中断）
 *         放在 stm32f10x_it.c 中
 */
void TIM2_IRQHandler(void)
{
    /* ===== 第1部分：HC-SR04 超声波捕获（如果使用）===== */
    /* 注意：需要 extern 声明在 hc_sr04.c 中定义的变量 */
    // extern volatile uint8_t  echo_state;
    // extern volatile uint32_t echo_time;
    // ...

    /* 溢出中断：HC-SR04 用 + 通用定时任务用 */
    if(TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        /* HC-SR04 溢出计数 */
        // if(echo_state == 1) { echo_ovf_cnt++; ... }

        /* 通用定时任务（如 10ms 控制周期标志）*/
        // control_10ms_flag = 1;

        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }

    /* 捕获中断：HC-SR04 专用 */
    if(TIM_GetITStatus(TIM2, TIM_IT_CC2) != RESET)
    {
        /* HC-SR04 捕获处理 */
        TIM_ClearITPendingBit(TIM2, TIM_IT_CC2);
    }

    /* 捕获中断 CH1（如果用了其他输入捕获功能）*/
    if(TIM_GetITStatus(TIM2, TIM_IT_CC1) != RESET)
    {
        TIM_ClearITPendingBit(TIM2, TIM_IT_CC1);
    }
}
```

### 26.10.5 编译前检查清单

将模板整合到一个工程后，按此清单逐项检查：

- [ ] `stm32f10x_conf.h` 中取消注释了所有需要的外设头文件
- [ ] Keil → Options → C/C++ → Define 包含了 `STM32F10X_MD,USE_STDPERIPH_DRIVER`
- [ ] Keil → Options → C/C++ → Include Paths 包含了 `User`, `Hardware`, `Libraries/CMSIS`, `Libraries/FWlib/inc`
- [ ] Keil → Options → Target → 勾选 `Use MicroLIB`（如果使用 printf）
- [ ] Keil → Options → Debug → 选择 `ST-Link Debugger`
- [ ] 所有 `.c` 文件已添加到 Keil 工程的对应 Group
- [ ] 启动文件是 `startup_stm32f10x_md.s`（中容量，64KB Flash）
- [ ] 没有重复定义的中断服务函数（在多个 `.c` 文件中搜索同名 ISR）
- [ ] 引脚分配无冲突（对照 26.10.1 冲突表）
- [ ] 定时器分配无冲突（对照 26.10.2 分配方案）
- [ ] `system_stm32f10x.c` 中 `SystemCoreClock` = 72000000
- [ ] 编译输出 `0 Error(s), 0 Warning(s)`

---

## 26.11 补充模板：步进电机 / CAN 总线 / 状态机

### 26.11.1 步进电机 A4988 `Hardware/motor_stepper/motor_stepper.h` 和 `Hardware/motor_stepper/motor_stepper.c`

**`Hardware/motor_stepper/motor_stepper.h`**：

```c
/**
 * @file    motor_stepper.h
 * @brief   步进电机驱动模块（A4988/DRV8825 驱动器）
 * @note    只需两根控制线：STEP（脉冲）和 DIR（方向）
 *          STEP 使用 TIM4_CH1(PB6) 定时器翻转模式产生精确脉冲
 *          DIR 使用 PB7 普通 GPIO
 *          
 *          【电赛修改指南】
 *          1. 修改细分 MS1~MS3 的硬件跳线（或接 GPIO 动态控制）
 *          2. 修改 STEPS_PER_REV 为你的细分×200（1.8°步进电机）
 *             全步=200, 1/2=400, 1/4=800, 1/8=1600, 1/16=3200
 *          3. 修改 STEP_PORT/PIN 和 DIR_PORT/PIN
 *          4. 步进电机电源 VMOT 必须独立供电（12V/24V）！
 *          5. A4988 的 VDD 可接 3.3V（逻辑电平与STM32兼容）
 *          6. 电机运转前确保已调好 A4988 上的电流限制电位器
 */
#ifndef __MOTOR_STEPPER_H
#define __MOTOR_STEPPER_H

#include "stm32f10x.h"

/* ===== 引脚配置 ===== */
#define STEPPER_STEP_PORT  GPIOB
#define STEPPER_STEP_PIN   GPIO_Pin_6   /* TIM4_CH1 */
#define STEPPER_DIR_PORT   GPIOB
#define STEPPER_DIR_PIN    GPIO_Pin_7
#define STEPPER_CLK_GPIO   RCC_APB2Periph_GPIOB

/* ===== 每圈脉冲数（按你的细分修改！）===== */
#define STEPS_PER_REV     3200  /* 1/16细分：200×16=3200 */

void Stepper_Init(void);
void Stepper_SetSpeed(int16_t rpm);      /* 设置转速（转/分），正=正转 */
void Stepper_Stop(void);                 /* 停止（电机保持锁定）*/
void Stepper_Disable(void);              /* 释放电机（可自由转动）*/

#endif
```

**`Hardware/motor_stepper/motor_stepper.c`**：

```c
/**
 * @file    motor_stepper.c
 * @brief   步进电机驱动实现
 */
#include "motor_stepper.h"

void Stepper_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef  TIM_OCInitStructure;

    RCC_APB2PeriphClockCmd(STEPPER_CLK_GPIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

    /* DIR 引脚：推挽输出 */
    GPIO_InitStructure.GPIO_Pin   = STEPPER_DIR_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(STEPPER_DIR_PORT, &GPIO_InitStructure);
    GPIO_SetBits(STEPPER_DIR_PORT, STEPPER_DIR_PIN); /* 默认正转 */

    /* STEP 引脚：复用推挽（TIM4_CH1）*/
    GPIO_InitStructure.GPIO_Pin  = STEPPER_STEP_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(STEPPER_STEP_PORT, &GPIO_InitStructure);

    /* TIM4 输出比较翻转模式 */
    TIM_TimeBaseStructure.TIM_Period    = 65535;  /* ARR 最大值 */
    TIM_TimeBaseStructure.TIM_Prescaler = 0;      /* 不分频 */
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

    TIM_OCInitStructure.TIM_OCMode      = TIM_OCMode_Toggle; /* 翻转模式！*/
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse       = 32767;  /* 50% 占空比 */
    TIM_OCInitStructure.TIM_OCPolarity  = TIM_OCPolarity_High;
    TIM_OC1Init(TIM4, &TIM_OCInitStructure);

    /* 初始停止 */
    TIM_Cmd(TIM4, DISABLE);
}

/**
 * @brief  设置步进电机转速
 * @param  rpm: 转/分钟（正=正转, 负=反转, 0=停止）
 *         公式：f_step = rpm × STEPS_PER_REV / 60
 *               ARR = 72MHz / (2 × f_step) - 1
 */
void Stepper_SetSpeed(int16_t rpm)
{
    if(rpm == 0)
    {
        Stepper_Stop();
        return;
    }

    /* 方向 */
    if(rpm > 0)
        GPIO_SetBits(STEPPER_DIR_PORT, STEPPER_DIR_PIN);
    else
    {
        GPIO_ResetBits(STEPPER_DIR_PORT, STEPPER_DIR_PIN);
        rpm = -rpm;
    }

    /* 计算 ARR */
    uint32_t f_step = (uint32_t)rpm * STEPS_PER_REV / 60;
    if(f_step < 10) f_step = 10;     /* 最低频率限制 */
    uint16_t arr = 72000000UL / (2 * f_step) - 1;
    if(arr > 65535) arr = 65535;
    if(arr < 10) arr = 10;

    TIM_SetAutoreload(TIM4, arr);
    TIM_SetCompare1(TIM4, arr / 2);  /* 保持 50% 占空比 */
    TIM_Cmd(TIM4, ENABLE);
}

void Stepper_Stop(void)
{
    TIM_Cmd(TIM4, DISABLE);  /* 停脉冲，电机保持锁定（有保持电流）*/
}

void Stepper_Disable(void)
{
    TIM_Cmd(TIM4, DISABLE);
    /*【可选】如果 ENABLE 引脚接了 GPIO，拉高它来释放电机 */
}
```

### 26.11.2 CAN 总线 `Hardware/can/can.h` 和 `Hardware/can/can.c`

**`Hardware/can/can.h`**：

```c
/**
 * @file    can.h
 * @brief   CAN 总线通信模块
 * @note    STM32F103 CAN1：PA11=RX, PA12=TX（挂在 APB1，36MHz）
 *          需要外部 CAN 收发器（TJA1050/SN65HVD230）！
 *          不能把 PA11/PA12 直接当 CANH/CANL 用！
 *          
 *          【电赛修改指南】
 *          1. CAN_BAUDRATE：改波特率（常用 500k/250k/125k）
 *             波特率 = 36MHz / (BRP+1) / (1+BS1+BS2)
 *             500k(近似)：BRP=3, BS1=13, BS2=4 → 1bit=20Tq→36M/4/18=500k
 *          2. CAN_FILTER_ID：改你要接收的 CAN ID（标准帧11位）
 *          3. 如需接收全部 ID，将 FilterMask 全设为 0
 *          4. 调试时先用回环模式 CAN_Mode_LoopBack（自收自发，无需收发器）
 */
#ifndef __CAN_H
#define __CAN_H

#include "stm32f10x.h"

/* ===== CAN 配置 ===== */
#define CAN_BAUDRATE   500   /* 波特率（kbps）*/
#define CAN_FILTER_ID  0x321 /* 接收滤波器 ID（标准帧 0x000~0x7FF）*/

void CAN1_Init(void);
uint8_t CAN_SendData(uint16_t std_id, uint8_t* data, uint8_t len);
/* 接收通过中断，在 USB_LP_CAN1_RX0_IRQHandler 中处理 */
extern volatile uint8_t  can_rx_flag;
extern volatile uint16_t can_rx_id;
extern volatile uint8_t  can_rx_data[8];
extern volatile uint8_t  can_rx_len;

#endif
```

**`Hardware/can/can.c`**：

```c
/**
 * @file    can.c
 * @brief   CAN 总线驱动实现
 */
#include "can.h"

volatile uint8_t  can_rx_flag = 0;
volatile uint16_t can_rx_id   = 0;
volatile uint8_t  can_rx_data[8];
volatile uint8_t  can_rx_len  = 0;

void CAN1_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    CAN_InitTypeDef   CAN_InitStructure;
    CAN_FilterInitTypeDef CAN_FilterInitStructure;
    NVIC_InitTypeDef  NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);

    /* PA11=CAN_RX（上拉输入）, PA12=CAN_TX（复用推挽）*/
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    CAN_DeInit(CAN1);
    CAN_InitStructure.CAN_TTCM = DISABLE;
    CAN_InitStructure.CAN_ABOM = ENABLE;
    CAN_InitStructure.CAN_AWUM = ENABLE;
    CAN_InitStructure.CAN_NART = DISABLE;
    CAN_InitStructure.CAN_RFLM = DISABLE;
    CAN_InitStructure.CAN_TXFP = DISABLE;
    CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;  /* 调试时改 LoopBack */
    CAN_InitStructure.CAN_SJW = CAN_SJW_1tq;
    CAN_InitStructure.CAN_BS1 = CAN_BS1_13tq;  /* 500kbps 参数 */
    CAN_InitStructure.CAN_BS2 = CAN_BS2_4tq;
    CAN_InitStructure.CAN_Prescaler = 3;
    CAN_Init(CAN1, &CAN_InitStructure);

    /* 滤波器：只接收指定 ID */
    CAN_FilterInitStructure.CAN_FilterNumber = 0;
    CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;
    CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;
    CAN_FilterInitStructure.CAN_FilterIdHigh = CAN_FILTER_ID << 5;
    CAN_FilterInitStructure.CAN_FilterIdLow  = 0x0000;
    CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x7FF << 5; /* 11位全匹配 */
    CAN_FilterInitStructure.CAN_FilterMaskIdLow  = 0x0000;
    CAN_FilterInitStructure.CAN_FilterFIFOAssignment = CAN_Filter_FIFO0;
    CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
    CAN_FilterInit(&CAN_FilterInitStructure);

    /* NVIC：CAN RX0 中断（与 USB 共用中断号！名称为 USB_LP_CAN1_RX0_IRQn）*/
    NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority       = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd               = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    CAN_ITConfig(CAN1, CAN_IT_FMP0, ENABLE);
}

uint8_t CAN_SendData(uint16_t std_id, uint8_t* data, uint8_t len)
{
    CanTxMsg TxMessage;
    if(len > 8) len = 8;
    TxMessage.StdId = std_id;
    TxMessage.ExtId = 0;
    TxMessage.IDE   = CAN_Id_Standard;
    TxMessage.RTR   = CAN_RTR_Data;
    TxMessage.DLC   = len;
    for(uint8_t i = 0; i < len; i++) TxMessage.Data[i] = data[i];

    uint8_t mbox = CAN_Transmit(CAN1, &TxMessage);
    if(mbox == CAN_NO_MB) return 1;

    uint32_t tout = 0;
    while(CAN_TransmitStatus(CAN1, mbox) == CAN_TxStatus_Pending)
        if(++tout > 100000) return 1;
    return 0;
}

/* CAN 接收中断（放在 stm32f10x_it.c）*/
/* 函数名必须是 USB_LP_CAN1_RX0_IRQHandler（不是 CAN1_RX0_IRQHandler！）*/
void USB_LP_CAN1_RX0_IRQHandler(void)
{
    if(CAN_GetITStatus(CAN1, CAN_IT_FMP0) != RESET)
    {
        CanRxMsg RxMessage;
        CAN_Receive(CAN1, CAN_FIFO0, &RxMessage);
        can_rx_id   = RxMessage.StdId;
        can_rx_len  = RxMessage.DLC;
        for(uint8_t i = 0; i < RxMessage.DLC; i++)
            can_rx_data[i] = RxMessage.Data[i];
        can_rx_flag = 1;
        CAN_ClearITPendingBit(CAN1, CAN_IT_FMP0);
    }
}
```

### 26.11.3 状态机框架 `Hardware/state_machine/state_machine.h`

**`Hardware/state_machine/state_machine.h`**：

```c
/**
 * @file    state_machine.h
 * @brief   通用状态机框架
 * @note    这是一个轻量级的状态机框架，不需要额外的 .c 文件。
 *          直接在你的模块（如 car_control.c）中使用即可。
 *          
 *          【电赛使用方式】
 *          1. 用 typedef enum 定义你的状态列表
 *          2. 定义状态机变量：static YourState_t state = STATE_INIT;
 *          3. 在主循环中写 switch(state) { case... }
 *          4. 每个 case 中判断跳转条件，满足就 state = NEW_STATE;
 *          5. 如果状态很多（>8个），建议每个状态写一个处理函数
 *          
 *          示例见下方注释。
 */
#ifndef __STATE_MACHINE_H
#define __STATE_MACHINE_H

#include "stm32f10x.h"

/* ===== 通用状态机结构体 ===== */
typedef struct {
    uint8_t current_state;     /* 当前状态 */
    uint8_t next_state;        /* 下一状态（用于延迟切换）*/
    uint32_t state_enter_time; /* 进入当前状态的时刻（ms）*/
    uint32_t state_timeout;    /* 状态超时时间（ms，0=不超时）*/
    uint8_t state_changed;     /* 状态刚切换标志（可用作初始化新状态）*/
} StateMachine_t;

void SM_Init(StateMachine_t* sm, uint8_t init_state);
void SM_Update(StateMachine_t* sm, uint32_t sys_time);
void SM_ChangeState(StateMachine_t* sm, uint8_t new_state);
uint32_t SM_StateElapsed(StateMachine_t* sm, uint32_t sys_time);

#endif
```

**`Hardware/state_machine/state_machine.c`**：

```c
/**
 * @file    state_machine.c
 * @brief   状态机辅助函数实现
 */
#include "state_machine.h"

void SM_Init(StateMachine_t* sm, uint8_t init_state)
{
    sm->current_state   = init_state;
    sm->next_state      = init_state;
    sm->state_enter_time = 0;
    sm->state_timeout    = 0;
    sm->state_changed    = 1;
}

void SM_Update(StateMachine_t* sm, uint32_t sys_time)
{
    if(sm->next_state != sm->current_state)
    {
        sm->current_state    = sm->next_state;
        sm->state_enter_time = sys_time;
        sm->state_changed    = 1;
    }
    else
    {
        sm->state_changed = 0;
    }
}

void SM_ChangeState(StateMachine_t* sm, uint8_t new_state)
{
    sm->next_state = new_state;
}

uint32_t SM_StateElapsed(StateMachine_t* sm, uint32_t sys_time)
{
    return sys_time - sm->state_enter_time;
}

/* ===== 使用示例：自动往返小车状态机 =====
// 1. 定义状态枚举
typedef enum {
    CAR_STATE_INIT = 0,
    CAR_STATE_FORWARD,
    CAR_STATE_BACKWARD,
    CAR_STATE_TURN_LEFT,
    CAR_STATE_STOP,
} CarState_t;

// 2. 定义状态机变量
static CarState_t car_state = CAR_STATE_INIT;
static uint32_t  car_state_time = 0;

// 3. 在主循环中调用（每 10ms 或 20ms 一次）
void Car_StateMachine(void)
{
    switch(car_state)
    {
        case CAR_STATE_INIT:
            Motor_Stop();
            car_state = CAR_STATE_FORWARD;
            car_state_time = sys_time;
            break;

        case CAR_STATE_FORWARD:
            MotorA_SetSpeed(50);
            MotorB_SetSpeed(50);
            // 条件跳转：前方 20cm 有障碍
            if(HCSR04_GetDistance() < 200)
            {
                car_state = CAR_STATE_BACKWARD;
                car_state_time = sys_time;
            }
            break;

        case CAR_STATE_BACKWARD:
            MotorA_SetSpeed(-50);
            MotorB_SetSpeed(-50);
            // 条件跳转：后退 1 秒后左转
            if(sys_time - car_state_time > 1000)
            {
                car_state = CAR_STATE_TURN_LEFT;
                car_state_time = sys_time;
            }
            break;

        case CAR_STATE_TURN_LEFT:
            MotorA_SetSpeed(-30);
            MotorB_SetSpeed(30);
            if(sys_time - car_state_time > 500)
            {
                car_state = CAR_STATE_FORWARD;
            }
            break;

        case CAR_STATE_STOP:
            Motor_Stop();
            break;
    }
}
*/
```

---

## 26.12 代码模板最终索引

| 序号 | 模板         | 文件                                              | 关键引脚                         | 使用定时器 | 中断需求                   |
| ---- | ------------ | ------------------------------------------------- | -------------------------------- | ---------- | -------------------------- |
| 0    | 工程基础     | main.c, sys_tick.c/.h, delay.c/.h, stm32f10x_it.c | -                                | SysTick    | SysTick_Handler            |
| 1    | LED          | led.c/.h                                          | PC13                             | -          | -                          |
| 2    | KEY          | key.c/.h                                          | PA0                              | -          | -                          |
| 3    | Buzzer       | buzzer.c/.h                                       | PB8                              | -          | -                          |
| 4    | USART        | usart.c/.h                                        | PA9(TX),PA10(RX)                 | -          | USART1_IRQHandler          |
| 5    | I2C Soft     | i2c_soft.c/.h                                     | PB6(SCL),PB7(SDA)                | -          | -                          |
| 6    | SPI Soft     | spi_soft.c/.h                                     | PA5(SCK),PA7(MOSI),PA6(MISO)     | -          | -                          |
| 7    | OLED         | oled_i2c.c/.h                                     | (复用I2C)                        | -          | -                          |
| 8    | HC-SR04      | hc_sr04.c/.h                                      | PA0(Trig),PA1(Echo)              | TIM2_CH2   | TIM2_IRQHandler            |
| 9    | DS18B20      | ds18b20.c/.h                                      | PA0(DQ)                          | -          | -                          |
| 10   | Motor DC     | motor_dc.c/.h                                     | PA0(PWMA),PA1(PWMB),PB3/PB4(AIN) | TIM2       | -                          |
| 11   | Servo        | servo.c/.h                                        | PA2(TIM2_CH3)                    | TIM2       | -                          |
| 12   | Encoder      | encoder.c/.h                                      | PA6(TIM3_CH1),PA7(TIM3_CH2)      | TIM3       | -                          |
| 13   | PID          | pid.c/.h                                          | -                                | -          | -                          |
| 14   | Filter       | filter.c/.h                                       | -                                | -          | -                          |
| 15   | Flash Store  | flash_store.c/.h                                  | (内部Flash)                      | -          | -                          |
| 16   | AT24C02      | at24c02.c/.h                                      | (复用I2C)                        | -          | -                          |
| 17   | NRF24L01     | nrf24l01.c/.h                                     | PB0(CE),PA4(CSN),PB1(IRQ)        | -          | -                          |
| 18   | Bluetooth    | bluetooth.c/.h                                    | (复用USART1)                     | -          | USART1_IRQHandler          |
| 19   | Stepper      | motor_stepper.c/.h                                | PB6(STEP),PB7(DIR)               | TIM4       | -                          |
| 20   | CAN          | can.c/.h                                          | PA11(RX),PA12(TX)                | CAN1       | USB_LP_CAN1_RX0_IRQHandler |
| 21   | StateMachine | state_machine.h/.c                                | -                                | -          | -                          |

---

**全文终。本文档共二十六章、两份附录、二十一个即用代码模板，以及完整的模板集成指南、引脚分配冲突表、定时器分配方案和中断合并模板。**

**现在，你已经拥有电赛一等奖所需的一切理论知识和工程武器。将这份文档中的知识内化为你的能力，将代码模板在开发板上跑通，在赛场上稳定发挥——这就是通往一等奖的唯一道路。**

**祝2026电赛，你，就是冠军！**
