#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    // 代码中非qt相关接口，函数统一使用大驼峰命名规范
    void DrawTextByPainter(QPainter &painter, const QFont &font, const QString &str);
    void DrawTextByPainterPath(QPainter &painter, const QFont &font, const QString &str);
    void DrawTextByPainterPathCalc(QPainter &painter, QFont &font, const QString &str);

protected:
    void paintEvent(QPaintEvent *event);

private:
    void TranslatePath(const QPainterPath &path);
    void AnalysisPath(const QPainterPath &path, QVector<QPointF> &points, QVector<quint32> &npoints);

private slots:
    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;
    bool m_clicked = false;
    QFont m_font;
    QString m_str;
};
#endif // MAINWINDOW_H
