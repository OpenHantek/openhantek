//
// Created by jacopo on 08/01/18.
//

#ifndef OPENHANTEKPROJECT_DSOCONFIGPROBEPAGE_H
#define OPENHANTEKPROJECT_DSOCONFIGPROBEPAGE_H


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
    DsoConfigProbePage(DsoSettings *settings, QWidget *parent = 0);
    ~DsoConfigProbePage();

    public slots:
        void saveSettings();


    private:
        QList<QLabel *> probeLabel;
        QList<QLineEdit*> probeAttenuations;
        DsoSettings *settings;
        QGridLayout *probeLayout;
        QVBoxLayout *mainLayout;
        QGroupBox *probeGroup;
};



#endif //OPENHANTEKPROJECT_DSOCONFIGPROBEPAGE_H
