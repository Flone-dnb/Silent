#pragma once

#include <QMainWindow>

class string;
class QMouseEvent;

namespace Ui {
class connectWindow;
}

class connectWindow : public QMainWindow
{
    Q_OBJECT

public:

    explicit connectWindow(QWidget *parent = nullptr);

    ~connectWindow();

signals:

    void connectTo(std::string adress, std::string port, std::string userName);

    void showMainWindow();

protected:

    void closeEvent(QCloseEvent *event);

private slots:

    void on_pushButton_clicked();

private:

    Ui::connectWindow *ui;
};
