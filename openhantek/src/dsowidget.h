// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <memory>

#include "exporter.h"
#include "glscope.h"
#include "levelslider.h"

class DataAnalyzer;
class DsoSettings;
class QGridLayout;

/// \class DsoWidget
/// \brief The widget for the oszilloscope-screen
/// This widget contains the scopes and all level sliders.
class DsoWidget : public QWidget {
    Q_OBJECT

  public:
    /// \brief Initializes the components of the oszilloscope-screen.
    /// \param settings The settings object containing the oscilloscope settings.
    /// \param dataAnalyzer The data analyzer that should be used as data source.
    /// \param parent The parent widget.
    /// \param flags Flags for the window manager.
    DsoWidget(DsoSettings *settings, QWidget *parent = 0, Qt::WindowFlags flags = 0);
    void showNewData(std::unique_ptr<DataAnalyzerResult> data);

  protected:
    void adaptTriggerLevelSlider(unsigned int channel);
    void setMeasurementVisible(unsigned int channel, bool visible);
    void updateMarkerDetails();
    void updateSpectrumDetails(unsigned int channel);
    void updateTriggerDetails();
    void updateVoltageDetails(unsigned int channel);

    QGridLayout *mainLayout;            ///< The main layout for this widget
    LevelSlider *offsetSlider;          ///< The sliders for the graph offsets
    LevelSlider *triggerPositionSlider; ///< The slider for the pretrigger
    LevelSlider *triggerLevelSlider;    ///< The sliders for the trigger level
    LevelSlider *markerSlider;          ///< The sliders for the markers

    QHBoxLayout *settingsLayout;        ///< The table for the settings info
    QLabel *settingsTriggerLabel;       ///< The trigger details
    QLabel *settingsRecordLengthLabel;  ///< The record length
    QLabel *settingsSamplerateLabel;    ///< The samplerate
    QLabel *settingsTimebaseLabel;      ///< The timebase of the main scope
    QLabel *settingsFrequencybaseLabel; ///< The frequencybase of the main scope

    QHBoxLayout *markerLayout;        ///< The table for the marker details
    QLabel *markerInfoLabel;          ///< The info about the zoom factor
    QLabel *markerTimeLabel;          ///< The time period between the markers
    QLabel *markerFrequencyLabel;     ///< The frequency for the time period
    QLabel *markerTimebaseLabel;      ///< The timebase for the zoomed scope
    QLabel *markerFrequencybaseLabel; ///< The frequencybase for the zoomed scope

    QGridLayout *measurementLayout;            ///< The table for the signal details
    QList<QLabel *> measurementNameLabel;      ///< The name of the channel
    QList<QLabel *> measurementGainLabel;      ///< The gain for the voltage (V/div)
    QList<QLabel *> measurementMagnitudeLabel; ///< The magnitude for the spectrum (dB/div)
    QList<QLabel *> measurementMiscLabel;      ///< Coupling or math mode
    QList<QLabel *> measurementAmplitudeLabel; ///< Amplitude of the signal (V)
    QList<QLabel *> measurementFrequencyLabel; ///< Frequency of the signal (Hz)

    DsoSettings *settings;  ///< The settings provided by the main window
    GlGenerator *generator; ///< The generator for the OpenGL vertex arrays
    GlScope *mainScope;     ///< The main scope screen
    GlScope *zoomScope;     ///< The optional magnified scope screen
    std::unique_ptr<Exporter> exportNextFrame;
    std::unique_ptr<DataAnalyzerResult> data;
  public slots:
    // Horizontal axis
    // void horizontalFormatChanged(HorizontalFormat format);
    void updateFrequencybase(double frequencybase);
    void updateSamplerate(double samplerate);
    void updateTimebase(double timebase);

    // Trigger
    void updateTriggerMode();
    void updateTriggerSlope();
    void updateTriggerSource();

    // Spectrum
    void updateSpectrumMagnitude(unsigned int channel);
    void updateSpectrumUsed(unsigned int channel, bool used);

    // Vertical axis
    void updateVoltageCoupling(unsigned int channel);
    void updateMathMode();
    void updateVoltageGain(unsigned int channel);
    void updateVoltageUsed(unsigned int channel, bool used);

    // Menus
    void updateRecordLength(unsigned long size);

    // Export
    void exportAs();
    void print();

    // Scope control
    void updateZoom(bool enabled);

    // Data analyzer
    void doShowNewData();

  private slots:
    // Sliders
    void updateOffset(unsigned channel, double value);
    void updateTriggerPosition(int index, double value);
    void updateTriggerLevel(int channel, double value);
    void updateMarker(int marker, double value);

  signals:
    // Sliders
    void offsetChanged(unsigned int channel, double value);       ///< A graph offset has been changed
    void triggerPositionChanged(double value);                    ///< The pretrigger has been changed
    void triggerLevelChanged(unsigned int channel, double value); ///< A trigger level has been changed
    void markerChanged(unsigned int marker, double value);        ///< A marker position has been changed
};
