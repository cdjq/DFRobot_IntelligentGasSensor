# DFRobot_IntelligentGasSensor

- [English Version](./README.md)

DFRobot SEN07xx 智能气体传感器通过 Modbus RTU 输出气体种类、浓度、测量状态和时间戳。
本 Python 库适用于 Raspberry Pi，可读取 CH4、H2S、CO、O2 等气体数据，并支持修改从站地址、
修改通信波特率、控制探头休眠以及在一条 RS-485 总线上轮询多个传感器。<br>
驱动基于 `DFRobot_RTU.py`，支持 TTL UART 和自动收发方向的 UART 转 RS-485 模块。

## 产品链接（[https://www.dfrobot.com.cn](https://www.dfrobot.com.cn)）

    SKU：SEN0738 / SEN0739 / SEN0740 / SEN0741

## 目录

  * [概述](#概述)
  * [库安装](#库安装)
  * [方法](#方法)
  * [兼容性](#兼容性)
  * [历史](#历史)
  * [创作者](#创作者)

## 概述

本库将 SEN07xx 作为 Modbus RTU 从机进行通信。默认通信参数为 **9600 8N1**，默认从站
地址为 **1**，有效地址范围为 1～247。

主要功能：

* 读取气体类型、浓度、状态和可选时间戳；
* 将 32 位浓度原码转换为实际浮点浓度；
* 修改并保存 Modbus 从站地址；
* 修改传感器波特率并同步切换 Raspberry Pi 串口；
* 控制探头进入 RUN 或 SLEEP 状态；
* 使用多个对象或单个对象轮询多个 Modbus 从机。

读取成功后，数据保存在 `sensor.last_measure` 字典中，常用字段为 `gas_type`、
`concentration_raw`、`decimal_point`、`unit`、`timestamp` 和 `data_valid`。实际浓度可直接
通过 `get_concentration_float()` 获取。

例程：

| 文件 | 功能 |
| ---- | ---- |
| `01.change_device_address.py` | 修改并保存传感器地址 |
| `02.change_device_baudrate.py` | 修改并保存传感器波特率 |
| `03.probe_sleep_wake.py` | 探头休眠、唤醒和状态查询 |
| `04.read_gas_multi_slave.py` | 多对象共享串口读取多个从机 |
| `05.read_gas_multi_slave_one_instance.py` | 单对象切换地址读取多个从机 |
| `06.read_gas_uart.py` | 通过 UART 连续读取气体浓度 |

完整寄存器说明请参考
[REGISTERMAP_MODBUS_CN.md](../../docs/REGISTERMAP_MODBUS_CN.md)。

## 库安装

要使用这个库，首先将库下载到 Raspberry Pi，并安装 `pyserial`。本目录已经包含所需的
`DFRobot_RTU.py`。

```bash

git clone https://github.com/DFRobot/DFRobot_IntelligentGasSensor.git
```

打开例程文件夹并运行需要的例程。例如，运行 UART 读取例程：

```bash
cd DFRobot_IntelligentGasSensor/python/raspberrypi
python3 example/06.read_gas_uart.py
```

运行例程前，请按照文件顶部说明设置正确的波特率和从站地。RS-485 接线需要使用自动收发方向的转换模块；当前 Python RTU 库不控制
独立的 `DE/RE` 引脚。

## 方法

```python
  '''!
    @brief 初始化串口并创建智能气体传感器对象
    @param baud 串口波特率，默认为9600
    @param slave_addr Modbus从站地址，范围1～247，默认为1
    @param bits 串口数据位，默认为8
    @param parity 串口校验位，默认为"N"
    @param stopbit 串口停止位，默认为1
    @param shared_rtu 需要共享串口时传入另一个DFRobot_RTU对象
  '''
  def __init__(self, baud=9600, slave_addr=1, bits=8, parity="N", stopbit=1, shared_rtu=None):

  '''!
    @brief 修改后续请求使用的主机侧从站地址
    @param addr Modbus从站地址，范围1～247
    @return 设置成功返回True，参数无效返回False
    @note 该方法不会修改传感器地址，也不会写入EEPROM
  '''
  def set_client_slave_addr(self, addr):

  '''!
    @brief 获取当前主机侧使用的从站地址
    @return 当前Modbus从站地址
  '''
  def get_client_slave_addr(self):

  '''!
    @brief 设置读取失败后的额外重试次数
    @param count 额外重试次数，必须为非负整数
    @return 设置成功返回True，参数无效返回False
  '''
  def set_read_retry_count(self, count):

  '''!
    @brief 获取读取失败后的额外重试次数
    @return 当前额外重试次数
  '''
  def get_read_retry_count(self):

  '''!
    @brief 修改Raspberry Pi串口波特率
    @param baud 新的主机串口波特率
    @return 修改成功返回True，失败返回False
    @note 该方法不会修改或保存传感器波特率
  '''
  def set_host_baudrate(self, baud):

  '''!
    @brief 将气体编码转换为气体名称
    @param gas_code 气体编码
    @return 已知编码返回气体名称，未知编码返回空字符串
  '''
  def gas_code_to_type_name(cls, gas_code):

  '''!
    @brief 获取气体编码对应的默认单位
    @param gas_code 气体编码
    @return 已知编码返回单位字符串，未知编码返回空字符串
  '''
  def gas_code_to_default_unit(cls, gas_code):

  '''!
    @brief 将未知气体编码格式化为十六进制字符串
    @param gas_code 气体编码
    @return 类似"0x1A"的字符串
  '''
  def gas_code_format_unknown(gas_code):

  '''!
    @brief 解析从地址0开始的输入寄存器并更新last_measure
    @param registers 16位输入寄存器列表
    @return 解析后的测量字典
  '''
  def fill_last_measure_from_input_regs(self, registers):

  '''!
    @brief 读取气体测量输入寄存器
    @param with_timestamp True同时读取时间戳，False只读取基础测量数据
    @return Modbus错误码，0表示成功
    @note 解析后的数据保存在last_measure中
  '''
  def read_gas_measurement_data(self, with_timestamp=False):

  '''!
    @brief 根据浓度原码和小数位计算实际浓度
    @return 浮点浓度；小数位大于12时返回NaN
  '''
  def get_concentration_float(self):

  '''!
    @brief 向当前从机写一个保持寄存器
    @param reg 保持寄存器地址
    @param value 写入的16位数值
    @return Modbus错误码，0表示成功
  '''
  def write_holding_reg(self, reg, value):

  '''!
    @brief 将当前配置保存到传感器EEPROM
    @return Modbus错误码，0表示成功
  '''
  def commit_configuration(self):

  '''!
    @brief 修改传感器Modbus地址并保存到EEPROM
    @param new_addr 新地址，范围1～247
    @return Modbus错误码，0表示成功
  '''
  def set_device_address(self, new_addr):

  '''!
    @brief 写入传感器波特率编码，但不保存到EEPROM
    @param code BAUD_CODE_2400～BAUD_CODE_115200之一
    @return Modbus错误码，0表示成功
  '''
  def write_device_baud_code(self, code):

  '''!
    @brief 写入并保存传感器波特率编码
    @param code BAUD_CODE_2400～BAUD_CODE_115200之一
    @return Modbus错误码，0表示成功
    @note 该方法不会切换Raspberry Pi串口波特率，建议参考02.change_device_baudrate.py
  '''
  def set_device_baud_code(self, code):

  '''!
    @brief 设置探头运行或休眠
    @param sleep True进入休眠，False进入运行状态
    @return Modbus错误码，0表示成功
    @note 探头状态立即生效，不保存到EEPROM
  '''
  def set_probe_sleep(self, sleep):

  '''!
    @brief 读取探头当前工作状态
    @return 返回(error_code, is_sleep)，error_code为0表示成功
  '''
  def get_probe_work_mode(self):
```

## 兼容性

| 主板         | 通过 | 未通过 | 未测试 | 备注 |
| ------------ | :--: | :----: | :----: | :--: |
| RaspberryPi2 |      |        |   √    |      |
| RaspberryPi3 |      |        |   √    |      |
| RaspberryPi4 |  √   |        |        |      |

* Python 版本

| Python  | 通过 | 未通过 | 未测试 | 备注 |
| ------- | :--: | :----: | :----: | ---- |
| Python2 |     |        |      √  |      |
| Python3 |  √   |        |        |      |

## 历史

- 2026/07/20 - 1.0.0 版本

## 创作者

Written by PLELES(li.jia@dfrobot.com), 2026. (Welcome to our [website](https://www.dfrobot.com.cn/))
