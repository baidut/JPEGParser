#include <QMessageBox>
#include <QFileDialog>
#include <QApplication>
#include <QStatusBar>
#include <QLabel>
#include <QMenuBar>
#include <QToolBar>
#include <QAction>
#include <QMenu>
#include <QUndoStack>
#include <QLabel>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "jpegimage.h"

#define COLUMN_OF_ADDR      4
#define COLUMN_OF_FIELD     0
#define ADDRESS_AREA_COLOR  QColor(0xd4, 0xd4, 0xd4, 0xff)//setAddressAreaColor(QColor(0xd4, 0xd4, 0xd4, 0xff));
#define HIGHLIGHTING_COLOR  QColor(0xff, 0xff, 0x99, 0xff)//setHighlightingColor(QColor(0xff, 0xff, 0x99, 0xff));
#define SELECTION_COLOR     QColor(0x6d, 0x9e, 0xff, 0xff)//setSelectionColor(QColor(0x6d, 0x9e, 0xff, 0xff));

// TODO 添加定位
// 为了更加方便，将文件指针和文件流入口作为类成员

// 并不读入marker（offset不移动）,只是存入变量marker中
typedef  quint16 JpegMarker;
JpegMarker MainWindow::nextJpegMarker(){
    quint16 marker;
    if(remainder){
        marker = remainder;
        remainder = 0;
    }
    else{
        Q_ASSERT(in);
        (*in)>>marker;
    }
    return marker;
}
quint32 MainWindow::readJpegParm(int size,QTreeWidgetItem* parent,QString field,QString infor=QString("")){
    Q_ASSERT(in);

    QStringList ls;
    quint8 parmU8;
    quint16 parmU16; // 不能处理负数的情况,如果读特殊参数就特殊处理 readJpegParm16 readJpegParm8 readJpegParm4
    quint32 parm;

    switch(size){
    case 4: // 注意这里代表两个占4bit的，放在一起，由于无法选中半个字节，所以放在一起处理
    case 8: (*in) >> parmU8;parm = parmU8;break;
    case 16: (*in) >> parmU16;parm = parmU16;break;
    }
    QString value;
    if(size==4)value = QString("%1 & %2").arg(parm/16).arg(parm%16);
    else  value = QString::number(parm);
    ls<<field<<"Parm"<<value<<infor<<QString("0x%1").arg(offset,0,16);//16进制,占2位,空位补0 QString::number(address,16);
    QTreeWidgetItem* item = new QTreeWidgetItem(parent,ls);
    parent->addChild(item);
    //item->setFlags(Qt::ItemIsEditable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    item->setCheckState (COLUMN_OF_FIELD, Qt::Unchecked);

    offset+=(size==4)?1:(size/8);

    return parm;
}
void MainWindow::readJpegBytes(int size,QTreeWidgetItem* parent,QString field,QString infor=QString("")){
    char s[500]; // 数组有溢出的危险 31，181
    QStringList ls;
    // TODO 不一定成功读入
    Q_ASSERT(size==in->readRawData(s,size));
    ls<<field<<"Data segment"<<QString(s)<<infor<<QString("0x%1").arg(offset,0,16); // QString(s) 太长不显示
    QTreeWidgetItem* item = new QTreeWidgetItem(parent,ls);
    parent->addChild(item);
    item->setCheckState (COLUMN_OF_FIELD, Qt::Unchecked);
    offset+=size;
}
void MainWindow::readJpegMarker(QTreeWidgetItem* parent,QString field,QString infor=QString("")){
    QStringList ls;
    ls<<field<<"Marker"<<""<<infor<<QString("0x%1").arg(offset,0,16);
    QTreeWidgetItem* item = new QTreeWidgetItem(parent,ls);
    parent->addChild(item);
    item->setCheckState (COLUMN_OF_FIELD, Qt::Unchecked);
    offset+=2;// 这时候确定读入一个marker
}
QTreeWidgetItem* MainWindow::newJpegItem(QTreeWidgetItem* parent,QString field,QString infor=QString("")){
    QStringList ls;
    ls<<field<<""<<""<<infor<<QString("0x%1").arg(offset,0,16);
    QTreeWidgetItem* item = new QTreeWidgetItem(parent,ls);
    parent->addChild(item);
    item->setCheckState (COLUMN_OF_FIELD, Qt::Unchecked);
    return item;
}
void MainWindow::readJpegTables(){
    QTreeWidgetItem * parent= (scan)? scan:frame;
    quint32 start;
    QTreeWidgetItem * item =NULL;
    quint16 marker;
    quint16 L;
    quint32 parm;

    for(;;){ // 可能有多个数据段
        switch (marker = nextJpegMarker()) { // & 0xFF 可以更高效的匹配，但这里不追求速度，故不做优化
            case 0xFFDB: /* DQT */
                item = newJpegItem(parent,"Quantization table-specification");
                readJpegMarker(item,"DQT",QString("Define quantization table"));
                start = offset; // 包括Lq的长度
                L = readJpegParm(16,item,"Lq","Quantization table definition length");
                do{
                    // 创建表，同时输出
                    parm = readJpegParm(4,item,"Pq&Tq","Quantization table element precision&destination identifier");
                    parm = parm/16; // Pq
                    for(int i=0;i<64;i++){
                        readJpegParm((parm)?16:8,item,QString("Q%1").arg(i),"Quantization table element");
                        // Pq定义了Qn的精度 占16位还是8位 Pq=1为16bit
                    }
                }while(offset<start+L);
                break;
            case 0xFFC4: /* DHT */
                item = newJpegItem(parent,"Huffman table-specification");
                readJpegMarker(item,"DHT",QString("Define Huffman table"));
                start = offset;// 注意头部包含Lh所以要放在前面
                L = readJpegParm(16,item,"Lh","Huffman table definition length");
                readJpegBytes(L-2,item,"Huffman data segment");
                /*QTreeWidgetItem * DHT;
                QTreeWidgetItem * assignment;
                quint16 Lh;
                quint8 L[16];
                  do{
                    // 创建表，同时输出
                    readJpegParm(4,DHT,"Tc&Th","Quantization table element precision&destination identifier");
                    for(int i=1;i<16;i++){
                        L[i]=readJpegParm(8,DHT,QString("L(%1)").arg(i),"Number of Huffman codes of length i");
                    }
                    assignment = newJpegItem(DHT,"Symbol-length assignment");
                    for(int i=1;i<16;i++){
                       for(int j=1;j<L[i];j++){
                           // TODO 大量重复说明问题
                           readJpegParm(8,assignment,QString("V(%1,%2)").arg(i).arg(j),"Value associated with each Huffman code");
                       }
                    }
                }while(offset<start+Lh);*/
                break;
            case 0xFFDD:
                item = newJpegItem(parent,"Restart interval definition");
                readJpegMarker(item,"DRI",QString("Define restart interval"));
                readJpegParm(16,item,"Lr","Define restart interval segment length");
                readJpegParm(16,item,"Ri","Restart interval");
                break;
            case 0xFFFE:
                {
                item = newJpegItem(parent,"Comment");
                readJpegMarker(item,"COM",QString("Comment"));
                L = readJpegParm(16,item,"Lc","Comment segment length");
                readJpegBytes(L-2,item,"Cmi(i=1~L-2)","Comment byte");
                break;
                }
            case 0xFFCC: /* DAC Define arithmetic coding conditioning(s) */
            //case 0xFFE0:
                break;
            default:
                if(marker>=0xFFE0&&marker<=0xFFEF){ // APP Application data syntax
                    item = newJpegItem(parent,"Application data");
                    readJpegMarker(item,"APPn",QString("Application data"));
                    quint16 L = readJpegParm(16,item,"Lp","Application data segment length");
                    readJpegBytes(L-2,item,"Api(i=1~L-2)","Application data byte");
                }
                else{//放回
                    remainder = marker;//(*remainder)<<marker;
                    return;
                }
                break;
        }
    }
}
// 为了避免频繁触发，间接触发，改成点击触发
void MainWindow::setSelection(int address){
    // 选择区域时，光标可能是开始处或者结束处，取决于从后往前还是从前往后
    // 当前地址改变触发 不能修改当前地址
    // 采用读取的方式比较麻烦，建议建立QMap查询对应的地址
    QTreeWidgetItem* node = image;
    int N = 0,i,startAddr,endAddr;
    bool ok=true;

    if(ui->treeWidget->hasFocus())return; // 解决选中树节点触发的情况造成一选父节点，即跳到孩子节点的问题。
    while( 0 != (N = (node->childCount()))){
        for(i=1;i<N-1;i++){
            endAddr = node->child(i)->text(COLUMN_OF_ADDR).toInt(&ok,16);
            Q_ASSERT(ok);
            if(endAddr>address)break;
        }
        node = node->child(i-1); // 只有一个孩子
    }
    // 没用到startAddr = node->text(COLUMN_OF_ADDR).toInt(&ok,16);
    ui->treeWidget->expandItem(node->parent());//setItemExpanded(node,false);展开元素，显示其子元素
    ui->treeWidget->setItemSelected(node,true);
    // qDebug("setSelection %x ~ %x",startAddr,endAddr);
    ui->treeWidget->setCurrentItem(node,0);//ui->treeWidget->setCurrentItem(node,0,QItemSelectionModel::Select);注明QItemSelectionModel::Select避免与多选模式冲突
    // ui->HexEdit->setHighlightedRange(startAddr,endAddr);
    //ui->HexEdit->gotoSelection(startAddr,endAddr);// 不能改变光标，否则当前地址再次改变。不改变光标则无法选中
    //ui->HexEdit->gotoSelection(startAddr,endAddr);
    ui->treeWidget->itemClicked(node,0);// 相互触发需要能结束，否则当机 这里确保on_treeWidget_itemClicked不改变地址
}
// 通过父亲定位到树形结构的下一个兄弟
//  下一个兄弟找不到的情况，如果是最后一个孩子，则需要取父亲的下一个兄弟，还要递归找 // 如果直接存开始地址和结束地址则更方便
void MainWindow::on_treeWidget_itemClicked(QTreeWidgetItem *item, int column) { // 改变光标地址
    (void)column;// 暂不使用选中列信息;
    // 存在父节点前提下递归寻找下一个元素
    QString start = item->text(COLUMN_OF_ADDR);
    QString end = "";
    int idx;
    QTreeWidgetItem* parent;
    QTreeWidgetItem* child = item; // 注意不能对item进行修改 最好添加const保证指针指向的元素不被修改
    while( (parent = child->parent()) ){
        idx = parent->indexOfChild(child)+1;
        if(parent->childCount()!=idx){
            end = parent->child(idx)->text(COLUMN_OF_ADDR);
            break;
        }
        else{
            child = parent; // 指针操作
        }
    }
    if(end!=""){
        bool ok = true;
        quint32 startAddr = start.toInt(&ok,16);
        Q_ASSERT(ok);
        quint32 endAddr = end.toInt(&ok,16); // when base = 0, If the string begins with "0x", base 16 is used;
        Q_ASSERT(ok);
        // 不仅要select，还要调到可见的地方 修改了HexEdit源码
        ui->HexEdit->gotoSelection(startAddr,endAddr);
        // 不支持多种颜色高亮
        if(item->checkState(COLUMN_OF_FIELD)){ //item->isSelected()
            ui->HexEdit->setHighlightedRange(startAddr,endAddr);
        }
        else{
            ui->HexEdit->removeHighlightedRange(startAddr,endAddr);
        }
    }
    else{
        qDebug("end not found!");// 到结束的地址
    }
}
void MainWindow::on_treeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column) {
// 双击节点合并
    (void)column;
    ui->treeWidget->collapseItem(item->parent());
}
/*****************************************************************************/
/* Public methods */
/*****************************************************************************/
MainWindow::MainWindow(const char* const fileName):
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    init();
    setCurrentFile(fileName);
    if (!curFile.isEmpty()) {
        loadFile(curFile);
    }
}

MainWindow::~MainWindow()
{
  delete ui;
}

/*****************************************************************************/
/* Protected methods */
/*****************************************************************************/
void MainWindow::closeEvent(QCloseEvent *)
{
    writeSettings();
}

/*****************************************************************************/
/* Private Slots */
/*****************************************************************************/
void MainWindow::about()
{
   QMessageBox::about(this, tr("About JPEGHexViewer"),
            tr("<h3>JPEGHexViewer</h3>"
               "The JPEGHexViewer is a tool to help you figure out the syntax structure of JPEG<br>"
               "It is written in C++, using QT framework.<hr>"
               "Contact me: <u>yingzhenqiang@163.com</u><br>"
               "View the source through: <br>"
               "<a href='https://github.com/baidut/JPEGParser/'>https://github.com/baidut/JPEGParser/</a>"));
}

void MainWindow::open()
{
    QString fileName = QFileDialog::getOpenFileName(this);
    if (!fileName.isEmpty()) {
        loadFile(fileName);


        QTreeWidgetItem * frameHeader = NULL;
        QTreeWidgetItem * components = NULL;
        QTreeWidgetItem * scanHeader = NULL;
        quint8  Nf;

        //remainder = new QDataStream(&buffer,QIODevice::ReadWrite);
        remainder = 0;
        image = frame = scan = NULL;
        QStringList ls;
        ls << "Image" ;//<< QString("%1 X %2 ,XXX bytes")
              //.arg(jpg->getWidth())
              //.arg(jpg->getHeight());
        image = new QTreeWidgetItem(ui->treeWidget,ls);

        QFile file(fileName);
        file.open(QIODevice::ReadOnly);
        in = new QDataStream(&file);// QDataStream in(&file);    // read the data serialized from the file
        offset = 0;

        if(nextJpegMarker()!=0XFFD8){ //&&marker!=0XD8FF
            QMessageBox::warning(this,tr("Err"),tr("Err: SOI is not detected!\n"));
            return;
        }
        readJpegMarker(image,"SOI",QString("Start Of Image"));

        frame = newJpegItem(image,"Frame");
        readJpegTables();

        frameHeader = newJpegItem(frame,"Frame Header");
        nextJpegMarker();/*
        if(readJpegMarker()!=0xFFC0){ // SOF0 (baseline JPEG)
            QMessageBox::warning(this,tr("Err"),tr("Err: Only support baseline JPEG!\n"));
            return;
        }*/
        readJpegMarker(frameHeader,"SOF",QString("Start Of Frame"));
        readJpegParm(16,frameHeader,"Lf","Frame header length");
        readJpegParm(8,frameHeader,"P","Sample precision");
        readJpegParm(16,frameHeader,"Y","Height,Number of lines");
        readJpegParm(16,frameHeader,"X","Width,Number of samples per line");
        Nf = readJpegParm(8,frameHeader,"Nf","Number of image components in frame"); // 3 YCbCr

        /* Check three image components*/
        components = newJpegItem(frameHeader,"component-parm");
        for (int i = 1; i <= Nf; i++) {
           readJpegParm(8,components,QString("C%1").arg(i),"Component identifier");
           readJpegParm(4,components,QString("H%1&V%1").arg(i),"Horizontal&Vertical sampling factor");
           readJpegParm(8,components,QString("Tq%1").arg(i),"Quantization table destination selector");
        }

        scan = newJpegItem(frame,"Scan");
        readJpegTables();

        scanHeader = newJpegItem(scan,"Scan Header");
        nextJpegMarker();/*
        if(readJpegMarker()!=0xFFDA){ // SOF0 (baseline JPEG)
            QMessageBox::warning(this,tr("Err"),tr("Err: scan!\n"));
            return;
        }*/
        readJpegMarker(scanHeader,"SOS",QString("Start Of Scan"));
        readJpegParm(16,scanHeader,"Ls","Scan header length");
        readJpegParm(8,scanHeader,"Ns","Number of image components in scan");
        components = newJpegItem(scanHeader,"component-parm");
        for (int i = 1; i <= Nf; i++) {
           readJpegParm(8,components,QString("Cs%1").arg(i),"Scan component selector");
           readJpegParm(4,components,QString("Td%1&Ta%1").arg(i),"DC&AC entropy coding table destination selector ");
        }
        readJpegParm(8,scanHeader,"Ss","Start of spectral or predictor selection");
        readJpegParm(8,scanHeader,"Se","End of spectral selection");
        readJpegParm(4,scanHeader,"Ah&Al","Successive approximation bit position high");

        connect(ui->HexEdit, SIGNAL(currentAddressChanged(int)), this, SLOT(setSelection(int)));
        for (;;) {
            switch (nextJpegMarker()) {
                case 0xFFD9:
                    readJpegMarker(image,"EOI",QString("End of image"));
                    return;
                default:break;
            }
        }
    }
}

void MainWindow::optionsAccepted()
{
    writeSettings();
    readSettings();
}

void MainWindow::findNext()
{
    searchDialog->findNext();
}

bool MainWindow::save()
{
    if (isUntitled) {
        return saveAs();
    } else {
        return saveFile(curFile);
    }
}

bool MainWindow::saveAs()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"),
                                                    curFile);
    if (fileName.isEmpty())
        return false;

    return saveFile(fileName);
}

void MainWindow::saveSelectionToReadableFile()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save To Readable File"));
    if (!fileName.isEmpty())
    {
        QFile file(fileName);
        if (!file.open(QFile::WriteOnly | QFile::Text)) {
            QMessageBox::warning(this, tr("QHexEdit"),
                                 tr("Cannot write file %1:\n%2.")
                                 .arg(fileName)
                                 .arg(file.errorString()));
            return;
        }

        QApplication::setOverrideCursor(Qt::WaitCursor);
        file.write(ui->HexEdit->selectionToReadableString().toLatin1());
        QApplication::restoreOverrideCursor();

        statusBar()->showMessage(tr("File saved"), 2000);
    }
}

void MainWindow::saveToReadableFile()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save To Readable File"));
    if (!fileName.isEmpty())
    {
        QFile file(fileName);
        if (!file.open(QFile::WriteOnly | QFile::Text)) {
            QMessageBox::warning(this, tr("QHexEdit"),
                                 tr("Cannot write file %1:\n%2.")
                                 .arg(fileName)
                                 .arg(file.errorString()));
            return;
        }

        QApplication::setOverrideCursor(Qt::WaitCursor);
        file.write(ui->HexEdit->toReadableString().toLatin1());
        QApplication::restoreOverrideCursor();

        statusBar()->showMessage(tr("File saved"), 2000);
    }
}

void MainWindow::setAddress(int address)
{
    lbAddress->setText(QString("%1").arg(address, 1, 16));
}

void MainWindow::setOverwriteMode(bool mode)
{
    if (mode)
        lbOverwriteMode->setText(tr("Overwrite"));
    else
        lbOverwriteMode->setText(tr("Insert"));
}

void MainWindow::setSize(int size)
{
    lbSize->setText(QString("%1").arg(size));
}

void MainWindow::showOptionsDialog()
{
    optionsDialog->show();
}

void MainWindow::showSearchDialog()
{
    searchDialog->show();
}

/*****************************************************************************/
/* Private Methods */
/*****************************************************************************/
void MainWindow::init()
{
    setAttribute(Qt::WA_DeleteOnClose);
    optionsDialog = new OptionsDialog(this);
    connect(optionsDialog, SIGNAL(accepted()), this, SLOT(optionsAccepted()));
    isUntitled = true;

    connect(ui->HexEdit, SIGNAL(overwriteModeChanged(bool)), this, SLOT(setOverwriteMode(bool)));
    searchDialog = new SearchDialog(ui->HexEdit, this);

    connectActions();
    createStatusBar();

    readSettings();

    setUnifiedTitleAndToolBarOnMac(true);
}

void MainWindow::connectActions()
{
    connect(ui->action_Open, SIGNAL(triggered()), this, SLOT(open()));
    connect(ui->action_Save, SIGNAL(triggered()), this, SLOT(save()));
    connect(ui->actionSave_As, SIGNAL(triggered()), this, SLOT(saveAs()));
    connect(ui->actionSave_Readable, SIGNAL(triggered()), this, SLOT(saveToReadableFile()));
    connect(ui->actionE_xit, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));
    connect(ui->actionSave_Readable, SIGNAL(triggered()), this, SLOT(saveSelectionToReadableFile()));
    connect(ui->action_About, SIGNAL(triggered()), this, SLOT(about()));
    connect(ui->actionAbout_Qt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(ui->action_Find_Replace, SIGNAL(triggered()), this, SLOT(showSearchDialog()));
    connect(ui->actionFind_Next, SIGNAL(triggered()), this, SLOT(findNext()));
    connect(ui->action_Options, SIGNAL(triggered()), this, SLOT(showOptionsDialog()));
}

void MainWindow::createStatusBar()
{
    // Address Label
    lbAddressName = new QLabel();
    lbAddressName->setText(tr("Address:"));
    statusBar()->addPermanentWidget(lbAddressName);
    lbAddress = new QLabel();
    lbAddress->setFrameShape(QFrame::Panel);
    lbAddress->setFrameShadow(QFrame::Sunken);
    lbAddress->setMinimumWidth(70);
    statusBar()->addPermanentWidget(lbAddress);
    connect(ui->HexEdit, SIGNAL(currentAddressChanged(int)), this, SLOT(setAddress(int)));

    // Size Label
    lbSizeName = new QLabel();
    lbSizeName->setText(tr("Size:"));
    statusBar()->addPermanentWidget(lbSizeName);
    lbSize = new QLabel();
    lbSize->setFrameShape(QFrame::Panel);
    lbSize->setFrameShadow(QFrame::Sunken);
    lbSize->setMinimumWidth(70);
    statusBar()->addPermanentWidget(lbSize);
    connect(ui->HexEdit, SIGNAL(currentSizeChanged(int)), this, SLOT(setSize(int)));

    // Overwrite Mode Label
    lbOverwriteModeName = new QLabel();
    lbOverwriteModeName->setText(tr("Mode:"));
    statusBar()->addPermanentWidget(lbOverwriteModeName);
    lbOverwriteMode = new QLabel();
    lbOverwriteMode->setFrameShape(QFrame::Panel);
    lbOverwriteMode->setFrameShadow(QFrame::Sunken);
    lbOverwriteMode->setMinimumWidth(70);
    statusBar()->addPermanentWidget(lbOverwriteMode);
    setOverwriteMode(ui->HexEdit->overwriteMode());

    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::loadFile(const QString &fileName)
{

    QFile file(fileName);
    if (!file.open(QFile::ReadOnly)) {
        QMessageBox::warning(this, tr("SDI"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    ui->HexEdit->setData(file.readAll());
    QApplication::restoreOverrideCursor();

    setCurrentFile(fileName);
    statusBar()->showMessage(tr("File loaded"), 2000);
}

void MainWindow::readSettings()
{
    QSettings settings;
    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(610, 460)).toSize();
    move(pos);
    resize(size);

    ui->HexEdit->setAddressArea(settings.value("AddressArea").toBool());
    ui->HexEdit->setAsciiArea(settings.value("AsciiArea").toBool());
    ui->HexEdit->setHighlighting(settings.value("Highlighting").toBool());
    ui->HexEdit->setOverwriteMode(settings.value("OverwriteMode").toBool());
    ui->HexEdit->setReadOnly(settings.value("ReadOnly").toBool());

    ui->HexEdit->setHighlightingColor(settings.value("HighlightingColor").value<QColor>());
    ui->HexEdit->setAddressAreaColor(settings.value("AddressAreaColor").value<QColor>());
    ui->HexEdit->setSelectionColor(settings.value("SelectionColor").value<QColor>());
    ui->HexEdit->setFont(settings.value("WidgetFont").value<QFont>());

    ui->HexEdit->setAddressWidth(settings.value("AddressAreaWidth").toInt());
}

bool MainWindow::saveFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("QHexEdit"),
                             tr("Cannot write file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return false;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    file.write(ui->HexEdit->data());
    QApplication::restoreOverrideCursor();

    setCurrentFile(fileName);
    statusBar()->showMessage(tr("File saved"), 2000);
    return true;
}

void MainWindow::setCurrentFile(const QString &fileName)
{
    curFile = QFileInfo(fileName).canonicalFilePath();
    isUntitled = fileName.isEmpty();
    setWindowModified(false);
    setWindowFilePath(curFile);
}

QString MainWindow::strippedName(const QString &fullFileName)
{
    return QFileInfo(fullFileName).fileName();
}

void MainWindow::writeSettings()
{
    QSettings settings;
    settings.setValue("pos", pos());
    settings.setValue("size", size());
}


void MainWindow::on_actionAbout_q_Hexedit2_triggered(){
    QMessageBox::about(this, tr("About QHexEdit"),
               tr("JPEGHexViewer includes code of the qhexedit2<hr>"
                  "QHexEdit is a hex editor widget written in C++ for the Qt (Qt5) framework.<br>"
                  "It is a simple editor for binary data, just like QPlainTextEdit is for text data. There are sip configuration files included, so it is easy to create bindings for PyQt and you can use this widget also in python.<br>"
                  "For more information:<a href='https://code.google.com/p/qhexedit2/'>https://code.google.com/p/qhexedit2/</a>"));
}
