#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/thread.hpp>

namespace net = boost::asio;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_show_teacher_schedule_btn_clicked();

    void on_prepare_data_btn_clicked();

private:
    Ui::MainWindow *ui;
    unsigned prepare_progressbar_max_value = 0u;

    net::io_context io_;
    net::ssl::context ssl_ctx_;

    boost::mutex mut_;

    net::executor_work_guard<net::io_context::executor_type> work_guard_;
    std::thread io_thread_;
    bool io_running_ = false;

};
#endif // MAINWINDOW_H
