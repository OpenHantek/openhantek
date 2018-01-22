// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QIcon>
#include <QString>

#include <memory>

class PPresult;

namespace Exporter {
class Registry;

/**
 * Implement this interface and register your Exporter to the ExporterRegistry instance
 * in the main routine to make an Exporter available.
 */
class ExporterInterface {
  public:
    /**
     * @return Return the icon representation of this exporter. Will be used in graphical
     * interfaces.
     */
    virtual QIcon icon() = 0;

    /**
     * @return Return the text representation / name of this exporter. Will be used in graphical
     * interfaces.
     */
    virtual QString name() = 0;

    /**
     * @return Return a shortcut for this exporter. Will be used in graphical
     * interfaces.
     */
    virtual QKeySequence shortcut() = 0;

    /**
     * Exporters can save only a single sample set or save data continously.
     */
    enum class Type { SnapshotExport, ContinousExport };

    /**
     * @return Return the type of this exporter.
     * @see ExporterInterface::Type
     */
    virtual Type type() = 0;

    /**
     * A new sample set arrived at the ExporterRegistry. The exporter needs to be a continous exporter: See type().
     * This method is called in the thread context of the ExporterRegistry.
     *
     * \return Report the used memory / reservered memory ratio here. If float>=1.0 then this exporter will be
     * deactivated again and will not receive anymore sample data.
     */
    virtual float samples(const std::shared_ptr<PPresult>) = 0;

    /**
     * Start the export process. If you are a snapshot exporter, now is the time to access the last sampleset
     * from the ExporterRegistry, convert it, ask for a filename and save the data.
     *
     * If you are a continous exporter, you should show a dialog to the user to inform about the progress and provide
     * a cancel option.
     *
     * This method will be called in the GUI thread context and can create and show dialogs if required.
     *
     * @param registry The exporter registry instance. This is used to obtain a reference
     *        to the settings and sample data.
     * @return Return true if saving or setting everything up succedded otherwise false. If this is a continous
     *        exporter and false is returned, then no sample data will be send via samples(...).
     */
    virtual bool exportNow(Registry *registry) = 0;

    /**
     * Implement this if you are a continous exporter and want the user to be able to stop the export process
     * via export checkbox in the main window.
     */
    virtual void stopContinous() {}

  protected:
    Registry *registry;
};
}
