# gpio_reg_drv - ZYNQ mpsoc
---
基于米联客MPSOC系列开发板  
通过寄存器控制bank0~5的任何一个IO（除了MIO7，8不能作为输入）  
通过ioctl可设置GPIO的方向、读取GOIO的值、设置GPIO的电平，提供了"
gpio_reg_app.cpp"这个文件作为应用层的调用展示。该程序基于07A-7CG/7EG/7EV制作，其他板卡须自行更换IO管脚号。