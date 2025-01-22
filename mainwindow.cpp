#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include <QFont>
#include <QDebug>
#include <QVector>

enum {
    PAINTER_DRAW,
    PAINTERPATH_DRAW,
    PAINTERPATH_CALC_DRAW
};

#define POS_X 100
QPointF s_posY[] = {{POS_X,50}, {POS_X, 100}, {POS_X,150}};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

// 直接通过QPainter的drawText进行绘制
void MainWindow::DrawTextByPainter(QPainter &painter, const QFont &font, const QString &str)
{
    painter.setFont(font);
    painter.drawText(s_posY[PAINTER_DRAW], str);
}

// 通过QPainterPath获取字体路径（轮廓线路径：直线、曲线），并可以与其他绘制坐标进行适配
void MainWindow::DrawTextByPainterPath(QPainter &painter, const QFont &font, const QString &str)
{
    QPainterPath path;
    path.addText(s_posY[PAINTERPATH_DRAW], font, str);
    TranslatePath(path); // 输出字体绘制的相对路径。如果addText时QPointF(0,0),则等同于绝对路径
    painter.drawPath(path);
}

// 通过QPainterPath获取字体路径（轮廓线路径），并将路径转为多段线路径，可以与其它绘制坐标进行适配
void MainWindow::DrawTextByPainterPathCalc(QPainter &painter, QFont &font, const QString &str)
{
    QPainterPath path;
    QPainterPath txtPath;
    txtPath.addText(s_posY[PAINTERPATH_CALC_DRAW], font, str);

    QVector<QPointF> points;
    QVector<quint32> npoints;
    // 获取path的多段线点集合
    AnalysisPath(txtPath, points, npoints);

#if 1
    { // 绘制字体，包括轮廓线，且填充内部空间颜色
        QPolygonF poly;
        int counts = 0;

        // 此处必须使用QPainterPath添加多边形，再通过QPainter进行绘制.DrawPath绘制时如果时封闭图形则自动填充
        // 不可以直接使用QPainter::drawPolygon直接绘制，因为部分字体可能由多个多边形组成，会导致填充区域发生覆盖
        for (int i = 0; i < npoints.size(); ++i) {
            poly = points.mid(counts, npoints[i]);
            counts += npoints[i];
            path.addPolygon(poly);
        }
        painter.drawPath(path);
    }
#else
    { // 绘制轮廓线，即不需要填充颜色
        int counts = 0;
        for (int i = 0; i < npoints.size(); ++i) {
            painter.drawPolyline(points.data()+counts, npoints[i]);
            counts += npoints[i];
        }
    }
#endif
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    if (m_clicked) {
        // 设置字体样式及大小
        QFont font(m_font);

        QPainter painter(this);
        painter.setPen(QPen(QColor(255,0,0))); // 轮廓线颜色
        painter.setBrush(QColor(122,163,39)); // 如果目标是封闭矩形，则会自动填充

        DrawTextByPainter(painter, font, m_str);
        DrawTextByPainterPath(painter, font, m_str);
        DrawTextByPainterPathCalc(painter, font, m_str);
        m_clicked = false;
    }
}

// 该接口会输出path的绘制路径，包含点、直线、曲线等相关信息
// 如果没有特殊要求，可直接通过该路径（moveTo, lineTo, cubicTo）进行字体绘制
void MainWindow::TranslatePath(const QPainterPath &path)
{
    QPainterPath::Element element;

    for (int i = 0; i < path.elementCount(); ++i) {
        element = path.elementAt(i);

        switch (element.type) {
        case QPainterPath::MoveToElement: // 每段线（直线或曲线）或点的起点
            qDebug()<<"Move to:"<<QPointF(element.x, element.y);
            break;
        case QPainterPath::LineToElement: // 一段连续直线上的点（起点除外）
            qDebug()<<"line to:"<<QPointF(element.x, element.y);
            break;
        default: // 其余均是描述曲线的数据，三次贝塞尔曲线描述点，c1,c2,endPoint
            qDebug()<<"cubic to:"<<QPointF(element.x, element.y);
            break;
        }
    }
}

/*******************************************************
 * 功能说明：将原始路径转为多段线路径
 * path: 原始绘制路径，直线、曲线
 * points：转译成全直线路径的点数集合
 * npoints：每一段连续路径的“点数个数”集合
********************************************************/
void MainWindow::AnalysisPath(const QPainterPath &path, QVector<QPointF> &points, QVector<quint32> &npoints)
{
    const int splitCounts = 20; // 贝塞尔曲线分割数,该值可以修改，值越大精度越准，效率越慢
    QPainterPath::Element element;

    int counts = 0;
    QPointF lastPoints;
    for (int i = 0; i < path.elementCount(); ++i) {
        element = path.elementAt(i);

        switch (element.type) {
        case QPainterPath::MoveToElement:{ // 每段线（直线或曲线）或点的起点
            if (counts) {
                npoints.push_back(counts);
                counts = 0;
            }
            lastPoints = QPointF(element.x, element.y);
            points.push_back(lastPoints);
            counts++;
            break;
        }
        case QPainterPath::LineToElement:{ // 一段连续直线上的点（起点除外）
            lastPoints = QPointF(element.x, element.y);
            points.push_back(lastPoints);
            counts++;
            break;
        }
        default:{ // 其余均是描述曲线的数据，三次贝塞尔曲线描述点，c1,c2,endPoint
            if (i + 2 >= path.elementCount()) {
                // 异常，需要特殊处理
                return;
            }

            QPainterPath::Element c1(path.elementAt(i));
            QPainterPath::Element c2(path.elementAt(i+1));
            QPainterPath::Element end(path.elementAt(i+2));
            for (int j = 0; j < splitCounts; ++j) {
                qreal t = j / qreal(splitCounts);
                qreal invt = 1 - t;

                // 点p的获取需要了解清楚贝塞尔曲线绘制方程
                QPointF p = invt*invt*invt*lastPoints + 3*t*invt*invt*QPointF(c1.x, c1.y) +
                            3*t*t*invt*QPointF(c2.x,c2.y) + t*t*t*QPointF(end.x, end.y);
                points.push_back(p);
                counts++;
            }
            lastPoints = QPointF(end.x, end.y);
            points.push_back(lastPoints);
            counts++;
            i = i + 2;
            break;
        }
        }
    }
    npoints.push_back(counts);
}

void MainWindow::on_pushButton_clicked()
{
    static int a = 0;
    m_clicked = true;
    qDebug()<<a++;
    m_font = ui->fontComboBox->currentFont();
    m_font.setPointSize(ui->spinBox->value());
    m_font.setBold(ui->checkBox_2->isChecked());
    m_font.setItalic(ui->checkBox->isChecked());
    m_str = ui->lineEdit->text();

    this->update();
}
