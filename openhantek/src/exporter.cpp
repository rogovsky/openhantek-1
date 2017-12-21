// SPDX-License-Identifier: GPL-2.0+

#include <algorithm>
#include <cmath>
#include <memory>

#include <QCoreApplication>
#include <QFile>
#include <QFileDialog>
#include <QImage>
#include <QMutex>
#include <QPainter>
#include <QPixmap>
#include <QPrintDialog>
#include <QPrinter>
#include <QTextStream>

#include "exporter.h"

#include "analyse/dataanalyzerresult.h"
#include "definitions.h"
#include "glgenerator.h"
#include "settings.h"
#include "utils/dsoStrings.h"
#include "utils/printutils.h"

#define tr(msg) QCoreApplication::translate("Exporter", msg)

Exporter::Exporter(DsoSettings *settings, const QString &filename, ExportFormat format)
    : settings(settings), filename(filename), format(format) {}

Exporter *Exporter::createPrintExporter(DsoSettings *settings) {
    std::unique_ptr<QPrinter> printer = printPaintDevice(settings);
    // Show the printing dialog
    QPrintDialog dialog(printer.get());
    dialog.setWindowTitle(tr("Print oscillograph"));
    if (dialog.exec() != QDialog::Accepted) { return nullptr; }

    Exporter *exporter = new Exporter(settings, QString(), EXPORT_FORMAT_PRINTER);
    exporter->selectedPrinter = std::move(printer);
    return exporter;
}

Exporter *Exporter::createSaveToFileExporter(DsoSettings *settings) {
    QStringList filters;
    filters << tr("Portable Document Format (*.pdf)") << tr("Image (*.png *.xpm *.jpg)")
            << tr("Comma-Separated Values (*.csv)");

    QFileDialog fileDialog(nullptr, tr("Export file..."), QString(), filters.join(";;"));
    fileDialog.setFileMode(QFileDialog::AnyFile);
    fileDialog.setAcceptMode(QFileDialog::AcceptSave);
    if (fileDialog.exec() != QDialog::Accepted) return nullptr;

    return new Exporter(settings, fileDialog.selectedFiles().first(),
                        (ExportFormat)(EXPORT_FORMAT_PDF + filters.indexOf(fileDialog.selectedNameFilter())));
}

std::unique_ptr<QPrinter> Exporter::printPaintDevice(DsoSettings *settings) {
    // We need a QPrinter for printing, pdf- and ps-export
    std::unique_ptr<QPrinter> printer = std::unique_ptr<QPrinter>(new QPrinter(QPrinter::HighResolution));
    printer->setOrientation(settings->view.zoom ? QPrinter::Portrait : QPrinter::Landscape);
    printer->setPageMargins(20, 20, 20, 20, QPrinter::Millimeter);
    return printer;
}

bool Exporter::exportSamples(const DataAnalyzerResult *result) {
    if (this->format == EXPORT_FORMAT_CSV) { return exportCVS(result); }

    // Choose the color values we need
    DsoSettingsColorValues *colorValues;
    if (this->format == EXPORT_FORMAT_IMAGE && settings->view.screenColorImages)
        colorValues = &(settings->view.screen);
    else
        colorValues = &(settings->view.print);

    std::unique_ptr<QPaintDevice> paintDevice;

    if (this->format == EXPORT_FORMAT_IMAGE) {
        // We need a QPixmap for image-export
        QPixmap *qPixmap = new QPixmap(settings->options.imageSize);
        qPixmap->fill(colorValues->background);
        paintDevice = std::unique_ptr<QPaintDevice>(qPixmap);
    } else if (this->format == EXPORT_FORMAT_PRINTER) {
        paintDevice = std::move(selectedPrinter);
    } else {
        std::unique_ptr<QPrinter> printer = printPaintDevice(settings);
        printer->setOutputFileName(this->filename);
        printer->setOutputFormat((this->format == EXPORT_FORMAT_PDF) ? QPrinter::PdfFormat : QPrinter::NativeFormat);
        paintDevice = std::move(printer);
    }

    if (!paintDevice) return false;

    // Create a painter for our device
    QPainter painter(paintDevice.get());

    // Get line height
    QFont font;
    QFontMetrics fontMetrics(font, paintDevice.get());
    double lineHeight = fontMetrics.height();

    painter.setBrush(Qt::SolidPattern);

    // Draw the settings table
    double stretchBase = (double)(paintDevice->width() - lineHeight * 10) / 4;

    // Print trigger details
    painter.setPen(colorValues->voltage[settings->scope.trigger.source]);
    QString levelString = valueToString(settings->scope.voltage[settings->scope.trigger.source].trigger, UNIT_VOLTS, 3);
    QString pretriggerString = tr("%L1%").arg((int)(settings->scope.trigger.position * 100 + 0.5));
    painter.drawText(QRectF(0, 0, lineHeight * 10, lineHeight),
                     tr("%1  %2  %3  %4")
                         .arg(settings->scope.voltage[settings->scope.trigger.source].name,
                              Dso::slopeString(settings->scope.trigger.slope), levelString, pretriggerString));

    double scopeHeight;

    { // DataAnalyser mutex lock
        // Print sample count
        painter.setPen(colorValues->text);
        painter.drawText(QRectF(lineHeight * 10, 0, stretchBase, lineHeight), tr("%1 S").arg(result->sampleCount()),
                         QTextOption(Qt::AlignRight));
        // Print samplerate
        painter.drawText(QRectF(lineHeight * 10 + stretchBase, 0, stretchBase, lineHeight),
                         valueToString(settings->scope.horizontal.samplerate, UNIT_SAMPLES) + tr("/s"),
                         QTextOption(Qt::AlignRight));
        // Print timebase
        painter.drawText(QRectF(lineHeight * 10 + stretchBase * 2, 0, stretchBase, lineHeight),
                         valueToString(settings->scope.horizontal.timebase, UNIT_SECONDS, 0) + tr("/div"),
                         QTextOption(Qt::AlignRight));
        // Print frequencybase
        painter.drawText(QRectF(lineHeight * 10 + stretchBase * 3, 0, stretchBase, lineHeight),
                         valueToString(settings->scope.horizontal.frequencybase, UNIT_HERTZ, 0) + tr("/div"),
                         QTextOption(Qt::AlignRight));

        // Draw the measurement table
        stretchBase = (double)(paintDevice->width() - lineHeight * 6) / 10;
        int channelCount = 0;
        for (int channel = settings->scope.voltage.count() - 1; channel >= 0; channel--) {
            if ((settings->scope.voltage[channel].used || settings->scope.spectrum[channel].used) &&
                result->data(channel)) {
                ++channelCount;
                double top = (double)paintDevice->height() - channelCount * lineHeight;

                // Print label
                painter.setPen(colorValues->voltage[channel]);
                painter.drawText(QRectF(0, top, lineHeight * 4, lineHeight), settings->scope.voltage[channel].name);
                // Print coupling/math mode
                if ((unsigned int)channel < settings->scope.physicalChannels)
                    painter.drawText(QRectF(lineHeight * 4, top, lineHeight * 2, lineHeight),
                                     Dso::couplingString(settings->scope.voltage[channel].coupling));
                else
                    painter.drawText(QRectF(lineHeight * 4, top, lineHeight * 2, lineHeight),
                                     Dso::mathModeString(settings->scope.voltage[channel].math));

                // Print voltage gain
                painter.drawText(QRectF(lineHeight * 6, top, stretchBase * 2, lineHeight),
                                 valueToString(settings->scope.voltage[channel].gain, UNIT_VOLTS, 0) + tr("/div"),
                                 QTextOption(Qt::AlignRight));
                // Print spectrum magnitude
                if (settings->scope.spectrum[channel].used) {
                    painter.setPen(colorValues->spectrum[channel]);
                    painter.drawText(QRectF(lineHeight * 6 + stretchBase * 2, top, stretchBase * 2, lineHeight),
                                     valueToString(settings->scope.spectrum[channel].magnitude, UNIT_DECIBEL, 0) +
                                         tr("/div"),
                                     QTextOption(Qt::AlignRight));
                }

                // Amplitude string representation (4 significant digits)
                painter.setPen(colorValues->text);
                painter.drawText(QRectF(lineHeight * 6 + stretchBase * 4, top, stretchBase * 3, lineHeight),
                                 valueToString(result->data(channel)->amplitude, UNIT_VOLTS, 4),
                                 QTextOption(Qt::AlignRight));
                // Frequency string representation (5 significant digits)
                painter.drawText(QRectF(lineHeight * 6 + stretchBase * 7, top, stretchBase * 3, lineHeight),
                                 valueToString(result->data(channel)->frequency, UNIT_HERTZ, 5),
                                 QTextOption(Qt::AlignRight));
            }
        }

        // Draw the marker table
        stretchBase = (double)(paintDevice->width() - lineHeight * 10) / 4;
        painter.setPen(colorValues->text);

        // Calculate variables needed for zoomed scope
        double divs = fabs(settings->scope.horizontal.marker[1] - settings->scope.horizontal.marker[0]);
        double time = divs * settings->scope.horizontal.timebase;
        double zoomFactor = DIVS_TIME / divs;
        double zoomOffset = (settings->scope.horizontal.marker[0] + settings->scope.horizontal.marker[1]) / 2;

        if (settings->view.zoom) {
            scopeHeight = (double)(paintDevice->height() - (channelCount + 5) * lineHeight) / 2;
            double top = 2.5 * lineHeight + scopeHeight;

            painter.drawText(QRectF(0, top, stretchBase, lineHeight),
                             tr("Zoom x%L1").arg(DIVS_TIME / divs, -1, 'g', 3));

            painter.drawText(QRectF(lineHeight * 10, top, stretchBase, lineHeight),
                             valueToString(time, UNIT_SECONDS, 4), QTextOption(Qt::AlignRight));
            painter.drawText(QRectF(lineHeight * 10 + stretchBase, top, stretchBase, lineHeight),
                             valueToString(1.0 / time, UNIT_HERTZ, 4), QTextOption(Qt::AlignRight));

            painter.drawText(QRectF(lineHeight * 10 + stretchBase * 2, top, stretchBase, lineHeight),
                             valueToString(time / DIVS_TIME, UNIT_SECONDS, 3) + tr("/div"),
                             QTextOption(Qt::AlignRight));
            painter.drawText(QRectF(lineHeight * 10 + stretchBase * 3, top, stretchBase, lineHeight),
                             valueToString(divs * settings->scope.horizontal.frequencybase / DIVS_TIME, UNIT_HERTZ, 3) +
                                 tr("/div"),
                             QTextOption(Qt::AlignRight));
        } else {
            scopeHeight = (double)paintDevice->height() - (channelCount + 4) * lineHeight;
            double top = 2.5 * lineHeight + scopeHeight;

            painter.drawText(QRectF(0, top, stretchBase, lineHeight), tr("Marker 1/2"));

            painter.drawText(QRectF(lineHeight * 10, top, stretchBase * 2, lineHeight),
                             valueToString(time, UNIT_SECONDS, 4), QTextOption(Qt::AlignRight));
            painter.drawText(QRectF(lineHeight * 10 + stretchBase * 2, top, stretchBase * 2, lineHeight),
                             valueToString(1.0 / time, UNIT_HERTZ, 4), QTextOption(Qt::AlignRight));
        }

        // Set DIVS_TIME x DIVS_VOLTAGE matrix for oscillograph
        painter.setMatrix(QMatrix((paintDevice->width() - 1) / DIVS_TIME, 0, 0, -(scopeHeight - 1) / DIVS_VOLTAGE,
                                  (double)(paintDevice->width() - 1) / 2, (scopeHeight - 1) / 2 + lineHeight * 1.5),
                          false);

        // Draw the graphs
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(Qt::NoBrush);

        for (int zoomed = 0; zoomed < (settings->view.zoom ? 2 : 1); ++zoomed) {
            switch (settings->scope.horizontal.format) {
            case Dso::GRAPHFORMAT_TY:
                // Add graphs for channels
                for (int channel = 0; channel < settings->scope.voltage.count(); ++channel) {
                    if (settings->scope.voltage[channel].used && result->data(channel)) {
                        painter.setPen(QPen(colorValues->voltage[channel], 0));

                        // What's the horizontal distance between sampling points?
                        double horizontalFactor =
                            result->data(channel)->voltage.interval / settings->scope.horizontal.timebase;
                        // How many samples are visible?
                        double centerPosition, centerOffset;
                        if (zoomed) {
                            centerPosition = (zoomOffset + DIVS_TIME / 2) / horizontalFactor;
                            centerOffset = DIVS_TIME / horizontalFactor / zoomFactor / 2;
                        } else {
                            centerPosition = DIVS_TIME / 2 / horizontalFactor;
                            centerOffset = DIVS_TIME / horizontalFactor / 2;
                        }
                        unsigned int firstPosition = qMax((int)(centerPosition - centerOffset), 0);
                        unsigned int lastPosition = qMin((int)(centerPosition + centerOffset),
                                                         (int)result->data(channel)->voltage.sample.size() - 1);

                        // Draw graph
                        QPointF *graph = new QPointF[lastPosition - firstPosition + 1];

                        for (unsigned int position = firstPosition; position <= lastPosition; ++position)
                            graph[position - firstPosition] = QPointF(position * horizontalFactor - DIVS_TIME / 2,
                                                                      result->data(channel)->voltage.sample[position] /
                                                                              settings->scope.voltage[channel].gain +
                                                                          settings->scope.voltage[channel].offset);

                        painter.drawPolyline(graph, lastPosition - firstPosition + 1);
                        delete[] graph;
                    }
                }

                // Add spectrum graphs
                for (int channel = 0; channel < settings->scope.spectrum.count(); ++channel) {
                    if (settings->scope.spectrum[channel].used && result->data(channel)) {
                        painter.setPen(QPen(colorValues->spectrum[channel], 0));

                        // What's the horizontal distance between sampling points?
                        double horizontalFactor =
                            result->data(channel)->spectrum.interval / settings->scope.horizontal.frequencybase;
                        // How many samples are visible?
                        double centerPosition, centerOffset;
                        if (zoomed) {
                            centerPosition = (zoomOffset + DIVS_TIME / 2) / horizontalFactor;
                            centerOffset = DIVS_TIME / horizontalFactor / zoomFactor / 2;
                        } else {
                            centerPosition = DIVS_TIME / 2 / horizontalFactor;
                            centerOffset = DIVS_TIME / horizontalFactor / 2;
                        }
                        unsigned int firstPosition = qMax((int)(centerPosition - centerOffset), 0);
                        unsigned int lastPosition = qMin((int)(centerPosition + centerOffset),
                                                         (int)result->data(channel)->spectrum.sample.size() - 1);

                        // Draw graph
                        QPointF *graph = new QPointF[lastPosition - firstPosition + 1];

                        for (unsigned int position = firstPosition; position <= lastPosition; ++position)
                            graph[position - firstPosition] =
                                QPointF(position * horizontalFactor - DIVS_TIME / 2,
                                        result->data(channel)->spectrum.sample[position] /
                                                settings->scope.spectrum[channel].magnitude +
                                            settings->scope.spectrum[channel].offset);

                        painter.drawPolyline(graph, lastPosition - firstPosition + 1);
                        delete[] graph;
                    }
                }
                break;

            case Dso::GRAPHFORMAT_XY:
                break;

            default:
                break;
            }

            // Set DIVS_TIME / zoomFactor x DIVS_VOLTAGE matrix for zoomed
            // oscillograph
            painter.setMatrix(QMatrix((paintDevice->width() - 1) / DIVS_TIME * zoomFactor, 0, 0,
                                      -(scopeHeight - 1) / DIVS_VOLTAGE,
                                      (double)(paintDevice->width() - 1) / 2 -
                                          zoomOffset * zoomFactor * (paintDevice->width() - 1) / DIVS_TIME,
                                      (scopeHeight - 1) * 1.5 + lineHeight * 4),
                              false);
        }
    } // dataanalyser mutex release

    drawGrids(painter, colorValues, lineHeight, scopeHeight, paintDevice->width());
    painter.end();

    if (this->format == EXPORT_FORMAT_IMAGE) static_cast<QPixmap *>(paintDevice.get())->save(this->filename);

    return true;
}

bool Exporter::exportCVS(const DataAnalyzerResult *result) {
    QFile csvFile(this->filename);
    if (!csvFile.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    QTextStream csvStream(&csvFile);

    int chCount = settings->scope.voltage.count();
    std::vector<const SampleValues *> voltageData(size_t(chCount), nullptr);
    std::vector<const SampleValues *> spectrumData(size_t(chCount), nullptr);
    size_t maxRow = 0;
    bool isSpectrumUsed = false;
    double timeInterval = 0;
    double freqInterval = 0;

    for (int channel = 0; channel < chCount; ++channel) {
        if (result->data(channel)) {
            if (settings->scope.voltage[channel].used) {
                voltageData[channel] = &(result->data(channel)->voltage);
                maxRow = std::max(maxRow, voltageData[channel]->sample.size());
                timeInterval = result->data(channel)->voltage.interval;
            }
            if (settings->scope.spectrum[channel].used) {
                spectrumData[channel] = &(result->data(channel)->spectrum);
                maxRow = std::max(maxRow, spectrumData[channel]->sample.size());
                freqInterval = result->data(channel)->spectrum.interval;
                isSpectrumUsed = true;
            }
        }
    }

    // Start with channel names
    csvStream << "\"t\"";
    for (int channel = 0; channel < chCount; ++channel) {
        if (voltageData[channel] != nullptr) { csvStream << ",\"" << settings->scope.voltage[channel].name << "\""; }
    }
    if (isSpectrumUsed) {
        csvStream << ",\"f\"";
        for (int channel = 0; channel < chCount; ++channel) {
            if (spectrumData[channel] != nullptr) {
                csvStream << ",\"" << settings->scope.spectrum[channel].name << "\"";
            }
        }
    }
    csvStream << "\n";

    for (unsigned int row = 0; row < maxRow; ++row) {

        csvStream << timeInterval * row;
        for (int channel = 0; channel < chCount; ++channel) {
            if (voltageData[channel] != nullptr) {
                csvStream << ",";
                if (row < voltageData[channel]->sample.size()) { csvStream << voltageData[channel]->sample[row]; }
            }
        }

        if (isSpectrumUsed) {
            csvStream << "," << freqInterval * row;
            for (int channel = 0; channel < chCount; ++channel) {
                if (spectrumData[channel] != nullptr) {
                    csvStream << ",";
                    if (row < spectrumData[channel]->sample.size()) { csvStream << spectrumData[channel]->sample[row]; }
                }
            }
        }
        csvStream << "\n";
    }

    csvFile.close();

    return true;
}

void Exporter::drawGrids(QPainter &painter, DsoSettingsColorValues *colorValues, double lineHeight, double scopeHeight,
                         int scopeWidth) {
    painter.setRenderHint(QPainter::Antialiasing, false);
    for (int zoomed = 0; zoomed < (settings->view.zoom ? 2 : 1); ++zoomed) {
        // Set DIVS_TIME x DIVS_VOLTAGE matrix for oscillograph
        painter.setMatrix(QMatrix((scopeWidth - 1) / DIVS_TIME, 0, 0, -(scopeHeight - 1) / DIVS_VOLTAGE,
                                  (double)(scopeWidth - 1) / 2,
                                  (scopeHeight - 1) * (zoomed + 0.5) + lineHeight * 1.5 + lineHeight * 2.5 * zoomed),
                          false);

        // Grid lines
        painter.setPen(QPen(colorValues->grid, 0));

        if (this->format < EXPORT_FORMAT_IMAGE) {
            // Draw vertical lines
            for (int div = 1; div < DIVS_TIME / 2; ++div) {
                for (int dot = 1; dot < DIVS_VOLTAGE / 2 * 5; ++dot) {
                    painter.drawLine(QPointF((double)-div - 0.02, (double)-dot / 5),
                                     QPointF((double)-div + 0.02, (double)-dot / 5));
                    painter.drawLine(QPointF((double)-div - 0.02, (double)dot / 5),
                                     QPointF((double)-div + 0.02, (double)dot / 5));
                    painter.drawLine(QPointF((double)div - 0.02, (double)-dot / 5),
                                     QPointF((double)div + 0.02, (double)-dot / 5));
                    painter.drawLine(QPointF((double)div - 0.02, (double)dot / 5),
                                     QPointF((double)div + 0.02, (double)dot / 5));
                }
            }
            // Draw horizontal lines
            for (int div = 1; div < DIVS_VOLTAGE / 2; ++div) {
                for (int dot = 1; dot < DIVS_TIME / 2 * 5; ++dot) {
                    painter.drawLine(QPointF((double)-dot / 5, (double)-div - 0.02),
                                     QPointF((double)-dot / 5, (double)-div + 0.02));
                    painter.drawLine(QPointF((double)dot / 5, (double)-div - 0.02),
                                     QPointF((double)dot / 5, (double)-div + 0.02));
                    painter.drawLine(QPointF((double)-dot / 5, (double)div - 0.02),
                                     QPointF((double)-dot / 5, (double)div + 0.02));
                    painter.drawLine(QPointF((double)dot / 5, (double)div - 0.02),
                                     QPointF((double)dot / 5, (double)div + 0.02));
                }
            }
        } else {
            // Draw vertical lines
            for (int div = 1; div < DIVS_TIME / 2; ++div) {
                for (int dot = 1; dot < DIVS_VOLTAGE / 2 * 5; ++dot) {
                    painter.drawPoint(QPointF(-div, (double)-dot / 5));
                    painter.drawPoint(QPointF(-div, (double)dot / 5));
                    painter.drawPoint(QPointF(div, (double)-dot / 5));
                    painter.drawPoint(QPointF(div, (double)dot / 5));
                }
            }
            // Draw horizontal lines
            for (int div = 1; div < DIVS_VOLTAGE / 2; ++div) {
                for (int dot = 1; dot < DIVS_TIME / 2 * 5; ++dot) {
                    if (dot % 5 == 0) continue; // Already done by vertical lines
                    painter.drawPoint(QPointF((double)-dot / 5, -div));
                    painter.drawPoint(QPointF((double)dot / 5, -div));
                    painter.drawPoint(QPointF((double)-dot / 5, div));
                    painter.drawPoint(QPointF((double)dot / 5, div));
                }
            }
        }

        // Axes
        painter.setPen(QPen(colorValues->axes, 0));
        painter.drawLine(QPointF(-DIVS_TIME / 2, 0), QPointF(DIVS_TIME / 2, 0));
        painter.drawLine(QPointF(0, -DIVS_VOLTAGE / 2), QPointF(0, DIVS_VOLTAGE / 2));
        for (double div = 0.2; div <= DIVS_TIME / 2; div += 0.2) {
            painter.drawLine(QPointF(div, -0.05), QPointF(div, 0.05));
            painter.drawLine(QPointF(-div, -0.05), QPointF(-div, 0.05));
        }
        for (double div = 0.2; div <= DIVS_VOLTAGE / 2; div += 0.2) {
            painter.drawLine(QPointF(-0.05, div), QPointF(0.05, div));
            painter.drawLine(QPointF(-0.05, -div), QPointF(0.05, -div));
        }

        // Borders
        painter.setPen(QPen(colorValues->border, 0));
        painter.drawRect(QRectF(-DIVS_TIME / 2, -DIVS_VOLTAGE / 2, DIVS_TIME, DIVS_VOLTAGE));
    }
}
