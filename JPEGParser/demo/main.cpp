#include <QApplication>

#include "mainwindow.h"

#include "jpegimage.h"

int main(int argc, char *argv[])
{
    qDebug("Test begin!");
    JpegImage* jpg = new JpegImage();
    jpg->open("jpeg.jpeg");
    qDebug("Test end!");

    Q_INIT_RESOURCE(qhexedit);
    QApplication app(argc, argv);
    app.setApplicationName("QHexEditor");
    app.setOrganizationName("QHexEdit");

    // Identify locale and load translation if available
    QString locale = QLocale::system().name();
    QTranslator translator;
    translator.load(QString("qhexedit_") + locale);
    app.installTranslator(&translator);

    MainWindow *mainWin = new MainWindow(argc > 1? argv[1]: "");
    mainWin->show();

//    QPixmap p(mainWin->size());
//    mainWin->render(&p);
//    p.save("screenshot.png");

    return app.exec();
}
