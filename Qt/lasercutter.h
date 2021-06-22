#ifndef LASERCUTTER_H
#define LASERCUTTER_H

#include <QMainWindow>

#include <QCamera>
#include <QCameraImageCapture>
#include <QCameraInfo>
#include <QCameraViewfinder>
#include <QMediaRecorder>
#include <QMediaService>
#include <QMediaMetaData>
#include <QScopedPointer>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/imgcodecs/imgcodecs.hpp>
#include <opencv2/aruco.hpp>
#include <opencv2/videoio.hpp>

#include <QtSvg>
#include <QPalette>

#include <QDial>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QElapsedTimer>

#include <QMessageBox>
#include <QFileDialog>

#include "dragwidget.h"
#include "svglabel.h"
#include "filehandling.h"
#include "hotkeys.h"

QT_BEGIN_NAMESPACE
namespace Ui { class LaserCutter; }
QT_END_NAMESPACE


/**
 * @brief Main Class for the whole application.
 * @details Handles most of the funcionalities of from the user's side. Implements the OpenCV methods and controls the workflow of the application.
 */
class LaserCutter : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief Default c'tor
     * @details Initializes the UI, connects majority of the signals and creates a list of available cameras
     */
    LaserCutter();

    /**
     * @brief Default d'tor
     * @details Deletes the UI and removes the TEMP directories
     */
    ~LaserCutter();

    /**
     * @brief Initializes the dial in the ui
     * @details Sets the value of the rotate dial based on the argument of the SvgLabel m_rotate member variable
     * @param aRotate Value used as the initializer
     */
    void RotateInit(int aRotate);

    /**
     * @brief Obtains the information about the SvgLabel instance
     * @details Acquires the position of the SvgLabel within the bounds of the cutting area(camera frame). Position "x", position "y" and rotation of the SvgLabel
     * The parameters are shown in the UI. Positions are calculated with regards to the top left corner [0,0].
     * @param aSlabel The Targeted SvgLabel
     */
    void SvgLabelInfo(const SvgLabel& aSlabel);

protected:
    /**
     * @brief mousePressEvent Overloaded event of the mousePress handling
     * @details The press event clears focus (deselects) of the UI components
     * @param event mouse press event
     */
    void mousePressEvent(QMouseEvent *event) override;

    /**
     * @brief keyReleaseEvent Overloaded event of the keyRelease handling
     * @details The event allows for the usage of hotkeys in the application
     * @param event key release event
     */
    void keyReleaseEvent(QKeyEvent *event) override;

    /**
     * @brief closeEvent Overloaded event of the closeEvent handling
     * @details Pops a dialog about quitting the application, automatically saves the <b>path</b> to the image source folder to the config file
     * @param event close event
     */
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::LaserCutter *ui;

    /**
     * @brief Holds the counter for the image placement
     */
    size_t m_image_source_counter;

    /**
     * @brief Used for Camera Hanlding, before the start of the  application
     */
    QScopedPointer<QCamera> m_camera;

    /**
     * @brief Used to determine the maximum resolution of the camera
     */
    QScopedPointer<QCameraImageCapture> m_cameraImageCapture;
    /**
     * @brief ActionGroup for the Devices menu
     */
    QActionGroup *m_videoDevicesGroup;

    /**
     * @brief Structure for Camera info, maximum resolution and currenlty seleceted camera
     */
    struct sCamera{
        QList<QSize> resolutions;
        int camera_selected = -1;
    }m_CamInfo;

    /**
     * @brief Updates the Devices menu when it is about to show
     */
    void UpdateCamerasMenu();

    /**
     * @brief Holds the position of the ArucoMarker's corners (corners with index 0)
     */
    std::vector<cv::Point_<double>> m_corners;

    /**
     * @brief Transform ratio of px~mm for each axis (x;y)
     */
    double m_transfer_ratio[2];

    /**
     * @brief Cutting area dimensions of the laser cutter (width, height)
     */
    std::vector<double> m_cutting_area;

    /**
     * @brief Holds the path to the folder of the source images
     */
    QString m_Path_to_source;

    /**
     * @brief Rotation of the currently selected SvgLabel instance
     */
    int m_rotate;

    /**
     * @brief GetRotatedPoint Calculates the coordinates of a point rotated around a refernce point
     * @param p Point being rotated
     * @param center Point of rotation (reference point)
     * @param angleRads Angle of rotation on radians
     * @return Returns the new coordinates of the rotated point
     */
    QPointF GetRotatedPoint(const QPointF &p, const QPointF &center, const qreal &angleRads);

    /**
     * @brief GetMinimumBoundingRect Calculates the minimum bounding box of a rotated image
     * @param r The bounding box of the image
     * @param angleRads Angle of rotation
     * @return Returns the minimum bounding box
     */
    QRectF GetMinimumBoundingRect(const QRect &r, const qreal &angleRads);

    /**
     * @brief RotateLabel Rotates an SvgLabel
     * @details Rotates the currently selected SvgLabels. Uses the \c GetMinimumBoundingRect method to calculate the dimensions of the bounding box.
     * Moves the SvgLabel by the offset regarding the rotation. Utilizes \c QTransform (affine transformation) to rotate the SvgLabel.
     */
    void RotateLabel();

    /**
     * @brief Used for cycling through the SvgLabel instances by pressing 'Tab'
     */
    int m_label_tab_index;

    /**
     * @brief Utilized for placing the image in the image_source DragWidget instance
     */
    std::array<size_t,2> m_img_source_placement;

    /**
     * @brief Vector of paths to the source images
     */
    std::vector<QString> m_res;

    /**
     * @brief Initializes the cutter parameters
     * @details Initializes the cutter parameters, width and height of the cutting area and the path to the image source folder, from the config.xml file.
     */
    void InitCutterInfo();

    /**
     * @brief Sets the Camera
     * @details Sets the currently selected camera and displays the transmission
     * @param cameraInfo Neccessary camera info
     */
    void SetCamera(const QCameraInfo &cameraInfo);

    /**
     * @brief Displays the camera error
     */
    void DisplayCameraError();

    /**
     * @brief Timer used for PerspectiveTransform()
     */
    QElapsedTimer *m_tm;

    /**
     * @brief Timer, its timeout used to call PerspectiveTransform()
     */
    QTimer *m_timer;

    /**
     * @brief Used for Camera Hanlding, after the start of the application
     */
    cv::VideoCapture inputVideo;

    /**
     * @brief Opens SVG files from the selected folder
     * @details A dialog window appears wanting to select the source image folder. Clears the <b>TEMP</b> dirs and the <b>image_source</b>, <b>image_destinantion</b>
     * DragWidget instances. Copies the images from the selected folder to the \c Temp_img_source folder
     */
    void OpenFiles();

    /**
     * @brief Saves the currently displayed SvgLabels
     * @details Copies each displayed SvgLabel from the \c Temp_img_source to the \c Temp_img folder. Edits the SVG file according to the applied transformation (transfrom matrix)
     */
    void SaveXML_copies();

    /**
     * @brief Clears the workspace (image_destination DragWidget instance)
     */
    void ClearWorkspace();

    /**
     * @brief Acquires the position of each of the images
     * @details The position is with regards to the top left corner [0,0]. All of the images on the image_destination DragWidget instance
     */
    void GetImagePositions();

    /**
     * @brief Sets up the dimensions of the \c camera frame and the  \c image_destination DragWidget instance
     */
    void SetupUiFrameSize();

    /**
     * @brief Updates the camera device on action trigger
     * @param action Trigger for the method (change of the camera in the Devices menu)
     */
    void UpdateCameraDevice(QAction *action);

    /**
     * @brief Detection of ArucoMarkers from camera input
     * @details Detects the four markers from the selected camera. If the markers are not detected after five seconds, returns false.
     * Also stores the positions of each of the [0] corneres of the Arucos to m_corners variable
     * @return Returns true if the all four markers are found, otherwise false
     */
    bool CameraMarkerDetection();

    /**
     * @brief Calculates the transform ratio for each axis (px~mm)
     */
    void GetTransferRatio();

    /**
     * @brief Normalizes a QLabel's dimensions
     * @details Utilizes the QTransform class to apply the scale transform
     * @param aTransform_ratio Transform ratio used for each axis (x,y)
     * @param label The QLabel to be transformed
     */
    void Transform_to_normalized(double *aTransform_ratio, QLabel& label);

    /**
     * @brief Sets up the image_source DragWidget instance
     * @details Places each QLabel to image_source DragWidget instance. Maximum of 60 images.
     * @param aPath Path to the image.
     */
    void SetImageSource(QString aPath);

    /**
     * @brief Dimensions of the cutting area
     */
    QSize m_dimensions;

    /**
     * @brief Calculates the Euclidian distance between two points in the image
     * @param aUno Point 1
     * @param aDuo Point 2
     * @return Returns the Euclidian distance
     */
    double Pythagoras(const cv::Point2d &aUno, const cv::Point2d &aDuo);

    /**
     * @brief Converts cv::Mat to QPixmap
     * @param src Source image
     * @return Returns the converted QPixmap
     */
    QPixmap MatToPixmap(cv::Mat src);

    /**
     * @brief Converts QImage to cv::Mat
     * @param inImage Input QImage
     * @param inCloneImageData Flag for creatin a copy
     * @return Returns the converted cv::Mat
     */
    cv::Mat QImageToMat( const QImage &inImage, bool inCloneImageData);

    /**
     * @brief Determines whether two cv::Mat instances are equal
     * @param mat1 Matrix A
     * @param mat2 Matrix B
     * @return Returns true, if A and B are equal, otherwise false
     */
    bool MatIsEqual(const cv::Mat &mat1, const cv::Mat &mat2);

    /**
     * @brief Checks the positions of the SvgLabel instance whether they are within the bounds of the material
     * @details Takes an image from the camera input and applies binary thresholding (inverse, non-inverse). Grabs the image from the image_destination DragWidget instance with black and white background.
     * Applies the same thresholding. The white_label is then negated. ANDs the corresponding images and compares them with original(of the camera frame thresholded images).
     * If the Images are equal, objects are within the bounds of the material
     * @return Returns true if images are within the bounds of the material, otherwise false.
     */
    bool CheckSvgsPositions();

    /**
     * @brief Track the material
     * @details Tracks the centroid of the conours (= material) and moves all the images based on the difference with the previous centroid position
     * @param tracking_enabled Flag for enabling the tracking
     */
    void TrackMaterial(bool tracking_enabled);

    bool m_tracking_flag;

    /**
     * @brief New position of the tracked centroid
     */
    QPoint m_centroid;

    /**
     * @brief Old position of the tracked centroid
     */
    QPoint m_old_centroid;

    /**
     * @brief Selected COM port, used for the Svg2GcodeSend as an argument
     */
    QString final_port;                         //storing the COM port number

    /**
     * @brief Fills the COM ports Info
     */
    void FillPortsInfo();

    /**
     * @brief Creates an instance of the Hotkeys class.(new window)
     */
    void HotkeysDialog();

    /**
     * @brief Pointer to the Hotkeys class instance
     */
    Hotkeys *m_hotkeys;

    /**
     * @brief Shows the user manual
     */
    void ShowManual();

    /**
     * @brief Pointer to the Svg2GcodeSend process
     */
    QProcess *m_process_cut;

private slots:

    /**
     * @brief PerspectiveTransform
     * @return
     */
    bool PerspectiveTransform();

    /**
     * @brief Shows the ports info based on the idx
     * @param idx Index of the list (COM port)
     */
    void ShowPortInfo(int idx);


    /**
     * @brief Initialization of the app
     * @details Calls the InitCutterInfo() method to obtain the neccessary paramters. Hands over the camera transmission to the CameraMarkerDetection() method, and later to the PerspectiveTransform() method.
     * Restarts the timers in place. and initializes the spinBox values. If marker detection succeeds, calls  SetupUiFrameSize(), GetTransferRatio() methods, creates the Temp folders, calls CopySvgsFromDir().
     * Connects the neccessary signals.
     */
    void Initialize();

    /**
     * @brief Reset of the app
     * @details Stops the perspective transform and reinitializes the app to the default state. Clearing the Temp folders and DragWidget instances.
     */
    void ResetApp();

    /**
     * @brief Prompts the user if the cutting process has finished
     * @param exitCode ExitCode of the Svg2GcodeSend process
     * @param exitStatus {Parameter unused}
     */
    void CuttingFinished(int exitCode, QProcess::ExitStatus exitStatus);

};
#endif // LASERCUTTER_H
