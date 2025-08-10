## 把Arduboy3D这个游戏移植到了STM32F411CEU6上，使用HAL库开发，方便移植到其它STM32单片机上。

## 演示
视频链接：[STM32 移植Arduboy3D](https://www.bilibili.com/video/BV1k1t8z3EAp/?spm_id_from=333.1387.homepage.video_card.click&vd_source=11274a63bea6fda976e2d7fdf7576ae3)

## 编译和烧录
使用Openocd烧录代码，下载器我选择的是STLink V2，请根据自己的情况选择正确的下载器。

打开终端后输入下面命令： 
编译：  
`cube-cmake --build ./build/Debug --`  
烧录：  
`openocd -f interface/stlink-v2.cfg -f target/stm32f4x.cfg -c 'program build/Debug/Arduboy3DGame.elf verify reset exit' -c 'adapter speed 2000'`  

源项目：https://github.com/jhhoward/Arduboy3D
