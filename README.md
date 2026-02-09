# gs_usb2can

`gs_usb2can` 是一个基于 STM32G0 的 USB 转 CAN/CAN FD 固件项目，目标是兼容 Linux `gs_usb`（CandleLight）生态，让主机侧可直接按 SocketCAN 方式使用。

## 项目状态

- 应用层已实现 `gs_usb` 控制请求与 Bulk 数据收发框架
- 当前默认启用 2 路 CAN 控制器（`FDCAN1` + `FDCAN2`）
- Bootloader 独立工程已集成，并带有串口下载脚本

## 主要特性

- `gs_usb` 兼容协议（Vendor Class）
- 支持 Classic CAN 与 CAN FD（含 BRS 标志透传）
- 双通道 CAN（可在 `Project/app/gs_usb/gs_usb.h` 中调整）
- CMake + Ninja 构建，支持 `Debug/Release` 预设
- Bootloader 工程可输出 `.elf/.bin/.hex`

## 仓库结构

- `Project/app`：主应用（USB + gs_usb + FDCAN）
- `Project/bootloader`：Bootloader 与下载协议实现
- `cmake`：工具链与 CubeMX CMake 集成
- `Drivers` / `Core`：STM32 HAL/CMSIS 与生成代码
- `gs_usb2can.ioc`：STM32CubeMX 工程文件

## 开发环境

- CMake >= 3.22
- Ninja
- ARM GCC 工具链（`arm-none-eabi-gcc`，需在 `PATH` 中）

可选（Bootloader 下载脚本）：

- Python 3
- `pyserial`
- `tqdm`

## 构建

在仓库根目录执行：

```bash
cmake --preset Debug
cmake --build --preset Debug
```

Release 构建：

```bash
cmake --preset Release
cmake --build --preset Release
```

常见产物（以 Debug 为例）：

- `build/Debug/gs_usb2can_app.elf`
- `build/Debug/gs_usb2can_bootloader.elf`
- `build/Debug/gs_usb2can_bootloader.bin`
- `build/Debug/gs_usb2can_bootloader.hex`

## Bootloader 下载

脚本位置：`Project/bootloader/scripts/download.py`

示例：

```bash
python Project/bootloader/scripts/download.py --port COM10 --file path/to/app.bin --use_crc True
```

脚本依赖串口通信，协议帧为固定 8 字节格式，详细可参考：

- `Project/bootloader/DOWNLOAD_PROTOCOL.md`

## 主机侧（Linux）快速验证

设备枚举后（默认 VID/PID 为 `0x1D50:0x606F`），可按 SocketCAN 常用流程测试：

```bash
sudo ip link set can0 up type can bitrate 500000
candump can0
cansend can0 123#11223344
```

## 关键注意事项

- 当前 USB 字符串描述符中厂商名为 `OpenAI`，建议改成你自己的品牌信息：`Project/app/usb/usb_desc.c`
- 当前应用与 Bootloader 的链接脚本都还是 CubeMX 默认全 Flash 布局，若要稳定共存，需要按实际分区修改：
  - `Project/app/STM32G0B1XX_FLASH.ld`
  - `Project/bootloader/STM32G0B1XX_FLASH.ld`
- `USB_VID/USB_PID` 目前为 CandleLight 常见测试值（`0x1D50:0x606F`），正式产品请替换为合法 VID/PID：`Project/app/usb/usb_def.h`

## License

MIT，详见 `LICENSE`。
