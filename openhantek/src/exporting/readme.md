# Content
This directory contains exporting functionality and exporters, namely

* Export to comma separated value file (CSV): Write to a user selected file,
* Export to an image/pdf: Writes an image/pdf to a user selected file,
* Print exporter: Creates a printable document and opens the print dialog.

All export classes (exportcsv, exportimage, exportprint) implement the
ExporterInterface and are registered to the ExporterRegistry in the main.cpp.

At the moment export classes are still using the legacyExportDrawer class to
paint all the labels. It is planned to unify this with the widget based solution of DsoWidget.

# Dependency
* Files in this directory depend on the result class of the post processing directory.
* Classes in here depend on the user settings (../settings/)
