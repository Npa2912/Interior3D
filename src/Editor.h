#pragma once

#include "Viewport.h"
#include <QMainWindow>
#include <QUndoStack>
#include <array>
#include <functional>

class QAction;
class QButtonGroup;
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QToolButton;

class Editor final : public QMainWindow {
    Q_OBJECT
public:
    explicit Editor(QWidget *parent = nullptr);
    bool openProject(const QString &path);
    bool saveProject(const QString &path);
    bool exportImage(const QString &path);
    const Scene &model() const { return scene; }
    Viewport *view() const { return viewport; }
protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
private:
    Scene scene = Scene::livingRoom();
    QUndoStack history;
    Scene dragBefore;
    Camera savedCamera;
    QString path;
    Viewport *viewport = nullptr;
    QAction *duplicateAction = nullptr, *deleteAction = nullptr;
    QLabel *projectLabel = nullptr, *fpsLabel = nullptr, *ratioLabel = nullptr;
    QComboBox *viewMode = nullptr, *material = nullptr;
    QWidget *fields = nullptr;
    QLineEdit *objectName = nullptr, *search = nullptr;
    QListWidget *library = nullptr;
    QPushButton *objectColor = nullptr;
    QCheckBox *locked = nullptr, *uniform = nullptr;
    QButtonGroup *categories = nullptr, *navigation = nullptr, *views = nullptr;
    std::array<std::array<QLineEdit *,3>,3> transform{};
    bool syncing = false;
    bool draggingObject = false;

    void buildUi();
    void refresh();
    void refreshProjectLabel();
    void refreshLibrary();
    void edit(const QString &label, const std::function<void()> &operation);
    void record(const QString &label, Scene before, int oldSelection);
    void applyEdit(const Scene &value, int selected);
    void resetScene(Scene next, const QString &file);
    bool confirmProjectChange();
    bool chooseSave(bool saveAs = false);
    void chooseOpen();
    void chooseExport();
    void addFurniture(const QString &kind);
    void showObjects();
    void showLights();
    void showCamera();
    void showPreview();
    void showSettings();
    void showHelp();
};
