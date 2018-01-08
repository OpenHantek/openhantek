//
// Created by jacopo on 08/01/18.
//

#include <QtWidgets/QApplication>
#include "DsoConfigProbePage.h"


////////////////////////////////////////////////////////////////////////////////
// class DsoConfigProbePage
/// \brief Creates the widgets and sets their initial value.
/// \param settings The target settings object.
/// \param parent The parent widget.
DsoConfigProbePage::DsoConfigProbePage(DsoSettings *settings, QWidget *parent ): QWidget(parent){
    this->settings = settings;

    this->probeLayout = new QGridLayout();

    //Add the labels for each channel
    for(unsigned int channel = 0; channel < this->settings->scope.voltage.size(); ++channel) {
        if(channel < this->settings->deviceSpecification->channels) {
            this->probeLabel.append(new QLabel(QApplication::tr("Probe Gain for Channel %L1").arg(channel)));
            // Fast join
            QString values;
            // TODO find a better way
            if(this->settings->scope.voltage[channel].probeGainSteps.size() > 0) {
                for (unsigned int idx = 0; idx < this->settings->scope.voltage[channel].probeGainSteps.size(); idx++) {
                    values += QString::number(this->settings->scope.voltage[channel].probeGainSteps[idx]);
                    if (idx != this->settings->scope.voltage[channel].probeGainSteps.size() - 1) {
                        values += ",";
                    }
                }
            }else{
                for (unsigned int idx = 0; idx < this->settings->scope.voltage[channel].defaultValues.size(); idx++) {
                    values += QString::number(this->settings->scope.voltage[channel].defaultValues[idx]);
                    if (idx != this->settings->scope.voltage[channel].defaultValues.size() - 1) {
                        values += ",";
                    }
                }
            }
            this->probeAttenuations.append(new QLineEdit(values));
        }
    }

    //add the widgets the layout
    for(unsigned int channel = 0; channel < this->settings->scope.voltage.size(); ++channel) {
        if(channel < this->settings->deviceSpecification->channels) {
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

/// \brief Cleans up the widget.
DsoConfigProbePage::~DsoConfigProbePage() {

}

/// \brief Saves the new settings.
void DsoConfigProbePage::saveSettings() {
    //TODO find a way to refresh a widget
    for(unsigned int channel = 0; channel < this->settings->scope.voltage.size(); ++channel) {
        if(channel < this->settings->deviceSpecification->channels) {
            // Clear the list
            this->settings->scope.voltage[channel].probeGainSteps.clear();
            QStringList values = this->probeAttenuations[channel]->text().split(',');
            // Try to convert the values in booleans and save them the the array
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
}
