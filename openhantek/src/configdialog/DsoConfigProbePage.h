// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QWidget>

#include "settings.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QLineEdit>

class DsoConfigProbePage: public QWidget {
    Q_OBJECT

    public:
    ////////////////////////////////////////////////////////////////////////////////
    // class DsoConfigProbePage
    /// \brief Creates the widgets and sets their initial value.
    /// \param settings The target settings object.
    /// \param parent The parent widget.
    DsoConfigProbePage(DsoSettings *settings, const Dso::ControlSpecification *spec, QWidget *parent = 0);

    public slots:
        /// \brief Saves the new settings.
        void saveSettings();


    private:
        std::vector<QLabel *> probeLabel;
        std::vector<QLineEdit*> probeAttenuations;
        DsoSettings *settings;
        const Dso::ControlSpecification *spec;

        QGridLayout *probeLayout;
        QVBoxLayout *mainLayout;
        QGroupBox *probeGroup;

};


