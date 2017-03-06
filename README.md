linux 下利用v4l2库封装的摄像头采图库
========================================
****
# 使用方法
### 编译
    代码clone到本地后，直接在目录下执行make命令，生成libv4l2Agent.so库文件
### 使用
    使用时需要引用头文件v4l2Agent.h，以及在makefile（或者任意编译器工程文件中的
    编译选项中）中增加libv4l2Agent.so的引用
### 调用
    具体调用方法及调用顺序请参见verifyEngineDemo工程
****
# verifyEngineDemo工程链接如下：
[verifyEngineDemo](http://192.168.30.222/verifyapp/verifyEngineDemo.git)