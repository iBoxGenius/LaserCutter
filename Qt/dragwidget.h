#ifndef DRAGWIDGET_H
#define DRAGWIDGET_H

#include <QFrame>
#include <QLabel>
#include <QtWidgets>

#include "svglabel.h"


QT_BEGIN_NAMESPACE
class QDragEnterEvent;
class QDropEvent;
QT_END_NAMESPACE

/**
 * @brief Custom Widget for drag&drop action
 * @details Serves as a custom Widget for drag&drop action (between two instances). In the UI reffered to as image_source, image_destination
 */

class DragWidget : public QFrame
{
public:
    /**
     * @brief Default c'tor
     * @details Creates a DragWidget instance on the parent Widget.
     * @param parent Widget to be created on
     */
    DragWidget(QWidget *parent = nullptr);

    /**
     * @brief Default d'tor
    */
    ~DragWidget();


    /**
     * @brief ClearWidget
     */
    void ClearWidget(bool flag);

protected:

    /**
     * @brief dragEnterEvent, overridden
     * @details Checks for the mime-data of the object that triggered the event. Chooses whether to accept the event.
     * @param event Event that triggers the method
     */
    void dragEnterEvent(QDragEnterEvent *event) override;


    /**
     * @brief dragMoveEvent, overridden
     * @details Checks for the mime-data of the object that triggered the event. Chooses whether to accept the event.
     * @param event Event that triggers the method
     */
    void dragMoveEvent(QDragMoveEvent *event) override;


    /**
     * @brief dropEvent, overridden
     * @details Extracts the data from mime-data and creates a new instance of SvgLabel class on the drop position with the source's pixmap
     *  and initializes some members of said SvgLabel instance
     * @param event Event that triggers the method
     */
    void dropEvent(QDropEvent *event) override;


    /**
     * @brief mousePressEvent, overridden
     * @details Checks whether the event source is a child (SvgLabel). If not, deselects all the children.
     * If so, sets the path to the image(which is later passed on to the child in the drop event) and initializes the start of the DragEvent process.
     * @param event Event that triggers the method
     */
    void mousePressEvent(QMouseEvent *event) override;

private:

    /**
     * @var
     * @brief pathToImg path to the image, which is later passed on to the child in the drop event
     */
    QString pathToImg;

};

#endif // DRAGWIDGET_H
