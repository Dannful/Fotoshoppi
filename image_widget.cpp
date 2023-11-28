#include <QGridLayout>
#include <QWidget>
#include <qlabel.h>
#include <QWindow>

#include <QtConcurrent/QtConcurrent>

#include <QtCharts/QChart>
#include <QtCharts/QStackedBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QChartView>

#include <bits/stdc++.h>

struct ImageData {
    int width, height;
    int size;
    QRgb *pixels;
    int index;
    int columnIndex, rowIndex;
};

class ImageWidget {
public:

    QString getImagePath() {
        return imagePath;
    }

    static ImageWidget* create(QString title, QString imagePath) {
        QPixmap pix(imagePath);
        auto window = new QWidget;
        QGridLayout *layout = new QGridLayout(window);
        auto image = new QLabel(QString());
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(image, 0, 0, Qt::AlignCenter);
        window->setLayout(layout);
        window->setWindowTitle(title);
        auto result = new ImageWidget(window, image, imagePath);
        result->updateImage(pix);
        window->show();
        return result;
    }

    void refreshImage(QString imagePath) {
        updateImage(QPixmap(imagePath));
    }

    void grayscale() {
        updateImage(grayscale(image->pixmap().toImage()));
    }

    void mirrorVertically() {
        auto pixmap = image->pixmap();
        auto convertedImage = pixmap.toImage();
        auto height = convertedImage.height();
        auto width = convertedImage.width();
        QRgb *bits = (QRgb*) convertedImage.bits();
        for(int column = 0; column < width; column++) {
            for(int row = 0; row < height / 2; row++) {
                auto pixel = column + row * width;
                auto mirror_pixel = column + (height - 1 - row) * width;
                QRgb aux = bits[pixel];
                bits[pixel] = bits[mirror_pixel];
                bits[mirror_pixel] = aux;
            }
        }
        updateImage(convertedImage);
    }

    void mirrorHorizontally() {
        auto pixmap = image->pixmap();
        auto convertedImage = pixmap.toImage();
        auto height = convertedImage.height();
        auto width = convertedImage.width();
        QRgb *bits = (QRgb*) convertedImage.bits();
        for(int i = 0; i < height; i++) {
            QRgb aux[width / 2];
            auto line_start = bits + i * width;
            std::reverse_copy(line_start + (int) ceil(width / 2), line_start + width, aux);
            std::reverse_copy(line_start, line_start + (int) (width / 2) + 1, line_start + (int) ceil(width / 2));
            memmove(line_start, aux, sizeof(QRgb) * (int) (width / 2));
        }
        updateImage(convertedImage);
    }

    void quantize(uint8_t tones) {
        if(tones == 0)
            return;
        int min_tone = 256, max_tone = 0;
        auto result = onPixels([&min_tone, &max_tone](ImageData data) {
            auto tone = qRed(data.pixels[data.index]);
            if(tone > max_tone)
                max_tone = tone;
            if(tone < min_tone)
                min_tone = tone;
        });
        int intervals = max_tone - min_tone + 1;
        if(tones >= intervals)
            return;
        auto offset = min_tone - 0.5f;
        float intervalLength = intervals / tones;
        result = onPixels(result, [offset, intervalLength, this](ImageData data) {
            auto newColor = retrieveNewQuantizedColor(qRed(data.pixels[data.index]), offset, intervalLength);
            data.pixels[data.pixels[data.index]] = QColor(newColor, newColor, newColor).rgb();
        });
        updateImage(result);
    }

    void showHistogram() {
        showHistogram("Histogram", image->pixmap().toImage());
    }

    void saveAsJPG(QString path) {
        image->pixmap().save(path);
    }

    void addBrightness(int brightness) {
        auto result = onPixels([brightness, this](ImageData data) {
            auto pixel = data.pixels[data.index];
            auto red = brightness + qRed(pixel);
            auto green = brightness + qGreen(pixel);
            auto blue = brightness + qBlue(pixel);
            data.pixels[data.index] = QColor(coerceWithinRange(red), coerceWithinRange(green), coerceWithinRange(blue)).rgb();
        });
        updateImage(result);
    }

    void addContrast(int contrast) {
        auto result = onPixels([contrast, this](ImageData data) {
            auto pixel = data.pixels[data.index];
            auto red = contrast * qRed(pixel);
            auto green = contrast * qGreen(pixel);
            auto blue = contrast * qBlue(pixel);
            data.pixels[data.index] = QColor(coerceWithinRange(red), coerceWithinRange(green), coerceWithinRange(blue)).rgb();
        });
        updateImage(result);
    }

    void negative() {
        auto result = onPixels([](ImageData data) {
            auto pixel = data.pixels[data.index];
            auto red = 255 - qRed(pixel);
            auto green = 255 - qGreen(pixel);
            auto blue = 255 - qBlue(pixel);
            data.pixels[data.index] = QColor(red, green, blue).rgb();
        });
        updateImage(result);
    }

    void equalize() {
        auto imageAux = image->pixmap().toImage();
        auto cumulativeHistogram = calculateNormalizedHistogram();
        auto result = onPixels([cumulativeHistogram](ImageData data) {
            auto pixel = data.pixels[data.index];
            data.pixels[data.index] = QColor(cumulativeHistogram[qRed(pixel)], cumulativeHistogram[qGreen(pixel)], cumulativeHistogram[qBlue(pixel)]).rgb();
        });
        updateImage(result);
        showHistogram("Original histogram", imageAux);
        showHistogram("New histogram", result);
    }

    void matchHistogram(QImage target) {
        grayscale();
        int histogramMatch[256];
        auto sourceHistogram = calculateNormalizedHistogram(image->pixmap().toImage());
        auto targetHistogram = calculateNormalizedHistogram(grayscale(target));
        for(int i = 0; i < 256; i++)
            histogramMatch[i] = findClosestShade(sourceHistogram, targetHistogram, i);
        updateImage(onPixels([histogramMatch](ImageData data) {
            auto shade = qRed(data.pixels[data.index]);
            data.pixels[data.index] = QColor(histogramMatch[shade], histogramMatch[shade], histogramMatch[shade]).rgb();
        }));
        delete sourceHistogram;
        delete targetHistogram;
    }

    void zoomOut(int offsetX, int offsetY) {
        QImage newImage = QImage(ceil(image->pixmap().width() / (double) offsetX), ceil(image->pixmap().height() / (double) offsetY), QImage::Format_RGB32);
        auto pixels = (QRgb*) newImage.bits();
        auto currentPixel = 0;
        onPixels([&currentPixel, pixels, offsetX, offsetY](ImageData data) {
            int count = 0;
            int red = 0, green = 0, blue = 0;
            if(data.columnIndex % offsetX != 0)
                return;
            if(data.rowIndex % offsetY != 0)
                return;
            for(int x = 0; x < offsetX; x++) {
                for(int y = 0; y < offsetY; y++) {
                    auto targetPixelIndex = data.index + x + y * data.width;
                    if(data.columnIndex + x >= data.width || data.columnIndex + x < 0)
                        continue;
                    if(data.rowIndex + y >= data.height || data.rowIndex + y < 0)
                        continue;
                    red += qRed(data.pixels[targetPixelIndex]);
                    green += qGreen(data.pixels[targetPixelIndex]);
                    blue += qBlue(data.pixels[targetPixelIndex]);
                    count++;
                }
            }
            if(count == 0)
                return;
            pixels[currentPixel++] = QColor(red / count, green / count, blue / count).rgb();
        });
        updateImage(newImage);
    }

    void rotateLeft() {
        QImage newImage = QImage(image->pixmap().height(), image->pixmap().width(), QImage::Format_RGB32);
        auto pixels = (QRgb*) newImage.bits();
        onPixels([pixels](ImageData data) {
            pixels[(data.width - data.columnIndex - 1) * data.height + data.rowIndex] = data.pixels[data.index];
        });
        updateImage(newImage);
    }

    void rotateRight() {
        QImage newImage = QImage(image->pixmap().height(), image->pixmap().width(), QImage::Format_RGB32);
        auto pixels = (QRgb*) newImage.bits();
        onPixels([pixels](ImageData data) {
            pixels[data.columnIndex * data.height + (data.height - data.rowIndex - 1)] = data.pixels[data.index];
        });
        updateImage(newImage);
    }

    void zoomIn() {
        QImage newImage = QImage(image->pixmap().width() * 2 - 1, image->pixmap().height() * 2 - 1, QImage::Format_RGB32);
        auto pixels = (QRgb*) newImage.bits();
        auto currentPixelIndex = 0;
        auto newImageWidth = newImage.width();
        onPixels([newImageWidth, &currentPixelIndex, pixels](ImageData data) {
            pixels[currentPixelIndex] = data.pixels[data.index];
            auto columnIndex = currentPixelIndex % newImageWidth;
            if(columnIndex + 2 >= newImageWidth) {
                currentPixelIndex += 2 * newImageWidth - columnIndex;
                return;
            }
            currentPixelIndex += 2;
        });
        auto result = onPixels(newImage, [this](ImageData data) {
            if(data.rowIndex % 2 == 0 && data.columnIndex % 2 == 1) {
                QList list = QList<QRgb>();
                list << data.pixels[data.index - 1];
                if(data.columnIndex < data.width - 1)
                    list << data.pixels[data.index + 1];
                data.pixels[data.index] = average(list);
                return;
            }
            if(data.rowIndex % 2 == 1) {
                QList list = QList<QRgb>();
                list << data.pixels[data.index - data.width];
                if(data.rowIndex < data.height - 1)
                    list << data.pixels[data.index + data.width];
                data.pixels[data.index] = average(list);
            }
        });
        updateImage(result);
    }

    void convolve(double kernel[3][3], bool add) {
        double kernel_copy[3][3];
        memcpy(kernel_copy, kernel, sizeof(double) * 9);
        flip(kernel_copy);
        QImage copy = image->pixmap().toImage();
        auto pixels = (QRgb*) copy.bits();
        auto result = onPixels([this, pixels, add, kernel_copy](ImageData data) {
            int red = 0, green = 0, blue = 0;
            for(int i = -1; i <= 1; i++) {
                for(int j = -1; j <= 1; j++) {
                    if(data.columnIndex + j < 0 || data.columnIndex + j >= data.width)
                        return;
                    if(data.rowIndex + i < 0 || data.rowIndex + i >= data.height)
                        return;
                    auto targetPixel = pixels[data.index + i * data.width + j];
                    float factor = kernel_copy[i + 1][j + 1];
                    red += factor * qRed(targetPixel);
                    green += factor * qGreen(targetPixel);
                    blue += factor * qBlue(targetPixel);
                }
            }
            auto increment = add ? 127 : 0;
            data.pixels[data.index] = QColor(coerceWithinRange(red + increment), coerceWithinRange(green + increment), coerceWithinRange(blue + increment)).rgb();
        });
        updateImage(result);
    }

private:
    QWidget *window;
    QLabel *image;
    QString imagePath;

    ImageWidget(QWidget *window, QLabel *image, QString imagePath) {
        this->window = window;
        this->image = image;
        this->imagePath = imagePath;
    }

    uint8_t retrieveNewQuantizedColor(int tone, float offset, uint8_t intervalLength) {
        auto index = floor((tone - offset) / intervalLength);
        auto lowerBound = offset + intervalLength * index;
        auto upperBound = lowerBound + intervalLength;
        return (lowerBound + upperBound) / 2;
    }

    void updateImage(QImage target) {
        QPixmap newPixmap = QPixmap::fromImage(target);
        updateImage(newPixmap);
    }

    void updateImage(QPixmap target) {
        window->setFixedSize(target.width(), target.height());
        image->setFixedSize(target.width(), target.height());
        image->setPixmap(target);
    }

    QImage onPixels(std::function<void(ImageData)> action) {
        auto convertedImage = image->pixmap().toImage();
        return onPixels(convertedImage, action);
    }

    QImage onPixels(QImage base, std::function<void(ImageData)> action) {
        auto convertedImage = base;
        auto height = convertedImage.height();
        auto width = convertedImage.width();
        auto size = width * height;
        ImageData imageData;
        imageData.width = width;
        imageData.height = height;
        imageData.size = size;
        imageData.pixels = (QRgb*) convertedImage.bits();
        for(int i = 0; i < size; i++) {
            imageData.index = i;
            imageData.columnIndex = i % width;
            imageData.rowIndex = i / width;
            action(imageData);
        }
        return convertedImage;
    }

    int coerceWithinRange(int value) {
        if(value > 255)
            return 255;
        if(value < 0)
            return 0;
        return value;
    }

    QImage grayscale(QImage target) {
        return onPixels(target, [](ImageData data) {
            auto pixelValue = data.pixels[data.index];
            int luminance = qRed(pixelValue) * 0.299 + qGreen(pixelValue) * 0.587 + qBlue(pixelValue) * 0.114;
            data.pixels[data.index] = QColor(luminance, luminance, luminance).rgb();
        });
    }

    uint32_t *calculateHistogram() {
        return calculateHistogram(image->pixmap().toImage());
    }

    uint32_t *calculateHistogram(QImage target) {
        QImage grayscaled = grayscale(target);
        const int pixelSize = 256;
        uint32_t *counts = new uint32_t[pixelSize]();
        onPixels(grayscaled, [counts](ImageData data) {
            counts[qRed(data.pixels[data.index])]++;
        });
        return counts;
    }

    uint32_t *calculateNormalizedHistogram(QImage target) {
        float factor = 255.0 / (target.size().width() * target.size().height());
        return calculateNormalizedHistogram(target, factor);
    }

    uint32_t *calculateNormalizedHistogram(QImage target, double factor) {
        auto histogram = calculateHistogram(target);
        uint32_t *cumulativeHistogram = new uint32_t[256];
        cumulativeHistogram[0] = factor * histogram[0];
        for(int i = 1; i < 256; i++)
            cumulativeHistogram[i] = cumulativeHistogram[i - 1] + factor * histogram[i];
        return cumulativeHistogram;
    }

    uint32_t *calculateNormalizedHistogram() {
        return calculateNormalizedHistogram(image->pixmap().toImage());
    }

    void showHistogram(QString title, QImage image) {
        auto height = image.height();
        auto width = image.width();
        auto size = width * height;
        int min = size, max = 0;
        const int pixelSize = 256;
        QBarSet *sets[pixelSize] = {};
        QStringList categories;
        auto counts = calculateHistogram(image);
        for(int i = 0; i < pixelSize; i++) {
            categories << QString::number(i);
            sets[i] = new QBarSet(QString::number(i));
            QList list = QList<qreal>(256, 0);
            list.insert(i, counts[i]);
            sets[i]->append(list);
            if(counts[i] > max)
                max = counts[i];
            if(counts[i] < min)
                min = counts[i];
        }
        QWidget *window = new QWidget;
        QGridLayout *layout = new QGridLayout;
        window->setLayout(layout);
        QChart *chart = new QChart();
        chart->setTitle(title);
        chart->setAnimationOptions(QChart::SeriesAnimations);
        QStackedBarSeries *series = new QStackedBarSeries();
        chart->addSeries(series);
        for(int i = 0; i < pixelSize; i++)
            series->append(sets[i]);
        QValueAxis *axisX = new QValueAxis();
        axisX->setRange(0, pixelSize - 1);
        chart->addAxis(axisX, Qt::AlignBottom);
        series->attachAxis(axisX);
        QValueAxis *axisY = new QValueAxis();
        axisY->setRange(min, max);
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisY);
        QChartView *chartView = new QChartView(chart);
        chartView->setRenderHint(QPainter::Antialiasing);
        chart->legend()->setVisible(false);
        layout->addWidget(chartView, 0, 0);
        window->setMinimumSize(720, 480);
        window->show();
        delete counts;
    }

    int findClosestShade(uint32_t *sourceHistogram, uint32_t *targetHistogram, int shade) {
        int minShade = shade, minDiff = abs((int) targetHistogram[shade] - (int) sourceHistogram[shade]);
        for(int i = 0; i < 256; i++) {
            auto diff = abs((int) targetHistogram[i] - (int) sourceHistogram[shade]);
            if(diff < minDiff) {
                minShade = i;
                minDiff = diff;
            }
        }
        return minShade;
    }

    QRgb average(QList<QRgb> values) {
        int red = 0, green = 0, blue = 0;
        for(QRgb value : values) {
            red += qRed(value);
            green += qGreen(value);
            blue += qBlue(value);
        }
        return QColor(red / values.length(), green / values.length(), blue / values.length()).rgb();
    }

    void flip(double kernel[3][3]) {
        double kernel_copy[3][3];
        memcpy(kernel_copy, kernel, sizeof(double) * 9);
        for(int i = 0; i < 3; i++)
            for(int j = 0; j < 3; j++)
                kernel[i][j] = kernel_copy[2 - i][2 - j];
    }
};
