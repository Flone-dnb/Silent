#include "settingswindow.h"
#include "ui_settingswindow.h"

// Qt
#include <QKeyEvent>
#include <QMouseEvent>
#include <QMessageBox>

// C++
#include <fstream>
#include <shlobj.h>

SettingsWindow::SettingsWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::SettingsWindow)
{
    ui->setupUi(this);
    setFixedSize(width(), height());

    bWaitingForButton = false;

    TCHAR my_documents[MAX_PATH];
    HRESULT result = SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, my_documents);

    if (result != S_OK)
    {
        QMessageBox::warning(nullptr, "Warning", "Error. Settings will not be saved.");
    }
    else
    {
        std::wstring adressToSettings = std::wstring(my_documents);
        adressToSettings += L"\\SilentSettings.data";

        std::ifstream settingsFile (adressToSettings, std::ios::binary);

        if (settingsFile.is_open())
        {
            settingsFile.read(reinterpret_cast<char*>(&iPushToTalkKey), 4);
            showKeyOnScreen(iPushToTalkKey);

            unsigned short int iVolume = 0;
            settingsFile.read(reinterpret_cast<char*>(&iVolume), 2);

            ui->horizontalSlider->setValue(iVolume);

            settingsFile.close();
        }
        else
        {
            std::ofstream newSettingFile(adressToSettings, std::ios::binary);

            // T button
            iPushToTalkKey = 0x54;
            newSettingFile.write(reinterpret_cast<char*>(&iPushToTalkKey), 4);

            // 100%
            unsigned short int iVolume = 65535;
            newSettingFile.write(reinterpret_cast<char*>(&iVolume), 2);

            // No name
            unsigned char nameSize = 0;
            newSettingFile.write(reinterpret_cast<char*>(&nameSize), 1);

            newSettingFile.close();

            ui->pushButton->setText("T");
        }
    }

    refreshSliderText();
}





void SettingsWindow::keyPressEvent(QKeyEvent *event)
{
    if (bWaitingForButton)
    {
        int key = -1;

        if ( (event->key() >= 0x41) && (event->key() <= 0x5a) )
        {
            // From A to Z

            key = event->key();

            ui->pushButton->setText( event->text().toUpper() );
        }
        else if ( event->key() == Qt::Key_Alt )
        {
            // 0x12 Virtual Code for GetAsyncKeyState

            key = 0x12;

            ui->pushButton->setText("Alt");
        }

        if (key != -1)
        {
            iPushToTalkKey = key;
            bWaitingForButton = false;
        }
    }
}

void SettingsWindow::mousePressEvent(QMouseEvent *event)
{
    if (bWaitingForButton)
    {
        int key = -1;

        if ( event->button() == Qt::XButton2 )
        {
            // 0x06 Virtual Code for GetAsyncKeyState

            key = 0x06;

            ui->pushButton->setText("X2");
        }

        if (key != -1)
        {
            iPushToTalkKey = key;
            bWaitingForButton = false;
        }
    }
}

void SettingsWindow::on_pushButton_clicked()
{
    ui->pushButton->setText("Waiting for button");
    bWaitingForButton = true;
}

void SettingsWindow::showKeyOnScreen(int key)
{
    QString buttonText = "";

    if ( (key >= 0x41) && (key <= 0x5a) )
    {
        // From A to Z
        buttonText += static_cast<char>(key);
    }
    else if ( key == 0x12 )
    {
        // Alt
        buttonText += "Alt";
    }
    else if ( key == 0x06 )
    {
        // X2
        buttonText += "X2";
    }

    ui->pushButton->setText(buttonText);
}

void SettingsWindow::setKeyInSettingsFile(int key)
{
    TCHAR my_documents[MAX_PATH];
    HRESULT result = SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, my_documents);

    if (result != S_OK)
    {
        QMessageBox::warning(nullptr, "Warning", "Error. Settings will not be saved.");
    }
    else
    {
        std::wstring adressToSettings = std::wstring(my_documents);
        std::wstring adressToTempSettings = std::wstring(my_documents);
        adressToSettings += L"\\SilentSettings.data";
        adressToTempSettings += L"\\SilentSettings~.data";

        std::ifstream settingsFile (adressToSettings, std::ios::binary);

        if (settingsFile.is_open())
        {
            // Settings file exists
            // Rewrite it

            // Get old settings file size
            int iFileSize = 0;
            settingsFile.seekg(0, std::ios::end);
            iFileSize = settingsFile.tellg();
            settingsFile.seekg(6);
            iFileSize -= 6;

            std::ofstream newSettingsFile(adressToTempSettings, std::ios::binary);

            unsigned short int volume = static_cast<unsigned short int>(ui->horizontalSlider->value());

            newSettingsFile.write(reinterpret_cast<char*>(&key), 4);
            newSettingsFile.write(reinterpret_cast<char*>(&volume), 2);

            char byte = 0;

            for (int i = 0; i < iFileSize; i++)
            {
                settingsFile.read(&byte, 1);
                newSettingsFile.write(&byte, 1);
            }

            settingsFile.close();
            newSettingsFile.close();

            _wremove(adressToSettings.c_str());
            _wrename(adressToTempSettings.c_str(), adressToSettings.c_str());
        }
        else
        {
            // Seems like settings file does not exist
            // Create settings file

            std::ofstream newSettingsFile(adressToSettings, std::ios::binary);

            newSettingsFile.write(reinterpret_cast<char*>(&key), 4);

            unsigned short int volume = static_cast<unsigned short int>(ui->horizontalSlider->value());
            newSettingsFile.write(reinterpret_cast<char*>(&volume), 2);

            newSettingsFile.close();
        }

        emit signalSetPushToTalkButton(key, static_cast<unsigned short int>(ui->horizontalSlider->value()));
    }
}

void SettingsWindow::refreshSliderText()
{
    // 0%    -  ui ->horizontalSlider ->minimum()
    // 100%  -  ui ->horizontalSlider ->maximum()

    int iInterval = ui ->horizontalSlider ->maximum()  -  ui ->horizontalSlider ->minimum();
    int iCurrent  = ui ->horizontalSlider ->value();

    ui ->label_4 ->setText (QString::number( (iCurrent / static_cast<double>(iInterval) * 100), 'f', 0 ) + "%");
}

void SettingsWindow::on_horizontalSlider_valueChanged(int value)
{
    refreshSliderText();
}



SettingsWindow::~SettingsWindow()
{
    delete ui;
}

void SettingsWindow::on_pushButton_2_clicked()
{
    setKeyInSettingsFile(iPushToTalkKey);
    close();
}

