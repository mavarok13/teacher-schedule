#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QMessageBox>

#include <filesystem>
#include <iostream>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/url.hpp>
#include <boost/xpressive/xpressive.hpp>

#include "../TeacherSchedule/src/core.hpp"
#include "../TeacherSchedule/src/app.hpp"
#include "../TeacherSchedule/src/file_loader.hpp"
#include "../TeacherSchedule/src/excel_utils.hpp"
#include "../TeacherSchedule/src/http_client.hpp"

namespace fs = std::filesystem;

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

using namespace boost::xpressive;

using Request = http::request<http::string_body>;
using Response = http::response<http::string_body>;
using RequestHandler = std::function<void(Response)>;

using ResponseFile = http::response<http::dynamic_body>;
using RequestFileHandler = std::function<void(ResponseFile)>;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow), io_{}, ssl_ctx_{net::ssl::context{net::ssl::context::sslv23}}, work_guard_(net::make_work_guard(io_))
{
    ssl_ctx_.set_default_verify_paths();

    ui->setupUi(this);

    io_running_ = true;
    io_thread_ = std::thread([this]() {
        io_.run();
    });
}

MainWindow::~MainWindow()
{
    work_guard_.reset();  // разрешаем io_context завершиться
    io_.stop();
    if (io_thread_.joinable()) {
        io_thread_.join();
    }

    delete ui;
}

void MainWindow::on_show_teacher_schedule_btn_clicked()
{
    ui->tableWidget->setRowCount(0);

    try {

    default_converter conv;

    fs::path schedules_directory_path{"schedules"};

    auto teacher_name = ui->teacher_name_input_field->text();

    if (fs::exists(schedules_directory_path)) {
        for (const auto & dir_entry : fs::directory_iterator(schedules_directory_path)) {
            if (dir_entry.path().extension() == ".csv") {
                try {
                app::TableWeekMarkup twm = app::MarkupTable(dir_entry.path().string());

                    if (twm.GetDaysCells()[0].x == -1 || twm.GetDaysCells()[0].y == -1) {
                        std::remove(schedules_directory_path.c_str());
                        continue;
                    }

                auto teacher_info = app::GetTeacherLessons(conv.from_bytes(teacher_name.toUtf8().constData()), twm);

                for (auto lesson : teacher_info) {
                    int row = ui->tableWidget->rowCount();
                    ui->tableWidget->insertRow(row);

                    ui->tableWidget->setItem(row, 0, new QTableWidgetItem(QString::fromStdWString(lesson.GetTeacherName())));
                    ui->tableWidget->setItem(row, 1, new QTableWidgetItem((lesson.IsEvenWeek() ? QString::fromUtf8("Четная") : QString::fromUtf8("Нечетная"))));

                    ui->tableWidget->setItem(row, 2, new QTableWidgetItem(QString::fromStdWString(app::DAYS_NAMES[lesson.GetDay()])));

                    ui->tableWidget->setItem(row, 3, new QTableWidgetItem(QString::fromStdWString(lesson.GetTime())));
                    ui->tableWidget->setItem(row, 4, new QTableWidgetItem(QString::fromStdWString(lesson.GetSubjectName())));
                    ui->tableWidget->setItem(row, 5, new QTableWidgetItem(QString::fromStdWString(lesson.GetSubjectType())));
                    ui->tableWidget->setItem(row, 6, new QTableWidgetItem(QString::fromStdWString(lesson.GetAuditorium())));

                }
                } catch (...) {}
            }
        }
    }

    } catch (const std::exception & ex) {
        QMessageBox::critical(this, "Ошибка", ex.what());
    }
}


void MainWindow::on_prepare_data_btn_clicked()
{
    try {

    boost::urls::url_view uv{"https://rguk.ru/students/schedule/"};
    boost::urls::url url = uv;

    Request req{http::verb::get, url.path(), 11};
    req.set(http::field::host, url.host());
    req.set(http::field::accept, "text/plain");
    req.set(http::field::user_agent, "Boost");

    boost::system::error_code ec;
    http::response<http::string_body> res;

    auto client = http_client::HttpClient<Request, Response, RequestHandler>::Create(io_, url, ssl_ctx_, std::move(req), std::move(res), [this, url] (Response res) {
        std::string html_text = res.body().data();

        sregex re = sregex::compile("<a[^>]*href=\"((?!https?:\\/\\/)[^\"]*\\/([^\\/]*\\.xlsx))\"[^>]*>[^<]*<span>");

        sregex_iterator words_begin(html_text.begin(), html_text.end(), re);
        sregex_iterator words_end;

        if (words_begin == words_end) {
            QMessageBox::critical(this, "Ошибка", "Ссылок не найдено.\n");
        } else {
            for (auto it = words_begin; it != words_end; ++it) {

                {
                    boost::lock_guard<boost::mutex> lock{mut_};
                    ++prepare_progressbar_max_value;
                }

                std::string schedule_file_name = (*it)[2];

                boost::urls::encoding_opts opt;
                opt.space_as_plus = false;

                std::string file_part = (*it)[1];

                // Разбиваем по слэшам
                std::stringstream ss(file_part);
                std::string segment;
                std::vector<std::string> segments;
                while (std::getline(ss, segment, '/')) {
                    segments.push_back(boost::urls::encode(segment, boost::urls::pchars, opt));
                }

                // Собираем обратно с '/'
                std::string encoded_path;
                for (size_t i = 0; i < segments.size(); ++i) {
                    encoded_path += segments[i];
                    if (i + 1 < segments.size())
                        encoded_path += '/';
                }

                std::string full_url = std::string("https://rguk.ru") + encoded_path;

                boost::urls::url_view uv_file{full_url};
                boost::urls::url url_file = uv_file;

                // std::cout << url_file << std::endl;

                fs::path schedule_file_path{"schedules/"};
                schedule_file_path+=schedule_file_name;

                std::cout << schedule_file_path.string() << std::endl;
                // std::cout << schedule_file_name << std::endl;

                Request req{http::verb::get, url_file.path(), 11};
                req.set(http::field::host, url.host());
                req.set(http::field::accept, "*/*");
                req.set(http::field::user_agent, "Boost");

                boost::system::error_code ec;

                auto client = http_client::HttpClient<Request, ResponseFile, RequestFileHandler>::Create(io_, url, ssl_ctx_, std::move(req), ResponseFile{}, [this, schedule_file_path] (ResponseFile res) {
                    {
                        boost::lock_guard<boost::mutex> lock{mut_};
                        ui->prepare_progressbar->setValue(ui->prepare_progressbar->value()+1);
                    }
                    net::post(io_, [this, res, schedule_file_path] () {
                    if (res.result() == http::status::ok) {
                        std::vector<char> body_data;
                        body_data.reserve(res.body().size());
                        for (const auto& buf : res.body().data()) {
                            const char* begin = static_cast<const char*>(buf.data());
                            const char* end = begin + buf.size();
                            body_data.insert(body_data.end(), begin, end);
                        }

                        std::ofstream file(schedule_file_path, std::ios::binary);
                        if (file.is_open()) {
                            file.write(
                                body_data.data(),
                                body_data.size()
                                );
                            file.close();

                            // Теперь конвертируем
                            net::post(io_, [this, schedule_file_path] () {
                            try {
                                // ++prepare_progressbar_max_value;
                                excel_utils::ConvertExcelToCsv(schedule_file_path.c_str(), "schedules");
                            } catch (const std::exception & ex) {
                                // QString err_msg = QString::fromStdString("[ERROR] Couldn't convert file " + schedule_file_path.string() + ": " + ex.what());
                                // QMessageBox::critical(this, "Ошибка", err_msg);
                            }
                            });
                        } else {
                            std::remove(schedule_file_path.c_str());
                        }
                    } else {
                        std::remove(schedule_file_path.c_str());
                    }
                    });
                });

                client->SendRequest();
            }

            {
                boost::lock_guard<boost::mutex> lock{mut_};
                ui->prepare_progressbar->setMaximum(prepare_progressbar_max_value);
            }
        }
    });

    client->SendRequest();

    // io_.run();

    } catch (const std::exception & ex) {
        QMessageBox::critical(this, "Ошибка", ex.what());
    }
}

