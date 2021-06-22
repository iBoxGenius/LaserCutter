#ifndef FILEHANDLING_H
#define FILEHANDLING_H

#include <QMainWindow>
#include <QApplication>
#include<QString>
#include<QDir>
#include<QDomDocument>
#include<QTextStream>
#include <QFileDialog>

#include"svglabel.h"

/**
 *@brief Namespace for file handling of the application
 */
namespace  FileHandler{
    /**
     * @brief Creates a temporary directory
     * @details Creates a temporary directory, which is used temporarily for storing copies of images
     * @param path Path for the to-be-created directory
     */
    void MakeTempDir(const QString &path);

    /**
     * @brief Removes a temporary directory
     * @details Removes a temporary directory recursivaly (all children)
     * @param path Path to the to-be-destroyed directory
     */
    void RemoveTempDir(const QString &path);

    /**
     * @brief Creates a copy of an Svg file
     * @details Creates a copy of an Svg file, used in the application as the image source
     * @param path Path to the image
     * @param image_counter Serves as a filename for each instance of a copied image
     * @return Returns the path to the copied image
     */
    QString MakeSvgCopy(const QString &path, size_t &image_counter);

    /**
     * @brief Creates the LaserCutter surface SVG
     * @details Creates the LaserCutter surface SVG with all the images placed correctly placed. Function creates a copy of the reference surface file
     * and copies all the vector properties (rect, circle etc.) of the placed images to the final SVG file, which is later used as a source for G-Code generation.
     * @param aCutter_area Defines the width and height of the final_surface SVG file
     * @return Returns false if the process failed, otherwise true
     */
    bool CreateSurfaceSVG(const std::vector<double> &aCutter_area);

    /**
     * @brief Clears the directory
     * @details Clears the contents of the given directory
     * @param aPath Path to the directory to be cleared
     */
    void ClearPathDir(const QString &aPath);


    /**
     * @brief Creates copies of Svgs in the given directory
     * @details Copies all svg files from the given directory into a temporary folder, which is used as a source
     * @param image_counter Counter that assigns the copies incremental names (0.svg ; 1.svg; ...)
     * @param aDir Directory to be copied from
     * @return Returns a vector of absolute paths of each of the copied Svg files (including the names of the copies of the Svg files)
     */
    std::vector<QString> CopySvgsFromDir(size_t &image_counter, const QDir &aDir);

    /**
     * @brief Sets the path to the config file
     * @details Sets the image source path to the config.xml file, which is used to initialize the application.
     * @param aPath Path to be set
     * @param aWidth Width to be set
     * @param aHeight Height to be set
     * @return Returns False if it has failed, otherwise true
     */
    bool SetConfig(const QString &aPath, const double &aWidth, const double &aHeight);
}

#endif // FILEHANDLING_H
