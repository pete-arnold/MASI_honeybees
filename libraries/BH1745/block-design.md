# Block design

Block design for Mixly

## begin

- Block Design: BH1745 设置引脚 SDA [pin] SCL [pin]
- Return: None
- Generator:
  - defination: `#include <BH1745.h>\nBH1745 bh = BH1745();`
  - code: `bh.begin(<sda>, <scl>);`

## read color

- Block Design: BH1745 读颜色
- Return: byte
- Generator:
  - code: `bh.readColor();`

## detect color

- Block Design: BH1745 检测到 [红色/橙色/黄色/绿色/青色/蓝色/紫色]
- Return: bool
- Generator:
  - code: `bh.isDetectColor(bh.RED/ORANGE/YELLOW/GREEN/CYAN/BLUE/PURPLE);`

## read

- Block Design: BH1745 读值
- Return: None
- Generator:
  - code: `bh.read();`

## Red/Green/Blue/Clear

- Block Design: BH1745 读 [RGB/R/G/B/H/S/L] 值
- Return: long
- Generator:
  - code: `bh.red/green/blue/rgb/hue/saturation/lightness;`

## Set RGBC Mode

- Block Design: BH1745 设置RGB模式为 [8位/16位]
- Return: None
- Generator:
  - code: `bh.setRgbcMode(bh.RGBC_8_BIT/bh.RGBC_16_BIT);`

## Set Gain

- Block Design: BH1745 设置增益为 [1倍/2倍/16倍]
- Return: None
- Generator:
  - code: `bh.setGain(bh.GAIN_1X/bh.GAIN_2X/bh.GAIN_16X);`
