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
    DsoConfigProbePage(DsoSettings *settings, const Dso::ControlSpecification *spec, QWidget *parent = 0);
    ~DsoConfigProbePage();

    public slots:
        void saveSettings();


    private:
        QList<QLabel *> probeLabel;
        QList<QLineEdit*> probeAttenuations;
        DsoSettings *settings;
        const Dso::ControlSpecification *spec;

        QGridLayout *probeLayout;
        QVBoxLayout *mainLayout;
        QGroupBox *probeGroup;

};


