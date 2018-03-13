// SPDX-License-Identifier: GPL-2.0+

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QSignalBlocker>
#include <QStringListModel>

#include <cmath>

#include "DebugDock.h"
#include "dockwindows.h"
#include "hantekdso/devicesettings.h"
#include "hantekdso/enums.h"
#include "hantekdso/dsocontrol.h"
#include "hantekdso/modelspecification.h"
#include "hantekprotocol/codes.h"
#include "iconfont/QtAwesome.h"
#include "scopesettings.h"
#include "utils/debugnotify.h"
#include "utils/enumhelper.h"
#include "utils/printutils.h"
#include "widgets/sispinbox.h"

#include <QDebug>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QTableView>

Q_DECLARE_METATYPE(std::vector<unsigned>)
Q_DECLARE_METATYPE(std::vector<double>)

template <typename... Args> struct SELECT {
    template <typename C, typename R> static constexpr auto OVERLOAD_OF(R (C::*pmf)(Args...)) -> decltype(pmf) {
        return pmf;
    }
};

DebugDock::DebugDock(DsoControl *dsocontrol, QWidget *parent, Qt::WindowFlags flags)
    : QDockWidget(tr("Debug"), parent, flags) {

    QWidget *dockWidget = new QWidget(this);
    QVBoxLayout *dockLayout = new QVBoxLayout;

    QHBoxLayout *manualCommandLayout = new QHBoxLayout;
    QHBoxLayout *manualCommandLayout2 = new QHBoxLayout;
    QComboBox *manualCommandType = new QComboBox(this);
    QComboBox *controlCodes = new QComboBox(this);
    QComboBox *bulkCodes = new QComboBox(this);
    QLineEdit *commandEdit = new QLineEdit(this);
    QPushButton *actionManualCommand = new QPushButton;

    commandEdit->setPlaceholderText(tr("0a ca (hex values)"));

    actionManualCommand->setIcon(iconFont->icon(fa::edit));
    manualCommandType->addItems(QStringList() << tr("Control") << tr("Bulk"));

    auto bulkMeta = QMetaEnum::fromType<HantekE::BulkCode>();
    for (int i = 0; i < bulkMeta.keyCount(); ++i) {
        int v = bulkMeta.value(i);
        if (dsocontrol->isCommandSupported((HantekE::BulkCode)v)) bulkCodes->addItem(bulkMeta.key(i), v);
    }
    bulkCodes->hide();

    auto controlMeta = QMetaEnum::fromType<HantekE::ControlCode>();
    for (int i = 0; i < controlMeta.keyCount(); ++i) {
        int v = controlMeta.value(i);
        if (dsocontrol->isCommandSupported((HantekE::ControlCode)v)) controlCodes->addItem(controlMeta.key(i), v);
    }

    connect(manualCommandType, SELECT<int>::OVERLOAD_OF(&QComboBox::currentIndexChanged), this,
            [controlCodes, bulkCodes](unsigned index) {
                controlCodes->setVisible(index == 0);
                bulkCodes->setVisible(index == 1);
            });

    manualCommandLayout->addWidget(manualCommandType);
    manualCommandLayout->addWidget(controlCodes);
    manualCommandLayout->addWidget(bulkCodes);
    manualCommandLayout2->addWidget(commandEdit, 1);
    manualCommandLayout2->addWidget(actionManualCommand);

    connect(this, &DebugDock::manualCommand, dsocontrol, &DsoControl::manualCommand);

    auto fnManualCommand = [this, commandEdit, manualCommandType, controlCodes, bulkCodes, dsocontrol]() {
        if (commandEdit->text().trimmed().isEmpty()) return;
        if (manualCommandType->currentIndex() == 1) {
            if (bulkCodes->currentIndex() < 0) return;
        } else if (controlCodes->currentIndex() < 0)
            return;
        QByteArray data(100, 0);
        data.resize((int)hexParse(commandEdit->text(), (unsigned char *)data.data(), (unsigned)data.size()));
        if (data.isEmpty()) return;

        // Use a signal instead of a direct function call to archive thread-safety
        emit manualCommand(manualCommandType->currentIndex() == 1,
                           (HantekE::BulkCode)bulkCodes->currentData(Qt::UserRole).toInt(),
                           (HantekE::ControlCode)controlCodes->currentData(Qt::UserRole).toInt(), data);
        commandEdit->clear();
    };

    connect(actionManualCommand, &QPushButton::toggled, this, fnManualCommand);
    connect(commandEdit, &QLineEdit::returnPressed, this, fnManualCommand);

    QTableView *logTable = new QTableView(this);
    logTable->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
    logTable->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
    logTable->horizontalHeader()->hide();
    logTable->horizontalHeader()->setStretchLastSection(true);
    logTable->setColumnWidth(0, 60);
    logTable->setColumnWidth(1, 60);
    logTable->verticalHeader()->hide();
    m_model = new Debug(this);
    logTable->setModel(m_model);
    connect(dsocontrol, &DsoControl::debugMessage, m_model, &Debug::addEntry);

    QCheckBox *showLoopLog = new QCheckBox("Verbose loop log", this);
    showLoopLog->setChecked(false);
    m_model->addToFilter(Debug::NotificationType::DSOLoop);
    connect(showLoopLog, &QCheckBox::toggled, this, [this](bool enable) {
        if (enable)
            m_model->clearFilter();
        else
            m_model->addToFilter(Debug::NotificationType::DSOLoop);
    });

    QPushButton *clearLog = new QPushButton;
    clearLog->setIcon(iconFont->icon(fa::remove));
    connect(clearLog, &QPushButton::clicked, m_model, &Debug::removeAll);

    QHBoxLayout *clearLogLayout = new QHBoxLayout;
    clearLogLayout->addWidget(new QLabel(tr("Logs"), this), 1);
    clearLogLayout->addWidget(clearLog);

    dockLayout->addLayout(clearLogLayout);
    dockLayout->addWidget(logTable, 1);
    dockLayout->addWidget(showLoopLog);
    dockLayout->addWidget(new QLabel(tr("Manual command"), this));
    dockLayout->addLayout(manualCommandLayout);
    dockLayout->addLayout(manualCommandLayout2);
    SetupDockWidget(this, dockWidget, dockLayout, QSizePolicy::Expanding);
}

/// \brief Don't close the dock, just hide it.
/// \param event The close event that should be handled.
void DebugDock::closeEvent(QCloseEvent *event) {
    this->hide();
    event->accept();
}
