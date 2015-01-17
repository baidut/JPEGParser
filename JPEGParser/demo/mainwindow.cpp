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

// TODO 添加定位
// 为了更加方便，将文件指针和文件流入口作为类成员
quint32 MainWindow::readJpegParm(int size,QTreeWidgetItem* parent,QString field,QString infor=QString("")){
    Q_ASSERT(in);

    QStringList ls;
    quint8 parmU8;
    quint16 parmU16; // 不能处理负数的情况
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

    offset+=(size==4)?1:(size/8);

    return parm;
}

void MainWindow::newJpegMarker(QTreeWidgetItem* parent,QString field,QString infor=QString("")){
    QStringList ls;
    ls<<field<<"Marker"<<""<<infor<<QString("0x%1").arg(offset-2,0,16);//注意此时marker已经读入
    QTreeWidgetItem* item = new QTreeWidgetItem(parent,ls);
    parent->addChild(item);
}
QTreeWidgetItem* MainWindow::newJpegItem(QTreeWidgetItem* parent,QString field,QString infor=QString("")){
    QStringList ls;
    ls<<field<<""<<""<<infor<<QString("0x%1").arg(offset-2,0,16);//注意此时marker已经读入
    QTreeWidgetItem* item = new QTreeWidgetItem(parent,ls);
    parent->addChild(item);
    return item;
}
void MainWindow::setSelection(int address){
    /*int N = Tree->item->childCount();
    Node* node = Tree;
    for(int i=0;i<N;i++){
        if(node->item->child(i))
    }*/

    if(address<2){
        ui->HexEdit->setSelection(0,2);
        ui->treeWidget->setCurrentItem(image->child(2)); // 从0开始计
    }
}

void MainWindow::on_treeWidget_itemClicked(QTreeWidgetItem *item, int column) {
#define COLUMN_OF_ADDR 4
    (void)column;// 暂不使用选中列信息;
    // 通过父亲定位到树形结构的下一个兄弟
    QTreeWidgetItem* parent = item->parent();
    QString start = item->text(COLUMN_OF_ADDR);
    QString end = parent->child(parent->indexOfChild(item)+1)->text(COLUMN_OF_ADDR);
    // TODO 下一个兄弟找不到的情况，如果是最后一个孩子，则需要取父亲的下一个兄弟，还要递归找 // 如果直接存开始地址和结束地址则更方便
    bool ok = true;
    int startAddr = start.toInt(&ok,16);
    Q_ASSERT(ok);
    int endAddr = end.toInt(&ok,16); // when base = 0, If the string begins with "0x", base 16 is used;
    Q_ASSERT(ok);
    ui->HexEdit->setSelection(startAddr,endAddr);
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
   QMessageBox::about(this, tr("About QHexEdit"),
            tr("The QHexEdit example is a short Demo of the QHexEdit Widget."));
}

void MainWindow::open()
{
    QString fileName = QFileDialog::getOpenFileName(this);
    if (!fileName.isEmpty()) {
        loadFile(fileName);

        QStringList ls;
        ls << "Image" ;//<< QString("%1 X %2 ,XXX bytes")
              //.arg(jpg->getWidth())
              //.arg(jpg->getHeight());
        image = new QTreeWidgetItem(ui->treeWidget,ls);

        QFile file(fileName);
        file.open(QIODevice::ReadOnly);
        in = new QDataStream(&file);// QDataStream in(&file);    // read the data serialized from the file
        offset = 0;


        QTreeWidgetItem * frame = NULL;
        QTreeWidgetItem * frameHeader = NULL;
        QTreeWidgetItem * components = NULL;

        QTreeWidgetItem * scan = NULL;
        QTreeWidgetItem * scanHeader = NULL;

        if(readJpegMarker()!=0XFFD8){ //&&marker!=0XD8FF
            QMessageBox::warning(this,tr("Err"),tr("Err: SOI is not detected!\n"));
            return;
        }
        newJpegMarker(image,"SOI",QString("Start Of Image"));
        frame = newJpegItem(image,"Frame");

        for (;;) {
            //quint16 Lf,Y,X;
            quint8  Nf;
            switch (readJpegMarker()) { // & 0xFF 可以更高效的匹配，但这里不追求速度，故不做优化
                case 0xFFC0:  /* SOF0 (baseline JPEG) */
                    frameHeader = newJpegItem(frame,"Frame Header");
                    newJpegMarker(frameHeader,"SOF",QString("Start Of Frame"));
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
                    break;

                case 0xFFDA:  /* SOS */
                    // 标准情况只有一个Scan
                    Q_ASSERT(scan);
                    scanHeader = newJpegItem(scan,"Scan Header");
                    newJpegMarker(scanHeader,"SOS",QString("Start Of Scan"));
                    readJpegParm(16,scanHeader,"Ls","Scan header length");
                    readJpegParm(8,scanHeader,"Ns","Number of image components in scan");
                    components = newJpegItem(scanHeader,"component-parm");
                    for (int i = 1; i <= Nf; i++) {
                       readJpegParm(8,components,QString("Cs%1").arg(i),"Scan component selector");
                       readJpegParm(4,components,QString("Td%1&Ta%1").arg(i),"DC&AC entropy coding table destination selector ");
                    }
                    readJpegParm(8,scanHeader,"Ss","Start of spectral or predictor selection");
                    readJpegParm(8,scanHeader,"Se","End of spectral selection");
                    readJpegParm(8,scanHeader,"Ah","Successive approximation bit position high");
                    readJpegParm(8,scanHeader,"Al","Successive approximation bit position low or point transform");
                    return;
                case 0xFFDB: /* DQT */
                    {
                        QTreeWidgetItem * parent;
                        QTreeWidgetItem * DQT;
                        quint16 Lq;
                        quint32 start;
                        parent = (scan)? scan:frame;
                        DQT = newJpegItem(parent,"Quantization table-specification");
                        newJpegMarker(DQT,"DQT",QString("Define quantization table"));
                        Lq = readJpegParm(16,DQT,"Lq","Quantization table definition length");
                        start = offset;
                        do{
                            // 创建表，同时输出
                            readJpegParm(4,DQT,"Pq&Tq","Quantization table element precision&destination identifier");
                            for(int i=0;i<64;i++){
                                readJpegParm(8,DQT,QString("Q%1").arg(i),"Quantization table element");
                            }
                        }while(offset<=start+Lq);
                    }
                    break;
                default:break;
            }
        }
        connect(ui->HexEdit, SIGNAL(currentAddressChanged(int)), this, SLOT(setSelection(int)));
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
