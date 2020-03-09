#pragma once
#include <QMainWindow>

namespace Ui {
class RoomPassInputWindow;
}

class RoomPassInputWindow : public QMainWindow
{
    Q_OBJECT

signals:

    void signalEnterRoomWithPassword(QString sRoomName, QString sPassword);

public:

    explicit RoomPassInputWindow(QString sRoomName, QWidget *parent = nullptr);



    ~RoomPassInputWindow() override;

protected:

    void keyPressEvent (QKeyEvent* event) override;
    void closeEvent    (QCloseEvent* ev) override;

private slots:

    void on_pushButton_clicked();

private:

    Ui::RoomPassInputWindow *ui;

    QString sRoomName;
};
