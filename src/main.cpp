#include "Editor.h"
#include <QApplication>
#include <QSurfaceFormat>

int main(int argc,char **argv) {
    QSurfaceFormat format;
    format.setVersion(3,3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setSamples(4);
    QSurfaceFormat::setDefaultFormat(format);
    QApplication application(argc,argv);
    application.setApplicationName("Interior3D");
    Editor editor;
    if(application.arguments().size()>1) editor.openProject(application.arguments().at(1));
    editor.show();
    return application.exec();
}
