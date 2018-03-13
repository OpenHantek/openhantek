// SPDX-License-Identifier: GPL-2.0+

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QStringListModel>

#include <cmath>

#include "gainAdjustDock.h"

#include "dockwindows.h"
#include "hantekdso/devicesettings.h"
#include "hantekdso/dsocontrol.h"
#include "hantekdso/enums.h"
#include "hantekdso/modelspecification.h"
#include "hantekprotocol/codes.h"
#include "iconfont/QtAwesome.h"
#include "post/selfcalibration.h"
#include "scopesettings.h"
#include "utils/debugnotify.h"
#include "utils/enumhelper.h"
#include "utils/printutils.h"
#include "widgets/sispinbox.h"

#include <QDebug>
#include <QHeaderView>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QTableView>

Q_DECLARE_METATYPE(std::vector<unsigned>)
Q_DECLARE_METATYPE(std::vector<double>)

template <typename... Args> struct SELECT {
    template <typename C, typename R> static constexpr auto OVERLOAD_OF(R (C::*pmf)(Args...)) -> decltype(pmf) {
        return pmf;
    }
};

GainAdjustDock::GainAdjustDock(DsoControl *dsocontrol, SelfCalibration *selfCalibration, QWidget *parent,
                               Qt::WindowFlags flags)
    : QDockWidget(tr("Calibration"), parent, flags) {

    Dso::ModelSpec *spec = const_cast<Dso::ModelSpec *>(dsocontrol->deviceSettings()->spec);

    QVBoxLayout *dockLayout = new QVBoxLayout;
    QScrollArea *scroll = new QScrollArea(this);
    QGridLayout *grid = new QGridLayout;
    QWidget *dockWidget = new QWidget(this);

    QWidget *scrollWidget = new QWidget(this);
    scrollWidget->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
    grid->setSizeConstraint(QLayout::SetFixedSize);
    scrollWidget->setLayout(grid);
    scroll->setWidget(scrollWidget);
    scroll->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
    dockLayout->addWidget(scroll, 1);

    QHBoxLayout *btns = new QHBoxLayout;
    QPushButton *btnCalibrationStart = new QPushButton(tr("Self-calibration"));
    btnCalibrationStart->setIcon(iconFont->icon(fa::warning));

    QPushButton *btnHelp = new QPushButton();
    btnHelp->setIcon(iconFont->icon(fa::info));

    QPushButton *btnApply = new QPushButton(this);
    btnApply->setIcon(iconFont->icon(fa::check));
    btnApply->setText(tr("Apply"));
    btnApply->setEnabled(false);
    dockLayout->addWidget(btnApply);

    ///// Dialog for self calibration /////
    QDialog *calibrationDialog = new QDialog(this);
    calibrationDialog->setWindowTitle(tr("Self-calibration"));
    calibrationDialog->setModal(true);
    QVBoxLayout *dialogLayout = new QVBoxLayout(calibrationDialog);
    QLabel *dialogLabel = new QLabel(calibrationDialog);
    QProgressBar *dialogProgress = new QProgressBar(calibrationDialog);
    dialogProgress->setRange(0, 100);
    connect(selfCalibration, &SelfCalibration::progress, this,
            [dialogLabel, dialogProgress](double progress, const QString &task) {
                dialogProgress->setValue(progress * 100);
                dialogLabel->setText(task);
            });
    QPushButton *btnCalibrationCancel = new QPushButton(tr("Cancel"), calibrationDialog);
    dialogLayout->addWidget(dialogLabel);
    dialogLayout->addWidget(dialogProgress);
    dialogLayout->addWidget(btnCalibrationCancel);

    QComboBox *selfCalibChannels = new QComboBox(this);
    for (ChannelID c = 0; c < spec->channels; ++c) selfCalibChannels->addItem(tr("Channel %1").arg(c + 1));
    selfCalibChannels->setCurrentIndex(0);

    connect(btnCalibrationCancel, &QPushButton::clicked, selfCalibration, &SelfCalibration::cancel);
    connect(btnCalibrationStart, &QPushButton::clicked, selfCalibration, [selfCalibration, selfCalibChannels]() {
        selfCalibration->start((unsigned)selfCalibChannels->currentIndex());
    });
    connect(selfCalibration, &SelfCalibration::runningChanged, this,
            [btnCalibrationStart, calibrationDialog, this](bool running) {
                calibrationDialog->setVisible(running);
                btnCalibrationStart->setDisabled(running);
                if (!running) emit selfCalibrationFinished();
            });

    connect(btnHelp, &QPushButton::clicked, this, [this, selfCalibChannels, spec]() {
        QMessageBox::information(
            this, tr("Self-calibration"),
            tr("Please connect the %1 probe of your oscilloscope to GND and the test signal "
               "generator. Self-calibration will adjust the gain values to match the amplitude "
               "of %2V. This may be inaccurate for low gain values, because of clipping in "
               "the signal.\n\nThe new values are not permanent and will be discarded on exit. If the "
               "new values are an improvement in your opinion, please visit our github page "
               "(Help->About) and post them in a new Issue.")
                .arg(selfCalibChannels->currentText())
                .arg(spec->testSignalAmplitude));
    });

    btns->addWidget(selfCalibChannels);
    btns->addWidget(btnCalibrationStart);
    btns->addStretch(1);
    btns->addWidget(btnHelp);

    dockLayout->addLayout(btns);

    int row = 0;

    // Header row
    QLabel *l = new QLabel(tr("Gain\nFactor*"), this);
    l->setToolTip(tr("The formula is 1V=Voltage=(RawSamplePoint/gainFactor-offset)*hardwareGainVoltage to archive "
                     "a 1V amplitude with the DSO included test signal."));
    grid->addWidget(l, row, 1);
    l = new QLabel(tr("Offset\nStart"), this);
    l->setToolTip(tr("Some models allow to set a hardware offset. That value is usually limited by 8, 10 or 16bits or "
                     "any value up to 16bits. To compute an accurate sample set, the offset range need to be known."));
    grid->addWidget(l, row, 2);
    grid->addWidget(new QLabel(tr("Offset\nEnd"), this), row, 3);
    l = new QLabel(tr("GND\nCorrection"), this);
    l->setToolTip(tr("The signal ground offset is usually auto-calibrated, but some models do not do that. "
                     "Adjust these values if the ground level is not correct for you."));
    grid->addWidget(l, row, 4);
    grid->setColumnStretch(0, 0); ///< Columns are fixed size
    grid->setColumnStretch(1, 0);
    grid->setColumnStretch(2, 0);
    grid->setColumnStretch(3, 0);

    ++row;

    auto *copy = new std::vector<Dso::ModelSpec::gainStepCalibration>(spec->calibration);
    connect(this, &QObject::destroyed, [copy]() { delete copy; }); // delete copy when this is destroyed

    QSpinBox *edit;
    QDoubleSpinBox *editD;
    for (unsigned gainId = 0; gainId < spec->gain.size(); ++gainId) {
        for (ChannelID channelID = 0; channelID < spec->channels; ++channelID) {
            auto *l = new QLabel(
                tr("%1 CH%2").arg(valueToString(spec->gain[gainId].gain, Unit::VOLTS, -1)).arg(channelID + 1), this);
            grid->addWidget(l, row, 0);

            editD = new QDoubleSpinBox(this);
            editD->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
            editD->setRange(1, 2000);
            editD->setSingleStep(0.1);
            editD->setValue(copy->at(channelID)[gainId].voltageLimit);
            editD->setToolTip(tr("Original value is: %1").arg(editD->value()));
            grid->addWidget(editD, row, 1);
            connect(this, &GainAdjustDock::selfCalibrationFinished, editD, [editD, spec, channelID, gainId]() {
                editD->setValue(spec->calibration[channelID][gainId].voltageLimit);
            });
            connect(editD, SELECT<double>::OVERLOAD_OF(&QDoubleSpinBox::valueChanged), this,
                    [btnApply, copy, channelID, gainId](double value) {
                        btnApply->setEnabled(true);
                        (*copy)[channelID][gainId].voltageLimit = value;
                    });

            edit = new QSpinBox(this);
            edit->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
            edit->setRange(0, UINT16_MAX);
            edit->setSingleStep(1);
            edit->setValue(copy->at(channelID)[gainId].offsetStart);
            edit->setToolTip(tr("Original value is: %1").arg(edit->value()));
            grid->addWidget(edit, row, 2);
            connect(this, &GainAdjustDock::selfCalibrationFinished, edit, [edit, spec, channelID, gainId]() {
                edit->setValue(spec->calibration[channelID][gainId].offsetStart);
            });
            connect(edit, SELECT<int>::OVERLOAD_OF(&QSpinBox::valueChanged), this,
                    [btnApply, copy, channelID, gainId](int value) {
                        btnApply->setEnabled(true);
                        (*copy)[channelID][gainId].offsetStart = (unsigned short)value;
                    });

            edit = new QSpinBox(this);
            edit->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
            edit->setRange(0, UINT16_MAX);
            edit->setSingleStep(1);
            edit->setValue(copy->at(channelID)[gainId].offsetEnd);
            edit->setToolTip(tr("Original value is: %1").arg(edit->value()));
            grid->addWidget(edit, row, 3);
            connect(this, &GainAdjustDock::selfCalibrationFinished, edit, [edit, spec, channelID, gainId]() {
                edit->setValue(spec->calibration[channelID][gainId].offsetEnd);
            });
            connect(edit, SELECT<int>::OVERLOAD_OF(&QSpinBox::valueChanged), this,
                    [btnApply, copy, channelID, gainId](int value) {
                        btnApply->setEnabled(true);
                        (*copy)[channelID][gainId].offsetEnd = (unsigned short)value;
                    });

            editD = new QDoubleSpinBox(this);
            editD->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
            editD->setSingleStep(0.01);
            editD->setRange(-1, 1);
            editD->setValue(copy->at(channelID)[gainId].offsetCorrection);
            editD->setToolTip(tr("Original value is: %1").arg(editD->value()));
            grid->addWidget(editD, row, 4);
            connect(this, &GainAdjustDock::selfCalibrationFinished, editD, [editD, spec, channelID, gainId]() {
                editD->setValue(spec->calibration[channelID][gainId].offsetCorrection);
            });
            connect(editD, SELECT<double>::OVERLOAD_OF(&QDoubleSpinBox::valueChanged), this,
                    [btnApply, copy, channelID, gainId](double value) {
                        btnApply->setEnabled(true);
                        (*copy)[channelID][gainId].offsetCorrection = value;
                    });

            ++row;
        }
    }

    connect(btnApply, &QPushButton::clicked, this, [dsocontrol, spec, copy, btnApply]() {
        spec->calibration = *copy;
        for (ChannelID channelID = 0; channelID < spec->channels; ++channelID)
            dsocontrol->setOffset(channelID, dsocontrol->deviceSettings()->voltage[channelID]->offset(), true);
        btnApply->setEnabled(false);
    });

    SetupDockWidget(this, dockWidget, dockLayout, QSizePolicy::Expanding);
}

/// \brief Don't close the dock, just hide it.
/// \param event The close event that should be handled.
void GainAdjustDock::closeEvent(QCloseEvent *event) {
    this->hide();
    event->accept();
}
