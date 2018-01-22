// SPDX-License-Identifier: GPL-2.0+

#include "DsoConfigColorsPage.h"
#include "widgets/colorbox.h"

DsoConfigColorsPage::DsoConfigColorsPage(Settings::DsoSettings *settings, QWidget *parent)
    : QWidget(parent), settings(settings) {
    // Initialize elements
    Settings::View &colorSettings = settings->view;
    enum { COL_LABEL = 0, COL_SCR_CHANNEL, COL_SCR_SPECTRUM, COL_PRT_CHANNEL, COL_PRT_SPECTRUM };

    QVBoxLayout *mainLayout;

    QGroupBox *colorsGroup;
    QGridLayout *colorsLayout;

    QLabel *screenColorsLabel, *printColorsLabel;
    QLabel *axesLabel, *backgroundLabel, *borderLabel, *gridLabel, *markersLabel, *textLabel;
    QLabel *graphLabel;
    QLabel *screenChannelLabel, *screenSpectrumLabel, *printChannelLabel, *printSpectrumLabel;

    // Plot Area
    graphLabel = new QLabel(tr("<hr width=\"100%\"/>")); // 4*80
    graphLabel->setAlignment(Qt::AlignRight);
    graphLabel->setTextFormat(Qt::RichText);

    screenColorsLabel = new QLabel(tr("Screen"));
    screenColorsLabel->setAlignment(Qt::AlignHCenter);
    printColorsLabel = new QLabel(tr("Print"));
    printColorsLabel->setAlignment(Qt::AlignHCenter);

    axesLabel = new QLabel(tr("Axes"));
    axesColorBox = new ColorBox(colorSettings.screen.axes());
    printAxesColorBox = new ColorBox(colorSettings.print.axes());

    backgroundLabel = new QLabel(tr("Background"));
    backgroundColorBox = new ColorBox(colorSettings.screen.background());
    printBackgroundColorBox = new ColorBox(colorSettings.print.background());

    borderLabel = new QLabel(tr("Border"));
    borderColorBox = new ColorBox(colorSettings.screen.border());
    printBorderColorBox = new ColorBox(colorSettings.print.border());

    gridLabel = new QLabel(tr("Grid"));
    gridColorBox = new ColorBox(colorSettings.screen.grid());
    printGridColorBox = new ColorBox(colorSettings.print.grid());

    markersLabel = new QLabel(tr("Markers"));
    markersColorBox = new ColorBox(colorSettings.screen.markers());
    printMarkersColorBox = new ColorBox(colorSettings.print.markers());

    textLabel = new QLabel(tr("Text"));
    textColorBox = new ColorBox(colorSettings.screen.text());
    printTextColorBox = new ColorBox(colorSettings.print.text());

    // Graph category
    screenChannelLabel = new QLabel(tr("Channel"));
    screenChannelLabel->setAlignment(Qt::AlignHCenter);
    screenSpectrumLabel = new QLabel(tr("Spectrum"));
    screenSpectrumLabel->setAlignment(Qt::AlignHCenter);
    printChannelLabel = new QLabel(tr("Channel"));
    printChannelLabel->setAlignment(Qt::AlignHCenter);
    printSpectrumLabel = new QLabel(tr("Spectrum"));
    printSpectrumLabel->setAlignment(Qt::AlignHCenter);

    // Plot Area Layout
    colorsLayout = new QGridLayout();
    colorsLayout->setColumnStretch(COL_LABEL, 1);
    colorsLayout->setColumnMinimumWidth(COL_SCR_CHANNEL, 80);
    colorsLayout->setColumnMinimumWidth(COL_SCR_SPECTRUM, 80);
    colorsLayout->setColumnMinimumWidth(COL_PRT_CHANNEL, 80);
    colorsLayout->setColumnMinimumWidth(COL_PRT_SPECTRUM, 80);

    int row = 0;
    colorsLayout->addWidget(screenColorsLabel, row, COL_SCR_CHANNEL, 1, 2);
    colorsLayout->addWidget(printColorsLabel, row, COL_PRT_CHANNEL, 1, 2);
    ++row;
    colorsLayout->addWidget(backgroundLabel, row, COL_LABEL);
    colorsLayout->addWidget(backgroundColorBox, row, COL_SCR_CHANNEL, 1, 2);
    colorsLayout->addWidget(printBackgroundColorBox, row, COL_PRT_CHANNEL, 1, 2);
    ++row;
    colorsLayout->addWidget(gridLabel, row, COL_LABEL);
    colorsLayout->addWidget(gridColorBox, row, COL_SCR_CHANNEL, 1, 2);
    colorsLayout->addWidget(printGridColorBox, row, COL_PRT_CHANNEL, 1, 2);
    ++row;
    colorsLayout->addWidget(axesLabel, row, COL_LABEL);
    colorsLayout->addWidget(axesColorBox, row, COL_SCR_CHANNEL, 1, 2);
    colorsLayout->addWidget(printAxesColorBox, row, COL_PRT_CHANNEL, 1, 2);
    ++row;
    colorsLayout->addWidget(borderLabel, row, COL_LABEL);
    colorsLayout->addWidget(borderColorBox, row, COL_SCR_CHANNEL, 1, 2);
    colorsLayout->addWidget(printBorderColorBox, row, COL_PRT_CHANNEL, 1, 2);
    ++row;
    colorsLayout->addWidget(markersLabel, row, COL_LABEL);
    colorsLayout->addWidget(markersColorBox, row, COL_SCR_CHANNEL, 1, 2);
    colorsLayout->addWidget(printMarkersColorBox, row, COL_PRT_CHANNEL, 1, 2);
    ++row;
    colorsLayout->addWidget(textLabel, row, COL_LABEL);
    colorsLayout->addWidget(textColorBox, row, COL_SCR_CHANNEL, 1, 2);
    colorsLayout->addWidget(printTextColorBox, row, COL_PRT_CHANNEL, 1, 2);
    ++row;

    // Graph
    colorsLayout->addWidget(graphLabel, row, COL_LABEL, 1, COL_PRT_SPECTRUM - COL_LABEL + 1);
    ++row;

    colorsLayout->addWidget(screenChannelLabel, row, COL_SCR_CHANNEL);
    colorsLayout->addWidget(screenSpectrumLabel, row, COL_SCR_SPECTRUM);
    colorsLayout->addWidget(printChannelLabel, row, COL_PRT_CHANNEL);
    colorsLayout->addWidget(printSpectrumLabel, row, COL_PRT_SPECTRUM);
    ++row;

    for (auto *channelSettings : settings->scope) {
        ChannelColors *cc = new ChannelColors(this);
        QLabel *colorLabel = new QLabel(channelSettings->name());
        cc->screenChannelColorBox = new ColorBox(colorSettings.screen.voltage(channelSettings->channelID()));
        cc->screenSpectrumColorBox = new ColorBox(colorSettings.screen.spectrum(channelSettings->channelID()));
        cc->printChannelColorBox = new ColorBox(colorSettings.print.voltage(channelSettings->channelID()));
        cc->printSpectrumColorBox = new ColorBox(colorSettings.print.spectrum(channelSettings->channelID()));
        m_channelColorMap.insert(std::make_pair(channelSettings->channelID(), cc));

        colorsLayout->addWidget(colorLabel, row, COL_LABEL);
        colorsLayout->addWidget(cc->screenChannelColorBox, row, COL_SCR_CHANNEL);
        colorsLayout->addWidget(cc->screenSpectrumColorBox, row, COL_SCR_SPECTRUM);
        colorsLayout->addWidget(cc->printChannelColorBox, row, COL_PRT_CHANNEL);
        colorsLayout->addWidget(cc->printSpectrumColorBox, row, COL_PRT_SPECTRUM);
        ++row;
    }

    colorsGroup = new QGroupBox(tr("Screen and Print Colors"));
    colorsGroup->setLayout(colorsLayout);

    // Main layout
    mainLayout = new QVBoxLayout();
    mainLayout->addWidget(colorsGroup);
    mainLayout->addStretch(1);

    setLayout(mainLayout);
}

/// \brief Saves the new settings.
void DsoConfigColorsPage::saveSettings() {
    Settings::View &colorSettings = settings->view;

    // Screen category
    colorSettings.screen._axes = axesColorBox->getColor();
    colorSettings.screen._background = backgroundColorBox->getColor();
    colorSettings.screen._border = borderColorBox->getColor();
    colorSettings.screen._grid = gridColorBox->getColor();
    colorSettings.screen._markers = markersColorBox->getColor();
    colorSettings.screen._text = textColorBox->getColor();

    // Print category
    colorSettings.print._axes = printAxesColorBox->getColor();
    colorSettings.print._background = printBackgroundColorBox->getColor();
    colorSettings.print._border = printBorderColorBox->getColor();
    colorSettings.print._grid = printGridColorBox->getColor();
    colorSettings.print._markers = printMarkersColorBox->getColor();
    colorSettings.print._text = printTextColorBox->getColor();

    // Graph category
    for (auto &c : m_channelColorMap) {
        colorSettings.screen.setVoltage(c.first, c.second->screenChannelColorBox->getColor());
        colorSettings.screen.setSpectrum(c.first, c.second->screenSpectrumColorBox->getColor());
        colorSettings.print.setVoltage(c.first, c.second->printChannelColorBox->getColor());
        colorSettings.print.setSpectrum(c.first, c.second->printSpectrumColorBox->getColor());
    }

    colorSettings.screen.observer()->update();
    colorSettings.print.observer()->update();
}

ChannelColors::ChannelColors(QObject *parent) : QObject(parent) {}
