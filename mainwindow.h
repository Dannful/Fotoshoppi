#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "image_widget.cpp"

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    bool requestImage();

private slots:
    void on_grayscale_button_clicked();

    void on_copy_clicked();

    void on_vert_mirror_clicked();

    void on_hor_mirror_clicked();

    void on_quantize_button_clicked();

    void on_saveButton_clicked();

    void on_histogramButton_clicked();

    void on_addBrightnessButton_clicked();

    void on_contrastButton_clicked();

    void on_negativeButton_clicked();

    void on_equalizeButton_clicked();

    void on_matchHistogramButton_clicked();

    void on_zoomOutButton_clicked();

    void on_zoomInButton_clicked();

    void on_rotateLeftButton_clicked();

    void on_rotateRightButton_clicked();

    void on_convEffect_currentIndexChanged(int index);

    void on_convolveButton_clicked();

private:
    ImageWidget *original_image = nullptr, *processed_image = nullptr;
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
