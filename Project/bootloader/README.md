# AT32 Bootloader

这是一个高可移植性的AT32 bootloader实现，bootloader位于flash的开头（从0KB开始），应用程序位于中间，参数区在最后1KB。

## 架构设计

### 1. 硬件抽象接口层（HW Interface Layer）

**核心文件：**
- `bootloader/inc/hw_interface.h` - 硬件抽象接口定义
- `bootloader/src/hw_interface_at32.c` - AT32平台的具体实现

**设计理念：**
- 所有硬件相关的操作都通过 `hw_interface.h` 定义的接口进行
- bootloader的核心逻辑不直接调用AT32硬件库，只调用接口函数
- 移植到其他平台时，只需实现 `hw_interface_xxx.c` 文件

**主要接口：**
- Flash操作（擦除、编程、读取）
- 系统操作（初始化、复位、延时、跳转）
- 通信接口（UART/USB等，待实现）
- GPIO操作（可选，用于状态指示）

### 2. Bootloader核心层

**核心文件：**
- `bootloader/inc/bootloader.h` - Bootloader主头文件
- `bootloader/src/bootloader.c` - Bootloader核心实现

**主要功能：**
- 应用验证：检查应用程序是否有效
- 应用跳转：跳转到应用程序执行
- Bootloader入口控制：通过RAM中的magic number控制是否进入bootloader

**启动流程：**
1. Bootloader启动
2. 检查应用程序是否有效
3. 检查是否有bootloader入口请求
4. 如果应用有效且无入口请求，直接跳转到应用
5. 否则，进入bootloader模式进行固件更新

### 3. 下载功能层（Download Layer）

**核心文件：**
- `bootloader/inc/download.h` - 下载功能接口定义
- `bootloader/src/download.c` - 下载功能实现框架

**设计说明：**
- 目前只定义了接口，具体实现待完成
- 支持进度回调和错误回调
- 支持固件验证功能

**待实现的接口：**
- 通信协议解析（如YMODEM、XMODEM或自定义协议）
- 数据接收和写入flash
- 固件验证（CRC、校验和等）

### 4. 应用入口辅助

**核心文件：**
- `bootloader/inc/bootloader_entry.h` - 应用跳转到bootloader的辅助函数

**使用方法：**
```c
#include "bootloader_entry.h"

// 在应用程序中调用此函数，系统会复位并进入bootloader
bootloader_request_entry();
```

## 内存布局

### Flash布局（128KB总大小）

```
0x08000000 - 0x0800FFFF (64KB):  Bootloader区域（开头）
0x08010000 - 0x0801FBFF (63KB):  应用程序区域（中间）
0x0801FC00 - 0x0801FFFF (1KB):   参数存储区域（最后）
```

### RAM布局（16KB总大小）

```
0x20000000 - 0x20003FFB: 应用程序和Bootloader共享RAM
0x20003FFC - 0x20003FFF: Bootloader控制区域（4字节）
```

**Bootloader控制区域：**
- `0x20003FFC`: Magic number（0xDEADBEEF = 请求进入bootloader）

## 链接脚本

**文件：** `bootloader/AT32M416xB_BOOTLOADER.ld`

Bootloader使用专用的链接脚本，将代码定位到flash的开头（从0x08000000开始，64KB大小）。

## 编译和烧录

### 编译Bootloader

使用bootloader专用的链接脚本编译：

```bash
# 需要修改CMakeLists.txt，使用bootloader/AT32M416xB_BOOTLOADER.ld作为链接脚本
```

### 应用程序配置

应用程序需要：
1. 修改应用程序的链接脚本，将应用程序区域设置为0x08010000-0x0801FBFF（63KB）
2. 应用程序的向量表应该位于0x08010000（应用程序起始地址）
3. 包含 `bootloader_entry.h` 以便能够跳转到bootloader
4. 注意：应用程序起始地址现在是0x08010000，不是0x08000000

## 使用流程

### 正常工作流程

1. 系统上电
2. Bootloader启动，检查应用程序
3. 如果应用程序有效且无bootloader请求，直接跳转到应用
4. 应用程序正常运行

### 固件更新流程

1. 应用程序调用 `bootloader_request_entry()` 请求进入bootloader
2. 系统复位
3. Bootloader检测到入口请求，不跳转到应用
4. Bootloader进入下载模式，等待固件数据
5. 通过通信接口接收固件数据并写入flash
6. 固件验证通过后，系统复位运行新固件

### 调试模式

由于bootloader位于flash开头，应用程序位于中间，参数区在最后，可以：
- 正常调试应用程序，bootloader位于开头不会影响调试
- 应用程序可以单独更新，bootloader保持不变
- 参数区独立存储，不会在固件更新时被擦除
- 如果应用程序损坏，bootloader仍然可以工作，允许固件恢复

## 待完成的功能

### 1. 下载功能具体实现

- [ ] 通信协议实现（建议实现YMODEM或自定义协议）
- [ ] UART通信接口实现
- [ ] USB通信接口实现（可选）
- [ ] CAN通信接口实现（可选）
- [ ] 固件数据接收和写入
- [ ] 固件验证（CRC32校验）

### 2. 错误处理

- [ ] 下载超时处理
- [ ] 写入失败重试机制
- [ ] 错误信息反馈（LED指示或串口输出）

### 3. 安全功能（可选）

- [ ] 固件签名验证
- [ ] 加密固件支持
- [ ] 固件回滚功能

## 移植指南

移植到其他平台只需：

1. 实现 `hw_interface.h` 中定义的所有接口
2. 创建新的实现文件，如 `hw_interface_stm32.c`、`hw_interface_nxp.c` 等
3. 修改链接脚本，设置正确的flash起始地址和大小
4. 修改 `bootloader.h` 中的地址定义

**注意：** Bootloader核心逻辑不需要修改，只需替换硬件接口实现即可。

## 注意事项

1. **Flash保护：** 确保bootloader区域不会被应用程序意外擦除
2. **RAM共享：** 应用程序和bootloader共享RAM，需要注意数据保护
3. **中断向量：** Bootloader有自己的中断向量表，跳转到应用前需要重置
4. **时钟配置：** 确保bootloader和应用使用兼容的时钟配置

## 许可证

（根据项目许可证添加）