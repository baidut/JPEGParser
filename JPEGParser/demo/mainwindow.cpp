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
void newJpegParm(QTreeWidgetItem* parent,int address,QString field,QString value,QString infor=QString("")){
    QStringList ls;
    ls<<field<<"Parm"<<value<<infor<<QString("0x%1").arg(address,0,16);//16进制,占2位,空位补0 QString::number(address,16);
    QTreeWidgetItem* item = new QTreeWidgetItem(parent,ls);
    parent->addChild(item);
}
void newJpegMarker(QTreeWidgetItem* parent,int address,QString field,QString infor=QString("")){
    QStringList ls;
    ls<<field<<"Marker"<<""<<infor<<QString("0x%1").arg(address,0,16);//QString::number(address);
    QTreeWidgetItem* item = new QTreeWidgetItem(parent,ls);
    parent->addChild(item);
}
QTreeWidgetItem* newJpegItem(QTreeWidgetItem* parent,int address,QString field,QString infor=QString("")){
    QStringList ls;
    ls<<field<<""<<""<<infor<<QString("0x%1").arg(address,0,16);//QString::number(address);
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
        QDataStream in(&file);    // read the data serialized from the file

        quint16 marker; //QDataStream & QDataStream::operator>>(quint32 & i)
        in >> marker;  // extract a marker
        qDebug("marker:%x",marker);

        if(marker!=0XFFD8){ //&&marker!=0XD8FF
            QMessageBox::warning(this,tr("Err"),tr("Err: SOI is not detected!\n"));
            return;
        }
        newJpegMarker(image,0,"SOI",QString("Start Of Image"));
        QTreeWidgetItem * frame;
        QTreeWidgetItem * frameHeader;

        int offset = 2;
        for (;;) {
            quint32 parmU32;
            quint16 parmU16;
            quint8  parmU8;
            qint32  parmS32;
            qint16  parmS16;
            quint16 Nf;

            in >> marker;
            switch (marker & 0xFF) {
                case 0xC0:  /* SOF0 (baseline JPEG) */
                       /* Load segment data */
                    frame = newJpegItem(image,offset,"Frame");
                    frameHeader = newJpegItem(frame,offset,"Frame Header",QString("Frame"));
                    newJpegMarker(frameHeader,offset,"SOF",QString("Start Of Frame"));
                    in >> parmU16;
                    newJpegParm(frameHeader,offset+=2,"Lf",QString::number(parmU16),"Frame header length");
                    in >> parmU8;
                    newJpegParm(frameHeader,offset++,"P",QString::number(parmU8),"Sample precision");
                    in >> parmU16;
                    newJpegParm(frameHeader,offset+=2,"Y",QString::number(parmU16),"Height,Number of lines");
                    in >> parmU16;
                    newJpegParm(frameHeader,offset+=2,"X",QString::number(parmU16),"Width,Number of samples per line");
                    in >> parmU8;
                    Nf = parmU8;
                    newJpegParm(frameHeader,offset++,"Nf",QString::number(parmU8),"Number of image components in frame"); // 3 YCbCr

                    /* Check three image components
                    components = newJpegItem(image,2,"component-parm",QString("Frame");
                    for (i = 0; i < Nf; i++) {
                       in >> parmU16;                           // Get sampling factor
                       newJpegItem(frameHeader,2,"Nf",QString::number(parmU16),"Number of image components in frame");
                    }*/
                    return;//break;
                default:offset+=2;break;
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



