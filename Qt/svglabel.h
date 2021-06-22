#ifndef SVGLABEL_H
#define SVGLABEL_H

#include <QWidget>
#include <QLabel>

#include <QMouseEvent>
#include <qmath.h>
#include <QtXml>

#include <opencv2/core/core.hpp>

/**
 * @brief Custom class used for handling SVG images and its transformations.
 * @details Its purpose is to be able to be able to move the instance with the mouse and rotate. Subsequently to be able to modify, the adequate changes, the SVG file.
 */
class SvgLabel : public QLabel
{
    Q_OBJECT
public:
    explicit SvgLabel(QWidget *parent = nullptr);
    ~SvgLabel();

    /**
     * @brief Pointer to self - should be a generic bool flag
     */
    SvgLabel *m_select;

    /**
     * @brief Rotation of the instance
     */
    double m_rotate;

    /**
     * @brief Holds the original QPixmap of the image
     */
    QPixmap m_img;
    QPixmap m_scaled_img;


    /**
     * @brief Holds the absolute path to the image. Used in the XML_edit() method to edit the SVG file
     */
    QString pathToImg;

    /**
     * @brief Position.x of non-rotated img
     */
    int m_OG_position_x;

    /**
     * @brief Position.y of non-rotated img
     */
    int m_OG_position_y;

    /**
     * @brief Offset.y of the rotated image with regards to the non-rotated
     */
    int m_y_pos_offset;  //offset for rotated img

    /**
     * @brief Offset.x of the rotated image with regards to the non-rotated
     */
    int m_x_pos_offset;

    /**
     * @brief The startPositions enum (expandable to contain scaling posibilites for the SvgLabel)
     */
    enum startPositions {drag} startPositions;

    /**
     * @brief .
     * @return Returns the ID
     */
    size_t GetID() const {return iID;}

    /**
     * @brief Edits the XML file of the instance
     * @details Adds the transform matrix according to the member variables of the instance
     *
     * | cos(a)  -sin(a)  -cx × cos(a) + cy × sin(a) + cx + tx |
     * | sin(a)   cos(a)  -cx × sin(a) - cy × cos(a) + cy + ty |
     * |   0        0                    1                     |
     *
     * @return Returns true if successful, otherwise false.
     */
    bool XML_edit();

    static std::vector<QString> GetXmlAttributes() {return m_XML_attributes;}
    static size_t GetTotal(){return iTotal;}


    /**
     * @brief Position of the image with regards to the top-left corner of the camera frame
     */
    cv::Point_<double> m_image_position;


protected:

    /**
     * @brief mousePressEvent
     * @details Frames the clicked instance to appear highlighted by its bounding box. Calls the methods of LasserCutter to initialize the information about the instance
     * Prepares for the move event
     * @param event MousePress event
     */
    void mousePressEvent(QMouseEvent *event);

    /**
     * @brief mouseMoveEvent
     * @details Calculates and moves the instance according to the position of the mouse. (The offset of the instance and the mouse event position)
     * @param event MouseMove event
     */
    void mouseMoveEvent(QMouseEvent *event);

private:
    const size_t iID;
    static size_t iLiving;
    static size_t iTotal;

    /**
     * @brief Start position of the move event
     */
    QPoint dragStartPosition;

    /**
     * @brief Starting geometry of the move event
     */
    QRect dragStartGeometry;
    enum startPositions startPos;

    static std::vector<QString> m_XML_attributes;
};


#endif // SVGLABEL_H
