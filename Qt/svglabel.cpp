#include "svglabel.h"
#include "lasercutter.h"

size_t SvgLabel::iLiving=0;
size_t SvgLabel::iTotal=0;
std::vector<QString> SvgLabel::m_XML_attributes = {"rect", "circle", "ellipse", "line", "polyline", "polygon", "path"};


SvgLabel::SvgLabel(QWidget *parent) : QLabel(parent), m_select(nullptr), m_rotate(0), pathToImg(""),m_image_position(0),iID(iTotal)
{
    iLiving++;
    iTotal++;
    //m_XML_attributes = {"rect", "circle", "ellipse", "line", "polyline", "polygon", "path"};
    setMouseTracking(true);
    setScaledContents(false);
    m_x_pos_offset = m_y_pos_offset = 0;
    m_OG_position_x = m_OG_position_y = 0;
}

SvgLabel::~SvgLabel()
{
    iLiving--;
}

void SvgLabel::mousePressEvent(QMouseEvent *event)
{
    static_cast<class LaserCutter*>(this->parent()->parent()->parent()->parent())->RotateInit(m_rotate);             //bad practice
    static_cast<class LaserCutter*>(this->parent()->parent()->parent()->parent())->SvgLabelInfo(*this);
    if (event->button() == Qt::LeftButton)
    {
        dragStartPosition = event->pos();
        dragStartGeometry = geometry();
        setFrameStyle(1);
        m_select = this;
        QList<class SvgLabel*> Slabels = parent()->findChildren<class SvgLabel*>();
        if(!Slabels.empty())
        {
            for(int i = 0; i < Slabels.size(); ++i)
            {
                if(Slabels.at(i) != this)
                {
                    Slabels.at(i)->m_select = nullptr;
                    Slabels.at(i)->setFrameStyle(0);
                }
            }
        }
    }

}


void SvgLabel::mouseMoveEvent(QMouseEvent *event)
{
    static_cast<class LaserCutter*>(this->parent()->parent()->parent()->parent())->SvgLabelInfo(*this);
    if (!(event->buttons() & Qt::LeftButton))
    {
        startPos = drag;
        setCursor(Qt::SizeAllCursor);
        return;
    }

    switch (startPos) {
    case drag:
    {
       setGeometry(dragStartGeometry.left() - (dragStartPosition.x() - event->x()),
                   dragStartGeometry.top() - (dragStartPosition.y() - event->y()),
                   width(),
                   height());
       dragStartGeometry = geometry();
       m_OG_position_x = pos().x()-m_x_pos_offset;
       m_OG_position_y = pos().y()-m_y_pos_offset;
        break;
    }

    default:
        break;
    }

    return;
}

bool SvgLabel::XML_edit()
{
    QFile file(pathToImg);
    if( !file.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
      return false;
    }
    QDomDocument doc;
    doc.setContent(&file);

    double img_width = 0;
    double img_height = 0;

    QDomNode root = doc.namedItem("svg");
    QDomNodeList nodeList = root.childNodes();
    QString svg_width = root.attributes().namedItem("width").nodeValue();
    QString svg_height = root.attributes().namedItem("height").nodeValue();

    img_width = svg_width.toDouble()/3.7795;
    img_height = svg_height.toDouble()/3.7795;


    for(QString i : m_XML_attributes)
    {
        QDomNodeList elements = doc.elementsByTagName(i);
        //Rectangle <rect>
        //Circle <circle>
        //Ellipse <ellipse>
        //Line <line>
        //Polyline <polyline>
        //Polygon <polygon>
        //Path <path>
        for(int i=0; i<elements.count(); ++i)
        {


            QDomElement el = elements.at(i).toElement();
            double a = qCos(qDegreesToRadians(m_rotate));
            double b = qSin(qDegreesToRadians(m_rotate));
            double c = -qSin(qDegreesToRadians(m_rotate));
            double d = qCos(qDegreesToRadians(m_rotate));
            double e = -img_width*0.5*qCos(qDegreesToRadians(m_rotate))+img_height*0.5*qSin(qDegreesToRadians(m_rotate))+img_width*0.5+m_image_position.x;
            double f = -img_width*0.5*qSin(qDegreesToRadians(m_rotate))-img_height*0.5*qCos(qDegreesToRadians(m_rotate))+img_height*0.5+m_image_position.y;

            // | cos(a)  -sin(a)  -cx × cos(a) + cy × sin(a) + cx + tx |
            // | sin(a)   cos(a)  -cx × sin(a) - cy × cos(a) + cy + ty |
            // |   0        0                    1                     |

            el.setAttribute("transform",
                            "matrix("+
                            QString::number(a)+","+QString::number(b)+","+QString::number(c)+","
                            +QString::number(d)+","+QString::number(e)+","+QString::number(f)
                            +")"
                            );
        }
    }


    QFile outFile( QCoreApplication::applicationDirPath().append("/Temp_img/"+QString::number(GetID())+".svg"));
      if( !outFile.open( QIODevice::WriteOnly | QIODevice::Text ) )
      {
        return false;
      }

    QTextStream stream(&outFile);
    stream << doc.toString();

    file.close();
    outFile.close();

    return true;
}




