#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <dazeus.h>
#include <QListWidget>

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void changeEvent(QEvent *e);

private slots:
    void on_pushButton_clicked();
    void newEvent(DaZeus::Event*);
    void getCurrentNetworks();
    void on_networks_currentTextChanged(QString currentText);
    void on_toolButton_clicked();
    void on_toolButton_2_clicked();
    void on_sendMessage_clicked();

private:
    Ui::MainWindow *ui;
    DaZeus *d;
};

#endif // MAINWINDOW_H
