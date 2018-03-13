// SPDX-License-Identifier: GPL-2.0+

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDockWidget>
#include <QLabel>

#include <cmath>

#include "dockwindows.h"
#include "hantekdso/enums.h"
#include "hantekprotocol/types.h"
#include "post/postprocessingsettings.h"

void SetupDockWidget(QDockWidget *dockWindow, QWidget *dockWidget, QLayout *layout, QSizePolicy::Policy vPolicy) {
    dockWindow->setObjectName(dockWindow->windowTitle());
    dockWindow->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    dockWidget->setLayout(layout);
    dockWidget->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, vPolicy, QSizePolicy::DefaultType));
    dockWindow->setWidget(dockWidget);
}

void registerDockMetaTypes() {
    qRegisterMetaType<DsoE::TriggerMode>();
    qRegisterMetaType<PostProcessingE::MathMode>();
    qRegisterMetaType<DsoE::Slope>();
    qRegisterMetaType<DsoE::Coupling>();
    qRegisterMetaType<DsoE::GraphFormat>();
    qRegisterMetaType<DsoE::ChannelMode>();
    qRegisterMetaType<PostProcessingE::WindowFunction>();
    qRegisterMetaType<DsoE::InterpolationMode>();
    qRegisterMetaType<std::vector<unsigned>>();
    qRegisterMetaType<std::vector<double>>();
    qRegisterMetaType<ChannelID>("ChannelID");
}
