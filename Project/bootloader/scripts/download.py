import serial
import argparse
import time
from tqdm import tqdm
def generate_crc32(data):
    # CRC-32 多项式
    polynomial = 0xEDB88320
    
    # 初始CRC值
    crc = 0xFFFFFFFF
    
    # 处理每个字节
    for byte in data:
        # 与当前字节异或
        crc ^= byte
        
        # 处理8位
        for _ in range(8):
            if crc & 1:  # 检查最低位是否为1
                crc = (crc >> 1) ^ polynomial
            else:
                crc >>= 1
    
    # 最终异或
    return crc ^ 0xFFFFFFFF
def read_bootloader_info(ser):
    frame = bytearray([0x04, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00])
    frame[4] = 1
    ser.write(frame)
    info = wait_info_response(ser)
    if info is not None:
        print(f'BOOTLOADER VERSION: {info[0]}.{info[1]}')
    frame[4] = 2
    ser.write(frame)
    info = wait_info_response(ser)
    if info is not None:
        series_id = 0
        for i in reversed(range(4)):
            series_id = (series_id << 8) | info[i]
        print(f'BOOTLODER PRODUCT SERIES: {hex(series_id)}')
    frame[4] = 3
    ser.write(frame)
    info = wait_info_response(ser)
    if info is not None:
        product_id = 0
        for i in reversed(range(4)):
            product_id = (product_id << 8) | info[i]
        print(f'BOOTLODER PRODUCT ID: {hex(product_id)}')
def send_header(ser, firmware_size, crc32=None):
    frame1 = bytearray([0x01, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00])
    frame2 = bytearray([0x01, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00])
    # little endian
    frame1[7] = (firmware_size >> 24) & 0xFF
    frame1[6] = (firmware_size >> 16) & 0xFF
    frame1[5] = (firmware_size >> 8) & 0xFF
    frame1[4] = firmware_size & 0xFF
    print(f'frame1: {hex(frame1[0]), hex(frame1[1]), hex(frame1[2]), hex(frame1[3]), hex(frame1[4]), hex(frame1[5]), hex(frame1[6]), hex(frame1[7])}')
    ser.write(frame1)
    wait_ack(ser)
    time.sleep(0.001)
    if crc32 is not None:
        frame2[7] = (crc32 >> 24) & 0xFF
        frame2[6] = (crc32 >> 16) & 0xFF
        frame2[5] = (crc32 >> 8) & 0xFF
        frame2[4] = crc32 & 0xFF
        print(f'frame2: {hex(frame2[0]), hex(frame2[1]), hex(frame2[2]), hex(frame2[3]), hex(frame2[4]), hex(frame2[5]), hex(frame2[6]), hex(frame2[7])}')

        ser.write(frame2)
        wait_ack(ser)
        time.sleep(0.001)
def send_data_chunk(ser, seq, data_chunk):
    length = len(data_chunk)
    frame = bytearray([0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00])
    frame[2] = (seq >> 8) & 0xFF
    frame[1] = seq & 0xFF
    frame[3] = length & 0xFF
    frame[4:] = data_chunk

    ser.write(frame)
    wait_ack(ser)
    # time.sleep(0.001)
def send_end(ser):
    time.sleep(0.01)
    frame = bytearray([0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00])
    ser.write(frame)

def wait_info_response(ser):
     while True:
        response = ser.read(8)
        if response and response[0] == 0x04:
            length = response[3]
            data = bytearray(response[4:4+length])
            return data
        else:
            return None
def wait_ack(ser):

    # wait for ACK and NAK, 8bytes start from 0x06 or 0x15
    while True:
        response = ser.read(8)
        if response and response[0] == 0x06 and all([i == 0x00 for i in response[3:]]):
            seq = (response[2] << 8) | response[1]
            # print(f'ACK seq: {seq}')
            return True
        elif response and response[0] == 0x15 and all([i == 0x00 for i in response[3:]]):
            seq = (response[2] << 8) | response[1]
            print(f'NAK seq: {seq}') 
            return False
        else:
            continue
def main():
    parser = argparse.ArgumentParser(description='Download firmware to STM32 bootloader')
    parser.add_argument('--port', type=str, help='Serial port name', default='COM10')
    parser.add_argument('--file', type=str, help='Firmware file name', default='D:\Embedded\stm32_bootloader\Project\\bootloader\scripts\stm32_bootloader_app.bin')
    parser.add_argument('--use_crc', type=bool, help='Use CRC32 checksum', default=True)
    args = parser.parse_args()
    ser= None
    ser = serial.Serial(args.port, 115200)
    data = None
    with open(args.file, 'rb') as f:
        data = f.read()

    crc = generate_crc32(data) if args.use_crc else None
    firmware_size = len(data)
    print(f'firmware_size: {firmware_size}')
    print(f'crc32: {crc}')
    read_bootloader_info(ser)
    send_header(ser, firmware_size, crc)
    print('header sent')
    # data chunk size is 4 bytes
    # send data chunk by chunk
    firmware_size_supp = firmware_size if firmware_size % 4 == 0 else firmware_size + (4 - firmware_size % 4)
    # append 0xFF to the end of data if firmware_size is not divisible by 4
    if firmware_size_supp != firmware_size:
        data += b'\xFF' * (firmware_size_supp - firmware_size)
    for i in tqdm(range(0, firmware_size_supp, 4)):
        # print(f'seq: {i//4}')
        send_data_chunk(ser, i//4, data[i:i+4])
    print('data sent')
    send_end(ser)
    print('end sent')
    ser.close()

if __name__ == '__main__':
    main()