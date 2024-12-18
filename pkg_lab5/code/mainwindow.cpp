#include "MainWindow.h"
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTextStream>
#include <cmath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), view(new QGraphicsView(this)), scene(new QGraphicsScene(this)) {
    setWindowTitle("Line Clipping with Liang-Barsky Algorithm");

    // Установка начального размера окна
    resize(800, 600);
    setMinimumSize(400, 400);

    // Создание интерфейса
    auto *layout = new QVBoxLayout();
    auto *loadButton = new QPushButton("Load File", this);
    auto *clipButton = new QPushButton("Apply Line Clipping", this);
    auto *clipPolygonButton = new QPushButton("Apply Polygon Clipping", this);  // Новая кнопка

    layout->addWidget(view);
    layout->addWidget(loadButton);
    layout->addWidget(clipButton);
    layout->addWidget(clipPolygonButton);

    auto *centralWidget = new QWidget(this);
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);

    // Настройка сцены
    view->setScene(scene);
    view->setRenderHint(QPainter::Antialiasing);

    connect(loadButton, &QPushButton::clicked, this, &MainWindow::onLoadFile);
    connect(clipButton, &QPushButton::clicked, this, &MainWindow::onApplyClipping);
    connect(clipPolygonButton, &QPushButton::clicked, this, &MainWindow::onApplyPolygonClipping);
}

MainWindow::~MainWindow() = default;

void MainWindow::loadFile() {
    // Диалог для выбора файла
    QString fileName = QFileDialog::getOpenFileName(this, "Open File", "", "Text Files (*.txt)");
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&file);

    int n;  // Число отрезков
    in >> n;

    lines.clear();  // Очистка предыдущих линий
    for (int i = 0; i < n; ++i) {
        Line line;
        in >> line.x1 >> line.y1 >> line.x2 >> line.y2;
        lines.push_back(line);
    }

    // Чтение координат окна отсечения
    int xmin, ymin, xmax, ymax;
    in >> xmin >> ymin >> xmax >> ymax;
    clippingWindow = QRect(QPoint(xmin, ymin),QPoint(xmax, ymax));

    // Чтение многоугольника
    int polyVertexCount;
    in >> polyVertexCount;  // Количество вершин многоугольника
    polygon.vertices.clear();  // Очистка предыдущего многоугольника
    for (int i = 0; i < polyVertexCount; ++i) {
        Point vertex;
        in >> vertex.x >> vertex.y;
        polygon.vertices.push_back(vertex);
    }

    file.close();  // Закрываем файл

    // Отображаем данные
    drawScene();
}

void MainWindow::drawAxesWithTicks() {
    const int axisLength = 500;  // Длина осей
    const int tickLength = 5;   // Длина делений
    const int step = 50;        // Шаг между делениями

    QPen axisPen(Qt::gray, 1);

    // Рисуем оси
    scene->addLine(-axisLength, 0, axisLength, 0, axisPen);  // Ось X
    scene->addLine(0, -axisLength, 0, axisLength, axisPen);  // Ось Y

    QFont font("Arial", 8);

    // Деления и подписи на оси X
    for (int x = -axisLength; x <= axisLength; x += step) {
        if (x == 0) continue;  // Пропустить центр (нет подписи на нуле)
        scene->addLine(x, -tickLength, x, tickLength, axisPen);  // Деление
        auto *label = new QGraphicsTextItem(QString::number(x));
        label->setFont(font);
        label->setDefaultTextColor(Qt::black);
        label->setPos(x - 10, tickLength + 2);  // Смещение текста
        scene->addItem(label);
    }

    // Деления и подписи на оси Y
    for (int y = -axisLength; y <= axisLength; y += step) {
        if (y == 0) continue;  // Пропустить центр
        scene->addLine(-tickLength, y, tickLength, y, axisPen);  // Деление
        auto *label = new QGraphicsTextItem(QString::number(-y));  // Инвертируем Y
        label->setFont(font);
        label->setDefaultTextColor(Qt::black);
        label->setPos(tickLength + 2, y - 10);  // Смещение текста
        scene->addItem(label);
    }

    // Подписи для осей
    auto *xLabel = new QGraphicsTextItem("X");
    xLabel->setFont(font);
    xLabel->setDefaultTextColor(Qt::black);
    xLabel->setPos(axisLength - 20, 10);  // Смещение текста
    scene->addItem(xLabel);

    auto *yLabel = new QGraphicsTextItem("Y");
    yLabel->setFont(font);
    yLabel->setDefaultTextColor(Qt::black);
    yLabel->setPos(10, -axisLength + 10);  // Смещение текста
    scene->addItem(yLabel);
}

void MainWindow::drawScene() {
    scene->clear();
    drawAxesWithTicks();  // Отрисовка осей с делениями

    // Отрисовка окна отсечения
    scene->addRect(clippingWindow, QPen(Qt::red));

    // Отрисовка исходных линий
    for (const auto &line : lines) {
        scene->addLine(line.x1, -line.y1, line.x2, -line.y2, QPen(QColor(0, 255, 0), 1, Qt::DashLine));
    }

    // Отрисовка исходного многоугольника
    QPolygonF originalPolygon;
    for (const auto &vertex : polygon.vertices) {
        originalPolygon << QPointF(vertex.x, -vertex.y);
    }
    scene->addPolygon(originalPolygon, QPen(QColor(254, 254, 34), 1, Qt::DashLine));
}

void MainWindow::applySutherlandHodgman() {
    scene->clear();
    drawAxesWithTicks();  // Отрисовка осей с делениями

    // Отрисовка окна отсечения
    scene->addRect(clippingWindow, QPen(Qt::red));

    // Отрисовка исходного многоугольника
    QPolygonF originalPolygon;
    for (const auto &vertex : polygon.vertices) {
        originalPolygon << QPointF(vertex.x, -vertex.y);
    }
    scene->addPolygon(originalPolygon, QPen(Qt::blue, 1, Qt::DashLine));

    // Применение алгоритма
    auto clippedVertices = sutherlandHodgmanClip(polygon, clippingWindow);

    // Отрисовка отсечённого многоугольника
    if (!clippedVertices.empty()) {
        QPolygonF clippedPolygon;
        for (const auto &vertex : clippedVertices) {
            clippedPolygon << QPointF(vertex.x, -vertex.y);
        }
        scene->addPolygon(clippedPolygon, QPen(Qt::green));
    }
}

std::vector<Point> MainWindow::sutherlandHodgmanClip(const Polygon &poly, const QRect &clipper) {
    std::vector<Point> output = poly.vertices;

    auto isInside = [](const Point &p, const QRect &clipper, int edge) {
        switch (edge) {
        case 0: return p.x >= clipper.left();   // Левая граница
        case 1: return p.x <= clipper.right();  // Правая граница
        case 2: return p.y <= clipper.bottom(); // Нижняя граница
        case 3: return p.y >= clipper.top();    // Верхняя граница
        default: return false;
        }
    };

    auto computeIntersection = [](const Point &p1, const Point &p2, const QRect &clipper, int edge) -> Point {
        switch (edge) {
        case 0: return Point{clipper.left(), p1.y + (clipper.left() - p1.x) * (p2.y - p1.y) / (p2.x - p1.x)};
        case 1: return Point{clipper.right(), p1.y + (clipper.right() - p1.x) * (p2.y - p1.y) / (p2.x - p1.x)};
        case 2: return Point{p1.x + (clipper.bottom() - p1.y) * (p2.x - p1.x) / (p2.y - p1.y), clipper.bottom()};
        case 3: return Point{p1.x + (clipper.top() - p1.y) * (p2.x - p1.x) / (p2.y - p1.y), clipper.top()};
        }
        return Point{0, 0};
    };

    std::vector<Point> temp;
    for (int edge = 0; edge < 4; ++edge) {
        temp.clear();
        for (size_t i = 0; i < output.size(); ++i) {
            Point current = output[i];
            Point prev = output[(i == 0) ? output.size() - 1 : i - 1];

            if (isInside(current, clipper, edge)) {
                if (!isInside(prev, clipper, edge)) {
                    temp.push_back(computeIntersection(prev, current, clipper, edge));
                }
                temp.push_back(current);
            } else if (isInside(prev, clipper, edge)) {
                temp.push_back(computeIntersection(prev, current, clipper, edge));
            }
        }
        output = temp;
    }

    return output;
}

void MainWindow::onApplyPolygonClipping() {
    applySutherlandHodgman();  // Вызов метода отсечения многоугольника
}

std::optional<ClippedLine> MainWindow::liangBarskyClip(double xmin, double ymin, double xmax, double ymax,
                                                       double xa, double ya, double xb, double yb) {
    double t_start = 0, t_end = 1;
    double p[4] = {xa - xb, xb - xa, ya - yb, yb - ya};
    double q[4] = {xa - xmin, xmax - xa, ya - ymin, ymax - ya};

    for (int i = 0; i < 4; ++i) {
        if (p[i] == 0) {  // Линия параллельна границе
            if (q[i] < 0)
                return std::nullopt;  // Полностью за границей
        } else {
            double t = q[i] / p[i];
            if (p[i] < 0)
                t_start = std::max(t_start, t);  // Входная точка
            else
                t_end = std::min(t_end, t);  // Выходная точка

            if (t_start > t_end)
                return std::nullopt;  // Полностью за границей
        }
    }

    double xstart = xa + t_start * (xb - xa);
    double ystart = ya + t_start * (yb - ya);
    double xend = xa + t_end * (xb - xa);
    double yend = ya + t_end * (yb - ya);

    return ClippedLine{xstart, ystart, xend, yend};
}

void MainWindow::applyLiangBarsky() {
    scene->clear();
    drawAxesWithTicks();  // Отрисовка осей с делениями

    // Отрисовка окна отсечения
    scene->addRect(clippingWindow, QPen(Qt::red));

    // Перебор всех линий
    for (const auto &line : lines) {
        auto clippedLine = liangBarskyClip(clippingWindow.left(), clippingWindow.top(),
                                           clippingWindow.right(), clippingWindow.bottom(),
                                           line.x1, -line.y1, line.x2, -line.y2);

        // Отображаем исходную линию
        scene->addLine(line.x1, -line.y1, line.x2, -line.y2, QPen(Qt::blue, 1, Qt::DashLine));

        // Отображаем отсечённую линию
        if (clippedLine) {
            scene->addLine(clippedLine->xstart, clippedLine->ystart,
                           clippedLine->xend, clippedLine->yend, QPen(Qt::green));
        }
    }
}

void MainWindow::onLoadFile() {
    loadFile();
}

void MainWindow::onApplyClipping() {
    applyLiangBarsky();
}
