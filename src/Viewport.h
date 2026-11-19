#pragma once

#include "Scene.h"
#include <QElapsedTimer>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QSet>
#include <QTimer>
#include <memory>

class QLabel;

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
    void resetRemote();
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
    std::unique_ptr<QOpenGLTexture> tvTexture;
    enum class RemoteState { OnTable, Held, Falling, Dropped };
    RemoteState remoteState = RemoteState::OnTable;
    QVector3D remotePosition, remoteVelocity;
    float remoteSpin = 0;
    int poweredTv = 0, uploadedCharacters = -1;
    QElapsedTimer typing;
    static constexpr int remoteHit = -1;
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
    const Object *firstObject(const QString &kind) const;
    QMatrix4x4 remoteTransform() const;
    void drawRemote(const QMatrix4x4 &transform, bool held);
    void drawTv(const Object &tv);
    void updateRemoteHint();
    QLabel *remoteHint = nullptr;
    void holdRemote();
    void releaseRemote();
    void stepRemote(float seconds);
    QMatrix4x4 viewProjection() const;
    void draw(const QMatrix4x4 &model, QColor color, const QString &material, bool cylinder = false, bool rounded = false);
    void drawSelection(const Object &object);
    bool visible(const Object &object) const;
};
