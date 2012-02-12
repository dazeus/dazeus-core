#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    MainWindow m;
    m.show();
    return app.exec();
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    d(new DaZeus())
{
    ui->setupUi(this);
    ui->tab_2->setDisabled(true);
    connect(d,  SIGNAL(newEvent(DaZeus::Event*)),
            this, SLOT(newEvent(DaZeus::Event*)));
    ui->tabWidget->setCurrentIndex(0);
}

void MainWindow::newEvent(DaZeus::Event *e)
{
    if(ui->networks->currentItem() == 0) return;
    if(ui->channels->currentItem() == 0) return;
    if(e->parameters.size() < 3) return;

    QString network = ui->networks->currentItem()->text();
    if(e->parameters[0] != network) return;
    QString channel = ui->channels->currentItem()->text();
    QString message = "<" + e->parameters[1] + "> " + e->parameters[3];
    ui->messages->addItem(message);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void MainWindow::on_pushButton_clicked()
{
    // Start connecting
    QString socketfile = ui->lineEdit->text();
    ui->label_3->setText("Connecting...");
    if(ui->comboBox->currentIndex() == 0)
        socketfile.prepend("unix:");
    else
        socketfile.prepend("tcp:");
    if(d->open(socketfile)) {
        ui->label_3->setText("");
        ui->tab_2->setDisabled(false);
        ui->tabWidget->setCurrentIndex(1);
        ui->tab->setDisabled(true);
        getCurrentNetworks();
        d->subscribe(QStringList() << "PRIVMSG");
    } else {
        ui->label_3->setText(d->error());
    }
}

void MainWindow::getCurrentNetworks()
{
    QStringList networks = d->networks();
    QModelIndex selection = ui->networks->currentIndex();
    ui->networks->clear();
    ui->networks->addItems(networks);
    ui->networks->setCurrentIndex(selection);
    ui->messages->clear();
}

void MainWindow::on_networks_currentTextChanged(QString network)
{
    QStringList channels = d->channels(network);
    QModelIndex selection = ui->networks->currentIndex();
    ui->channels->clear();
    ui->channels->addItems(channels);
    ui->channels->setCurrentIndex(selection);
    ui->messages->clear();
}

void MainWindow::on_toolButton_clicked()
{
    getCurrentNetworks();
}

void MainWindow::on_toolButton_2_clicked()
{
    on_networks_currentTextChanged(ui->networks->currentItem()->text());
}

void MainWindow::on_sendMessage_clicked()
{
    if(ui->networks->currentItem() == 0) return;
    if(ui->channels->currentItem() == 0) return;

    QString msg = ui->message->text();
    ui->message->clear();
    d->message(ui->networks->currentItem()->text(),
               ui->channels->currentItem()->text(), msg);
    // TODO use getNick()
    QString message = "<YOU> " + msg;
    ui->messages->addItem(message);
}
