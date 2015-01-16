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

QTreeWidgetItem* newJpegItem(QTreeWidgetItem* parent,int address,QString field,QString value){
    QStringList ls;
    ls<<field<<value<<QString::number(address);
    QTreeWidgetItem* item = new QTreeWidgetItem(parent,ls);
    parent->addChild(item);
    return item;
}

/*
typedef struct{
    QTreeWidgetItem *Tables;
    QTreeWidgetItem *frameHeader;
}JpegFrameTree;

typedef struct{
    QTreeWidgetItem *SOI;
    JpegFrameTree   *frame;
    QTreeWidgetItem *EOI;

    // select corresponding item according to address(when mouse over or clicked)
    void selectItem(int address){
        if(address<2){
            return SOI;
        }
        else if(address>){
            return EOI;
        }
        else return
    }
}JpegImageTree;


// 解析时构建节点
QStringList ls = QString("Image");
Node* Tree = new Node(ui->treeWidget,ls,0);
Node* soi = new Node(Tree,ls,0);
// 触发选中


// 要实现每个item都有addr与其匹配，一种方式是建表查表
// 这里采用另一种方法，封装树操作
typedef struct{
    int addr;
    QTreeWidgetItem *item;
    QList<Node*>     child;
    // 根据父节点和内容构造
    Node(Node* parent,QStringList content,int addr){
        this->item = new QTreeWidgetItem(parent->item,content);
        this->addr = addr;

        parent->item->addChild(child->item);//parent->append(this);
        parent->child.append(this);
    }
    void selectAddress(int addr){
        int N = item->childCount();
        if(N==0)item->setSelected(true);
        else{
            for(int i=0;i<N;i++){
                if()
            }
        }
    }
}Node;*/
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
        qDebug("Test begin!");
        JpegImage* jpg = new JpegImage();
        QByteArray ba = fileName.toLatin1();
        char *mm = ba.data();
        jpg->open(mm);
        qDebug("Test end!");

        /* 放在ui里设置 QStringList headers;
        headers << "Field" << "Value" << "Addr" << "Infor";
        ui->treeWidget->setColumnCount(headers.length());
        ui->treeWidget->setHeaderLabels(headers);*/

        /*QList<QTreeWidgetItem *> items;
        for (int i = 0; i < 3; ++i)
            items.append(new QTreeWidgetItem((QTreeWidget*)0, QStringList(QString("item: %1").arg(i))));
        ui->treeWidget->insertTopLevelItems(0, items);*/
        QStringList ls;
        ls << "Image" << QString("%1 X %2 ,XXX bytes")
              .arg(jpg->getWidth())
              .arg(jpg->getHeight());
        image = new QTreeWidgetItem(ui->treeWidget,ls);

        /*image = newJpegItem(ui->treeWidget,0,"Image",QString("%1 X %2 ,XXX bytes")
                            .arg(jpg->getWidth())
                            .arg(jpg->getHeight()));
                            //ui->treeWidget 不能直接传入
        */
        //imageItem1->setIcon(0,QIcon("xxx.png"));
        QTreeWidgetItem *width = newJpegItem(image,0,"Width",QString("%1 pixels").arg(jpg->getWidth()));
        QTreeWidgetItem *height = newJpegItem(image,0,"Height",QString("%1 pixels").arg(jpg->getHeight()));
        QTreeWidgetItem *soi = newJpegItem(image,0,"SOI",QString("0XFFD8 Start Of Image"));
        QTreeWidgetItem *eoi = newJpegItem(image,0,"EOI",QString("0XFFD9 End Of Image"));

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



