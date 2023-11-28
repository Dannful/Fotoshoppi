#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QPixmap>
#include <QFileDialog>
#include <QCloseEvent>
#include <QtConcurrent/QtConcurrent>

const double GAUSSIAN[3][3] = {{0.0625, 0.125, 0.0625}, {0.125, 0.25, 0.125}, {0.0625, 0.125, 0.0625}};
const double LAPLACIAN[3][3] = {{0, -1, 0}, {-1, 4, -1}, {0, -1, 0}};
const double HIGH_PASS[3][3] = {{-1, -1, -1}, {-1, 8, -1}, {-1, -1, -1}};
const double PREWITT_HX[3][3] = {{-1, 0, 1}, {-1, 0, 1}, {-1, 0, 1}};
const double PREWITT_HY[3][3] = {{-1, -1, -1}, {0, 0, 0}, {1, 1, 1}};
const double SOBEL_HX[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
const double SOBEL_HY[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

bool MainWindow::requestImage() {
    auto file_name = QFileDialog::getOpenFileName(this, "Select image", ".", "Image files (*.jpg *.jpeg *.png *.bmp)");
    if(file_name.isNull() || file_name.isEmpty())
        return false;
    original_image = ImageWidget::create("Original image", file_name);
    processed_image = ImageWidget::create("Processed image", file_name);
    return true;
}

MainWindow::~MainWindow()
{
    delete original_image;
    delete processed_image;
    delete ui;
}


void MainWindow::on_grayscale_button_clicked()
{
    processed_image->grayscale();
}


void MainWindow::on_copy_clicked()
{
    processed_image->refreshImage(original_image->getImagePath());
}


void MainWindow::on_vert_mirror_clicked()
{
    processed_image->mirrorVertically();
}


void MainWindow::on_hor_mirror_clicked()
{
    processed_image->mirrorHorizontally();
}


void MainWindow::on_quantize_button_clicked()
{
    processed_image->quantize(ui->quantize_tones->value());
}

void MainWindow::on_saveButton_clicked()
{
    auto fileName = QFileDialog::getSaveFileName(this, tr("Save Image File"), QString(), tr("Images (*.jpg)"));
    if(fileName.isNull() || fileName.isEmpty())
        return;
    processed_image->saveAsJPG(fileName);
}


void MainWindow::on_histogramButton_clicked()
{
    processed_image->showHistogram();
}


void MainWindow::on_addBrightnessButton_clicked()
{
    processed_image->addBrightness(ui->brightness->value());
}


void MainWindow::on_contrastButton_clicked()
{
    processed_image->addContrast(ui->contrast->value());
}


void MainWindow::on_negativeButton_clicked()
{
    processed_image->negative();
}


void MainWindow::on_equalizeButton_clicked()
{
    processed_image->equalize();
}


void MainWindow::on_matchHistogramButton_clicked()
{
    auto fileName = QFileDialog::getOpenFileName(this, tr("Open Image File"), QString(), tr("Image files (*.jpg *.jpeg *.png *.bmp)"));
    if(fileName.isNull() || fileName.isEmpty())
        return;
    processed_image->matchHistogram(QPixmap(fileName).toImage());
}


void MainWindow::on_zoomOutButton_clicked()
{
    processed_image->zoomOut(ui->zoomOutX->value(), ui->zoomOutY->value());
}


void MainWindow::on_zoomInButton_clicked()
{
    processed_image->zoomIn();
}


void MainWindow::on_rotateLeftButton_clicked()
{
    processed_image->rotateLeft();
}


void MainWindow::on_rotateRightButton_clicked()
{
    processed_image->rotateRight();
}

void MainWindow::on_convEffect_currentIndexChanged(int index)
{
    double kernels[7][3][3];
    for(int i = 0; i < 3; i++) {
        for(int j = 0; j < 3; j++) {
            kernels[0][i][j] = GAUSSIAN[i][j];
            kernels[1][i][j] = LAPLACIAN[i][j];
            kernels[2][i][j] = HIGH_PASS[i][j];
            kernels[3][i][j] = PREWITT_HX[i][j];
            kernels[4][i][j] = PREWITT_HY[i][j];
            kernels[5][i][j] = SOBEL_HX[i][j];
            kernels[6][i][j] = SOBEL_HY[i][j];
        }
    }
    ui->conv0->setValue(kernels[index][0][0]);
    ui->conv1->setValue(kernels[index][0][1]);
    ui->conv2->setValue(kernels[index][0][2]);
    ui->conv3->setValue(kernels[index][1][0]);
    ui->conv4->setValue(kernels[index][1][1]);
    ui->conv5->setValue(kernels[index][1][2]);
    ui->conv6->setValue(kernels[index][2][0]);
    ui->conv7->setValue(kernels[index][2][1]);
    ui->conv8->setValue(kernels[index][2][2]);
}


void MainWindow::on_convolveButton_clicked()
{
    double chosenKernel[3][3] = {
        {ui->conv0->value(), ui->conv1->value(), ui->conv2->value()},
        {ui->conv3->value(), ui->conv4->value(), ui->conv5->value()},
        {ui->conv6->value(), ui->conv7->value(), ui->conv8->value()}
    };
    bool add = false;
    for(int i = 0; i < 3; i++)
        for(int j = 0; j < 3; j++)
            if(chosenKernel[i][j] != GAUSSIAN[i][j] && chosenKernel[i][j] != LAPLACIAN[i][j] && chosenKernel[i][j] != HIGH_PASS[i][j])
                add = true;
    processed_image->convolve(chosenKernel, add);
}

