#ifndef DATAGRID_H
#define DATAGRID_H

#include <QGroupBox>

class QPushButton;
class QButtonGroup;
class QLabel;
class QGridLayout;

class DataGrid : public QGroupBox
{
    Q_OBJECT
public:
    explicit DataGrid(QWidget *parent = nullptr);

    struct CursorInfo {
        QPushButton *selector;  ///< The name of the channel
        QPushButton *shape;     ///< The cursor shape
        QLabel *deltaXLabel;    ///< The horizontal distance between cursors
        QLabel *deltaYLabel;    ///< The vertical distance between cursors

        CursorInfo();
        void configure(const QString &text, const QColor &bgColor, const QColor &fgColor);
    };

    unsigned addItem(const QString &text, const QColor &bgColor, const QColor &fgColor);
    void updateInfo(unsigned index, bool visible, const QString &strShape = QString(),
                    const QString &strX = QString(), const QString &strY = QString());

signals:
    void itemSelected(unsigned index);
    void itemUpdated(unsigned index);

public slots:
    void selectItem(unsigned index);

private:
    QButtonGroup *cursorsSelectorGroup;
    QGridLayout *cursorsLayout;
    std::vector<CursorInfo> items;
};

#endif // DATAGRID_H
