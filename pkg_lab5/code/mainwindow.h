#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QFileDialog>
#include <optional>
#include <vector>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGraphicsTextItem>
#include <QRect>
#include <QGraphicsPolygonItem>

// Структура линии
struct Line {
    int x1, y1, x2, y2;
};

// Структура отсечённой линии
struct ClippedLine {
    double xstart, ystart, xend, yend;
};

// Структура вершины
struct Point {
    double x, y;
};

// Структура многоугольника
struct Polygon {
    std::vector<Point> vertices;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    QGraphicsView *view;
    QGraphicsScene *scene;
    std::vector<Line> lines;
    QRect clippingWindow;
    Polygon polygon;  // Исходный многоугольник

    std::vector<Point> sutherlandHodgmanClip(const Polygon &poly, const QRect &clipper);
    void applySutherlandHodgman();
    void loadFile();
    void drawAxesWithTicks();
    void drawScene();
    void applyLiangBarsky();
    std::optional<ClippedLine> liangBarskyClip(double xmin, double ymin, double xmax, double ymax,
                                               double xa, double ya, double xb, double yb);

private slots:
    void onLoadFile();
    void onApplyClipping();
    void onApplyPolygonClipping();
};

#endif // MAINWINDOW_H
