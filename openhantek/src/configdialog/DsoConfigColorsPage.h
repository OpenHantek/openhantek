// SPDX-License-Identifier: GPL-2.0+

#include <QWidget>

#include "settings.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

class ColorBox;

struct ChannelColors : protected QObject {
    ChannelColors(QObject *parent);

    ColorBox *screenChannelColorBox;
    ColorBox *screenSpectrumColorBox;
    ColorBox *printChannelColorBox;
    ColorBox *printSpectrumColorBox;

  private:
    Q_OBJECT
};

/// \brief Config page for the colors.
class DsoConfigColorsPage : public QWidget {
    Q_OBJECT

  public:
    DsoConfigColorsPage(Settings::DsoSettings *settings, QWidget *parent = 0);

  public slots:
    void saveSettings();

  private:
    Settings::DsoSettings *settings;

    ColorBox *axesColorBox, *backgroundColorBox, *borderColorBox, *gridColorBox, *markersColorBox, *textColorBox,
        *printAxesColorBox, *printBackgroundColorBox, *printBorderColorBox, *printGridColorBox, *printMarkersColorBox,
        *printTextColorBox;

    std::map<ChannelID, ChannelColors *> m_channelColorMap;
};
