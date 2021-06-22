#include "lasercutter.h"
#include "ui_lasercutter.h"


Q_DECLARE_METATYPE(QCameraInfo)

LaserCutter::LaserCutter()
    : ui(new Ui::LaserCutter), m_image_source_counter(0),m_corners(4),m_transfer_ratio{1,1}, m_cutting_area(2,0)
    ,m_rotate(0), m_img_source_placement({0,0}), m_res(0,0), m_centroid(0,0), m_old_centroid(0,0)
{
    m_timer = nullptr;
    m_tm = nullptr;
    m_label_tab_index = 0;
    m_tracking_flag = true;

    ui->setupUi(this);

    setWindowTitle("LaserCutter");


    m_videoDevicesGroup = new QActionGroup(this);                                           //ActionGroup == only one device is active
    m_videoDevicesGroup->setExclusive(true);                                                              //same
    const QList<QCameraInfo> availableCameras = QCameraInfo::availableCameras();                        //list from available cameras
    for(int i =0; i < availableCameras.length(); ++i){
        QAction *videoDeviceAction = new QAction(availableCameras.at(i).description(), m_videoDevicesGroup);
        videoDeviceAction->setCheckable(true);
        videoDeviceAction->setData(QVariant::fromValue(availableCameras.at(i)));                                    //QVariant ~ Union
        if (availableCameras.at(i) == QCameraInfo::defaultCamera())
                    videoDeviceAction->setChecked(true);
        videoDeviceAction->setObjectName(QVariant(i).toString());
        ui->menuDevices->addAction(videoDeviceAction);
    }
    if(!m_videoDevicesGroup->actions().empty())
        UpdateCameraDevice(m_videoDevicesGroup->actions().at(0));                                             //sets the default camera
    else
        QMessageBox::critical(this, tr("Camera Error"), "No cameras detected in the system");


    connect(m_videoDevicesGroup, &QActionGroup::triggered, this, &LaserCutter::UpdateCameraDevice);
    connect(ui->menuDevices, &QMenu::aboutToShow, this, &LaserCutter::UpdateCamerasMenu);             //
    connect(ui->start_btn, &QPushButton::released, this, &LaserCutter::Initialize);
    connect(ui->actionOpen_files, &QAction::triggered, this, &LaserCutter::OpenFiles);
    connect(ui->actionExit, &QAction::triggered, this, &QMainWindow::close);

    ui->image_source->setAcceptDrops(false);

    connect(ui->rotate_dial, &QDial::sliderMoved, this, &LaserCutter::RotateLabel);
    connect(ui->clear_btn, &QPushButton::released, this, &LaserCutter::ClearWorkspace);

    connect(ui->serialport, QOverload<int>::of(&QComboBox::currentIndexChanged),this, &LaserCutter::ShowPortInfo);
    FillPortsInfo();

    ui->rotate_dial->setRange(-1,360);
    ui->rotate_dial->setSingleStep(1);
    ui->rotate_dial->setToolTip("Rotate the dial to rotate the selected image(s)");

    ui->serialport->setToolTip("Choose the according COM port");

    ui->doubleSpinBox_width->setRange(0,2000);
    ui->doubleSpinBox_height->setRange(0,2000);

    connect(ui->actionHotkeys, &QAction::triggered, this, &LaserCutter::HotkeysDialog);
    connect(ui->actionManual, &QAction::triggered, this, &LaserCutter::ShowManual);

    ui->horSlider_threshContours->setRange(0,255);
    ui->horSlider_threshContours->setValue(120);
    ui->horSlider_threshContours->setToolTip("Choose the correct threshold, such that, the material becomes outlined");
}


LaserCutter::~LaserCutter()
{
    FileHandler::RemoveTempDir("/Temp_img_source");
    FileHandler::RemoveTempDir("/Temp_img");

    if(m_tm)
        delete m_tm;

    delete ui;
}

void LaserCutter::RotateInit(int aRotate)
{
    ui->rotate_dial->setValue(aRotate);
}

void LaserCutter::SvgLabelInfo(const SvgLabel &aSlabel)
{
    //showing the Label info in the app [x y rotation]
    if(ui->comboBox_units->currentIndex() == 0)
    {
        ui->x_label->setText(QString::number(aSlabel.pos().x()*m_transfer_ratio[0])+" mm");
        ui->y_label->setText(QString::number(aSlabel.pos().y()*m_transfer_ratio[1])+" mm");
    }
    else
    {
        ui->x_label->setText(QString::number(aSlabel.pos().x()));
        ui->y_label->setText(QString::number(aSlabel.pos().y()));
    }
    ui->rotation_label->setText(QString::number(aSlabel.m_rotate)+"°");
}

void LaserCutter::mousePressEvent(QMouseEvent *event)
{
    //clearing focus of the input widgets

    if(event->button() == Qt::LeftButton)
    {
        ui->doubleSpinBox_width->clearFocus();
        ui->doubleSpinBox_height->clearFocus();

        ui->comboBox_units->clearFocus();
        ui->rotate_dial->clearFocus();
        ui->horSlider_threshContours->clearFocus();
    }
}


void LaserCutter::RotateLabel()
{
    // rotating the selected Labels

    QList<class SvgLabel*> Slabels = ui->image_destination->findChildren<class SvgLabel*>();
    if(Slabels.empty())
        return;

    for(auto &label : Slabels)
    {
        if(!label->m_select)
            continue;

        ui->rotation_label->setText(QString::number(label->m_rotate)+"°");
        label->m_rotate=ui->rotate_dial->value();
        QRect rec(QPoint(0,0), QPoint(label->m_scaled_img.width(), label->m_scaled_img.height()));
        QRectF minimumBoundingRect = GetMinimumBoundingRect( rec, qDegreesToRadians(label->m_rotate));      //getting the minimum bounding box

        label->move(label->pos().x() - label->m_x_pos_offset, label->pos().y()-label->m_y_pos_offset);      //moving the label to the non-rotated position

        label->m_x_pos_offset = minimumBoundingRect.x();
        label->m_y_pos_offset = minimumBoundingRect.y();

        label->m_rotate=ui->rotate_dial->value();
        label->m_select->setScaledContents(false);


        QPixmap img(label->m_img);          //m_img is the original pixmap when the image was loaded, not rotated
        QTransform tm;
        tm.rotate(label->m_rotate);
        img = img.transformed(tm,Qt::TransformationMode::SmoothTransformation);

        label->m_select->setPixmap(img);
        label->m_select->show();
        label->m_select->update();
        label->setGeometry(label->pos().x()+label->m_x_pos_offset ,label->pos().y()+label->m_y_pos_offset , minimumBoundingRect.width(), minimumBoundingRect.height());

        label->m_OG_position_x = label->pos().x()-label->m_x_pos_offset;
        label->m_OG_position_y = label->pos().y()-label->m_y_pos_offset;

        if(label->m_rotate >= 360)
            label->m_rotate = 0;
    }
}

void LaserCutter::SaveXML_copies()
{

    QList<class SvgLabel*> Slabels = ui->image_destination->findChildren<class SvgLabel*>();
    if(Slabels.empty())
        return;

    if(!CheckSvgsPositions())
    {
        return;
    }
    GetImagePositions();

    FileHandler::ClearPathDir("/Temp_img");

    for(auto &label: Slabels)
    {
        if(!label->XML_edit())
        {
            QMessageBox::critical(this, tr("File Error"), "Error while editing SVG files, cannot generate the final SVG file for cutting");
            return;
        }
    }

    if(!FileHandler::CreateSurfaceSVG(m_cutting_area))
    {
        QMessageBox::critical(this, tr("File Error"), "Couldn't create surface SVG");
        return;
    }

    QStringList arg;

    m_process_cut = new QProcess(this);
    QString path = QApplication::applicationDirPath().append("/SVG2GcodeSend.exe");

    arg.clear();
    arg << "-s";
    arg << QString(QApplication::applicationDirPath().append("/Temp_img/surface_final.svg"));
    arg << "-p";
    arg << QString(QApplication::applicationDirPath().append("/g-code/surface_final.gcode"));
    arg << "-c";
    arg << QString(final_port);

    m_process_cut->start(path, arg);
    connect(m_process_cut, SIGNAL(finished(int, QProcess::ExitStatus)),this, SLOT(CuttingFinished(int, QProcess::ExitStatus)));
    QMessageBox::information(this, tr("Success!"),"Cutting is commencing");

    //disabling the tracking of the material
    m_tracking_flag = false;
}

void LaserCutter::SetCamera(const QCameraInfo &cameraInfo)
{
    m_camera.reset(new QCamera(cameraInfo));                                                                                //set camera to argument
    m_cameraImageCapture.reset(new QCameraImageCapture(m_camera.data()));
    connect(m_camera.data(), QOverload<QCamera::Error>::of(&QCamera::error), this, &LaserCutter::DisplayCameraError);           //camera error
    m_camera->setViewfinder(ui->viewfinder);
    m_camera->load();

    QCameraViewfinderSettings settings;
    m_CamInfo.resolutions = m_cameraImageCapture->supportedResolutions();       //Qt support for Video.supported resolutions is not working on Windows?
    settings.setResolution(m_CamInfo.resolutions.last());                       //resolution

    m_camera->setViewfinderSettings(settings);
    m_camera->start();


}

void LaserCutter::SetupUiFrameSize()
{
    double ratio = double(m_dimensions.height())/double(m_dimensions.width());
    double base = 850;

    m_dimensions.setWidth(int(double(base)/ratio));
    m_dimensions.setHeight(base);
    ui->camera_frame->setScaledContents(true);

    ui->camera_frame->setGeometry(0,0,m_dimensions.width(),m_dimensions.height());
    ui->image_destination->setGeometry(0,0,m_dimensions.width(),m_dimensions.height());

    return;
}

void LaserCutter::keyReleaseEvent(QKeyEvent * event)
{
    if(event->key() == Qt::Key_Delete)
    {
        QList<class SvgLabel*> Slabels = ui->image_destination->findChildren<class SvgLabel*>();
        if(!Slabels.empty())
        {
            for(auto &label : Slabels)
                {
                    if(label->m_select)
                    {
                       delete label;
                    }
                }
        }
    }

    if (event->key() == Qt::Key_A)
    {
        if ( QApplication::keyboardModifiers () == Qt::ControlModifier)
        {
            QList<class SvgLabel*> Slabels = ui->image_destination->findChildren<class SvgLabel*>();
            if(!Slabels.empty())
            {
                for(auto &label : Slabels)
                    {
                        label->m_select = label;
                        label->setFrameStyle(1);
                    }
            }
        }
    }

    if (event->key() == Qt::Key_E)
    {
        if ( QApplication::keyboardModifiers () == Qt::ControlModifier)
        {
            QList<class SvgLabel*> Slabels = ui->image_destination->findChildren<class SvgLabel*>();
            if(!Slabels.empty())
            {
                for(auto &label : Slabels)
                    {
                        if(label->m_select)
                        {
                            label->m_rotate = label->m_rotate+1;

                            ui->rotation_label->setText(QString::number(label->m_rotate)+"°");
                            QRect rec(QPoint(0,0), QPoint(label->m_scaled_img.width(), label->m_scaled_img.height()));
                            QRectF minimumBoundingRect = GetMinimumBoundingRect( rec, qDegreesToRadians(label->m_rotate));

                            label->move(label->pos().x() - label->m_x_pos_offset, label->pos().y()-label->m_y_pos_offset);


                            label->m_x_pos_offset = minimumBoundingRect.x();
                            label->m_y_pos_offset = minimumBoundingRect.y();

                            label->m_select->setScaledContents(false);


                            QPixmap img(label->m_img);
                            QTransform tm;
                            tm.rotate(label->m_rotate);
                            img = img.transformed(tm,Qt::TransformationMode::SmoothTransformation);

                            label->m_select->setPixmap(img);
                            label->m_select->show();
                            label->m_select->update();
                            label->setGeometry(label->m_OG_position_x+label->m_x_pos_offset ,label->m_OG_position_y+label->m_y_pos_offset , minimumBoundingRect.width(), minimumBoundingRect.height());

                            label->m_OG_position_x = label->pos().x()-label->m_x_pos_offset;
                            label->m_OG_position_y = label->pos().y()-label->m_y_pos_offset;


                            if(label->m_rotate >= 360)
                                label->m_rotate = 0;
                            ui->rotate_dial->setValue(label->m_rotate);
                        }
                    }
                }
        }
    }

    if (event->key() == Qt::Key_Q)
    {
        if ( QApplication::keyboardModifiers () == Qt::ControlModifier)
        {
            QList<class SvgLabel*> Slabels = ui->image_destination->findChildren<class SvgLabel*>();
            if(!Slabels.empty())
            {
                for(auto &label : Slabels)
                    {
                        if(label->m_select)
                        {
                            label->m_rotate = label->m_rotate-1;
                            if(label->m_rotate < 0)
                                label->m_rotate = label->m_rotate + 360;

                            ui->rotation_label->setText(QString::number(label->m_rotate)+"°");
                            QRect rec(QPoint(0,0), QPoint(label->m_scaled_img.width(), label->m_scaled_img.height()));
                            QRectF minimumBoundingRect = GetMinimumBoundingRect( rec, qDegreesToRadians(label->m_rotate));

                            label->move(label->pos().x() - label->m_x_pos_offset, label->pos().y()-label->m_y_pos_offset);


                            label->m_x_pos_offset = minimumBoundingRect.x();
                            label->m_y_pos_offset = minimumBoundingRect.y();

                            label->m_select->setScaledContents(false);


                            QPixmap img(label->m_img);
                            QTransform tm;
                            tm.rotate(label->m_rotate);
                            img = img.transformed(tm,Qt::TransformationMode::SmoothTransformation);

                            label->m_select->setPixmap(img);
                            label->m_select->show();
                            label->m_select->update();
                            label->setGeometry(label->m_OG_position_x+label->m_x_pos_offset ,label->m_OG_position_y+label->m_y_pos_offset , minimumBoundingRect.width(), minimumBoundingRect.height());

                            label->m_OG_position_x = label->pos().x()-label->m_x_pos_offset;
                            label->m_OG_position_y = label->pos().y()-label->m_y_pos_offset;


                            if(label->m_rotate >= 360)
                                label->m_rotate = 0;
                            ui->rotate_dial->setValue(label->m_rotate);
                        }
                    }
                }
        }
    }

    if(event->key() == Qt::Key_Tab)
    {
        QList<class SvgLabel*> Slabels = ui->image_destination->findChildren<class SvgLabel*>();
        if(!Slabels.empty())
        {
            for(auto &label : Slabels)
                {
                    label->m_select = nullptr;
                    label->setFrameStyle(00);
                }

            if(m_label_tab_index >= Slabels.size())
                m_label_tab_index = 0;

            Slabels.at(m_label_tab_index)->m_select = Slabels.at(m_label_tab_index);
            Slabels.at(m_label_tab_index)->setFrameStyle(1);
            ++m_label_tab_index;
        }
    }

    if(event->key() == Qt::Key_W)
    {
        QList<class SvgLabel*> Slabels = ui->image_destination->findChildren<class SvgLabel*>();
        if(!Slabels.empty())
        {
            for(auto &label : Slabels)
                {
                    if(label->m_select)
                    {
                        label->move(label->m_OG_position_x+label->m_x_pos_offset, label->m_OG_position_y+label->m_y_pos_offset-1);
                        --label->m_OG_position_y;
                        SvgLabelInfo(*label);
                    }
                }
        }
    }

    if(event->key() == Qt::Key_S)
    {
        QList<class SvgLabel*> Slabels = ui->image_destination->findChildren<class SvgLabel*>();
        if(!Slabels.empty())
        {
            for(auto &label : Slabels)
                {
                    if(label->m_select)
                    {
                        label->move(label->m_OG_position_x+label->m_x_pos_offset, label->m_OG_position_y+label->m_y_pos_offset+1);
                        ++label->m_OG_position_y;
                        SvgLabelInfo(*label);
                    }
                }
        }
    }

    if(event->key() == Qt::Key_A)
    {
        QList<class SvgLabel*> Slabels = ui->image_destination->findChildren<class SvgLabel*>();
        if(!Slabels.empty())
        {
            for(auto &label : Slabels)
                {
                    if(label->m_select)
                    {
                        label->move(label->m_OG_position_x+label->m_x_pos_offset-1, label->m_OG_position_y+label->m_y_pos_offset);
                        --label->m_OG_position_x;
                        SvgLabelInfo(*label);
                    }
                }
        }
    }

    if(event->key() == Qt::Key_D)
    {
        QList<class SvgLabel*> Slabels = ui->image_destination->findChildren<class SvgLabel*>();
        if(!Slabels.empty())
        {
            for(auto &label : Slabels)
                {
                    if(label->m_select)
                    {
                        label->move(label->m_OG_position_x+label->m_x_pos_offset+1, label->m_OG_position_y+label->m_y_pos_offset);
                        ++label->m_OG_position_x;
                        SvgLabelInfo(*label);
                    }
                }
        }
    }

}

QPointF LaserCutter::GetRotatedPoint(const QPointF &p, const QPointF &center, const qreal &angleRads)
{
    qreal x = p.x();
    qreal y = p.y();

    float s = qSin( angleRads );
    float c = qCos( angleRads );

    x -= center.x();
    y -= center.y();

    float xnew = x * c - y * s;
    float ynew = x * s + y * c;

    x = xnew + center.x();
    y = ynew + center.y();

    return QPointF( x, y );
}

QRectF LaserCutter::GetMinimumBoundingRect(const QRect &r, const qreal &angleRads)
{
    QPointF topLeft     = GetRotatedPoint( r.topLeft(),     r.center(), angleRads );
    QPointF bottomRight = GetRotatedPoint( r.bottomRight(), r.center(), angleRads );
    QPointF topRight    = GetRotatedPoint( r.topRight(),    r.center(), angleRads );
    QPointF bottomLeft  = GetRotatedPoint( r.bottomLeft(),  r.center(), angleRads );

    std::vector<double> tempX= {topLeft.x(), bottomRight.x(), topRight.x(), bottomLeft.x()};
    std::vector<double> tempY= {topLeft.y(), bottomRight.y(), topRight.y(), bottomLeft.y()};


    qreal minX = *std::min_element(tempX.begin(), tempX.end());
    qreal minY = *std::min_element(tempY.begin(), tempY.end());

    qreal maxX = *std::max_element(tempX.begin(), tempX.end());
    qreal maxY = *std::max_element(tempY.begin(), tempY.end());

    return QRectF( QPointF( minX, minY ), QPointF( maxX, maxY ) );
}

void LaserCutter::Transform_to_normalized(double *aTransform_ratio, QLabel &label)
{
    QPixmap img(label.pixmap(Qt::ReturnByValue));
    QTransform tm;
    tm.scale((((img.width()/3.7795)/aTransform_ratio[0])/img.width()),(((img.height()/3.7795)/aTransform_ratio[1])/img.height()));
    img = img.transformed(tm,Qt::TransformationMode::SmoothTransformation);

    resize((ceil(img.width())/aTransform_ratio[0]), ceil((img.height())/aTransform_ratio[1]));
    label.setPixmap(img);
}

void LaserCutter::SetImageSource(QString aPath)
{
    if(m_img_source_placement.at(1) == 12)
    {
        QMessageBox::warning(this, tr("Setup Error"), "Maximum number of source images exceeded");
        return;
    }

    QLabel *icon = new QLabel(ui->image_source);
    icon->setPixmap(QPixmap(aPath));
    Transform_to_normalized(m_transfer_ratio, *(icon));

    if(m_img_source_placement.at(0) == 5)
    {
        m_img_source_placement.at(0) = 0;
        m_img_source_placement.at(1)++;
    }

    icon->move( (10*(m_img_source_placement.at(0)+1)+60*m_img_source_placement.at(0)) , (10*(m_img_source_placement.at(1)+1)+60*m_img_source_placement.at(1)));
    m_img_source_placement.at(0)++;

    icon->resize(50,50);
    icon->setScaledContents(true);
    icon->setAttribute(Qt::WA_DeleteOnClose);
    icon->setObjectName(aPath);                     //setting the path to the image as an object name, later on used to extract the pixmap
    icon->show();

    m_image_source_counter++;
}


void LaserCutter::DisplayCameraError()
{
  QMessageBox::warning(this, tr("Camera Error"), m_camera->errorString());
}

void LaserCutter::UpdateCameraDevice(QAction *action)
{
    m_CamInfo.camera_selected =  QVariant(action->objectName()).toULongLong();
    SetCamera(qvariant_cast<QCameraInfo>(action->data()));
}

void LaserCutter::closeEvent(QCloseEvent *event)
{
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setText("Are you sure you want to exit?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);

    int ret = msgBox.exec();

    switch (ret) {

        case QMessageBox::Yes:

        if(!FileHandler::SetConfig(m_Path_to_source, ui->doubleSpinBox_width->value(), ui->doubleSpinBox_height->text().toDouble()))
        {
            QMessageBox::critical(this, tr("Config file Error"), "Couldn't set the path to the image source folder");
            event->ignore();
        }
        else
           event->accept();

            break;
        case QMessageBox::No:

            event->ignore();
            break;

        default:
            event->ignore();
    }
}

void LaserCutter::UpdateCamerasMenu()
{
    ui->menuDevices->clear();
    const QList<QCameraInfo> availableCameras = QCameraInfo::availableCameras();                        //list from available cameras
    for(int i =0; i < availableCameras.length(); ++i){
        QAction *videoDeviceAction = new QAction(availableCameras.at(i).description(), m_videoDevicesGroup);
        videoDeviceAction->setCheckable(true);
        videoDeviceAction->setData(QVariant::fromValue(availableCameras.at(i)));                                    //QVariant ~ Union
        if (availableCameras.at(i) == QCameraInfo::defaultCamera())
                    videoDeviceAction->setChecked(true);
        videoDeviceAction->setObjectName(QVariant(i).toString());
        ui->menuDevices->addAction(videoDeviceAction);
    }
}

void LaserCutter::Initialize()
{
    if(ui->start_btn->text() == "Start")
    {
        InitCutterInfo();
        m_camera->stop();
        ui->viewfinder->hide();
        ui->camera_frame->show();

        if(m_CamInfo.camera_selected < 0)
        {
            QMessageBox::critical(this, tr("Initialization Error"), "No camera has been selected");
            return;
        }

        //handing over the camera handler to inputVideo//OpenCV
        inputVideo.open(m_CamInfo.camera_selected);
        inputVideo.set(cv::CAP_PROP_FRAME_WIDTH,m_CamInfo.resolutions.last().width());
        inputVideo.set(cv::CAP_PROP_FRAME_HEIGHT,m_CamInfo.resolutions.last().height());

        if(CameraMarkerDetection())
        {
            if(!m_tm){
                m_tm = new QElapsedTimer();
            }
            if(!m_timer){
                m_timer = new QTimer(this);
            }

            if(ui->doubleSpinBox_width->value()  == double(0))
                 ui->doubleSpinBox_width->setValue(m_cutting_area.at(0));
            if(ui->doubleSpinBox_height->value()  == double(0))
                ui->doubleSpinBox_height->setValue(m_cutting_area.at(1));

            if(PerspectiveTransform())      //if markers get detected
            {
                connect(m_timer, SIGNAL(timeout()), this, SLOT(PerspectiveTransform()));
                m_timer->start(150);
                SetupUiFrameSize();
                GetTransferRatio();
                //m_camera->start();

                FileHandler::MakeTempDir("/Temp_img_source");
                FileHandler::MakeTempDir("/Temp_img");

                // insert if start has been pressed -> user opens files -> make reset, then open files
                m_image_source_counter=0;
                m_res = FileHandler::CopySvgsFromDir(m_image_source_counter, QDir(m_Path_to_source));
                m_image_source_counter=0;

                if(m_res.empty())
                    QMessageBox::information(this, tr("Source Folder"), "Source folder doesn't contain any SVG files");

                for(auto &resource : m_res)
                {
                    SetImageSource(FileHandler::MakeSvgCopy(resource, m_image_source_counter));
                }

                connect(ui->GO_btn,&QPushButton::released, this, &LaserCutter::SaveXML_copies);

                ui->start_btn->setText("Reset");
                ui->menuDevices->setDisabled(true);
                ui->doubleSpinBox_width->setEnabled(false);
                ui->doubleSpinBox_height->setEnabled(false);
            }
        }
    }
    else
    {
        ResetApp();
    }
}

void LaserCutter::ResetApp()
{
    disconnect(m_timer, SIGNAL(timeout()), this, SLOT(PerspectiveTransform()));
    disconnect(ui->GO_btn,&QPushButton::released, this, &LaserCutter::SaveXML_copies);
    m_tm->~QElapsedTimer();
    m_timer->~QTimer();

    m_tm = nullptr;
    m_timer = nullptr;

    m_image_source_counter = 0;
    FileHandler::ClearPathDir("/Temp_img");
    FileHandler::ClearPathDir("/Temp_img_source");
    m_img_source_placement.at(0) = 0;
    m_img_source_placement.at(1) = 0;
    ui->image_source->ClearWidget(0);
    ui->image_destination->ClearWidget(1);

    inputVideo.release();
    ui->camera_frame->hide();
    ui->viewfinder->show();
    m_camera->start();

    ui->doubleSpinBox_width->setEnabled(true);
    ui->doubleSpinBox_height->setEnabled(true);
    ui->start_btn->setText("Start");

    ui->menuDevices->setDisabled(false);
}

void LaserCutter::CuttingFinished(int exitCode, QProcess::ExitStatus exitStatus)
{

    switch (exitCode) {
        case -2:

            QMessageBox::critical(this, tr("Failed to start cutting"),  m_process_cut->errorString()+"Cannot start the process");
            break;
        case -1:

            QMessageBox::critical(this, tr("Failed to start cutting"),  m_process_cut->errorString()+"Process has crashed");
            break;
        case 0:

            QMessageBox::information(this, tr("Success!"), "Cutting has finished");
            break;
        case 1:

            QMessageBox::critical(this, tr("Failed to start cutting"),  m_process_cut->errorString());
            break;

         default:
            QMessageBox::critical(this, tr("Failed to start cutting"),  m_process_cut->errorString());
            break;
    }

    m_tracking_flag = true;
    return;
}

void LaserCutter::OpenFiles()
{
    if(ui->start_btn->text() == "Start")
        return;

    QFileDialog dialog;
    //"Choose the source folder for SVGs"
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOption(QFileDialog::ShowDirsOnly);
    QDir dir = dialog.getExistingDirectory();
    if(dir.path() == "." || !dir.exists())
        return;
    else
    {
        FileHandler::ClearPathDir("/Temp_img");
        FileHandler::ClearPathDir("/Temp_img_source");
        m_Path_to_source =dir.path();
        m_img_source_placement.at(0) = 0;
        m_img_source_placement.at(1) = 0;
        ui->image_source->ClearWidget(0);
        ui->image_destination->ClearWidget(1);

        m_image_source_counter=0;
        m_res = FileHandler::CopySvgsFromDir(m_image_source_counter, dir);
        m_image_source_counter=0;
        for(auto &resource : m_res)
        {
            SetImageSource(FileHandler::MakeSvgCopy(resource, m_image_source_counter));
        }
    }
}


void LaserCutter::InitCutterInfo()
{
    QFile cutter_dimensions(QCoreApplication::applicationDirPath().append("/config/cutter_config.xml"));
    if( !cutter_dimensions.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
      QMessageBox::critical(this, tr("Initialization error"), "Failed to open configuration file");
      return;
    }

    QDomDocument doc;
    doc.setContent(&cutter_dimensions);
    QDomNodeList elements = doc.elementsByTagName("CutterParameters");
    QDomElement el = elements.at(0).toElement();
    QDomNamedNodeMap map = el.attributes();

    if(ui->doubleSpinBox_width->value() == double(0))
        m_cutting_area.at(0) = (map.namedItem("cutting_area_width").nodeValue().toDouble());
    else
        m_cutting_area.at(0) = ui->doubleSpinBox_width->value();

    if(ui->doubleSpinBox_height->value() == double(0))
        m_cutting_area.at(1) = (map.namedItem("cutting_area_height").nodeValue().toDouble());
    else
        m_cutting_area.at(1) = ui->doubleSpinBox_height->value();


    elements = doc.elementsByTagName("Paths");
    el = elements.at(0).toElement();
    map = el.attributes();
    m_Path_to_source = (map.namedItem("imageSource").nodeValue());

    cutter_dimensions.close();
}

void LaserCutter::GetImagePositions()       //img pos in mm
{
    QList<class SvgLabel*> Slabels = ui->image_destination->findChildren<class SvgLabel*>();
    for(auto &label: Slabels)
    {
        label->m_image_position.x = static_cast<double>(label->m_OG_position_x)*m_transfer_ratio[0];
        label->m_image_position.y = static_cast<double>(label->m_OG_position_y)*m_transfer_ratio[1];
    }

    return;
}

bool LaserCutter::CameraMarkerDetection()
{
    cv::Ptr<cv::aruco::Dictionary> dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_250);
    QElapsedTimer tm;           //timer, detection
    tm.start();

    QProgressBar pb(this);
    pb.move(width()/2, height()-50);
    pb.setMaximum(5000);
    pb.setMinimum(0);
    pb.show();

    while (inputVideo.grab())
    {
        cv::Mat image, imageCopy;
        inputVideo.retrieve(image);
        image.copyTo(imageCopy);
        std::vector<int> ids;
        std::vector<std::vector<cv::Point2f>> corners;
        cv::aruco::detectMarkers(image, dictionary, corners, ids);

        pb.setValue(tm.elapsed());
        if(tm.elapsed() > 5000)                                                 //if !detect after 5s, return
        {
            QMessageBox::critical(this, tr("Setup Error, Marker detection"), "Couldn't detect Markers \nSelect a different camera or make sure all markers are visible");
            inputVideo.release();
            m_camera->start();
            ui->viewfinder->show();
            ui->camera_frame->hide();
            ui->doubleSpinBox_width->setEnabled(true);
            ui->doubleSpinBox_height->setEnabled(true);
            ui->start_btn->setText("Start");
            return false;
        }
        if(ids.size()>3)
        {
            size_t temp_index[4];
            for(size_t i =0; i< 4; ++i)                     //getting which corners are from what IDs
            {
                if(ids[i] == 0)
                    temp_index[0]=i;
                if(ids[i] == 1)
                    temp_index[1]=i;
                if(ids[i] == 2)
                    temp_index[2]=i;
                if(ids[i] == 3)
                    temp_index[3]=i;
            }
            m_corners[0] = corners[temp_index[0]][0];       //top left
            m_corners[1] = corners[temp_index[1]][0];       //top right
            m_corners[2] = corners[temp_index[2]][0];       //OG of the cutter (bot left)
            m_corners[3] = corners[temp_index[3]][0];       //bot right

            break;

        }
    }
    pb.setValue(pb.maximum());

    return true;
}

bool LaserCutter::PerspectiveTransform()
{
    std::vector<double> dimensions(2);
    cv::Ptr<cv::aruco::Dictionary> dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_250);
    cv::Mat perspective_matrix;
    cv::Mat image, imageCopy;

               //timer, detection
    m_tm->restart();
    while (inputVideo.grab())
    {
        inputVideo.retrieve(image);
        image.copyTo(imageCopy);
        std::vector<int> ids;
            cv::Point2f src_vertices[4];
            src_vertices[0] = m_corners[0]; // TopLeft
            src_vertices[1] = m_corners[1]; // Top Right
            src_vertices[2] = m_corners[2]; // bottomRight
            src_vertices[3] = m_corners[3]; // bottomleft

            dimensions.at(0) = Pythagoras(m_corners[1], m_corners[0]);          //getting the Euclidian distance
            dimensions.at(1) = Pythagoras(m_corners[2], m_corners[0]);

            cv::Point2f dst_vertices[4];
            dst_vertices[0] = cv::Point(0, 0);
            dst_vertices[1] = cv::Point(dimensions.at(0), 0);
            dst_vertices[2] = cv::Point(0, dimensions.at(1));
            dst_vertices[3] = cv::Point(dimensions.at(0), dimensions.at(1));

            perspective_matrix = cv::getPerspectiveTransform(src_vertices, dst_vertices);
            break;
     }



    cv::Size size(dimensions.at(0), dimensions.at(1));
    cv::Mat rotated, img_gray, thresh;
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;


    if(imageCopy.empty())
    {
        QMessageBox::critical(this, tr("Camera Error"), "Camera has been disconnected");
        ResetApp();
        return false;
    }

    warpPerspective(imageCopy, rotated, perspective_matrix, size, cv::INTER_NEAREST);
    cvtColor(rotated, img_gray, cv::COLOR_BGR2GRAY);
    threshold(img_gray, thresh, ui->horSlider_threshContours->value(), 255, cv::THRESH_BINARY_INV);

    findContours(thresh, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    for (auto it = contours.begin(); it != contours.end();)         //erasing the countours(area) smaller than a reference number
     {
         if(cv::contourArea(*it) < 10000)
             it = contours.erase(it);
         else
         {
             ++it;
         }
     }

    if(contours.size() < 2)                          //if only one object is present in the cutter's area - subject to change
    {
        for (auto &c : contours)
        {
            drawContours(rotated, contours, -1, cv::Scalar(0, 255, 0), 1);
            cv::Rect rec = cv::boundingRect(c);
            m_centroid.setX(rec.x+rec.width/2);
            m_centroid.setY(rec.y+rec.height/2);
            TrackMaterial(m_tracking_flag);
        }
    }

    m_dimensions.setWidth(dimensions.at(0));
    m_dimensions.setHeight(dimensions.at(1));

    ui->camera_frame->setPixmap(MatToPixmap(rotated));

    return true;
}

double LaserCutter::Pythagoras(const cv::Point2d &aUno, const cv::Point2d &aDuo)
{
    double x = qAbs(aDuo.x - aUno.x);
    double y = qAbs(aDuo.y - aUno.y);

    return sqrt(x*x+y*y);
}

QPixmap LaserCutter::MatToPixmap(cv::Mat src)
{
    QImage::Format format=QImage::Format_Grayscale8;
    int bpp=src.channels();
    if(bpp==3)
        format=QImage::Format_RGB888;
    QImage img(src.cols,src.rows,format);
    uchar *sptr,*dptr;
    int linesize=src.cols*bpp;
    for(int y=0;y<src.rows;y++)
    {
        sptr=src.ptr(y);
        dptr=img.scanLine(y);
        memcpy(dptr,sptr,linesize);
    }
    if(bpp==3)
        return QPixmap::fromImage(img.rgbSwapped());
    return QPixmap::fromImage(img);
}


cv::Mat LaserCutter::QImageToMat( const QImage &inImage, bool inCloneImageData = true )
   {
      switch ( inImage.format() )
      {
         case QImage::Format_ARGB32:
         case QImage::Format_ARGB32_Premultiplied:
         {
            cv::Mat  mat( inImage.height(), inImage.width(),
                          CV_8UC4,
                          const_cast<uchar*>(inImage.bits()),
                          static_cast<size_t>(inImage.bytesPerLine())
                          );

            return (inCloneImageData ? mat.clone() : mat);
         }

        case QImage::Format_RGB32:
         {
            cv::Mat  mat( inImage.height(), inImage.width(),
                          CV_8UC4,
                          const_cast<uchar*>(inImage.bits()),
                          static_cast<size_t>(inImage.bytesPerLine())
                          );

            cv::Mat  matNoAlpha;

            cv::cvtColor( mat, matNoAlpha, cv::COLOR_BGRA2BGR );

            return matNoAlpha;
         }

         case QImage::Format_RGB888:
         {
            QImage   swapped = inImage.rgbSwapped();

            return cv::Mat( swapped.height(), swapped.width(),
                            CV_8UC3,
                            const_cast<uchar*>(swapped.bits()),
                            static_cast<size_t>(swapped.bytesPerLine())
                            ).clone();
         }

         case QImage::Format_Indexed8:
         {
            cv::Mat  mat( inImage.height(), inImage.width(),
                          CV_8UC1,
                          const_cast<uchar*>(inImage.bits()),
                          static_cast<size_t>(inImage.bytesPerLine())
                          );

            return (inCloneImageData ? mat.clone() : mat);
         }

         default:
            break;
      }
      return cv::Mat();
}


void LaserCutter::ClearWorkspace()
{
    QList<class SvgLabel*> Slabels = ui->image_destination->findChildren<class SvgLabel*>();
    for(auto &label : Slabels)
        {
               label->~SvgLabel();
        }
}

bool LaserCutter::CheckSvgsPositions()
{
    bool ret = true;

    if(ui->image_destination->children().empty())
        return false;

    QImage image = ui->camera_frame->grab().toImage();
    cv::Mat camera_frame = QImageToMat(image);
    cv::Mat camera_frame_white;
    cv::Mat camera_frame_black;

    cvtColor(camera_frame,camera_frame, CV_RGB2GRAY);
    threshold(camera_frame, camera_frame_white, 0, 255, cv::THRESH_BINARY_INV+cv::THRESH_OTSU);  //black material on a white background ===>    white material
    threshold(camera_frame, camera_frame_black, 0, 255, cv::THRESH_BINARY+cv::THRESH_OTSU);      //black material on a white background ===>    black material

    QList<class SvgLabel*> Slabels = findChildren<class SvgLabel*>();
    for(auto &label : Slabels)
    {
        label->setFrameStyle(0);
        label->hide();
    }

    bool aux;
    for(int i=0; i<Slabels.size(); ++i)
    {
        Slabels.at(i)->show();

        ui->image_destination->setStyleSheet("background: white");
        QImage bl = ui->image_destination->grab().toImage();
        cv::Mat black_label = QImageToMat(bl);
        cvtColor(black_label,black_label, CV_RGB2GRAY);
        threshold(black_label, black_label, 0, 255, cv::THRESH_BINARY+cv::THRESH_OTSU);

        ui->image_destination->setStyleSheet("background: transparent");
        QImage wh =ui->image_destination->grab().toImage();
        cv::Mat white_label = QImageToMat(wh);

        cvtColor(white_label,white_label, CV_RGB2GRAY);
        threshold(white_label, white_label, 0, 255, cv::THRESH_BINARY_INV+cv::THRESH_OTSU);

        white_label = ~white_label;

        cv::Mat aux_white = camera_frame_white & white_label;
        cv::Mat aux_black = camera_frame_black & black_label;

        if(MatIsEqual(camera_frame_white, aux_white) || MatIsEqual(camera_frame_black, aux_black))
            aux = true;
        else
            aux = false;

        if(!aux)
        {
        Slabels.at(i)->setStyleSheet(QStringLiteral("QLabel{color: rgb(255, 0, 0);}"));
        Slabels.at(i)->setFrameStyle(1);
        Slabels.at(i)->setLineWidth(2);
        }
        else
        {
            Slabels.at(i)->setStyleSheet(QStringLiteral("QLabel{color: rgb(0, 0, 0);}"));
            Slabels.at(i)->setFrameStyle(0);
            Slabels.at(i)->setLineWidth(1);
        }

        if(!aux)
            ret = false;


        Slabels.at(i)->hide();
    }

    for(auto &label : Slabels)
    {
        label->show();
    }

    if(!ret)
    {
        QMessageBox::critical(this, tr("Image positions"), "One or more images are not within the bounds of the material !");
    }

   return ret;
}

void LaserCutter::TrackMaterial(bool tracking_enabled)
{
    if(tracking_enabled)
    {
        if(m_old_centroid == m_centroid)
            return;

        if(m_centroid.x() == m_old_centroid.x()+1 ||  m_centroid.y() == m_old_centroid.y()+1)
            return;

        QPoint diff = m_centroid-m_old_centroid;
        QList<class SvgLabel*> Slabels = findChildren<class SvgLabel*>();

        for(auto &label : Slabels)
        {
            if(label->pos().x()+diff.x() > ui->image_destination->width() || label->pos().x()+diff.x() < 0)   //making sure the images are not moving out of bounds
                continue;
            if(label->pos().y()+diff.y() > ui->image_destination->height() ||label->pos().y()+diff.y() < 0)
                continue;

            label->m_OG_position_x = label->m_OG_position_x + diff.x();
            label->m_OG_position_y = label->m_OG_position_y + diff.y();

            label->move(label->pos().x()+diff.x(), label->pos().y()+diff.y());
            if(label->m_select)
                SvgLabelInfo(*label);
        }

        m_old_centroid = m_centroid;
    }
}

bool LaserCutter::MatIsEqual(const cv::Mat &mat1, const cv::Mat &mat2)
{
   if (mat1.empty() && mat2.empty()) {
       return true;
   }
   if (mat1.cols != mat2.cols || mat1.rows != mat2.rows || mat1.dims != mat2.dims) {
       return false;
   }
   cv::Mat diff;
   cv::compare(mat1, mat2, diff, cv::CMP_NE);
   int nz = cv::countNonZero(diff);
   return nz==0;
}

void LaserCutter::GetTransferRatio()
{
    m_transfer_ratio[0] = double(m_cutting_area.at(0))/double(m_dimensions.width()); //px ~~ mm
    m_transfer_ratio[1] = double(m_cutting_area.at(1))/double(m_dimensions.height());
    return;
}


void LaserCutter::ShowPortInfo(int idx)
{
    if (idx == -1)
        return;

    const QStringList list = ui->serialport->itemData(idx).toStringList();
    final_port = list.at(0);
}

void LaserCutter::FillPortsInfo()
{
    ui->serialport->clear();
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        QStringList list;
        list << info.portName();

        ui->serialport->addItem(list.first(), list);
    }
}

void LaserCutter::HotkeysDialog()
{
    m_hotkeys = new Hotkeys(this);
    m_hotkeys->show();
}

void LaserCutter::ShowManual()
{
    if(!QDesktopServices::openUrl(QUrl::fromLocalFile(QCoreApplication::applicationDirPath().append("/User_Manual.pdf"))))
        QMessageBox::critical(this, tr("File Error"), "Cannot open the manual file");
}


