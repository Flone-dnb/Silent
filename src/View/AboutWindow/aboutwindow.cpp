#include "aboutwindow.h"
#include "ui_aboutwindow.h"

// Qt
#include <QDesktopServices>
#include <QUrl>

AboutWindow::AboutWindow(QString sSilentVersion, QWidget *parent) : QMainWindow(parent), ui(new Ui::AboutWindow)
{
    ui->setupUi(this);
    setFixedSize ( width (), height () );


    ui ->label_appIcon       ->setPixmap ( QPixmap(":/appMainIcon.png").scaled (128, 128, Qt::KeepAspectRatio) );
    ui ->label_silentVersion ->setText   ( "Silent. Version: " + sSilentVersion + "." );
    ui ->label_copyright     ->setText   ( "Copyright (c) 2019.\nAleksandr \"Flone\" Tretyakov." );
}



void AboutWindow::closeEvent(QCloseEvent *pEvent)
{
    deleteLater ();
}




void AboutWindow::on_pushButton_clicked()
{
    QDesktopServices::openUrl (QUrl("https://github.com/Flone-dnb/FChat-client"));
}


AboutWindow::~AboutWindow()
{
    delete ui;
}
