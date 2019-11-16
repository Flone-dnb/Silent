#include "singleusersettings.h"
#include "ui_singleusersettings.h"


// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------


SingleUserSettings::SingleUserSettings(QString userName, float fCurrentVolume, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::SingleUserSettings)
{
    ui ->setupUi(this);




    // Hide maximize & minimize buttons

    Qt::WindowFlags flags = windowFlags();
    flags &= ~Qt::WindowMaximizeButtonHint;
    flags &= ~Qt::WindowMinimizeButtonHint;
    setWindowFlags(flags);




    this ->userName = userName;

    ui   ->label                   ->setText  ( userName + "'s volume:" );
    ui   ->horizontalSlider_volume ->setValue ( static_cast <int> (fCurrentVolume * 100) );

    fCurrentUserVolume = fCurrentVolume;
}


void SingleUserSettings::on_horizontalSlider_volume_valueChanged(int value)
{
    ui ->label_percent ->setText( QString::number(value) + "%" );
}


SingleUserSettings::~SingleUserSettings()
{
    delete ui;
}

void SingleUserSettings::on_pushButton_clicked()
{
    float fNewVolume = ui ->horizontalSlider_volume ->value() / 100.0f;

    emit signalChangeUserVolume( userName, fNewVolume );

    close();
}
