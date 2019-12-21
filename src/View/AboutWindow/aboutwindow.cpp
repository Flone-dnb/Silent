#include "aboutwindow.h"
#include "ui_aboutwindow.h"


// Qt
#include <QFile>
#include <QDesktopServices>
#include <QUrl>

// Custom
#include "View/StyleAndInfoPaths.h"


// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------


AboutWindow::AboutWindow(QString sSilentVersion, QWidget *parent) : QMainWindow(parent), ui(new Ui::AboutWindow)
{
    // Setup window

    ui->setupUi(this);
    setFixedSize( width(), height() );




    // Set CSS classes

    ui ->label_appIcon     ->setProperty("cssClass", "appIcon");

    ui ->pushButton_github ->setProperty("cssClass", "github");




    // Set appearance

    ui ->label_appIcon       ->setPixmap ( QPixmap(RES_ICONS_MAIN_PATH)
                                           .scaled (RES_ICONS_MAIN_SIZE_XY, RES_ICONS_MAIN_SIZE_XY, Qt::KeepAspectRatio) );

    ui ->label_silentVersion ->setText   ( QString(INFO_APP_NAME) + " (v" + sSilentVersion + ")." );

    ui ->label_copyright     ->setText   ( INFO_COPYRIGHT );
}



void AboutWindow::closeEvent(QCloseEvent *pEvent)
{
    Q_UNUSED(pEvent)

    deleteLater();
}




void AboutWindow::on_pushButton_github_clicked()
{
    QDesktopServices::openUrl( QUrl(INFO_REPO_URL) );
}


AboutWindow::~AboutWindow()
{
    delete ui;
}
