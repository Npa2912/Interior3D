#include "Viewport.h"
#include "TvGreeting.h"

#include <QKeyEvent>
#include <QCursor>
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QtMath>
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace {
constexpr float tau = 6.283185307f;
constexpr int cubeCount = 36;
constexpr int cylinderCount = 24 * 12;
constexpr float bevelRadius = .105f;
const QVector3D axes[]{{1,0,0},{0,1,0},{0,0,1}};
const QColor axisColors[]{QColor("#f45559"),QColor("#81db48"),QColor("#37a8ff")};

int materialCode(const QString &material) {
    if(material=="Gỗ") return 1;
    if(material=="Kim loại") return 2;
    if(material=="Vải") return 3;
    if(material=="Kính") return 4;
    if(material=="Phát sáng") return 5;
    if(material=="Bầu trời") return 6;
    return 0;
}

float wrap(float degrees) { return std::remainder(degrees,360.f); }

QVector<float> vertices() {
    QVector<float> buffer;
    auto vertex=[&](QVector3D p,QVector3D n) { buffer.append({p.x(),p.y(),p.z(),n.x(),n.y(),n.z()}); };
    for(int axis=0;axis<3;++axis) for(int sign:{-1,1}) {
        QVector3D normal=axes[axis]*sign, face=normal*.5f;
        QVector3D u=axes[(axis+1)%3]*.5f, v=axes[(axis+2)%3]*.5f;
        for(QVector3D p:{face-u-v,face+u-v,face+u+v,face-u-v,face+u+v,face-u+v}) vertex(p,normal);
    }
    for(int i=0;i<24;++i) {
        float a=i*tau/24,b=(i+1)*tau/24;
        QVector3D n1(std::cos(a),0,std::sin(a)), n2(std::cos(b),0,std::sin(b));
        QVector3D p=n1*.5f,q=n2*.5f,top(0,.5f,0);
        vertex(p-top,n1); vertex(q-top,n2); vertex(q+top,n2);
        vertex(p-top,n1); vertex(q+top,n2); vertex(p+top,n1);
        for(int sign:{-1,1}) {
            QVector3D cap=top*sign,n(0,sign,0);
            vertex(cap,n); vertex(p+cap,n); vertex(q+cap,n);
        }
    }
    // Round upholstery by projecting face samples onto the rounded unit box.
    const float inner=.5f-bevelRadius;
    const float stops[]{-.5f,-.47f,-inner,inner,.47f,.5f};
    for(int axis=0;axis<3;++axis) for(int sign:{-1,1}) {
        const auto face=axes[axis]*(sign*.5f);
        const auto u=axes[(axis+1)%3],v=axes[(axis+2)%3];
        auto rounded=[&](float x,float y) {
            QVector3D p=face+u*x+v*y,core;
            for(int i=0;i<3;++i) core[i]=std::clamp(p[i],-inner,inner);
            QVector3D normal=(p-core).normalized();
            vertex(core+normal*bevelRadius,normal);
        };
        for(int i=0;i<5;++i) for(int j=0;j<5;++j) {
            rounded(stops[i],stops[j]);
            rounded(stops[i+1],stops[j]);
            rounded(stops[i+1],stops[j+1]);
            rounded(stops[i],stops[j]);
            rounded(stops[i+1],stops[j+1]);
            rounded(stops[i],stops[j+1]);
        }
    }
    return buffer;
}

const char *vertexSource=R"GLSL(
#version 330 core
layout(location=0) in vec3 vertex;
layout(location=1) in vec3 normal;
uniform mat4 vp;
uniform mat4 model;
uniform mat3 normalMatrix;
out vec3 world;
out vec3 local;
out vec3 surface;
void main() {
    vec4 p=model*vec4(vertex,1);
    world=p.xyz;
    local=vertex;
    surface=normalMatrix*normal;
    gl_Position=vp*p;
}
)GLSL";

const char *fragmentSource=R"GLSL(
#version 330 core
in vec3 world;
in vec3 local;
in vec3 surface;
uniform vec3 color;
uniform vec3 lightColor;
uniform vec3 eye;
uniform vec3 intensity;
uniform int material;
uniform sampler2D tvScreen;
out vec4 fragment;
void main() {
    if(material==7 && local.z>.499) {
        fragment=texture(tvScreen,vec2(local.x+.5,.5-local.y));
        return;
    }
    vec3 n=normalize(surface);
    vec3 lightVector=vec3(-.55,2.73,-.42)-world;
    float distanceSquared=dot(lightVector,lightVector);
    vec3 light=normalize(lightVector);
    float sun=max(dot(n,normalize(vec3(-.40,.78,.52))),0);
    float point=max(dot(n,light),0)/(1+.36*distanceSquared);
    float sheen=pow(max(dot(n,normalize(light+normalize(eye-world))),0),material==2?24:45);
    vec3 base=color;
    if(material==1) {
        float grain=sin(local.z*168+sin(local.x*5+local.y*11)*2.4);
        base*=.98+.016*grain+.008*sin(local.z*79+local.x*13);
    }
    if(material==3) base*=.99+.011*sin(local.x*104)*sin((local.y+local.z)*97);
    if(material==4) {
        base*=.71+.29*(world.y/3.1);
        sheen*=1.5;
    }
    float overhead=max(n.y,0);
    float facingWindow=max(-n.z,0);
    float bounce=.79+.21*smoothstep(0.,1.9,world.y);
    vec3 ambiance=mix(vec3(1),lightColor,.35)*intensity.x*bounce;
    ambiance+=intensity.x*vec3(.22,.17,.12)*smoothstep(.35,2.7,world.y);
    vec3 lit=base*(ambiance + lightColor*(sun*intensity.y+point*intensity.z));
    lit+=base*vec3(.65,.82,1)*intensity.y*(.25*overhead+.16*facingWindow+.12);
    if(material==6) {
        float sky=clamp((world.y-.7)/2.0,0,1);
        vec3 horizon=mix(vec3(.89,.95,1),base,sky);
        float cloud=sin(world.x*4+cos(world.y*3))*sin(world.x*2-world.y*12);
        lit=mix(horizon,vec3(.91,.96,1),smoothstep(.56,.91,cloud)*.19);
    }
    lit+=lightColor*sheen*(intensity.y*.18+intensity.z*.14)*(material==2||material==4?.5:.08);
    fragment=vec4(material==5?base:lit,1);
}
)GLSL";

bool intersectBox(QVector3D origin, QVector3D direction, float &distance, bool cylinder) {
    // Clip the ray to the three slabs of the unit cube in mesh coordinates.
    float lo=0, hi=distance;
    for(int i=0;i<3;++i) {
        if(std::abs(direction[i])<1e-7f) {
            if(std::abs(origin[i])>.5f) return false;
            continue;
        }
        float a=(-.5f-origin[i])/direction[i],b=(.5f-origin[i])/direction[i];
        if(a>b) std::swap(a,b);
        lo=std::max(lo,a);
        hi=std::min(hi,b);
        if(lo>hi) return false;
    }
    if(cylinder) {
        const float a=direction.x()*direction.x()+direction.z()*direction.z();
        const float b=origin.x()*direction.x()+origin.z()*direction.z();
        const float c=origin.x()*origin.x()+origin.z()*origin.z()-.25f;
        if(a<1e-12f) {
            if(c>0) return false;
        } else {
            const float discriminant=b*b-a*c;
            if(discriminant<0) return false;
            const float root=std::sqrt(discriminant);
            lo=std::max(lo,(-b-root)/a);
            hi=std::min(hi,(-b+root)/a);
            if(lo>hi) return false;
        }
    }
    distance=lo;
    return true;
}

float segmentDistance(QPointF p,QPointF a,QPointF b) {
    QPointF delta=b-a;
    const double length=QPointF::dotProduct(delta,delta);
    if(length<60) return 10000;
    double t=std::clamp(QPointF::dotProduct(p-a,delta)/length,0.,1.);
    QPointF difference=p-(a+delta*t);
    return std::hypot(difference.x(),difference.y());
}
}

Viewport::Viewport(Scene &model,QWidget *parent):QOpenGLWidget(parent),scene(model) {
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(360,310);
    remoteHint=new QLabel(this);
    remoteHint->setAttribute(Qt::WA_TransparentForMouseEvents);
    remoteHint->setStyleSheet(
        "QLabel { background:rgba(28,33,45,225); color:#fff0d6;"
        "border:1px solid #aa9174; border-radius:8px; padding:5px 9px;"
        "font-size:13px; font-weight:600; }");
    remoteHint->hide();
    walkReticle=new QLabel("+",this);
    walkReticle->setAttribute(Qt::WA_TransparentForMouseEvents);
    walkReticle->setAlignment(Qt::AlignCenter);
    walkReticle->setStyleSheet("color:#fff4db; font-size:16px; font-weight:600; background:transparent;");
    walkReticle->setFixedSize(20,20);
    walkReticle->hide();
    connect(&animation,&QTimer::timeout,this,&Viewport::tick);
    connect(this,&QOpenGLWidget::frameSwapped,this,[this]{++frameCount;});
    animation.setTimerType(Qt::PreciseTimer);
    animation.start(16);
    meter.start();
    step.start();
}

Viewport::~Viewport() {
    releaseCursor();
    if(context()) disconnect(context(),nullptr,this,nullptr);
    cleanup();
}

void Viewport::cleanup() {
    if(!context() || !shader) return;
    makeCurrent();
    vao.destroy();
    vbo.destroy();
    tvTexture.reset();
    uploadedCharacters=-1;
    shader.reset();
    doneCurrent();
}

void Viewport::initializeGL() {
    if(!initializeOpenGLFunctions()) {
        error="Cần trình điều khiển OpenGL 3.3 Core.";
        return;
    }
    connect(context(),&QOpenGLContext::aboutToBeDestroyed,this,&Viewport::cleanup);
    glEnable(GL_DEPTH_TEST);
    glClearColor(.14f,.17f,.20f,1);
    auto program=std::make_unique<QOpenGLShaderProgram>();
    if(!program->addShaderFromSourceCode(QOpenGLShader::Vertex,vertexSource)||
       !program->addShaderFromSourceCode(QOpenGLShader::Fragment,fragmentSource)||!program->link()) {
        error=program->log();
        return;
    }
    error.clear();
    shader=std::move(program);
    modelUniform=shader->uniformLocation("model");
    normalUniform=shader->uniformLocation("normalMatrix");
    colorUniform=shader->uniformLocation("color");
    materialUniform=shader->uniformLocation("material");
    const auto data=vertices();
    roundedCount=data.size()/6-cubeCount-cylinderCount;
    vao.create();
    QOpenGLVertexArrayObject::Binder binder(&vao);
    vbo.create();
    vbo.bind();
    vbo.allocate(data.constData(),int(data.size()*sizeof(float)));
    shader->bind();
    shader->enableAttributeArray(0);
    shader->enableAttributeArray(1);
    shader->setAttributeBuffer(0,GL_FLOAT,0,3,6*sizeof(float));
    shader->setAttributeBuffer(1,GL_FLOAT,3*sizeof(float),3,6*sizeof(float));
    shader->release();
    vbo.release();
}

void Viewport::resizeGL(int w,int h) {
    Q_UNUSED(w);
    Q_UNUSED(h);
    walkReticle->move(rect().center()-QPoint(10,10));
    refreshCursorClip();
    update();
}

QMatrix4x4 Viewport::viewProjection() const {
    return scene.camera.projection(float(std::max(1,width()))/std::max(1,height()))*scene.camera.view();
}

bool Viewport::visible(const Object &o) const {
    return scene.camera.mode!=ViewMode::Plan || (o.kind!="ceiling" && o.kind!="pendant");
}

void Viewport::draw(const QMatrix4x4 &model,QColor color,const QString &material,bool cylinder,bool rounded) {
    shader->setUniformValue(modelUniform,model);
    shader->setUniformValue(normalUniform,model.normalMatrix());
    shader->setUniformValue(colorUniform,QVector3D(color.redF(),color.greenF(),color.blueF()));
    shader->setUniformValue(materialUniform,materialCode(material));
    if(cylinder) glDrawArrays(GL_TRIANGLES,cubeCount,cylinderCount);
    else if(rounded) glDrawArrays(GL_TRIANGLES,cubeCount+cylinderCount,roundedCount);
    else glDrawArrays(GL_TRIANGLES,0,cubeCount);
}

const QVector<Part> &Viewport::partsFor(const Object &object) const {
    if(cachedRoom!=scene.room || geometryCache.size()>scene.objects.size()+64) {
        geometryCache.clear();
        cachedRoom=scene.room;
    }
    auto &entry=geometryCache[object.id];
    if(entry.kind!=object.kind || entry.color!=object.color) {
        entry.kind=object.kind;
        entry.color=object.color;
        entry.parts=::geometry(object,scene.room);
    }
    return entry.parts;
}

void Viewport::drawSelection(const Object &o) {
    QMatrix4x4 transform=o.matrix();
    QVector3D minimum(1000,1000,1000),maximum(-1000,-1000,-1000);
    for(const Part &part: partsFor(o)) {
        const auto matrix=part.matrix();
        for(float x:{-.5f,.5f}) for(float y:{-.5f,.5f}) for(float z:{-.5f,.5f}) {
            const auto p=matrix.map(QVector3D(x,y,z));
            for(int i=0;i<3;++i) {
                minimum[i]=std::min(minimum[i],p[i]);
                maximum[i]=std::max(maximum[i],p[i]);
            }
        }
    }
    const auto center=(minimum+maximum)*.5f;
    const auto extent=maximum-minimum;
    const QColor outline(o.locked?"#f2bc69":"#46adff");
    glDisable(GL_DEPTH_TEST);
    for(int i=0;i<3;++i) {
        int a=(i+1)%3,b=(i+2)%3;
        for(int sa:{-1,1}) for(int sb:{-1,1}) {
            QVector3D point=center,size(.014f,.014f,.014f);
            point[a]+=sa*extent[a]*.5f;
            point[b]+=sb*extent[b]*.5f;
            size[i]=extent[i];
            QMatrix4x4 edge=transform;
            edge.translate(point);
            edge.scale(size);
            draw(edge,outline,"Phát sáng");
        }
    }
    if(!o.locked && tool==EditTool::Move) {
        const auto origin=o.position+QVector3D(0,.35f,0);
        for(int i=0;i<3;++i) {
            QMatrix4x4 stem;
            stem.translate(origin+axes[i]*.43f);
            stem.scale(axes[i]*.84f+QVector3D(.025f,.025f,.025f));
            draw(stem,axisColors[i],"Phát sáng");
            QMatrix4x4 tip;
            tip.translate(origin+axes[i]*.87f);
            tip.scale(axes[i]*.13f+QVector3D(.075f,.075f,.075f));
            draw(tip,axisColors[i],"Phát sáng");
        }
    }
    glEnable(GL_DEPTH_TEST);
}

void Viewport::paintGL() {
    if(!shader) {
        QPainter p(this);
        p.fillRect(rect(),QColor("#282f36"));
        p.setPen(Qt::white);
        p.drawText(rect().adjusted(24,24,-24,-24),Qt::AlignCenter|Qt::TextWordWrap,
                   error.isEmpty()?"OpenGL 3.3":error);
        return;
    }
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    shader->bind();
    QOpenGLVertexArrayObject::Binder binder(&vao);
    shader->setUniformValue("vp",viewProjection());
    shader->setUniformValue("eye",scene.camera.position());
    const auto &light=scene.light;
    shader->setUniformValue("lightColor",QVector3D(light.color.redF(),light.color.greenF(),light.color.blueF()));
    shader->setUniformValue("intensity",QVector3D(light.ambient,light.directional,light.point));
    for(const auto &o:scene.objects) {
        if(!visible(o)) continue;
        const auto transform=o.matrix();
        for(const auto &part: partsFor(o)) {
            const QString material=part.tint && o.material!="Mặc định" ? o.material : part.material;
            const bool rounded=part.material=="Vải" && (o.kind=="sofa" || o.kind=="chair" || o.kind=="bed");
            draw(transform*part.matrix(),part.color,material,part.cylinder,rounded);
        }
    }
    if(const auto *tv=std::as_const(scene).find(poweredTv);tv && tv->kind=="tv" && scene.camera.mode!=ViewMode::Plan) drawTv(*tv);
    if(remoteState!=RemoteState::Held && (remoteState!=RemoteState::OnTable || firstObject("table")))
        drawRemote(remoteTransform(),false);
    if(grid) {
        const int width=int(scene.room.x()),depth=int(scene.room.z());
        for(int i=-width;i<=width;++i) {
            QMatrix4x4 m;
            m.translate(i*.5f,.011f,0);
            m.scale(.009f,.004f,scene.room.z());
            draw(m,QColor("#788c99"),"Phát sáng");
        }
        for(int i=-depth;i<=depth;++i) {
            QMatrix4x4 m;
            m.translate(0,.011f,i*.5f);
            m.scale(scene.room.x(),.004f,.009f);
            draw(m,QColor("#788c99"),"Phát sáng");
        }
    }
    if(const auto *o=std::as_const(scene).find(selected);o && visible(*o) && scene.camera.mode!=ViewMode::Walk) drawSelection(*o);
    if(remoteState==RemoteState::Held && scene.camera.mode!=ViewMode::Plan) {
        glDisable(GL_DEPTH_TEST);
        drawRemote(remoteTransform(),true);
        glEnable(GL_DEPTH_TEST);
    }
    shader->release();
    updateRemoteHint();
}

QPointF Viewport::project(QVector3D world) const {
    QVector4D clip=viewProjection()*QVector4D(world,1);
    if(qFuzzyIsNull(clip.w())) return {};
    return {(clip.x()/clip.w()+1)*width()/2,(1-clip.y()/clip.w())*height()/2};
}

int Viewport::pick(QPointF pixel) const {
    const auto inverse=viewProjection().inverted();
    float x=float(2*pixel.x()/width()-1),y=float(1-2*pixel.y()/height());
    const auto rayStart=(inverse*QVector4D(x,y,-1,1)).toVector3DAffine();
    const auto rayEnd=(inverse*QVector4D(x,y,1,1)).toVector3DAffine();
    const auto direction=(rayEnd-rayStart).normalized();
    float closest=(rayEnd-rayStart).length();
    int id=0;
    for(const auto &o:scene.objects) {
        if(!visible(o)) continue;
        const auto transform=o.matrix();
        for(const auto &part: partsFor(o)) {
            const auto local=(transform*part.matrix()).inverted();
            // Keeping the transformed direction unnormalized preserves world-ray distance.
            if(intersectBox(local.map(rayStart),local.mapVector(direction),closest,part.cylinder)) id=o.id;
        }
    }
    if(remoteState!=RemoteState::Held && (remoteState!=RemoteState::OnTable || firstObject("table"))) {
        auto volume=remoteTransform();
        volume.scale(.104f,.046f,.24f);
        bool invertible=false;
        const auto local=volume.inverted(&invertible);
        if(invertible && intersectBox(local.map(rayStart),local.mapVector(direction),closest,false)) return remoteHit;
    }
    return id;
}

void Viewport::select(int id) {
    if(id && !scene.find(id)) id=0;
    if(selected==id) return;
    selected=id;
    emit selectionChanged(id);
    update();
}

void Viewport::setTool(EditTool value) { tool=value; update(); }
void Viewport::setGrid(bool enabled) { grid=enabled; update(); }

void Viewport::setMode(ViewMode mode) {
    if(mode==ViewMode::Walk && scene.camera.mode!=ViewMode::Walk)
        beforeWalk=scene.camera.mode;
    scene.camera.mode=mode;
    keys.clear();
    button=Qt::NoButton;
    setMouseTracking(mode==ViewMode::Walk);
    if(mode==ViewMode::Walk) {
        setFocus(Qt::OtherFocusReason);
        captureCursor();
    } else {
        releaseCursor(true);
    }
    emit modeChanged(mode);
    update();
}

void Viewport::refreshCursorClip() {
#ifdef Q_OS_WIN
    if(!cursorCaptured) return;
    const QWidget *top=window();
    const auto handle=reinterpret_cast<HWND>(top->winId());
    const qreal scale=top->devicePixelRatioF();
    const QPoint offset=mapTo(top,QPoint());
    POINT origin{qRound(offset.x()*scale),qRound(offset.y()*scale)};
    if(!ClientToScreen(handle,&origin)) return;
    RECT viewportRect{origin.x,origin.y,origin.x+qRound(width()*scale),origin.y+qRound(height()*scale)};
    const QRect next(viewportRect.left,viewportRect.top,viewportRect.right-viewportRect.left,
                     viewportRect.bottom-viewportRect.top);
    if(next!=clippedRect && ClipCursor(&viewportRect)) {
        nativeClip=true;
        clippedRect=next;
    }
#endif
}

void Viewport::captureCursor() {
    if(cursorCaptured || scene.camera.mode!=ViewMode::Walk || !isVisible() ||
       !hasFocus() || !window()->isActiveWindow()) return;
    previousCursor=QCursor::pos();
    cursorCaptured=true;
    walkReticle->move(rect().center()-QPoint(10,10));
    walkReticle->show();
    QCursor::setPos(mapToGlobal(rect().center()));
    grabMouse(QCursor(Qt::BlankCursor));
    refreshCursorClip();
}

void Viewport::releaseCursor(bool restorePosition) {
    if(!cursorCaptured) return;
    cursorCaptured=false;
    walkReticle->hide();
    keys.clear();
#ifdef Q_OS_WIN
    if(nativeClip) {
        ClipCursor(nullptr);
        nativeClip=false;
        clippedRect={};
    }
#endif
    if(mouseGrabber()==this) releaseMouse();
    if(restorePosition && window()->isActiveWindow()) QCursor::setPos(previousCursor);
}

bool Viewport::event(QEvent *event) {
    switch(event->type()) {
    case QEvent::WindowDeactivate:
    case QEvent::Hide:
        keys.clear();
        releaseCursor();
        break;
    case QEvent::WindowActivate:
    case QEvent::Show:
        QTimer::singleShot(0,this,&Viewport::captureCursor);
        break;
    default:
        break;
    }
    return QOpenGLWidget::event(event);
}

bool Viewport::focusNextPrevChild(bool next) {
    return !cursorCaptured && QOpenGLWidget::focusNextPrevChild(next);
}

void Viewport::resetCamera() {
    const auto mode=scene.camera.mode;
    scene.camera=Camera{};
    setMode(mode);
}

void Viewport::mousePressEvent(QMouseEvent *event) {
    setFocus();
    button=event->button();
    initial=scene.camera.mode==ViewMode::Walk && cursorCaptured?QPointF(rect().center()):event->position();
    last=initial;
    dragAxis={};
    if(button!=Qt::LeftButton) return;
    const int hit=pick(initial);
    if(hit==remoteHit) {
        holdRemote();
        button=Qt::NoButton;
        return;
    }
    if(scene.camera.mode==ViewMode::Walk) return;
    if(const auto *o=scene.find(selected);o && !o->locked && tool==EditTool::Move) {
        const auto origin=o->position+QVector3D(0,.35f,0);
        float best=11;
        for(auto axis:axes) {
            const float d=segmentDistance(initial,project(origin+axis*.18f),project(origin+axis*.92f));
            if(d<best) { best=d; dragAxis=axis; }
        }
        if(!dragAxis.isNull()) return;
    }
    select(hit);
}

void Viewport::mouseMoveEvent(QMouseEvent *event) {
    Camera &camera=scene.camera;
    if(camera.mode==ViewMode::Walk) {
        if(!cursorCaptured) return;
        const QPoint center=mapToGlobal(rect().center());
        const QPoint delta=event->globalPosition().toPoint()-center;
        // Recentring makes relative aiming work even when the user keeps turning.
        // The event generated by warping back to the centre has no movement.
        if(delta.isNull()) return;
        camera.walkYaw=wrap(camera.walkYaw+delta.x()*.25f);
        camera.walkPitch=std::clamp(camera.walkPitch-delta.y()*.25f,-85.f,85.f);
        QCursor::setPos(center);
        update();
        return;
    }
    if(button==Qt::NoButton) return;
    const QPointF delta=event->position()-last;
    if(delta.isNull()) return;
    last=event->position();
    if(button==Qt::RightButton && camera.mode==ViewMode::Orbit) {
        camera.yaw=wrap(camera.yaw-float(delta.x())*.38f);
        camera.pitch=std::clamp(camera.pitch+float(delta.y())*.38f,-20.f,82.f);
    } else if(button==Qt::MiddleButton || (button==Qt::RightButton && camera.mode==ViewMode::Plan)) {
        const auto inverse=camera.view().inverted();
        auto &target=camera.mode==ViewMode::Plan?camera.planTarget:camera.target;
        const float distance=camera.mode==ViewMode::Plan?camera.planDistance:camera.distance;
        target+=inverse.mapVector(QVector3D(-delta.x(),delta.y(),0))*distance*.0015f;
        for(int i=0;i<3;++i) target[i]=std::clamp(target[i],-90.f,90.f);
    } else if(button==Qt::LeftButton && tool!=EditTool::Select) {
        auto *o=scene.find(selected);
        if(!o || o->locked) return;
        if(!dragging) {
            dragging=true;
            emit editStarted();
        }
        if(tool==EditTool::Move) {
            if(!dragAxis.isNull()) {
                const auto origin=o->position+QVector3D(0,.35f,0);
                const QPointF axis=project(origin+dragAxis)-project(origin);
                const auto squared=QPointF::dotProduct(axis,axis);
                if(squared>4) o->position+=dragAxis*float(QPointF::dotProduct(delta,axis)/squared);
            } else {
                // The free drag stays on the floor plane. The green handle moves vertically.
                const auto inverse=camera.view().inverted();
                auto right=inverse.mapVector({1,0,0}), up=inverse.mapVector({0,1,0});
                right.setY(0); up.setY(0);
                o->position+=(right.normalized()*float(delta.x())-up.normalized()*float(delta.y()))*camera.distance*.0021f;
            }
            for(int i=0;i<3;++i) o->position[i]=std::clamp(o->position[i],-100.f,100.f);
        } else if(tool==EditTool::Rotate) {
            o->rotation.setY(wrap(o->rotation.y()+float(delta.x())*.65f));
            if(camera.mode!=ViewMode::Plan) o->rotation.setX(wrap(o->rotation.x()+float(delta.y())*.65f));
        } else {
            float factor=std::exp(float(delta.x()-delta.y())*.009f);
            QVector3D scale=o->scale;
            if(uniformScale) {
                const float minimum=std::min({scale.x(),scale.y(),scale.z()});
                const float maximum=std::max({scale.x(),scale.y(),scale.z()});
                scale*=std::clamp(factor,.05f/minimum,20.f/maximum);
            } else {
                scale.setX(std::clamp(scale.x()*factor,.05f,20.f));
            }
            o->scale=scale;
        }
        emit objectChanged();
    }
    update();
}

void Viewport::mouseReleaseEvent(QMouseEvent *event) {
    if(event->button()!=button) return;
    button=Qt::NoButton;
    if(dragging) {
        dragging=false;
        emit editFinished();
    }
}

void Viewport::wheelEvent(QWheelEvent *event) {
    if(scene.camera.mode==ViewMode::Walk) return;
    float &distance=scene.camera.mode==ViewMode::Plan?scene.camera.planDistance:scene.camera.distance;
    distance=std::clamp(distance*std::exp(-event->angleDelta().y()/900.f),1.f,35.f);
    update();
    event->accept();
}

void Viewport::keyPressEvent(QKeyEvent *event) {
    const bool walkthrough=scene.camera.mode==ViewMode::Walk;
    if(event->key()==Qt::Key_X && (remoteState==RemoteState::Held || walkthrough)) {
        if(!event->isAutoRepeat() && (!walkthrough || cursorCaptured)) {
            if(remoteState==RemoteState::Held) releaseRemote();
            else if(remoteState!=RemoteState::OnTable || firstObject("table")) holdRemote();
        }
        event->accept();
        return;
    }
    if(event->key()==Qt::Key_M && remoteState==RemoteState::Held) {
        if(!event->isAutoRepeat()) {
            if(const auto *tv=firstObject("tv")) {
                if(poweredTv==tv->id) poweredTv=0;
                else {
                    poweredTv=tv->id;
                    uploadedCharacters=-1;
                    typing.start();
                }
                update();
            }
        }
        event->accept();
        return;
    }
    if(scene.camera.mode!=ViewMode::Walk) { QOpenGLWidget::keyPressEvent(event); return; }
    if(!event->isAutoRepeat() && cursorCaptured) {
        switch(event->key()) {
        case Qt::Key_T:
            setMode(beforeWalk);
            break;
        case Qt::Key_W: case Qt::Key_A: case Qt::Key_S: case Qt::Key_D:
            if(!(event->modifiers() & (Qt::ControlModifier|Qt::AltModifier|Qt::MetaModifier)))
                keys.insert(event->key());
            break;
        default:
            break;
        }
    }
    event->accept();
}

void Viewport::keyReleaseEvent(QKeyEvent *event) {
    if(!event->isAutoRepeat()) keys.remove(event->key());
    QOpenGLWidget::keyReleaseEvent(event);
}

void Viewport::focusOutEvent(QFocusEvent *event) {
    keys.clear();
    releaseCursor();
    button=Qt::NoButton;
    if(dragging) {
        dragging=false;
        emit editFinished();
    }
    QOpenGLWidget::focusOutEvent(event);
}

void Viewport::focusInEvent(QFocusEvent *event) {
    QOpenGLWidget::focusInEvent(event);
    captureCursor();
}

void Viewport::tick() {
    const float seconds=std::min(.05f,step.restart()/1000.f);
    if(!isVisible()) return;
    if(cursorCaptured) refreshCursorClip();
    bool animationChanged=remoteState==RemoteState::Falling;
    if(animationChanged) stepRemote(seconds);
    if(const auto *tv=std::as_const(scene).find(poweredTv);tv && tv->kind=="tv" &&
       scene.camera.mode!=ViewMode::Plan && uploadedCharacters<greetingText().size()) {
        animationChanged|=uploadedCharacters!=std::min(int(greetingText().size()),int(typing.elapsed()/135));
    }
    if(cursorCaptured && !keys.isEmpty()) {
        auto &camera=scene.camera;
        auto front=camera.forward();
        front.setY(0);
        front.normalize();
        const auto right=QVector3D::crossProduct(front,{0,1,0});
        const auto advance=front*(int(keys.contains(Qt::Key_W))-int(keys.contains(Qt::Key_S)))+
                           right*(int(keys.contains(Qt::Key_D))-int(keys.contains(Qt::Key_A)));
        const auto previous=camera.eye;
        camera.eye+=advance.normalized()*seconds*2.1f;
        camera.eye.setX(std::clamp(camera.eye.x(),-scene.room.x()/2+.23f,scene.room.x()/2-.23f));
        camera.eye.setY(std::clamp(camera.eye.y(),.5f,scene.room.y()-.15f));
        camera.eye.setZ(std::clamp(camera.eye.z(),-scene.room.z()/2+.23f,scene.room.z()/2-.23f));
        animationChanged|=previous!=camera.eye;
    }
    if(meter.elapsed()>=1000) {
        emit statsChanged(qRound(frameCount*1000./meter.restart()));
        frameCount=0;
    }
    if(animationChanged) update();
}

const Object *Viewport::firstObject(const QString &kind) const {
    for(const auto &object:std::as_const(scene.objects))
        if(object.kind==kind) return &object;
    return nullptr;
}

void Viewport::resetRemote() {
    remoteState=RemoteState::OnTable;
    poweredTv=0;
    typing.invalidate();
    uploadedCharacters=-1;
    update();
}

QMatrix4x4 Viewport::remoteTransform() const {
    QMatrix4x4 transform;
    if(remoteState==RemoteState::Held) {
        transform=scene.camera.view().inverted();
        transform.translate(.205f,-.165f,-.53f);
        transform.rotate(-23,0,0,1);
        transform.rotate(33,1,0,0);
    } else if(remoteState==RemoteState::OnTable) {
        if(const auto *table=firstObject("table")) transform=table->matrix();
        transform.translate(.23f,.499f,.238f);
        transform.rotate(-19,0,1,0);
    } else {
        transform.translate(remotePosition);
        transform.rotate(18,0,1,0);
        transform.rotate(remoteSpin,1,0,0);
    }
    return transform;
}

void Viewport::drawRemote(const QMatrix4x4 &transform,bool held) {
    auto part=[&](QVector3D position,QVector3D size,const char *color,float roll=0,bool round=true) {
        auto matrix=transform;
        matrix.translate(position);
        matrix.rotate(roll,0,0,1);
        matrix.scale(size);
        draw(matrix,QColor(color),"Phát sáng",false,round);
    };
    if(held) {
        part({.052f,-.037f,.062f},{.065f,.068f,.105f},"#ba795a",16);
        part({.075f,-.057f,.117f},{.089f,.079f,.101f},"#d6b79d",22);
        part({.100f,-.076f,.153f},{.097f,.083f,.091f},"#c8a488",22);
        for(int i=0;i<4;++i)
            part({-.040f,-.017f,-.026f+i*.026f},{.039f,.043f,.018f},"#dd9c79",12);
    }
    part({0,0,0},{.080f,.028f,.195f},"#263044");
    part({0,.015f,0},{.071f,.007f,.180f},"#374254");
    part({.018f,.022f,-.065f},{.023f,.010f,.024f},poweredTv?"#ff6358":"#d64143");
    part({0,.022f,-.023f},{.038f,.010f,.034f},"#7f91a4");
    part({0,.029f,-.023f},{.021f,.007f,.021f},"#3d5065");
    for(int row=0;row<3;++row) for(int column=-1;column<=1;++column)
        part({column*.017f,.022f,.013f+row*.023f},{.011f,.008f,.012f},"#adbac5");
    if(held) part({.043f,.022f,.046f},{.027f,.032f,.058f},"#edab87",-16);
}

void Viewport::drawTv(const Object &tv) {
    const int characters=std::min(int(greetingText().size()),int(typing.elapsed()/135));
    if(!tvTexture) {
        tvTexture=std::make_unique<QOpenGLTexture>(QOpenGLTexture::Target2D);
        tvTexture->setFormat(QOpenGLTexture::RGBA8_UNorm);
        tvTexture->setSize(1024,536);
        tvTexture->setMipLevels(1);
        tvTexture->allocateStorage(QOpenGLTexture::RGBA,QOpenGLTexture::UInt8);
        tvTexture->setMinMagFilters(QOpenGLTexture::Linear,QOpenGLTexture::Linear);
        tvTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
    }
    if(uploadedCharacters!=characters) {
        const auto card=greetingCard(characters);
        tvTexture->setData(QOpenGLTexture::RGBA,QOpenGLTexture::UInt8,card.constBits());
        uploadedCharacters=characters;
    }
    auto panel=tv.matrix();
    panel.translate(0,.51f,.052f);
    panel.scale(1.91f,1.f,.002f);
    tvTexture->bind(0);
    shader->setUniformValue("tvScreen",0);
    shader->setUniformValue(modelUniform,panel);
    shader->setUniformValue(normalUniform,panel.normalMatrix());
    shader->setUniformValue(materialUniform,7);
    glDrawArrays(GL_TRIANGLES,0,cubeCount);
    tvTexture->release(0);
}

void Viewport::updateRemoteHint() {
    QString text;
    QPointF marker;
    if(remoteState==RemoteState::Held) {
        if(scene.camera.mode!=ViewMode::Plan) text="M  Bật/tắt TV     X  Thả remote";
        if(scene.camera.mode==ViewMode::Walk) text+="     T  Thoát tham quan";
    } else if(remoteState==RemoteState::Falling) {
        text=scene.camera.mode==ViewMode::Walk?"X  Nhặt lại remote     T  Thoát tham quan":"Đang thả remote…";
    } else if(scene.camera.mode==ViewMode::Walk) {
        text="W A S D  Di chuyển";
        if(remoteState!=RemoteState::OnTable || firstObject("table")) text+="     X  Nhặt remote";
        text+="     T  Thoát tham quan";
    } else if(remoteState!=RemoteState::OnTable || firstObject("table")) {
        const auto center=remoteTransform().map(QVector3D());
        const auto clip=viewProjection()*QVector4D(center,1);
        marker=project(center);
        if(clip.w()>0 && rect().adjusted(70,98,-70,-10).contains(marker.toPoint()) && pick(marker)==remoteHit)
            text="↓  Nhấp để nhặt remote";
    }
    if(text.isEmpty()) {
        remoteHint->hide();
        return;
    }
    if(remoteHint->text()!=text) {
        remoteHint->setText(text);
        remoteHint->adjustSize();
    }
    const int w=remoteHint->width();
    const bool tagged=!marker.isNull();
    const int x=tagged?std::clamp(qRound(marker.x())-w/2,4,std::max(4,width()-w-4)):18;
    const int y=tagged?qRound(marker.y())-remoteHint->height()-4:height()-remoteHint->height()-15;
    remoteHint->move(x,y);
    remoteHint->show();
}

void Viewport::holdRemote() {
    remoteState=RemoteState::Held;
    select(0);
    if(scene.camera.mode!=ViewMode::Walk) {
        if(const auto *tv=firstObject("tv")) {
            const auto transform=tv->matrix();
            const auto center=transform.map(QVector3D(0,.51f,.055f));
            QVector3D normal=transform.mapVector({0,0,1});
            normal.setY(0);
            normal.normalize();
            if(normal.isNull()) normal={-1,0,0};
            const auto cameraRight=QVector3D::crossProduct(-normal,{0,1,0});
            auto &camera=scene.camera;
            camera.eye=center+normal*2.45f+cameraRight*.26f;
            camera.eye.setX(std::clamp(camera.eye.x(),-scene.room.x()/2+.25f,scene.room.x()/2-.25f));
            camera.eye.setZ(std::clamp(camera.eye.z(),-scene.room.z()/2+.25f,scene.room.z()/2-.25f));
            camera.eye.setY(std::clamp(center.y()+.13f,.54f,scene.room.y()-.2f));
            const auto aim=(center-camera.eye).normalized();
            camera.walkYaw=qRadiansToDegrees(std::atan2(aim.z(),aim.x()));
            camera.walkPitch=qRadiansToDegrees(std::asin(aim.y()));
        }
        setMode(ViewMode::Walk);
    }
    update();
}

void Viewport::releaseRemote() {
    remotePosition=remoteTransform().map(QVector3D());
    remoteVelocity=scene.camera.forward()*1.05f+QVector3D(0,.2f,0);
    remoteSpin=33;
    remoteState=RemoteState::Falling;
    update();
}

void Viewport::stepRemote(float seconds) {
    if(remoteState!=RemoteState::Falling) return;
    remoteVelocity.setY(remoteVelocity.y()-9.81f*seconds);
    remotePosition+=remoteVelocity*seconds;
    remoteSpin+=385.f*seconds;
    const float x=std::max(.06f,scene.room.x()/2-.1f),z=std::max(.06f,scene.room.z()/2-.1f);
    remotePosition.setX(std::clamp(remotePosition.x(),-x,x));
    remotePosition.setZ(std::clamp(remotePosition.z(),-z,z));
    float height=.055f;
    if(const auto *table=firstObject("table")) {
        const QVector3D top=table->matrix().map(QVector3D(0,.4825f,0));
        const auto local=table->matrix().inverted().map(QVector3D(remotePosition.x(),top.y(),remotePosition.z()));
        if(std::abs(local.x())<.71f && std::abs(local.z())<.39f)
            height=std::max(height,top.y()+.016f);
    }
    if(remotePosition.y()<=height) {
        remotePosition.setY(height);
        remoteSpin=0;
        remoteState=RemoteState::Dropped;
    }
}
