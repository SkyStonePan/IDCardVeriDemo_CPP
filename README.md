快速上手：

    环境配置：
		1.	安装VS2013环境安装包(vcredist_x86_vs2013.exe)
		2.	安装Qt环境安装包(qt-opensource-windows-x86-msvc2013-5.8.0.exe)
		3.	从官网(http://ai.arcsoft.com.cn/product/verification.html)申请sdk，下载对应的sdk版本(x86或x64)并解压
		4.	头文件配置：inc文件夹内文件放入\IDCardVeriDemo\inc文件夹内
		5.	SDK库配置：
		将libarcsoft_idcardveri.lib放至\IDCardVeriDemo\lib_x86(lib_x64)文件夹下
		6.	在运行代码的时候将对应版本SDK和OpenCV的dll库放至项目根目录下，以免运行时找不到对应的dll库
		7.	将官网申请的APPID及SDKKEY填写至IDCardVeriDemo.cpp文件中，注意平台和版本对应
		8.	在Debug或者Release中选择配置管理器，选择对应的平台

    运行程序：
		1.	本Demo暂不提供身份证阅读器的实现，用户需自己实现
		2.	连接摄像头和阅读器
		3.	按F5启动程序

常见问题：

	1.启动后引擎初始化失败	
		(1)请选择对应的平台，如x64,x86 
		(2)删除工程目录下对应的idv_install.dat
		(3)请确保IDCardVeriDemo.cpp中appid和appkey与当前sdk版本一致 

	2.SDK支持那些格式的图片人脸检测？	
		目前SDK支持的图片格式有jpg，jpeg，png，bmp等。
    
    更多常见问题请访问 https://ai.arcsoft.com.cn/manual/faqs.html。
