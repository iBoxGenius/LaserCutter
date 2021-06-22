#include "filehandling.h"



void FileHandler::MakeTempDir(const QString &path)
{
    QDir dir(QCoreApplication::applicationDirPath().append(path));
    if(dir.exists())
       return;
    dir.mkdir(dir.path());
}


void FileHandler::RemoveTempDir(const QString &path)
{
    QDir dir(QCoreApplication::applicationDirPath().append(path));
    if(!dir.exists())
       return;
    dir.removeRecursively();
}

QString FileHandler::MakeSvgCopy(const QString &path, size_t &image_counter)
{
    QFile file(path);
    if( !file.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
      return "";
    }

    QDomDocument doc;
    doc.setContent(&file);
    QFile outFile( QCoreApplication::applicationDirPath().append("/Temp_img_source/"+QString::number(image_counter)+".svg"));
      if( !outFile.open( QIODevice::WriteOnly | QIODevice::Text ) )
      {
        image_counter--;
        return "";
      }

    QTextStream stream(&outFile);
    stream << doc.toString();

    file.close();
    outFile.close();

    return (QCoreApplication::applicationDirPath().append("/Temp_img_source/"+QString::number(image_counter)+".svg"));
}


bool FileHandler::CreateSurfaceSVG(const std::vector<double> &aCutter_area)
{
    QFile cutter(":/images/surface.svg");
    if( !cutter.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
      return false;
    }
    QDomDocument surface;
    surface.setContent(&cutter);

    QDomNode root = surface.namedItem("svg");
    root.attributes().namedItem("width").setNodeValue(QString::number(aCutter_area.at(0))+"mm");
    root.attributes().namedItem("height").setNodeValue(QString::number(aCutter_area.at(1))+"mm");

    QDomNodeList g = surface.elementsByTagName("g");

    //SvgLabel aux;
    for(size_t i=0; i<SvgLabel::GetTotal(); ++i)
    {
        QFile file(QCoreApplication::applicationDirPath().append("/Temp_img/"+QString::number(i)+".svg"));
        if( !file.open( QIODevice::ReadOnly | QIODevice::Text ) )
        {
          continue;
        }
        QDomDocument doc;
        doc.setContent(&file);

        for(auto &attributes : SvgLabel::GetXmlAttributes())
        {
            QDomNodeList elements = doc.elementsByTagName(attributes);
            //Rectangle <rect>
            //Circle <circle>
            //Ellipse <ellipse>
            //Line <line>
            //Polyline <polyline>
            //Polygon <polygon>
            //Path <path>

            for(int k=0; k<elements.count(); ++k)
            {
                g.at(0).appendChild(elements.item(k));
            }
        }
        file.close();
    }
    cutter.close();

    QFile outFile( QString(QApplication::applicationDirPath().append("/Temp_img/surface_final.svg")));
      if( !outFile.open( QIODevice::WriteOnly | QIODevice::Text ) )
      {
        return false;
      }

    QTextStream stream(&outFile);
    stream << surface.toString();

    outFile.close();
    return true;
}

std::vector<QString> FileHandler::CopySvgsFromDir(size_t &image_counter, const QDir &aDir)
{
    QStringList svg_files = aDir.entryList(QStringList() << "*.svg" ,QDir::Files);
    std::vector<QString> ret(svg_files.size());

    for(size_t i =0; i < ret.size(); ++i)
    {
        ret.at(i) = aDir.path().append("/"+svg_files.at(i));
    }

    for(auto &paths : ret)
    {
        paths = MakeSvgCopy(paths,image_counter);
        image_counter++;
    }

    return ret;
}

void FileHandler::ClearPathDir(const QString &aPath)
{
    QDir dir(QCoreApplication::applicationDirPath().append(aPath));
    if(!dir.entryList().empty())
    {
        foreach(QString dirFile, dir.entryList())
        {
            dir.remove(dirFile);
        }
    }

    return;
}

bool FileHandler::SetConfig(const QString &aPath, const double &aWidth, const double &aHeight)
{
    if(aPath == "." || aPath == "")             //if QDialog was cancelled or was not invoked
        return true;


    QFile config_file(QCoreApplication::applicationDirPath().append("/config/cutter_config.xml"));
    if( !config_file.open( QIODevice::ReadWrite | QIODevice::Text ) )
    {
      return false;
    }
    QDomDocument doc;
    doc.setContent(&config_file);
    QDomNodeList elements = doc.elementsByTagName("Paths");
    QDomElement el = elements.at(0).toElement();
    QDomNamedNodeMap map = el.attributes();
    map.namedItem("imageSource").setNodeValue(aPath);

    if(aWidth != 0 && aHeight !=0)
    {
        elements = doc.elementsByTagName("CutterParameters");
        el = elements.at(0).toElement();
        map = el.attributes();
        map.namedItem("cutting_area_width").setNodeValue(QString::number(aWidth));
        map.namedItem("cutting_area_height").setNodeValue(QString::number(aHeight));
    }
    config_file.close();

    QFile config_file_write(QCoreApplication::applicationDirPath().append("/config/cutter_config.xml"));
      if( !config_file_write.open( QIODevice::WriteOnly | QIODevice::Text ) )
      {
        return false;
      }

    QTextStream stream(&config_file_write);
    stream << doc.toString();

    config_file_write.close();

    return true;
}
