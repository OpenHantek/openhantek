// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QDoubleSpinBox>
#include <QStringList>

#include "utils/printutils.h"

/// \brief A spin box with SI prefix support.
/// This spin box supports the SI prefixes (k/M/G/T) after its value and allows
/// floating point values. The step size is increasing in an exponential way, to
/// keep the percentual difference between the steps at equal levels.
class SiSpinBox : public QDoubleSpinBox {
    Q_OBJECT

  public:
    /// \brief Initializes the SiSpinBox, allowing the user to choose the unit.
    /// \param unit The unit shown for the value in the spin box.
    /// \param parent The parent widget.
    explicit SiSpinBox(Unit m_unit = Unit::Undefined, QWidget *parent = 0);

    /// \brief Validates the text after user input.
    /// \param input The content of the text box.
    /// \param pos The position of the cursor in the text box.
    /// \return Validity of the current text.
    QValidator::State validate(QString &input, int &pos) const override;

    /// \brief Parse value from input text.
    /// \param text The content of the text box.
    /// \return Value in base unit.
    inline double valueFromText(const QString &text) const override { return stringToValue(text, m_unit); }

    /// \brief Get string representation of value.
    /// \param val Value in base unit.
    /// \return String representation containing value and (prefix+)unit.
    inline QString textFromValue(double val) const override { return valueToString(val, m_unit, -1) + m_unitPostfix; }

    /// \brief Fixes the text after the user finished changing it.
    /// \param input The content of the text box.
    void fixup(QString &input) const override;

    /// \brief Increase/decrease the values in fixed steps.
    /// \param steps The number of steps, positive means increase.
    void stepBy(int m_steps) override;

    /// \brief Set the unit for this spin box.
    /// \param unit The unit shown for the value in the spin box.
    /// \return true on success, false on invalid unit.
    inline void setUnit(Unit unit) { m_unit = unit; }

    /// \brief Set the unit postfix for this spin box.
    /// \param postfix the string shown after the unit in the spin box.
    inline void setUnitPostfix(const QString &postfix) { this->m_unitPostfix = postfix; }

    /// \brief Set the steps the spin box will take.
    /// \param steps The steps, will be extended with the ratio from the start after
    /// the last element.
    inline void setSteps(const std::vector<double> &steps) { this->m_steps = steps; }

    void setValue(double value);

  private:
    /// To realize stepBy() in an efficient way, we store the decade/magnitude and stepId of the value
    /// given in setValue().
    /// This way the value() can easily be stepped up or down (by adjusting stepId and if necessary the decade)
    /// to compute an increased/decreased value.
    struct StepCache {
        int decade = 0;
        unsigned stepId = 0;
    };

    Unit m_unit;                 ///< The SI unit used for this spin box
    QString m_unitPostfix;       ///< Shown after the unit
    std::vector<double> m_steps; ///< The steps, begins from start after last element
    StepCache m_stepUpper;       ///< Describes the currently set value as decade/magnitude+stepId (upper bound)
    StepCache m_stepLower;       ///< Describes the currently set value as decade/magnitude+stepId (lower bound)
};
