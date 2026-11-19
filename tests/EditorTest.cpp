#include "Editor.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QCursor>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QLineEdit>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QMessageBox>
#include <QSurfaceFormat>
#include <QTemporaryDir>
#include <QToolButton>
#include <QtTest>

class EditorTest final : public QObject {
    Q_OBJECT
private slots:
    void sceneRoundTrip();
    void viewportWorkflow();
    void editorWorkflow();
    void projectActions();
    void remoteGreeting();
};

void EditorTest::sceneRoundTrip() {
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    Scene source=Scene::livingRoom();
    const int id=source.add("chair",{.5f,.3f,.4f});
    QVERIFY(id>0);
    QVERIFY(!source.add("unknown"));
    auto *chair=source.find(id);
    chair->rotation={10,35,17};
    chair->scale={2,1,.75f};
    chair->color=QColor("#3e69a7");
    chair->material="Kim loại";
    chair->locked=true;
    QVERIFY(!source.remove(id));
    const int copy=source.duplicate(id);
    QVERIFY(copy!=0 && copy!=id);
    QVERIFY(source.remove(copy));
    source.light.directional=.7f;
    source.light.color=QColor("#f5ca96");
    source.camera.mode=ViewMode::Plan;
    const QString path=folder.filePath("phòng.int3d");
    QVERIFY(source.save(path));
    Scene loaded;
    QVERIFY(Scene::load(path,loaded));
    QCOMPARE(loaded.room,source.room);
    QCOMPARE(loaded.objects.size(),source.objects.size());
    QCOMPARE(loaded.camera.mode,ViewMode::Plan);
    QCOMPARE(loaded.light.color,source.light.color);
    QCOMPARE(loaded.light.directional,.7f);
    QVERIFY(loaded.find(id));
    QCOMPARE(loaded.find(id)->position,QVector3D(.5f,.3f,.4f));
    QCOMPARE(loaded.find(id)->rotation,QVector3D(10,35,17));
    QCOMPARE(loaded.find(id)->scale,QVector3D(2,1,.75f));
    QCOMPARE(loaded.find(id)->color,QColor("#3e69a7"));
    QCOMPARE(loaded.find(id)->material,QString("Kim loại"));
    QVERIFY(loaded.find(id)->locked);
    QFile broken(folder.filePath("bad.int3d"));
    QVERIFY(broken.open(QIODevice::WriteOnly));
    broken.write("{\"version\":1,\"objects\":[]}");
    broken.close();
    QVERIFY(!Scene::load(broken.fileName(),loaded));
    QVERIFY(loaded.find(id));
}

void EditorTest::viewportWorkflow() {
    Scene scene=Scene::livingRoom();
    const int item=scene.add("table",{0,0,1});
    Viewport view(scene);
    view.resize(720,520);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QTRY_VERIFY(view.isValid());
    QVERIFY2(view.rendererError().isEmpty(),qPrintable(view.rendererError()));
    const QImage start=view.grabFramebuffer();
    QVERIFY(!start.isNull());
    QVERIFY(start.pixelColor(40,40)!=start.pixelColor(start.width()/2,start.height()/2));
    auto drag=[&](Qt::MouseButton button,QPoint from,QPoint to) {
        QTest::mousePress(&view,button,Qt::NoModifier,from);
        QMouseEvent event(QEvent::MouseMove,to,view.mapToGlobal(to),Qt::NoButton,button,Qt::NoModifier);
        QApplication::sendEvent(&view,&event);
        QTest::mouseRelease(&view,button,Qt::NoModifier,to);
    };
    auto yaw=scene.camera.yaw;
    drag(Qt::RightButton,{240,250},{278,265});
    QVERIFY(scene.camera.yaw!=yaw);
    auto target=scene.camera.target;
    drag(Qt::MiddleButton,{240,250},{280,270});
    QVERIFY(scene.camera.target!=target);
    QWheelEvent wheel(QPointF(360,260),view.mapToGlobal(QPoint(360,260)),{},QPoint(0,120),
        Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);
    auto prior=scene.camera.distance;
    QApplication::sendEvent(&view,&wheel);
    QVERIFY(scene.camera.distance<prior);
    view.resetCamera();
    view.setMode(ViewMode::Plan);
    const auto point=view.project({0,.48f,1}).toPoint();
    QCOMPARE(view.pick(point),item);
    QTest::mouseClick(&view,Qt::LeftButton,Qt::NoModifier,point);
    QCOMPARE(view.selection(),item);
    view.setTool(EditTool::Move);
    drag(Qt::LeftButton,point,point+QPoint(23,14));
    QVERIFY(scene.find(item)->position!=QVector3D(0,0,1));
    const auto moved=view.project(scene.find(item)->position+QVector3D(0,.48f,0)).toPoint();
    view.setTool(EditTool::Rotate);
    drag(Qt::LeftButton,moved,moved+QPoint(22,0));
    QVERIFY(scene.find(item)->rotation.y()!=0);
    view.setTool(EditTool::Scale);
    view.setUniformScale(true);
    drag(Qt::LeftButton,moved,moved+QPoint(18,0));
    QVERIFY(scene.find(item)->scale.x()>1);
    QCOMPARE(scene.find(item)->scale.x(),scene.find(item)->scale.y());
    const QImage lit=view.grabFramebuffer();
    scene.light.ambient=0;
    scene.light.directional=0;
    scene.light.point=0;
    QVERIFY(view.grabFramebuffer()!=lit);
    view.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    view.setMode(ViewMode::Walk);
    QTRY_COMPARE(QWidget::mouseGrabber(),&view);
    const QPoint center=view.rect().center(), globalCenter=view.mapToGlobal(center);
    QTRY_COMPARE(QCursor::pos(),globalCenter);
    auto eye=scene.camera.eye;
    QTest::keyPress(&view,Qt::Key_W);
    QTest::qWait(110);
    QTest::keyRelease(&view,Qt::Key_W);
    QVERIFY(scene.camera.eye.z()<eye.z());
    QCOMPARE(scene.camera.eye.y(),eye.y());
    const auto releasedEye=scene.camera.eye;
    QKeyEvent repeat(QEvent::KeyPress,Qt::Key_W,Qt::NoModifier,QString(),true);
    QApplication::sendEvent(&view,&repeat);
    QTest::qWait(140);
    QCOMPARE(scene.camera.eye,releasedEye);

    const auto yawBefore=scene.camera.walkYaw;
    const QPoint mouseTarget=center+QPoint(40,-20);
    QMouseEvent move(QEvent::MouseMove,mouseTarget,view.mapToGlobal(mouseTarget),
                     Qt::NoButton,Qt::NoButton,Qt::NoModifier);
    QApplication::sendEvent(&view,&move);
    QCOMPARE(scene.camera.walkYaw,yawBefore+10.f);
    QTRY_COMPARE(QCursor::pos(),globalCenter);
    QTest::keyClick(&view,Qt::Key_Escape);
    QTest::keyClick(&view,Qt::Key_Tab);
    QCOMPARE(scene.camera.mode,ViewMode::Walk);
    QCOMPARE(QWidget::mouseGrabber(),&view);

    QTest::keyPress(&view,Qt::Key_W);
    QWidget otherWindow;
    otherWindow.show();
    otherWindow.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&otherWindow));
    QTRY_VERIFY(QWidget::mouseGrabber()!=&view);
    const auto inactiveEye=scene.camera.eye;
    view.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QTRY_COMPARE(QWidget::mouseGrabber(),&view);
    QTest::qWait(130);
    QCOMPARE(scene.camera.eye,inactiveEye);
    QTest::keyRelease(&view,Qt::Key_W);

    QTest::keyClick(&view,Qt::Key_T);
    QCOMPARE(scene.camera.mode,ViewMode::Plan);
    QVERIFY(QWidget::mouseGrabber()!=&view);
    QCoreApplication::processEvents();
    QSignalSpy idleFrames(&view,&QOpenGLWidget::frameSwapped);
    QTest::qWait(220);
    QVERIFY(idleFrames.count()<=1);
}

void EditorTest::editorWorkflow() {
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    QTRY_VERIFY(editor.view()->isValid());
    QVERIFY(editor.view()->rendererError().isEmpty());
    if(qEnvironmentVariableIsSet("INTERIOR3D_SCREENSHOT")) {
        auto *meter=editor.findChild<QLabel *>("fps");
        QVERIFY(meter);
        QTRY_VERIFY_WITH_TIMEOUT(meter->text()!=QString("FPS: …"),2500);
        QVERIFY(editor.grab().save(qEnvironmentVariable("INTERIOR3D_SCREENSHOT")));
        qInfo() << meter->text();
    }
    const auto viewportMid=editor.view()->mapTo(&editor,editor.view()->rect().center());
    QCOMPARE(editor.childAt(viewportMid),editor.view());
    auto *search=editor.findChild<QLineEdit *>("catalogSearch");
    auto *library=editor.findChild<QListWidget *>("catalog");
    QVERIFY(search && library);
    search->setText("plant");
    QCOMPARE(library->count(),1);
    const auto count=editor.model().objects.size();
    const auto tile=library->visualItemRect(library->item(0));
    QVERIFY(tile.isValid());
    QTest::mouseClick(library->viewport(),Qt::LeftButton,Qt::NoModifier,tile.center());
    QCOMPARE(editor.model().objects.size(),count+1);
    const int id=editor.view()->selection();
    QVERIFY(editor.model().find(id));
    QCOMPARE(editor.model().find(id)->kind,QString("plant"));
    auto *x=editor.findChild<QDoubleSpinBox *>("transform00");
    auto *rotation=editor.findChild<QDoubleSpinBox *>("transform11");
    auto *scale=editor.findChild<QDoubleSpinBox *>("transform22");
    auto *material=editor.findChild<QComboBox *>("objectMaterial");
    auto *locked=editor.findChild<QCheckBox *>("objectLock");
    QVERIFY(x && rotation && scale && material && locked);
    const auto old=editor.model().find(id)->position.x();
    x->setValue(1.25);
    QCOMPARE(editor.model().find(id)->position.x(),1.25f);
    auto tool=[&](const QString &name) {
        for(auto *button:editor.findChildren<QToolButton *>()) if(button->text()==name) return button;
        return static_cast<QToolButton *>(nullptr);
    };
    auto *undo=tool("Hoàn tác"),*redo=tool("Làm lại");
    QVERIFY(undo && redo);
    QTest::mouseClick(undo,Qt::LeftButton);
    QCOMPARE(editor.model().find(id)->position.x(),old);
    QTest::mouseClick(redo,Qt::LeftButton);
    QCOMPARE(editor.model().find(id)->position.x(),1.25f);
    rotation->setValue(42);
    scale->setValue(1.70);
    QCOMPARE(editor.model().find(id)->rotation.y(),42.f);
    QCOMPARE(editor.model().find(id)->scale.z(),1.7f);
    const auto plain=editor.view()->grabFramebuffer();
    material->setCurrentText("Gỗ");
    QCOMPARE(editor.model().find(id)->material,QString("Gỗ"));
    QVERIFY(plain!=editor.view()->grabFramebuffer());
    const auto oldColorFrame=editor.view()->grabFramebuffer();
    auto *colorButton=editor.findChild<QPushButton *>("objectColor");
    QVERIFY(colorButton);
    bool colorDialogOpened=false;
    QTimer::singleShot(0,&editor,[&]{
        auto *dialog=qobject_cast<QColorDialog *>(QApplication::activeModalWidget());
        colorDialogOpened=dialog;
        if(dialog) {
            dialog->setCurrentColor(QColor("#bd4125"));
            dialog->accept();
        } else if(auto *other=qobject_cast<QDialog *>(QApplication::activeModalWidget())) {
            other->reject();
        }
    });
    QTest::mouseClick(colorButton,Qt::LeftButton);
    QVERIFY(colorDialogOpened);
    QCOMPARE(editor.model().find(id)->color,QColor("#bd4125"));
    QVERIFY(oldColorFrame!=editor.view()->grabFramebuffer());
    QPushButton *lightingButton=nullptr;
    for(auto *button:editor.findChildren<QPushButton *>()) if(button->text()=="Ánh sáng") lightingButton=button;
    QVERIFY(lightingButton);
    const auto lightFrame=editor.view()->grabFramebuffer();
    bool lightDialogOpened=false;
    QTimer::singleShot(0,&editor,[&]{
        auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if(!dialog) return;
        const auto strengths=dialog->findChildren<QDoubleSpinBox *>();
        lightDialogOpened=strengths.size()==3;
        if(lightDialogOpened) strengths.first()->setValue(.85);
        dialog->accept();
    });
    QTest::mouseClick(lightingButton,Qt::LeftButton);
    QVERIFY(lightDialogOpened);
    QCOMPARE(editor.model().light.ambient,.85f);
    QVERIFY(lightFrame!=editor.view()->grabFramebuffer());
    auto *duplicate=editor.findChild<QAction *>("duplicateAction");
    auto *remove=editor.findChild<QAction *>("deleteAction");
    QVERIFY(duplicate && remove);
    editor.activateWindow();
    editor.view()->setFocus();
    QTRY_VERIFY(editor.view()->hasFocus());
    QTest::keyClick(editor.view(),Qt::Key_D,Qt::ControlModifier);
    QTRY_COMPARE(editor.model().objects.size(),count+2);
    const int copied=editor.view()->selection();
    locked->setChecked(true);
    QVERIFY(!remove->isEnabled());
    locked->setChecked(false);
    editor.view()->setFocus();
    QTest::keyClick(editor.view(),Qt::Key_Delete);
    QVERIFY(!editor.model().find(copied));
    QTest::mouseClick(undo,Qt::LeftButton);
    QVERIFY(editor.model().find(copied));
    auto *views=editor.findChild<QComboBox *>("viewMode");
    QVERIFY(views);
    views->setCurrentIndex(1);
    QCOMPARE(editor.model().camera.mode,ViewMode::Plan);
    const auto path=folder.filePath("dự án.int3d");
    QVERIFY(editor.saveProject(path));
    QVERIFY(editor.exportImage(folder.filePath("image.png")));
    QVERIFY(editor.exportImage(folder.filePath("image.jpg")));
    QVERIFY(!QImage(folder.filePath("image.png")).isNull());
    QVERIFY(!QImage(folder.filePath("image.jpg")).isNull());
    QVERIFY(editor.openProject(path));
    QCOMPARE(editor.model().camera.mode,ViewMode::Plan);
    QCOMPARE(editor.model().find(id)->position.x(),1.25f);
    QCOMPARE(editor.model().find(id)->material,QString("Gỗ"));
    QCOMPARE(editor.model().find(id)->color,QColor("#bd4125"));
    QCOMPARE(editor.model().light.ambient,.85f);
    editor.resize(1020,700);
    QTest::qWait(50);
    QVERIFY(editor.view()->geometry().width()>=360);
    QVERIFY(editor.view()->geometry().height()>=310);
    QVERIFY(!editor.view()->grabFramebuffer().isNull());
    QTest::keyClick(editor.view(),Qt::Key_F11);
    QTRY_VERIFY(editor.isFullScreen());
    QTest::keyClick(editor.view(),Qt::Key_F11);
    QTRY_VERIFY(!editor.isFullScreen());
}

void EditorTest::projectActions() {
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    QVERIFY(editor.view()->isValid());
    auto focusViewport=[&]{
        editor.activateWindow();
        editor.view()->setFocus();
        return QTest::qWaitForWindowActive(&editor);
    };
    QVERIFY(focusViewport());
    QTRY_VERIFY(editor.view()->hasFocus());
    auto sendFileShortcut=[&](Qt::Key key,Qt::KeyboardModifiers modifiers,const QString &path) {
        if(!focusViewport()) return false;
        bool opened=false;
        QTimer::singleShot(0,&editor,[&]{
            auto *modal=QApplication::activeModalWidget();
            auto *dialog=qobject_cast<QFileDialog *>(modal);
            if(!dialog) {
                if(auto *other=qobject_cast<QDialog *>(modal)) other->reject();
                return;
            }
            opened=true;
            dialog->selectFile(path);
            QMetaObject::invokeMethod(dialog,"accept",Qt::QueuedConnection);
        });
        QTest::keyClick(editor.view(),key,modifiers);
        return opened;
    };
    const auto file=folder.filePath("first.int3d");
    QVERIFY(sendFileShortcut(Qt::Key_S,Qt::ControlModifier,file));
    QVERIFY(QFileInfo::exists(file));
    auto *x=editor.findChild<QDoubleSpinBox *>("transform00");
    QVERIFY(x);
    const int table=editor.view()->selection();
    const float previous=editor.model().find(table)->position.x();
    x->setValue(1.1);
    QVERIFY(focusViewport());
    QTest::keyClick(editor.view(),Qt::Key_Z,Qt::ControlModifier);
    QCOMPARE(editor.model().find(table)->position.x(),previous);
    const auto second=folder.filePath("second.int3d");
    QVERIFY(sendFileShortcut(Qt::Key_S,Qt::ControlModifier|Qt::ShiftModifier,second));
    QVERIFY(QFileInfo::exists(second));
    const auto image=folder.filePath("export.png");
    QVERIFY(sendFileShortcut(Qt::Key_E,Qt::ControlModifier,image));
    QVERIFY(!QImage(image).isNull());
    QVERIFY(focusViewport());
    QTest::keyClick(editor.view(),Qt::Key_N,Qt::ControlModifier);
    QCOMPARE(editor.view()->selection(),0);
    QVERIFY(sendFileShortcut(Qt::Key_O,Qt::ControlModifier,second));
    QCOMPARE(editor.model().find(table)->position.x(),previous);

    auto checkDialog=[&](QWidget *button,const QString &title) {
        if(!button) return false;
        bool matched=false;
        QTimer::singleShot(0,&editor,[&]{
            auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());
            if(!dialog) return;
            matched=dialog->windowTitle()==title;
            dialog->reject();
        });
        QTest::mouseClick(button,Qt::LeftButton);
        return matched;
    };
    auto tool=[&](const QString &name) -> QToolButton * {
        for(auto *button:editor.findChildren<QToolButton *>()) if(button->text()==name) return button;
        return nullptr;
    };
    auto nav=[&](const QString &name) -> QPushButton * {
        for(auto *button:editor.findChildren<QPushButton *>()) if(button->text()==name) return button;
        return nullptr;
    };
    QVERIFY(checkDialog(tool("Xem trước"),"Xem trước ảnh render"));
    QVERIFY(checkDialog(tool("Cài đặt"),"Cài đặt"));
    QVERIFY(checkDialog(tool("Trợ giúp"),"Interior3D"));
    QVERIFY(checkDialog(nav("Trang chủ"),"Interior3D"));
    QVERIFY(checkDialog(nav("Bố trí nội thất"),"Đối tượng trong dự án"));
    QVERIFY(checkDialog(nav("Camera"),"Camera"));
    QVERIFY(checkDialog(nav("Render ảnh"),"Xuất ảnh"));
    QVERIFY(checkDialog(nav("Quản lý dự án"),"Quản lý dự án"));
}

void EditorTest::remoteGreeting() {
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    auto *view=editor.view();
    QVERIFY(view->isValid());
    QVERIFY(view->rendererError().isEmpty());
    editor.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&editor));
    const auto folder=qEnvironmentVariable("INTERIOR3D_REMOTE_SCREENSHOTS");
    auto capture=[&](const QString &name) {
        return folder.isEmpty() || editor.grab().save(QDir(folder).filePath(name));
    };
    const Object *table=nullptr;
    const Object *tv=nullptr;
    for(const auto &o:editor.model().objects) {
        if(o.kind=="table") table=&o;
        if(o.kind=="tv") tv=&o;
    }
    QVERIFY(table && tv);
    const QPoint point=view->project(table->matrix().map(QVector3D(.23f,.499f,.238f))).toPoint();
    QCOMPARE(view->pick(point),-1);
    QTest::qWait(100);
    QVERIFY(capture("01-remote-tren-ban.png"));
    QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,point);
    QCOMPARE(editor.model().camera.mode,ViewMode::Walk);
    const auto screen=view->project(tv->matrix().map(QVector3D(-.6f,.75f,.054f))).toPoint();
    const auto off=view->grabFramebuffer();
    QVERIFY(off.rect().contains(screen));
    QTest::keyClick(view,Qt::Key_M);
    const auto early=view->grabFramebuffer();
    QVERIFY(early.pixelColor(screen)!=off.pixelColor(screen));
    QTest::qWait(1580);
    const auto middle=view->grabFramebuffer();
    QVERIFY(early!=middle);
    QVERIFY(capture("02-TV-dang-hien-chu.png"));
    QTest::qWait(2150);
    const auto full=view->grabFramebuffer();
    QVERIFY(full!=middle);
    QVERIFY(capture("03-TV-loi-chuc-va-bo-hoa.png"));
    QTest::keyClick(view,Qt::Key_M);
    QCOMPARE(view->grabFramebuffer().pixelColor(screen),off.pixelColor(screen));
    QTest::keyClick(view,Qt::Key_M);
    QTest::keyClick(view,Qt::Key_X);
    QTest::qWait(130);
    QVERIFY(capture("04-tha-remote.png"));
    QTest::keyClick(view,Qt::Key_M);
    QVERIFY(view->grabFramebuffer().pixelColor(screen)!=off.pixelColor(screen));
    view->setMode(ViewMode::Plan);
    QTest::qWait(700);
    // The remote falls to the floor in front of the first-person camera.
    const auto floorNear=view->project({1.77f,.055f,-.20f}).toPoint();
    QPoint remoteOnFloor;
    for(int x=-19;x<=19 && remoteOnFloor.isNull();x+=2)
        for(int y=-15;y<=15;y+=2) {
            const QPoint p=floorNear+QPoint(x,y);
            if(view->pick(p)==-1) { remoteOnFloor=p; break; }
        }
    QVERIFY(!remoteOnFloor.isNull());
    QVERIFY(capture("05-remote-da-rot.png"));
    QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,remoteOnFloor);
    QCOMPARE(editor.model().camera.mode,ViewMode::Walk);

    // Grab the remote from the table using only the keyboard while looking at the TV.
    view->resetRemote();
    QTest::keyClick(view,Qt::Key_X);
    QTest::keyClick(view,Qt::Key_M);
    QVERIFY(view->grabFramebuffer().pixelColor(screen)!=off.pixelColor(screen));
    QTest::keyClick(view,Qt::Key_X);
    QKeyEvent repeatedX(QEvent::KeyPress,Qt::Key_X,Qt::NoModifier,QString(),true);
    QApplication::sendEvent(view,&repeatedX);
    QTest::keyClick(view,Qt::Key_M);
    QVERIFY(view->grabFramebuffer().pixelColor(screen)!=off.pixelColor(screen));
    QTest::keyClick(view,Qt::Key_X);
    QTest::keyClick(view,Qt::Key_M);
    QCOMPARE(view->grabFramebuffer().pixelColor(screen),off.pixelColor(screen));
}

int main(int argc,char **argv) {
    QSurfaceFormat format;
    format.setVersion(3,3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    QSurfaceFormat::setDefaultFormat(format);
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QApplication application(argc,argv);
    EditorTest test;
    return QTest::qExec(&test,argc,argv);
}

#include "EditorTest.moc"
