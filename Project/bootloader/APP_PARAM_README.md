# 应用程序参数存储接口

## 概述

应用程序参数存储接口提供了在Flash最后1KB区域读写参数的功能。这个区域独立于应用程序和Bootloader，用于保存掉电不丢失的配置参数。

## Flash内存布局

```
0x08000000 - 0x0800FFFF (64KB):   Bootloader区域（开头）
0x08010000 - 0x0801FBFF (63KB):   应用程序区域（中间）
0x0801FC00 - 0x0801FFFF (1KB):    参数存储区域（最后）<-- 参数区
```

## 特性

- **掉电保存**：参数存储在Flash中，掉电不丢失
- **CRC校验**：自动进行CRC32校验，确保数据完整性
- **版本管理**：支持参数版本号，每次写入自动递增
- **结构体支持**：提供便捷的结构体读写接口
- **安全擦除**：支持完整的扇区擦除操作

## 接口说明

### 初始化/去初始化

```c
app_param_status_t app_param_init(void);
app_param_status_t app_param_deinit(void);
```

### 读取参数

```c
// 读取原始数据
app_param_status_t app_param_read(uint8_t *data, uint32_t max_length, uint32_t *actual_length);

// 读取结构体
app_param_status_t app_param_read_struct(void *param_struct, uint32_t struct_size);
```

### 写入参数

```c
// 写入原始数据
app_param_status_t app_param_write(const uint8_t *data, uint32_t length);

// 写入结构体
app_param_status_t app_param_write_struct(const void *param_struct, uint32_t struct_size);
```

### 其他操作

```c
// 检查参数是否有效
bool app_param_is_valid(void);

// 获取参数版本号
uint32_t app_param_get_version(void);

// 获取最大参数长度（不包括头部）
uint32_t app_param_get_max_length(void);

// 擦除参数区域
app_param_status_t app_param_erase(void);
```

## 使用示例

### 示例1：定义和使用参数结构体

```c
#include "app_param.h"

// 定义参数结构体
typedef struct
{
    uint32_t device_id;
    uint32_t baudrate;
    uint8_t  mode;
    float    calibration_value;
} app_params_t;

// 初始化并读取参数
void read_my_params(void)
{
    app_params_t params;
    app_param_status_t status;
    
    // 初始化参数存储
    app_param_init();
    
    // 读取参数
    status = app_param_read_struct(&params, sizeof(app_params_t));
    if (status == APP_PARAM_OK)
    {
        // 参数读取成功，使用参数
        printf("Device ID: %lu\n", params.device_id);
        printf("Baudrate: %lu\n", params.baudrate);
    }
    else if (status == APP_PARAM_CRC_ERROR)
    {
        // CRC错误，使用默认值
        params.device_id = 0x12345678;
        params.baudrate = 115200;
        params.mode = 0;
        params.calibration_value = 1.0f;
    }
    else
    {
        // 无有效参数，使用默认值
        params.device_id = 0x12345678;
        params.baudrate = 115200;
        params.mode = 0;
        params.calibration_value = 1.0f;
    }
    
    // 去初始化
    app_param_deinit();
}

// 写入参数
void write_my_params(void)
{
    app_params_t params;
    app_param_status_t status;
    
    // 准备参数
    params.device_id = 0x12345678;
    params.baudrate = 115200;
    params.mode = 1;
    params.calibration_value = 1.234f;
    
    // 初始化参数存储
    app_param_init();
    
    // 写入参数
    status = app_param_write_struct(&params, sizeof(app_params_t));
    if (status == APP_PARAM_OK)
    {
        // 参数写入成功
        printf("Parameters saved successfully\n");
    }
    else if (status == APP_PARAM_SIZE_TOO_LARGE)
    {
        // 参数结构体太大
        printf("Parameter structure is too large\n");
    }
    
    // 去初始化
    app_param_deinit();
}
```

### 示例2：读写原始数据

```c
#include "app_param.h"

void raw_data_example(void)
{
    uint8_t write_buffer[256] = "My parameter data";
    uint8_t read_buffer[256];
    uint32_t actual_length;
    app_param_status_t status;
    
    // 初始化
    app_param_init();
    
    // 写入数据
    status = app_param_write(write_buffer, sizeof(write_buffer));
    if (status == APP_PARAM_OK)
    {
        printf("Data written successfully\n");
    }
    
    // 读取数据
    status = app_param_read(read_buffer, sizeof(read_buffer), &actual_length);
    if (status == APP_PARAM_OK)
    {
        printf("Data read successfully, length: %lu\n", actual_length);
        printf("Data: %s\n", read_buffer);
    }
    
    // 去初始化
    app_param_deinit();
}
```

### 示例3：检查参数有效性

```c
#include "app_param.h"

void check_params(void)
{
    bool is_valid;
    uint32_t version;
    
    // 初始化
    app_param_init();
    
    // 检查参数是否有效
    is_valid = app_param_is_valid();
    if (is_valid)
    {
        // 参数有效，获取版本号
        version = app_param_get_version();
        printf("Parameters are valid, version: %lu\n", version);
    }
    else
    {
        printf("Parameters are invalid or not exist\n");
    }
    
    // 去初始化
    app_param_deinit();
}
```

## 参数存储格式

参数存储区域采用以下格式：

```
Offset  Size    Description
------  ----    -----------
0x00    4       Magic number (0x50415241, "PARA" in ASCII)
0x04    4       Version number (每次写入递增)
0x08    4       CRC32 checksum (参数数据的CRC32)
0x0C    4       Data length (参数数据长度，不包括头部)
0x10    16      Reserved (保留，填充0)
0x20    ...     Parameter data (实际参数数据)
```

## 注意事项

1. **最大数据长度**：参数数据最大长度为 `app_param_get_max_length()` 字节（通常约为960字节，扣除32字节头部后）

2. **Flash扇区擦除**：参数区域只有1KB，但Flash最小擦除单位是4KB扇区。擦除参数时会擦除包含参数区域的整个扇区，这可能影响扇区内的其他数据。因此参数区域应该独占一个扇区或位于扇区末尾。

3. **CRC校验**：写入时会自动计算并保存CRC32，读取时会自动验证。如果CRC不匹配，读取操作将返回 `APP_PARAM_CRC_ERROR`。

4. **版本管理**：每次写入参数时，版本号会自动递增。首次写入时版本号为1。

5. **初始化/去初始化**：使用参数存储接口前必须调用 `app_param_init()`，使用完后应调用 `app_param_deinit()`。

6. **线程安全**：当前实现不是线程安全的，如果多线程访问，需要添加互斥锁。

## 错误码说明

- `APP_PARAM_OK`: 操作成功
- `APP_PARAM_ERROR`: 一般错误
- `APP_PARAM_INVALID_PARAM`: 无效参数
- `APP_PARAM_SIZE_TOO_LARGE`: 数据太大，超出最大长度
- `APP_PARAM_NOT_INITIALIZED`: 未初始化
- `APP_PARAM_CRC_ERROR`: CRC校验失败，数据可能损坏

## 移植说明

参数存储接口使用了硬件抽象接口（`hw_interface.h`），因此可以轻松移植到其他平台。只需要实现相应的硬件接口即可。