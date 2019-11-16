#include "settingswindow.h"
#include "ui_settingswindow.h"


// Qt
#include <QKeyEvent>
#include <QMouseEvent>
#include <QMessageBox>
#include <QDir>

// Custom
#include "../src/View/StyleAndInfoPaths.h"
#include "../src/Model/SettingsManager/SettingsFile.h"


// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------


SettingsWindow::SettingsWindow(SettingsFile* pSettingsFile, QWidget *parent) : QMainWindow(parent), ui(new Ui::SettingsWindow)
{
    ui ->setupUi(this);
    setFixedSize( width(), height() );


    bWaitingForPushToTalkButtonInput = false;
    bPushToTalkChanged               = false;
    bMasterVolumeChanged             = false;



    iOriginalPushToTalkButton = pSettingsFile ->iPushToTalkButton;
    iOriginalMasterVolume     = pSettingsFile ->iMasterVolume;


    updateUIToSettings(pSettingsFile);
}





void SettingsWindow::keyPressEvent(QKeyEvent *event)
{
    if ( bWaitingForPushToTalkButtonInput )
    {
        int key = -1;

        if ( (event ->key() >= 0x41)
             &&
             (event ->key() <= 0x5a) )
        {
            // From A to Z

            key = event ->key();

            ui ->pushButton_pushtotalk ->setText( event ->text() .toUpper() );
        }
        else if ( event ->key() == Qt::Key_Alt )
        {
            // 0x12 Virtual Code for GetAsyncKeyState

            key = 0x12;

            ui ->pushButton_pushtotalk ->setText("Alt");
        }

        if (key != -1)
        {
            iPushToTalkKey     = key;
            bWaitingForPushToTalkButtonInput = false;
            bPushToTalkChanged = true;
        }
    }
}

void SettingsWindow::mousePressEvent(QMouseEvent *event)
{
    if (bWaitingForPushToTalkButtonInput)
    {
        int key = -1;

        if      ( event ->button() == Qt::XButton1 )
        {
            // 0x05 Virtual Code for GetAsyncKeyState

            key = 0x05;

            ui ->pushButton_pushtotalk ->setText("X1");
        }
        else if ( event ->button() == Qt::XButton2 )
        {
            // 0x06 Virtual Code for GetAsyncKeyState

            key = 0x06;

            ui ->pushButton_pushtotalk ->setText("X2");
        }

        if (key != -1)
        {
            iPushToTalkKey     = key;
            bWaitingForPushToTalkButtonInput = false;
            bPushToTalkChanged = true;
        }
    }
}

void SettingsWindow::on_pushButton_pushtotalk_clicked()
{
    ui ->pushButton_pushtotalk ->setText("Waiting for button");

    bWaitingForPushToTalkButtonInput = true;
}

void SettingsWindow::refreshVolumeSliderText()
{
    // 0%    -  ui ->horizontalSlider ->minimum()
    // 100%  -  ui ->horizontalSlider ->maximum()

    int iInterval = ui ->horizontalSlider_volume ->maximum()  -  ui ->horizontalSlider_volume ->minimum();
    int iCurrent  = ui ->horizontalSlider_volume ->value();

    iMasterVolume = static_cast <unsigned short int> (iCurrent);

    ui ->label_4 ->setText ( QString::number( (iCurrent / static_cast<double>(iInterval) * 100), 'f', 0 ) + "%" );
}

void SettingsWindow::on_horizontalSlider_volume_valueChanged(int value)
{
    Q_UNUSED(value)

    refreshVolumeSliderText();

    bMasterVolumeChanged = true;
}



SettingsWindow::~SettingsWindow()
{
    delete ui;
}

void SettingsWindow::on_pushButton_2_clicked()
{
    SettingsFile* pNewSettingsFile = new SettingsFile();

    if (bPushToTalkChanged)
    {
        pNewSettingsFile ->iPushToTalkButton = iPushToTalkKey;
    }
    else
    {
        pNewSettingsFile ->iPushToTalkButton = iOriginalPushToTalkButton;
    }

    if (bMasterVolumeChanged)
    {
        pNewSettingsFile ->iMasterVolume = iMasterVolume;
    }
    else
    {
        pNewSettingsFile ->iMasterVolume = iOriginalMasterVolume;
    }

    pNewSettingsFile ->sThemeName = ui ->comboBox_themes ->currentText() .toStdString();

    emit signalSaveSettings( pNewSettingsFile );
    close();
}

void SettingsWindow::updateUIToSettings(SettingsFile* pSettingsFile)
{
    ui ->pushButton_pushtotalk   ->setText  ( QString::fromStdString( pSettingsFile ->getPushToTalkButtonName() ) );
    ui ->horizontalSlider_volume ->setValue ( pSettingsFile ->iMasterVolume );

    showThemes();

    ui ->comboBox_themes ->setCurrentText( QString::fromStdString(pSettingsFile ->sThemeName) );
}

void SettingsWindow::showThemes()
{
    QDir directory(STYLE_THEMES_PATH_FROM_EXE);
    QStringList themes = directory .entryList(QDir::Files);

    for (int i = 0;   i < themes .size();   i++)
    {
        int pos = themes[i].lastIndexOf(QChar('.'));
        ui ->comboBox_themes ->addItem(themes[i].left(pos));
    }
}

