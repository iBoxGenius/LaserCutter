#include "dragwidget.h"

DragWidget::DragWidget(QWidget *parent)
    : QFrame(parent), pathToImg("")
{
    setMinimumSize(200, 200);
    setAcceptDrops(true);
}

DragWidget::~DragWidget()
{
}


void DragWidget::ClearWidget(bool flag)
{
    if(flag)
    {
        QList<class SvgLabel*> Slabels = findChildren<class SvgLabel*>();
        for(auto &label : Slabels)
        {
            label->~SvgLabel();
        }
    }

    QList<QLabel*> Slabels = findChildren<QLabel*>();
    for(auto &label : Slabels)
    {
        label->~QLabel();
    }
}


void DragWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-dnditemdata"))
    {
        if (event->source() == this)
        {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        }
        else
        {
            event->acceptProposedAction();
        }
    }
    else
    {
        event->ignore();
    }
}

void DragWidget::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-dnditemdata"))
    {
        if (event->source() == this)
        {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        }
        else
        {
            event->acceptProposedAction();
        }
    }
    else
    {
        event->ignore();
    }
}

void DragWidget::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-dnditemdata"))
    {
        QByteArray itemData = event->mimeData()->data("application/x-dnditemdata");
        QDataStream dataStream(&itemData, QIODevice::ReadOnly);

        QList<QLabel*> Slabels = event->source()->findChildren<QLabel*>();

        QPixmap pixmap;
        QPoint offset;
        dataStream >> pixmap >> offset;

        class SvgLabel *newIcon = new class SvgLabel(this);
        //QLabel *newIcon = new QLabel(this);
        newIcon->setPixmap(pixmap);
        newIcon->m_img = pixmap;
        newIcon->m_scaled_img=pixmap;


        newIcon->move(event->pos() - offset);
        newIcon->show();                    //important
        newIcon->m_OG_position_x = newIcon->pos().x();
        newIcon->m_OG_position_y = newIcon->pos().y();

        newIcon->setScaledContents(false);

        newIcon->pathToImg = static_cast<DragWidget*>(event->source())->pathToImg; // monkaGIGA... newIcon gets the pathToImg (path to newIcon's img)

        newIcon->setAttribute(Qt::WA_DeleteOnClose);

        newIcon->m_select = newIcon;
        newIcon->setFrameStyle(1);



        if (event->source() == this)
        {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        }
        else
        {
            event->acceptProposedAction();
        }
    }
    else
    {
        event->ignore();
    }
}


void DragWidget::mousePressEvent(QMouseEvent *event)
{
    class SvgLabel *child = static_cast<class SvgLabel*>(childAt(event->pos()));
    //QLabel *child = static_cast<QLabel*>(childAt(event->pos()));
    QList<class SvgLabel*> Slabels = findChildren<class SvgLabel*>();
    if (!child)
    {
        if(!Slabels.empty())
        {
            for(int i = 0; i < Slabels.size(); ++i)
            {
            Slabels.at(i)->m_select = nullptr;
            Slabels.at(i)->setFrameStyle(0);
            }
        }

       return;
    }


    pathToImg = childAt(event->pos())->objectName();                        //press ===> dragwidget pathToImg = pathToImg of the child

    QPixmap pixmap = child->pixmap(Qt::ReturnByValue);

    QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);
    dataStream << pixmap << QPoint(event->pos() - child->pos());

    QMimeData *mimeData = new QMimeData;
    mimeData->setData("application/x-dnditemdata", itemData);

    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->setPixmap(pixmap);
    drag->setHotSpot(event->pos() - child->pos());


    QPixmap tempPixmap = pixmap;
    QPainter painter;
    painter.begin(&tempPixmap);
    painter.fillRect(pixmap.rect(), QColor(127, 127, 127, 30));
    painter.end();

    child->setPixmap(tempPixmap);

    if (drag->exec(Qt::CopyAction | Qt::MoveAction, Qt::CopyAction) == Qt::MoveAction)
    {
        child->close();
    }
    else
    {
        child->show();
        child->setPixmap(pixmap);
    }
}
