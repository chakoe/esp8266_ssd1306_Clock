#!/usr/bin/env python3
"""
固件URL批量更新工具

用于批量更新多个ESP8266设备的固件URL配置
"""

import serial
import serial.tools.list_ports
import time
import sys

# 固件URL配置
FIRMWARE_URLS = {
    "v2.1.0": "https://github.com/chakoe/esp8266_ssd1306_Clock/releases/download/Releasev2.1.0/esp8266_ssd1306_Clock.ino.bin",
    "v2.2.0": "https://github.com/chakoe/esp8266_ssd1306_Clock/releases/download/Releasev2.2.0/esp8266_ssd1306_Clock.ino.bin",
    "v2.3.0": "https://github.com/chakoe/esp8266_ssd1306_Clock/releases/download/Releasev2.3.0/esp8266_ssd1306_Clock.ino.bin",
    # 添加更多版本...
}

# 使用CDN加速（推荐）
CDN_FIRMWARE_URLS = {
    "v2.1.0": "https://cdn.jsdelivr.net/gh/chakoe/esp8266_ssd1306_Clock@Releasev2.1.0/esp8266_ssd1306_Clock.ino.bin",
    "v2.2.0": "https://cdn.jsdelivr.net/gh/chakoe/esp8266_ssd1306_Clock@Releasev2.2.0/esp8266_ssd1306_Clock.ino.bin",
    "v2.3.0": "https://cdn.jsdelivr.net/gh/chakoe/esp8266_ssd1306_Clock@Releasev2.3.0/esp8266_ssd1306_Clock.ino.bin",
    # 添加更多版本...
}


def list_serial_ports():
    """列出所有可用的串口"""
    ports = serial.tools.list_ports.comports()
    print("\n可用的串口：")
    for i, port in enumerate(ports):
        print(f"  [{i}] {port.device} - {port.description}")
    return ports


def connect_to_port(port_name, baudrate=115200):
    """连接到指定串口"""
    try:
        ser = serial.Serial(port_name, baudrate, timeout=2)
        time.sleep(2)  # 等待串口稳定
        return ser
    except Exception as e:
        print(f"❌ 无法连接到 {port_name}: {e}")
        return None


def send_command(ser, command, wait_time=1):
    """发送命令到设备"""
    try:
        ser.write(command.encode())
        time.sleep(wait_time)

        # 读取响应
        response = ""
        while ser.in_waiting > 0:
            response += ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
            time.sleep(0.1)

        return response
    except Exception as e:
        print(f"❌ 发送命令失败: {e}")
        return ""


def check_ota_status(ser):
    """检查OTA状态"""
    print("\n📊 检查OTA状态...")
    response = send_command(ser, 'o', wait_time=2)

    if "OTA Status Information" in response:
        print("✓ OTA状态信息已获取")
        print(response)
        return True
    else:
        print("❌ 无法获取OTA状态")
        return False


def update_firmware_url(ser, new_url):
    """更新固件URL"""
    print(f"\n🔄 更新固件URL...")
    print(f"新URL: {new_url}")

    # 发送 'v' 命令进入URL配置模式
    response = send_command(ser, 'v', wait_time=1)

    # 发送新URL
    response = send_command(ser, new_url + '\n', wait_time=2)

    if "Firmware URL updated" in response or "URL updated" in response:
        print("✓ 固件URL更新成功")
        return True
    else:
        print("❌ 固件URL更新失败")
        print(f"响应: {response}")
        return False


def trigger_ota_update(ser):
    """触发OTA更新"""
    print("\n🚀 触发OTA更新...")
    response = send_command(ser, 'u', wait_time=5)

    if "OTA update started" in response or "OTA Update started" in response:
        print("✓ OTA更新已触发")
        print(response)

        # 等待更新完成
        print("⏳ 等待更新完成...")
        for i in range(60):  # 最多等待60秒
            time.sleep(1)
            if ser.in_waiting > 0:
                response = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
                print(response, end='')

                if "OTA Update successful" in response or "Update successful" in response:
                    print("\n✓ OTA更新成功！")
                    return True

        print("\n⏰ 超时，请检查设备状态")
        return False
    else:
        print("❌ OTA更新触发失败")
        print(f"响应: {response}")
        return False


def interactive_mode():
    """交互模式"""
    print("\n" + "="*60)
    print("  ESP8266 固件URL更新工具 - 交互模式")
    print("="*60)

    # 列出串口
    ports = list_serial_ports()
    if len(ports) == 0:
        print("❌ 未找到可用的串口")
        return

    # 选择串口
    try:
        port_index = int(input("\n请选择串口编号: "))
        if port_index < 0 or port_index >= len(ports):
            print("❌ 无效的串口编号")
            return
    except ValueError:
        print("❌ 无效的输入")
        return

    # 连接串口
    ser = connect_to_port(ports[port_index].device)
    if not ser:
        return

    print(f"\n✓ 已连接到 {ports[port_index].device}")

    # 主菜单
    while True:
        print("\n" + "="*60)
        print("  主菜单")
        print("="*60)
        print("  1. 检查OTA状态")
        print("  2. 更新固件URL")
        print("  3. 触发OTA更新")
        print("  4. 更新并触发OTA（一键操作）")
        print("  5. 列出可用版本")
        print("  0. 退出")
        print("="*60)

        choice = input("\n请选择操作: ")

        if choice == '1':
            check_ota_status(ser)

        elif choice == '2':
            print("\n可用版本：")
            for version, url in CDN_FIRMWARE_URLS.items():
                print(f"  {version}: {url}")

            version = input("\n请输入版本号（如v2.2.0）: ")
            if version in CDN_FIRMWARE_URLS:
                update_firmware_url(ser, CDN_FIRMWARE_URLS[version])
            else:
                print("❌ 无效的版本号")

        elif choice == '3':
            trigger_ota_update(ser)

        elif choice == '4':
            print("\n可用版本：")
            for version, url in CDN_FIRMWARE_URLS.items():
                print(f"  {version}: {url}")

            version = input("\n请输入版本号（如v2.2.0）: ")
            if version in CDN_FIRMWARE_URLS:
                if update_firmware_url(ser, CDN_FIRMWARE_URLS[version]):
                    trigger_ota_update(ser)
            else:
                print("❌ 无效的版本号")

        elif choice == '5':
            print("\n可用版本：")
            print("\nGitHub Releases:")
            for version, url in FIRMWARE_URLS.items():
                print(f"  {version}")
                print(f"    {url}")

            print("\nCDN加速（推荐）:")
            for version, url in CDN_FIRMWARE_URLS.items():
                print(f"  {version}")
                print(f"    {url}")

        elif choice == '0':
            print("\n👋 退出")
            ser.close()
            break

        else:
            print("❌ 无效的选择")


def batch_mode(port_list, version):
    """批量更新模式"""
    print("\n" + "="*60)
    print("  ESP8266 固件URL批量更新工具")
    print("="*60)

    if version not in CDN_FIRMWARE_URLS:
        print(f"❌ 无效的版本号: {version}")
        return

    firmware_url = CDN_FIRMWARE_URLS[version]
    print(f"\n目标版本: {version}")
    print(f"固件URL: {firmware_url}")
    print(f"\n将更新 {len(port_list)} 个设备\n")

    success_count = 0
    fail_count = 0

    for i, port_name in enumerate(port_list, 1):
        print(f"\n[{i}/{len(port_list)}] 处理设备: {port_name}")

        ser = connect_to_port(port_name)
        if not ser:
            print(f"❌ 无法连接到 {port_name}")
            fail_count += 1
            continue

        print(f"✓ 已连接到 {port_name}")

        # 检查状态
        check_ota_status(ser)

        # 更新URL
        if update_firmware_url(ser, firmware_url):
            # 触发更新
            if trigger_ota_update(ser):
                success_count += 1
            else:
                fail_count += 1
        else:
            fail_count += 1

        ser.close()
        time.sleep(2)  # 等待设备重启

    print("\n" + "="*60)
    print("  批量更新完成")
    print("="*60)
    print(f"✓ 成功: {success_count}")
    print(f"❌ 失败: {fail_count}")
    print(f"总计: {len(port_list)}")
    print("="*60)


def main():
    """主函数"""
    if len(sys.argv) > 1:
        # 命令行模式
        if sys.argv[1] == '--help' or sys.argv[1] == '-h':
            print("用法:")
            print("  python update_firmware_url.py                    # 交互模式")
            print("  python update_firmware_url.py <COM端口> <版本>   # 批量更新模式")
            print("\n示例:")
            print("  python update_firmware_url.py COM3 v2.2.0")
            print("  python update_firmware_url.py COM3 COM4 v2.2.0")
            sys.exit(0)

        # 批量更新模式
        if len(sys.argv) >= 3:
            ports = []
            version = sys.argv[-1]

            # 解析端口列表
            for arg in sys.argv[1:-1]:
                if arg.startswith('COM'):
                    ports.append(arg)

            if len(ports) > 0:
                batch_mode(ports, version)
            else:
                print("❌ 未指定有效的串口")
                print("使用 --help 查看帮助")
        else:
            print("❌ 参数不足")
            print("使用 --help 查看帮助")
    else:
        # 交互模式
        interactive_mode()


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n👋 用户中断")
        sys.exit(0)
    except Exception as e:
        print(f"\n❌ 发生错误: {e}")
        sys.exit(1)
