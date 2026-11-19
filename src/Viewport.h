#pragma once

#include "Scene.h"
#include <QElapsedTimer>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QSet>
#include <QTimer>
#include <memory>

enum class EditTool { Select, Move, Rotate, Scale };

class Viewport final : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
    Q_OBJECT
public:
    explicit Viewport(Scene &scene, QWidget *parent = nullptr);
    ~Viewport() override;
    void select(int id);
    int selection() const { return selected; }
    void setTool(EditTool value);
    void setMode(ViewMode mode);
    void resetCamera();
    void setGrid(bool enabled);
    void setUniformScale(bool enabled) { uniformScale=enabled; }
    QString rendererError() const { return error; }
    int pick(QPointF pixel) const;
    QPointF project(QVector3D world) const;
signals:
    void selectionChanged(int id);
    void editStarted();
    void objectChanged();
    void editFinished();
    void modeChanged(ViewMode mode);
    void statsChanged(int fps);
protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
private:
    Scene &scene;
    std::unique_ptr<QOpenGLShaderProgram> shader;
    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vbo;
    QElapsedTimer meter, step;
    QTimer animation;
    QSet<int> keys;
    QString error;
    int selected = 0, frameCount = 0, roundedCount = 0;
    bool grid = false, uniformScale = false, dragging = false, walkMouseReady = false;
    EditTool tool = EditTool::Select;
    QPointF last, initial;
    QVector3D dragAxis;
    Qt::MouseButton button = Qt::NoButton;

    void cleanup();
    void tick();
    QMatrix4x4 viewProjection() const;
    void draw(const QMatrix4x4 &model, QColor color, const QString &material, bool cylinder = false, bool rounded = false);
    void drawSelection(const Object &object);
    bool visible(const Object &object) const;
};
