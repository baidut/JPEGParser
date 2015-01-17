
Qt Hex 

基于qhexedit2十六进制插件完成，对各种协议进行分析。

qhexedit2 
 * [Google Code](https://code.google.com/p/qhexedit2/)
 * [qhexedit2-devel-0.6.3-2.20141212svnr41.fc20 RPM for i686](http://hany.sk/~hany/RPM/f-updates-20-i386/qhexedit2-devel-0.6.3-2.20141212svnr41.fc20.i686.html)

 GoogleCode被墙可以上github搜索fork工程

 JPEG是字节对齐的，所以可以很直观通过十六进制查看协助理解



问题解决：

Qt下QString转char*
```
QString str = “hello”; //QString转char *  
QByteArray ba = str.toLatin1();  
char *mm = ba.data();  
qDebug()<<mm<<endl;  //调试时，在console中输出  
```

树形结构
http://www.cnblogs.com/Romi/archive/2012/04/16/2452709.html
包括点击节点的事件响应

collect2: ld return 1 exit status
程序异常结束。的情形经常会出现这种问题
快捷键 Ctrl+Shift+Esc启动任务管理器，选择 性能选项卡，然后点击下方的 打开资源监视器。 
关闭进程

QTreeWidget如何设置选中节点?
setCurrentItem(QTreeWidgetItem* item);

方案1：分层显示，类似wireshark
方案2：树形显示，先采用这种方式

方案1：一次分析完成生成，先采用这种方式
方案2：点击事件触发分析

只支持baseline JPEG（标准型） Y/Cb/Cr format 4:4:4, 4:2:0 or 4:2:2
JPEG (baseline)压缩综述 http://www.compotech.com.cn/cms/a/changshangdabenying/2007/0205/5320.html
使用渐进式 JPEG 来提升用户体验 http://blog.jobbole.com/44038/

一些英文词的标准缩写 http://blog.csdn.net/carpinter/article/details/6975740
Parameter*	PARM*


Qt数字转字符串
QString::number(num);
Qt字符串转数字

TODO：
tab、左右键可以切换到下一个块

调试
    QString s = item->text(0);
    QByteArray ba = s.toLatin1();
    char *mm = ba.data();
    qDebug("Data:%s",mm);
    // qDebug相当于printf，但是加了回车，不支持QString qDebug("Data:%s",mm);
    
    最简单的方式
    qDebug()<<s;
    
获取文本text()


QTextEdit自动滚动

QTextEdit中滚动条定位
//滚动条滑块置底
    QScrollBar *scrollbar = ui->textEdit->verticalScrollBar();
    scrollbar->setSliderPosition(scrollbar->maximum());