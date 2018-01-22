#include <cfloat>
#include <cmath>

#include <QDebug>

#include "sispinbox.h"

#include "utils/printutils.h"

SiSpinBox::SiSpinBox(Unit unit, QWidget *parent) : QDoubleSpinBox(parent) {
    setMinimum(1e-12);
    setMaximum(1e12);
    setDecimals(DBL_MAX_10_EXP + DBL_DIG); // Disable automatic rounding
    m_unit = unit;
    m_steps = {1.0, 2.0, 5.0, 10.0};
    setValue(0);
}

QValidator::State SiSpinBox::validate(QString &input, int &pos) const {
    Q_UNUSED(pos);

    bool ok;
    double value = stringToValue(input, m_unit, &ok);

    if (!ok) return QValidator::Invalid;

    if (input == textFromValue(value)) return QValidator::Acceptable;
    return QValidator::Intermediate;
}

void SiSpinBox::fixup(QString &input) const {
    bool ok;
    double new_value = stringToValue(input, m_unit, &ok);
    if (!ok) new_value = value();

    input = textFromValue(new_value);
}

inline unsigned pmod(int i, int n) { return unsigned((i % n + n) % n); }

void SiSpinBox::stepBy(int steps) {
    const double v = value();
    const double vmin = minimum();
    const double vmax = maximum();
    if (steps < 0 && v <= vmin) return;
    if (steps > 0 && v >= vmax) return;

    // consts
    const double logbase = m_steps.back() / m_steps.front();
    const int stepsCount = (int)m_steps.size() - 1;

    // state
    unsigned stepId = (steps > 0) ? m_stepUpper.stepId : m_stepLower.stepId;
    int decade = (steps > 0) ? m_stepUpper.decade : m_stepLower.decade;

    int stepIdTemp = (int)stepId + steps;                  ///< If less then stepsCount, only the stepId will change
    stepId = pmod(stepIdTemp, stepsCount);                 ///< We ensure a valid step id with modulo
    decade += std::floor((double)stepIdTemp / stepsCount); ///< Overflowing steps will change the decade/magnitude
    double newvalue = std::pow(logbase, decade) * m_steps[stepId];

    setValue(std::min(vmax, std::max(vmin, newvalue)));
}

void SiSpinBox::setValue(double v) {
    if (std::isinf(v)) return;
    const double logbase = m_steps.back() / m_steps.front();
    const unsigned stepsCount = (unsigned)m_steps.size() - 1;
    // Recompute decade and stepId (stepID refers to an entry in m_steps)
    const double decade = log(v) / log(logbase);
    m_stepUpper.decade = m_stepLower.decade = (int)std::floor(decade);
    double vNorm = v / pow(logbase, m_stepUpper.decade);
    m_stepLower.stepId = stepsCount;
    while (m_stepLower.stepId && m_steps[m_stepLower.stepId] > vNorm) { --m_stepLower.stepId; }
    m_stepUpper.stepId = 0;
    while (m_stepUpper.stepId < stepsCount && m_steps[m_stepUpper.stepId] < vNorm) { ++m_stepUpper.stepId; }
    QDoubleSpinBox::setValue(v);
}
