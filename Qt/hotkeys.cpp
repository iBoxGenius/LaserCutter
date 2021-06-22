#include "hotkeys.h"
#include "ui_hotkeys.h"


Hotkeys::Hotkeys(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Hotkeys), m_table(nullptr)
{
    ui->setupUi(this);

    setWindowTitle("Hotkeys");

    m_table = new QTableWidget(10,2,this);
    m_table->setGeometry(10,10,579,325);
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    m_table->setColumnWidth(0,140);
    m_table->setColumnWidth(1,415);

    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    setGeometry(0,0,m_table->width()+20, m_table->height()+20);

    QStringList labels = {"Hotkey","Description"};
    m_table->setHorizontalHeaderLabels(labels);
    //tab; delete; ctrl+a; ctrl+e ; ctrl+q; w; a; s; d;

    m_table->setItem(0,0,new QTableWidgetItem(" Delete"));
    m_table->setItem(0,1, new QTableWidgetItem(" Delete the selected images"));

    m_table->setItem(1,0,new QTableWidgetItem(" Tab"));
    m_table->setItem(1,1, new QTableWidgetItem(" Select the next image"));

    m_table->setItem(2,0,new QTableWidgetItem(" Ctrl+a"));
    m_table->setItem(2,1, new QTableWidgetItem(" Select all images"));

    m_table->setItem(3,0,new QTableWidgetItem(" Ctrl+e"));
    m_table->setItem(3,1, new QTableWidgetItem("  Rotate clockwise (+1°)"));

    m_table->setItem(4,0,new QTableWidgetItem(" Ctrl+q"));
    m_table->setItem(4,1, new QTableWidgetItem("  Rotate counter-clockwise (-1°)"));

    m_table->setItem(5,0,new QTableWidgetItem(" w"));
    m_table->setItem(5,1, new QTableWidgetItem(" Move upwards by one pixel"));

    m_table->setItem(6,0,new QTableWidgetItem(" a"));
    m_table->setItem(6,1, new QTableWidgetItem(" Move downards by one pixel"));

    m_table->setItem(7,0,new QTableWidgetItem(" s"));
    m_table->setItem(7,1, new QTableWidgetItem(" Move to the left by one pixel"));

    m_table->setItem(8,0,new QTableWidgetItem(" d"));
    m_table->setItem(8,1, new QTableWidgetItem(" Move to the right by one pixel"));

}

Hotkeys::~Hotkeys()
{
    delete ui;
    if(m_table)
        delete m_table;
}
