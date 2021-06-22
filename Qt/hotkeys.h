#ifndef HOTKEYS_H
#define HOTKEYS_H

#include <QDialog>
#include <QLabel>
#include <QTableWidget>

namespace Ui {
class Hotkeys;
}

/**
 * @brief A pop window to show the hotkeys of the application. A table of hotkeys is shown.
 */
class Hotkeys : public QDialog
{
    Q_OBJECT

public:
    explicit Hotkeys(QWidget *parent = nullptr);
    ~Hotkeys();

private:
    Ui::Hotkeys *ui;

    /**
     * @brief Pointer to the table
     */
    QTableWidget *m_table;
};

#endif // HOTKEYS_H
