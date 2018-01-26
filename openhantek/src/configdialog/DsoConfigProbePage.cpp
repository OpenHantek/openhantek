// SPDX-License-Identifier: GPL-2.0+

#include <QCoreApplication>
#include "DsoConfigProbePage.h"



DsoConfigProbePage::DsoConfigProbePage(DsoSettings *settings, const Dso::ControlSpecification *spec,
                                       QWidget *parent) : QWidget(parent), settings(settings), spec(spec){

    this->probeLayout = new QGridLayout();

    //Add the labels for each channel
    for(ChannelID channel = 0; channel < this->settings->scope.voltage.size(); ++channel) {
        if(channel >= spec->channels) continue;

        QString values;
        std::vector<double> probeSteps;

        this->probeLabel.push_back(new QLabel(QCoreApplication::tr("Probe Gain for Channel %L1").arg(channel)));

        if(this->settings->scope.voltage[channel].probeGainSteps.empty()) {
            probeSteps = this->settings->scope.voltage[channel].defaultValues;
        }
        else {
            probeSteps = this->settings->scope.voltage[channel].probeGainSteps;
        }
        // Fast join

        for (unsigned int idx = 0; idx < probeSteps.size(); idx++) {
            values += QString::number(probeSteps[idx]);
            if (idx != probeSteps.size() - 1) {
                values += ",";
            }
        }

        this->probeAttenuations.push_back(new QLineEdit(values));

    }

    //add the widgets the layout
    for(ChannelID channel = 0; channel < this->settings->scope.voltage.size(); ++channel) {
        if(channel < spec->channels) {
            this->probeLayout->addWidget(this->probeLabel[channel]);
            this->probeLayout->addWidget(this->probeAttenuations[channel]);
        }
    }

    this->probeGroup = new QGroupBox(tr("Probe Attenuation"));
    this->probeGroup->setLayout(this->probeLayout);

    this->mainLayout = new QVBoxLayout();
    this->mainLayout->addWidget(this->probeGroup);
    this->mainLayout->addStretch(1);
    this->setLayout(this->mainLayout);


}

void DsoConfigProbePage::saveSettings() {
    //TODO find a way to refresh a widget
    for(ChannelID channel = 0; channel < this->settings->scope.voltage.size(); ++channel) {
        if(channel >= spec->channels) continue;

        // Clear the list
        this->settings->scope.voltage[channel].probeGainSteps.clear();
        QStringList values = this->probeAttenuations[channel]->text().split(',');
        // Try to convert the values in doubles and save them the the array
        for(int idx=0; idx < values.size(); idx ++){
            bool correct;
            double attenuation = values[idx].toDouble(&correct);
            if(correct) {
                this->settings->scope.voltage[channel].probeGainSteps.push_back(attenuation);
            }
        }
        // Try to have a fallback solution in the case of non valid settings
        if(this->settings->scope.voltage[channel].probeGainSteps.empty()){
            for(double defaultValue: this->settings->scope.voltage[channel].defaultValues) {
                this->settings->scope.voltage[channel].probeGainSteps.push_back(defaultValue);
            }
        }

    }
}
