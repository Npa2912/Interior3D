#include "Editor.h"

#include <QAction>
#include <QButtonGroup>
#include <QCheckBox>
#include <QCloseEvent>
#include <QColorDialog>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStackedLayout>
#include <QStyle>
#include <QTabWidget>
#include <QToolButton>
#include <algorithm>
#include <functional>
#include <limits>

namespace {
class SceneEdit final : public QUndoCommand {
public:
    SceneEdit(const QString &label, std::function<void(bool)> apply):QUndoCommand(label), apply(std::move(apply)) {}
    void undo() override { apply(false); }
    void redo() override { apply(true); }
private:
    std::function<void(bool)> apply;
};

QLabel *label(const QString &title,const char *name=nullptr) {
    auto *item=new QLabel(title);
    if(name) item->setObjectName(name);
    return item;
}

QToolButton *toolButton(const QString &title,const QIcon &icon) {
    auto *button=new QToolButton;
    button->setText(title);
    button->setIcon(icon);
    button->setIconSize({28,28});
    button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    button->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    button->setMinimumHeight(69);
    return button;
}

QIcon icon(const QString &symbol,QColor color=QColor("#edf2f7")) {
    QPixmap image(64,64);
    image.fill(Qt::transparent);
    QPainter p(&image);
    p.setRenderHint(QPainter::Antialiasing);
    p.scale(2,2);
    p.setPen(QPen(color,1.6,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
    auto lines=[&](std::initializer_list<QPointF> list) {
        QPolygonF poly;
        for(auto point:list) poly.append(point);
        p.drawPolyline(poly);
    };
    if(symbol=="cube") {
        lines({{16,3},{28,9},{28,22},{16,29},{4,22},{4,9},{16,3}});
        lines({{4,9},{16,16},{28,9}});
        p.drawLine(16,16,16,29);
    } else if(symbol=="new") {
        lines({{8,3},{19,3},{25,9},{25,28},{8,28},{8,3}});
        lines({{19,3},{19,9},{25,9}});
    } else if(symbol=="folder") {
        lines({{4,25},{4,7},{12,7},{15,10},{28,10},{25,25},{4,25},{9,13},{26,13}});
    } else if(symbol=="save") {
        p.drawRoundedRect(QRectF(5,4,22,24),2,2);
        p.drawRect(QRectF(10,4,11,8));
        p.drawRect(QRectF(10,19,12,9));
    } else if(symbol=="cursor") {
        lines({{8,3},{25,16},{17,17},{21,26},{17,28},{12,19},{8,24},{8,3}});
    } else if(symbol=="move" || symbol=="scale") {
        if(symbol=="scale") {p.translate(16,16);p.rotate(45);p.translate(-16,-16);}
        for(int i=0;i<4;++i) {
            lines({{12,7},{16,3},{20,7}});
            p.drawLine(16,4,16,16);
            p.translate(16,16);p.rotate(90);p.translate(-16,-16);
        }
    } else if(symbol=="undo"||symbol=="redo"||symbol=="rotate") {
        if(symbol=="undo") {p.translate(32,0);p.scale(-1,1);}
        if(symbol=="rotate") p.drawArc(QRectF(5,5,22,22),35*16,293*16);
        else p.drawArc(QRectF(6,11,20,15),-90*16,180*16);
        lines({{20,6},{26,11},{20,16}});
        if(symbol!="rotate") p.drawLine(16,11,26,11);
    } else if(symbol=="copy") {
        p.drawRoundedRect(QRectF(11,10,15,17),2,2);
        lines({{8,22},{5,22},{5,5},{20,5},{20,7}});
    } else if(symbol=="trash") {
        lines({{6,9},{26,9}});lines({{12,8},{12,5},{20,5},{20,8}});
        lines({{9,11},{11,27},{21,27},{23,11}});
        p.drawLine(14,14,18,22);p.drawLine(18,14,14,22);
    } else if(symbol=="image") {
        p.drawRoundedRect(QRectF(4,6,24,21),2,2);
        p.drawEllipse(QPointF(11,12),2,2);
        lines({{5,24},{12,17},{17,21},{23,14},{28,20}});
    } else if(symbol=="eye") {
        QPainterPath eye(QPointF(3,16));eye.quadTo(16,-1,29,16);eye.quadTo(16,33,3,16);
        p.drawPath(eye);p.drawEllipse(QPointF(16,16),4,4);
    } else if(symbol=="sofa") {
        lines({{4,21},{4,15},{8,15},{9,19},{23,19},{24,15},{28,15},{28,24},{4,24},{4,21}});
        lines({{7,15},{7,9},{25,9},{25,15}});p.drawLine(7,25,7,28);p.drawLine(25,25,25,28);
    } else if(symbol=="layers") {
        lines({{3,12},{16,5},{29,12},{16,19},{3,12}});
        lines({{3,17},{16,24},{29,17}});lines({{3,22},{16,29},{29,22}});
    } else if(symbol=="home") {
        lines({{3,15},{16,4},{29,15},{25,15},{25,27},{19,27},{19,20},{13,20},{13,27},{7,27},{7,15}});
    } else if(symbol=="camera") {
        lines({{4,10},{10,10},{12,6},{20,6},{22,10},{28,10},{28,26},{4,26},{4,10}});
        p.drawEllipse(QPointF(16,18),5,5);
    } else if(symbol=="sun"||symbol=="gear") {
        p.drawEllipse(QPointF(16,16),symbol=="sun"?6:8,symbol=="sun"?6:8);
        if(symbol=="gear") p.drawEllipse(QPointF(16,16),3,3);
        for(int i=0;i<8;++i) {
            p.drawLine(16,2,16,7);
            p.translate(16,16);p.rotate(45);p.translate(-16,-16);
        }
    } else if(symbol=="plan") {
        p.drawRect(QRectF(5,5,22,22));lines({{15,5},{15,15},{27,15}});lines({{5,21},{15,21},{15,27}});
    } else if(symbol=="walk") {
        p.drawEllipse(QPointF(18,5),2.6,2.6);
        lines({{13,28},{18,21},{15,17},{18,9},{22,15},{26,17}});
        lines({{10,16},{13,11},{18,9}});lines({{15,17},{12,23},{6,27}});
    } else if(symbol=="fullscreen") {
        for(int i=0;i<4;++i) {
            lines({{5,11},{5,5},{11,5}});
            p.translate(16,16);p.rotate(90);p.translate(-16,-16);
        }
    } else {
        p.drawEllipse(QPointF(16,16),12,12);
        QFont font=p.font();font.setPixelSize(18);p.setFont(font);
        p.drawText(QRectF(0,0,32,31),Qt::AlignCenter,"?");
    }
    return QIcon(image);
}

QPixmap brandLogo() {
    QPixmap image(96,96);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(3,3);
    painter.setPen(Qt::NoPen);
    const QPointF tip(16,1), left(2,9), right(30,9), middle(16,17);
    painter.setBrush(QColor("#6ccefa"));
    painter.drawPolygon(QPolygonF{tip,right,middle,left});
    painter.setBrush(QColor("#167ccd"));
    painter.drawPolygon(QPolygonF{left,middle,{16,31},{2,23}});
    painter.setBrush(QColor("#35b4f6"));
    painter.drawPolygon(QPolygonF{middle,right,{30,23},{16,31}});
    painter.setBrush(QColor("#393e45"));
    painter.drawPolygon(QPolygonF{{16,7},{24,12},{24,21},{16,26},{8,21},{8,12}});
    return image;
}

QPixmap thumbnail(const CatalogItem &entry) {
    QPixmap result(144,116);
    result.fill(QColor("#cbd0d8"));
    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing);
    Object o;
    o.kind=entry.kind;
    o.color=entry.color;
    QMatrix4x4 view;
    view.lookAt({2,1.65f,3},{0,0,0},{0,1,0});
    struct Face { QPolygonF polygon; float depth; QColor color; };
    QVector<Face> faces;
    QRectF bounds;
    const QVector3D axes[]{{1,0,0},{0,1,0},{0,0,1}};
    for(const auto &part:geometry(o,{7,3,6})) {
        const auto model=part.matrix();
        for(int a=0;a<3;++a) for(int sign:{-1,1}) {
            auto normal=view.mapVector(model.normalMatrix()(a,a)*axes[a]*sign);
            if(normal.z()<=0) continue;
            auto c=axes[a]*sign*.5f,u=axes[(a+1)%3]*.5f,v=axes[(a+2)%3]*.5f;
            Face face{{},0,part.color.lighter(a==1?114:(a==0?87:100))};
            for(auto point:{c-u-v,c+u-v,c+u+v,c-u+v}) {
                auto p=view.map(model.map(point));
                face.depth+=p.z();
                face.polygon.append({p.x(),-p.y()});
            }
            bounds=bounds.united(face.polygon.boundingRect());
            faces.append(face);
        }
    }
    std::sort(faces.begin(),faces.end(),[](const Face &a,const Face &b){return a.depth<b.depth;});
    const auto scale=std::min(119./std::max(.1,bounds.width()),85./std::max(.1,bounds.height()));
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(59,51,46,45));
    painter.drawEllipse(QRectF(24,94,96,13));
    painter.translate(72,57);
    painter.scale(scale,scale);
    painter.translate(-bounds.center());
    for(const auto &face:faces) {
        painter.setBrush(face.color);
        painter.drawPolygon(face.polygon);
    }
    return result;
}

QDoubleSpinBox *number(double min,double max,double step) {
    auto *field=new QDoubleSpinBox;
    field->setFocusPolicy(Qt::StrongFocus);
    field->setRange(min,max);
    field->setSingleStep(step);
    field->setDecimals(2);
    field->setKeyboardTracking(false);
    field->setAlignment(Qt::AlignCenter);
    field->setButtonSymbols(QAbstractSpinBox::NoButtons);
    return field;
}

const QStringList materials{"Mặc định","Gỗ","Kim loại","Vải","Kính"};
const QStringList categoryNames{"Tất cả","Phòng khách","Phòng ngủ","Phòng bếp","Phòng tắm","Trang trí"};
}

Editor::Editor(QWidget *parent):QMainWindow(parent) {
    setWindowTitle("Interior3D");
    resize(1600,900);
    setMinimumSize(960,670);
    history.setUndoLimit(80);
    buildUi();
    for(const auto &o:scene.objects) if(o.kind=="table") viewport->select(o.id);
    refresh();
}

void Editor::buildUi() {
    auto *root=new QWidget;
    root->setObjectName("root");
    setCentralWidget(root);
    auto *layout=new QVBoxLayout(root);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    auto *header=new QWidget;
    header->setObjectName("header");
    auto *titleRow=new QHBoxLayout(header);
    titleRow->setContentsMargins(34,11,20,11);
    auto *logo=label("");
    logo->setPixmap(brandLogo().scaled(51,51,Qt::KeepAspectRatio,Qt::SmoothTransformation));
    titleRow->addWidget(logo);
    titleRow->addWidget(label("Interior3D","brand"));
    titleRow->addSpacing(15);
    auto *titleDivider=new QFrame;
    titleDivider->setObjectName("titleDivider");
    titleDivider->setFixedSize(1,34);
    titleRow->addWidget(titleDivider);
    auto *projectTitle=label("XÂY DỰNG ỨNG DỤNG THIẾT KẾ VÀ BỐ TRÍ NỘI THẤT 3D TƯƠNG TÁC BẰNG <span style='color:#56c3ef'>C++/OPENGL</span>","title");
    projectTitle->setWordWrap(true);
    titleRow->addWidget(projectTitle,1);
    layout->addWidget(header);

    auto *bar=new QWidget;
    bar->setObjectName("toolbar");
    auto *tools=new QHBoxLayout(bar);
    tools->setContentsMargins(15,6,15,6);
    tools->setSpacing(4);
    auto action=[&](const QString &name,const QString &shape,const QKeySequence &key,const std::function<void()> &operation) {
        auto *a=new QAction(icon(shape),name,this);
        if(!key.isEmpty()) a->setShortcut(key);
        addAction(a);
        connect(a,&QAction::triggered,this,operation);
        auto *b=toolButton(name,a->icon());
        b->setDefaultAction(a);
        b->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        tools->addWidget(b,1);
        return a;
    };
    auto divider=[&] {
        auto *line=new QFrame;
        line->setFrameShape(QFrame::VLine);
        line->setObjectName("separator");
        tools->addWidget(line);
    };
    action("Mới","new",QKeySequence::New,[this]{if(confirmProjectChange()) resetScene(Scene::livingRoom(),{});});
    action("Mở","folder",QKeySequence::Open,[this]{chooseOpen();});
    action("Lưu","save",QKeySequence::Save,[this]{chooseSave();});
    action("Lưu như","save",QKeySequence::SaveAs,[this]{chooseSave(true);});
    divider();
    auto *undo=action("Hoàn tác","undo",QKeySequence::Undo,[this]{history.undo();});
    auto *redo=action("Làm lại","redo",QKeySequence::Redo,[this]{history.redo();});
    undo->setEnabled(false);
    redo->setEnabled(false);
    connect(&history,&QUndoStack::canUndoChanged,undo,&QAction::setEnabled);
    connect(&history,&QUndoStack::canRedoChanged,redo,&QAction::setEnabled);
    divider();
    auto *editTools=new QButtonGroup(this);
    const QStringList toolNames{"Chọn","Di chuyển","Xoay","Tỷ lệ"};
    const QStringList toolIcons{"cursor","move","rotate","scale"};
    for(int i=0;i<4;++i) {
        auto *b=toolButton(toolNames[i],icon(toolIcons[i]));
        b->setObjectName("modeTool");
        b->setCheckable(true);
        editTools->addButton(b,i);
        tools->addWidget(b,1);
    }
    editTools->button(0)->setChecked(true);
    connect(editTools,&QButtonGroup::idClicked,this,[this](int i){viewport->setTool(EditTool(i));});
    duplicateAction=action("Sao chép","copy",QKeySequence("Ctrl+D"),[this]{
        if(scene.objects.size()>=500) return;
        edit("Sao chép",[this]{const int id=scene.duplicate(viewport->selection());if(id) viewport->select(id);});
    });
    duplicateAction->setObjectName("duplicateAction");
    deleteAction=action("Xóa","trash",QKeySequence(Qt::Key_Delete),[this]{
        edit("Xóa",[this]{if(scene.remove(viewport->selection())) viewport->select(0);});
    });
    deleteAction->setObjectName("deleteAction");
    // A text field uses Delete to edit its contents; scene deletion is contextual.
    deleteAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    removeAction(deleteAction);
    divider();
    action("Render","image",QKeySequence("Ctrl+E"),[this]{chooseExport();});
    action("Xem trước","eye",{},[this]{showPreview();});
    divider();
    action("Cài đặt","gear",{},[this]{showSettings();});
    action("Trợ giúp","help",QKeySequence::HelpContents,[this]{showHelp();});
    layout->addWidget(bar);

    auto *splitter=new QSplitter;
    splitter->setChildrenCollapsible(false);
    auto *left=new QWidget;
    left->setObjectName("sidebar");
    left->setMinimumWidth(156);
    left->setMaximumWidth(254);
    auto *leftLayout=new QVBoxLayout(left);
    leftLayout->setContentsMargins(7,15,7,10);
    leftLayout->setSpacing(6);
    navigation=new QButtonGroup(this);
    const QStringList navNames{"Trang chủ","Thiết kế 3D","Bố trí nội thất","Vật liệu && Màu sắc","Ánh sáng","Camera","Render ảnh","Quản lý dự án"};
    const QStringList navIcons{"home","cube","sofa","layers","sun","camera","image","folder"};
    for(int i=0;i<navNames.size();++i) {
        auto *b=new QPushButton(icon(navIcons[i]),navNames[i]);
        b->setObjectName("nav");
        b->setIconSize({27,27});
        b->setCheckable(true);
        b->setMinimumHeight(51);
        navigation->addButton(b,i);
        leftLayout->addWidget(b);
    }
    navigation->button(1)->setChecked(true);
    leftLayout->addStretch();
    auto *mark=label("");
    mark->setPixmap(icon("sofa",QColor("#8b9caa")).pixmap(103,103));
    mark->setAlignment(Qt::AlignCenter);
    leftLayout->addWidget(mark);
    splitter->addWidget(left);

    auto *stage=new QWidget;
    stage->setObjectName("stage");
    auto *stack=new QStackedLayout(stage);
    stack->setStackingMode(QStackedLayout::StackAll);
    viewport=new Viewport(scene);
    viewport->setObjectName("viewport");
    viewport->addAction(deleteAction);
    stack->addWidget(viewport);
    auto *overlay=new QWidget;
    auto *overlayLayout=new QVBoxLayout(overlay);
    overlayLayout->setContentsMargins(14,13,14,13);
    auto *top=new QHBoxLayout;
    viewMode=new QComboBox;
    viewMode->setObjectName("viewMode");
    viewMode->addItem(icon("cube"),"Chế độ: Thiết kế 3D");
    viewMode->addItem(icon("plan"),"Chế độ: Mặt bằng 2D");
    viewMode->addItem(icon("walk"),"Chế độ: Tham quan FPS");
    top->addWidget(viewMode);
    top->addStretch();
    auto *grid=new QToolButton;
    grid->setText("▦");
    grid->setToolTip("Lưới mặt sàn");
    grid->setCheckable(true);
    connect(grid,&QToolButton::toggled,viewport,&Viewport::setGrid);
    top->addWidget(grid);
    auto *reset=new QToolButton;
    reset->setIcon(icon("cube"));
    reset->setToolTip("Đặt lại camera");
    connect(reset,&QToolButton::clicked,viewport,&Viewport::resetCamera);
    top->addWidget(reset);
    auto *full=new QToolButton;
    full->setIcon(icon("fullscreen"));
    full->setToolTip("Toàn màn hình");
    auto toggleFull=[this]{if(isFullScreen()) showNormal();else showFullScreen();};
    connect(full,&QToolButton::clicked,this,toggleFull);
    top->addWidget(full);
    // The overlay occupies only the top strip; mouse events below go to OpenGL.
    overlayLayout->addLayout(top);
    overlay->setFixedHeight(61);
    overlay->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    stack->addWidget(overlay);
    stack->setAlignment(overlay,Qt::AlignTop);
    stack->setCurrentWidget(overlay);
    splitter->addWidget(stage);

    auto *right=new QSplitter(Qt::Vertical);
    right->setChildrenCollapsible(false);
    right->setMinimumWidth(306);
    right->setMaximumWidth(510);
    auto *tabs=new QTabWidget;
    tabs->setObjectName("libraryTabs");
    auto *catalogPanel=new QWidget;
    catalogPanel->setObjectName("paper");
    auto *catalogLayout=new QVBoxLayout(catalogPanel);
    catalogLayout->setContentsMargins(9,9,9,8);
    search=new QLineEdit;
    search->setObjectName("catalogSearch");
    search->setPlaceholderText("Tìm kiếm (ví dụ: sofa, bàn, ghế...)");
    search->setClearButtonEnabled(true);
    catalogLayout->addWidget(search);
    auto *catalogRow=new QHBoxLayout;
    auto *categoryList=new QVBoxLayout;
    categoryList->setSpacing(3);
    categories=new QButtonGroup(this);
    for(int i=0;i<categoryNames.size();++i) {
        auto *b=new QPushButton(icon(i==0?"cube":i==5?"layers":"sofa",QColor("#252b33")),categoryNames.at(i));
        b->setObjectName("category");
        b->setCheckable(true);
        categories->addButton(b,i);
        categoryList->addWidget(b);
    }
    categories->button(0)->setChecked(true);
    categoryList->addStretch();
    catalogRow->addLayout(categoryList);
    library=new QListWidget;
    library->setObjectName("catalog");
    library->setViewMode(QListView::IconMode);
    library->setResizeMode(QListView::Adjust);
    library->setMovement(QListView::Static);
    library->setWordWrap(true);
    library->setUniformItemSizes(true);
    library->setGridSize({76,76});
    library->setIconSize({66,53});
    catalogRow->addWidget(library,1);
    catalogLayout->addLayout(catalogRow,1);
    tabs->addTab(catalogPanel,icon("sofa"),"Thư viện nội thất");
    auto *palette=new QWidget;
    palette->setObjectName("paper");
    auto *paletteLayout=new QVBoxLayout(palette);
    for(const QString &name:materials) {
        auto *b=new QPushButton(name);
        connect(b,&QPushButton::clicked,this,[this,name]{
            if(fields->isEnabled()) material->setCurrentText(name);
        });
        paletteLayout->addWidget(b);
    }
    auto *surfaceList=new QPushButton("Chọn tường, sàn hoặc đồ vật…");
    connect(surfaceList,&QPushButton::clicked,this,&Editor::showObjects);
    paletteLayout->addWidget(surfaceList);
    paletteLayout->addStretch();
    tabs->addTab(palette,icon("layers"),"Vật liệu");
    right->addWidget(tabs);

    auto *properties=new QWidget;
    properties->setObjectName("paper");
    auto *props=new QVBoxLayout(properties);
    props->setContentsMargins(12,10,12,12);
    props->addWidget(label("◇  Thuộc tính đối tượng","paperTitle"));
    fields=new QWidget;
    auto *form=new QFormLayout(fields);
    form->setContentsMargins(0,0,0,0);
    auto *nameRow=new QHBoxLayout;
    objectName=new QLineEdit;
    objectName->setObjectName("objectName");
    objectName->setMaxLength(160);
    locked=new QCheckBox("Khóa");
    locked->setObjectName("objectLock");
    nameRow->addWidget(objectName,1);
    nameRow->addWidget(locked);
    form->addRow(nameRow);
    const QStringList names{"Vị trí (X, Y, Z)","Xoay (°)","Tỷ lệ"};
    for(int row=0;row<3;++row) {
        auto *line=new QHBoxLayout;
        for(int col=0;col<3;++col) {
            auto *f=number(row==2?.05:row==1?-360:-100,row==2?20:row==1?360:100,row==1?5:.1);
            f->setObjectName(QString("transform%1%2").arg(row).arg(col));
            f->setAccessibleName(names[row]+QString(" %1").arg(QChar::fromLatin1("XYZ"[col])));
            f->setMinimumWidth(42);
            transform[row][col]=f;
            line->addWidget(f,1);
            connect(f,&QDoubleSpinBox::valueChanged,this,[this,row,col](double value){
                if(syncing) return;
                auto *o=scene.find(viewport->selection());
                if(!o || o->locked) return;
                edit("Chỉnh thuộc tính",[&]{
                    if(row==0) o->position[col]=float(value);
                    else if(row==1) o->rotation[col]=float(value);
                    else if(uniform->isChecked()) {
                        float min=std::min({o->scale.x(),o->scale.y(),o->scale.z()});
                        float max=std::max({o->scale.x(),o->scale.y(),o->scale.z()});
                        o->scale*=std::clamp(float(value)/o->scale[col],.05f/min,20.f/max);
                    } else o->scale[col]=float(value);
                });
            });
        }
        form->addRow(names[row],line);
    }
    uniform=new QCheckBox("Giữ tỷ lệ XYZ");
    connect(uniform,&QCheckBox::toggled,viewport,&Viewport::setUniformScale);
    form->addRow(QString(),uniform);
    material=new QComboBox;
    material->setObjectName("objectMaterial");
    material->addItems(materials);
    form->addRow("Vật liệu",material);
    objectColor=new QPushButton("Chọn màu…");
    objectColor->setObjectName("objectColor");
    form->addRow("Màu sắc",objectColor);
    props->addWidget(fields);
    props->addStretch();
    right->addWidget(properties);
    right->setSizes({344,254});
    splitter->addWidget(right);
    splitter->setStretchFactor(0,0);
    splitter->setStretchFactor(1,1);
    splitter->setStretchFactor(2,0);
    splitter->setSizes({216,958,396});
    auto *body=new QHBoxLayout;
    body->setContentsMargins(7,10,7,10);
    body->addWidget(splitter);
    layout->addLayout(body,1);

    auto *footer=new QWidget;
    footer->setObjectName("footer");
    auto *status=new QHBoxLayout(footer);
    status->setContentsMargins(19,6,15,6);
    projectLabel=label("");
    projectLabel->setMinimumWidth(180);
    projectLabel->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
    status->addWidget(projectLabel,1);
    views=new QButtonGroup(this);
    const QStringList captions{"Xem 3D","Xem 2D (Mặt bằng)","Tham quan (FPS)"};
    const QStringList symbols{"cube","plan","walk"};
    for(int i=0;i<3;++i) {
        auto *b=new QPushButton(icon(symbols[i]),captions[i]);
        b->setObjectName("viewButton");
        b->setIconSize({24,24});
        b->setMinimumHeight(46);
        b->setCheckable(true);
        views->addButton(b,i);
        status->addWidget(b);
    }
    views->button(0)->setChecked(true);
    status->addStretch(1);
    fpsLabel=label("FPS: …","fps");
    ratioLabel=label("");
    status->addWidget(fpsLabel);
    status->addSpacing(13);
    status->addWidget(ratioLabel);
    auto *statusFull=new QToolButton;
    statusFull->setIcon(icon("fullscreen"));
    statusFull->setToolTip("Toàn màn hình (F11)");
    connect(statusFull,&QToolButton::clicked,this,toggleFull);
    status->addWidget(statusFull);
    auto *fullAction=new QAction(this);
    fullAction->setShortcut(QKeySequence(Qt::Key_F11));
    addAction(fullAction);
    connect(fullAction,&QAction::triggered,this,toggleFull);
    layout->addWidget(footer);
    connect(viewport,&Viewport::statsChanged,this,[this](int fps){
        refreshProjectLabel();
        fpsLabel->setText(fps>0 ? QString("FPS: %1").arg(fps) : QString("Đang nghỉ"));
        ratioLabel->setText(QString("%1:1").arg(double(viewport->width())/std::max(1,viewport->height()),0,'f',2));
    });

    connect(views,&QButtonGroup::idClicked,this,[this](int i){viewport->setMode(ViewMode(i));});
    connect(viewMode,&QComboBox::currentIndexChanged,this,[this](int i){viewport->setMode(ViewMode(i));});
    connect(viewport,&Viewport::modeChanged,this,[this](ViewMode mode){
        QSignalBlocker block(viewMode);
        viewMode->setCurrentIndex(int(mode));
        views->button(int(mode))->setChecked(true);
        refreshProjectLabel();
    });
    connect(navigation,&QButtonGroup::idClicked,this,[this,tabs](int i){
        switch(i) {
        case 0: showHelp(); break;
        case 1: viewport->setMode(ViewMode::Orbit); tabs->setCurrentIndex(0); break;
        case 2: tabs->setCurrentIndex(0); showObjects(); break;
        case 3: tabs->setCurrentIndex(1); break;
        case 4: showLights(); break;
        case 5: showCamera(); break;
        case 6: chooseExport(); break;
        case 7: {
            QMessageBox box(this);
            box.setWindowTitle("Quản lý dự án");
            box.setText(path.isEmpty()?"Dự án chưa được lưu.":path);
            auto *open=box.addButton("Mở dự án…",QMessageBox::ActionRole);
            auto *save=box.addButton("Lưu dự án",QMessageBox::ActionRole);
            box.addButton("Đóng",QMessageBox::RejectRole);
            box.exec();
            if(box.clickedButton()==open) chooseOpen();
            if(box.clickedButton()==save) chooseSave();
            break;
        }
        }
    });
    connect(search,&QLineEdit::textChanged,this,&Editor::refreshLibrary);
    connect(categories,&QButtonGroup::idClicked,this,&Editor::refreshLibrary);
    auto *addSelected=new QAction(library);
    addSelected->setShortcuts({QKeySequence(Qt::Key_Return),QKeySequence(Qt::Key_Enter)});
    addSelected->setShortcutContext(Qt::WidgetShortcut);
    library->addAction(addSelected);
    connect(addSelected,&QAction::triggered,this,[this]{
        if(const auto *item=library->currentItem()) addFurniture(item->data(Qt::UserRole).toString());
    });
    connect(library,&QListWidget::itemClicked,this,[this](QListWidgetItem *item){addFurniture(item->data(Qt::UserRole).toString());});
    connect(viewport,&Viewport::selectionChanged,this,[this]{refresh();});
    connect(viewport,&Viewport::objectChanged,this,&Editor::refresh);
    connect(viewport,&Viewport::editStarted,this,[this]{dragBefore=scene;dragBefore.objects.detach();draggingObject=true;});
    connect(viewport,&Viewport::editFinished,this,[this]{
        if(!draggingObject) return;
        draggingObject=false;
        record("Biến đổi đối tượng",std::move(dragBefore),viewport->selection());
    });
    connect(&history,&QUndoStack::cleanChanged,this,[this]{refresh();});
    connect(objectName,&QLineEdit::editingFinished,this,[this]{
        const QString name=objectName->text().trimmed();
        auto *o=scene.find(viewport->selection());
        if(o && !o->locked && !name.isEmpty() && name!=o->name) edit("Đổi tên",[&]{o->name=name;});
        refresh();
    });
    connect(locked,&QCheckBox::toggled,this,[this](bool value){
        auto *o=scene.find(viewport->selection());
        if(!syncing && o) edit("Khóa đối tượng",[&]{o->locked=value;});
    });
    connect(material,&QComboBox::currentTextChanged,this,[this](const QString &value){
        auto *o=scene.find(viewport->selection());
        if(!syncing && o && !o->locked && value!=o->material) edit("Đổi vật liệu",[&]{o->material=value;});
    });
    connect(objectColor,&QPushButton::clicked,this,[this]{
        auto *o=scene.find(viewport->selection());
        if(!o || o->locked) return;
        const QColor color=QColorDialog::getColor(o->color,this,"Màu đối tượng");
        if(color.isValid() && color!=o->color) edit("Đổi màu",[&]{o->color=color;});
    });
    setStyleSheet(R"QSS(
QMainWindow, QWidget#root { background:#4d525b; color:#f4f6f9; font-size:14px; }
QWidget#header, QWidget#toolbar { background:#383b40; color:#fff; border-bottom:1px solid #676e78; }
QWidget#header { border-bottom:1px solid #58a9d7; }
QFrame#titleDivider { background:#8e959e; }
QLabel#brand { color:#f7f9fc; font-size:30px; font-weight:700; }
QLabel#title { color:#f7f8fa; font-size:21px; font-weight:700; padding-left:12px; }
QToolButton { color:#f3f6fb; background:#40454c; border:1px solid transparent; border-radius:8px; padding:3px; }
QWidget#toolbar QToolButton { background:transparent; font-size:14px; }
QToolButton:hover, QWidget#toolbar QToolButton:hover { background:#526172; }
QToolButton:checked, QWidget#toolbar QToolButton:checked, QPushButton#nav:checked, QPushButton#viewButton:checked { background:#087cf0; border:1px solid #42adff; color:white; }
QToolButton:pressed, QPushButton:pressed { background:#0864bd; }
QToolButton:disabled { color:#a0a1a5; }
QFrame#separator { color:#646c75; margin-top:10px; margin-bottom:10px; }
QSplitter::handle { background:#4d525b; width:8px; height:6px; }
QWidget#sidebar { background:#383f47; border-radius:15px; }
QPushButton#nav { text-align:left; color:#f2f4f8; background:#383f47; border:1px solid transparent; border-radius:11px; padding:7px 10px; font-size:14px; }
QPushButton#nav:hover { background:#485660; }
QWidget#stage { background:#26313b; border:1px solid #a8afb7; border-radius:16px; }
QComboBox#viewMode { color:white; background:#44484f; border:1px solid #757983; border-radius:8px; padding:5px 10px; }
QWidget#paper { color:#161d29; background:#d9dde5; border-radius:14px; }
QWidget#paper QLabel, QWidget#paper QCheckBox { color:#19202a; }
QLabel#paperTitle { font-size:17px; font-weight:700; padding:2px 0 8px 0; }
QTabWidget::pane { border:0; background:#d9dde5; border-radius:12px; }
QTabBar::tab { padding:10px 11px; background:#444a54; color:#f7f9fc; border-top-left-radius:10px; border-top-right-radius:10px; }
QTabBar::tab:selected { background:#087cf0; color:white; }
QLineEdit, QDoubleSpinBox, QWidget#paper QComboBox { min-height:23px; padding:1px 4px; color:#161d29; background:#edf0f5; border:1px solid #bdc4d0; border-radius:6px; }
QDoubleSpinBox QLineEdit { min-height:0px; border:0; border-radius:0; padding:0; color:#161d29; background:transparent; }
QLineEdit#catalogSearch { background:#c7ccd6; padding:4px 7px; }
QListWidget#catalog { background:transparent; border:0; color:#19202a; font-size:10px; }
QListWidget#catalog::item { border:1px solid transparent; border-radius:8px; }
QListWidget#catalog::item:hover, QListWidget#catalog::item:selected { background:#a9d4ff; border-color:#087cf0; color:#101a24; }
QPushButton#category { background:transparent; color:#1a202a; padding:4px; border:0; text-align:left; border-radius:7px; font-size:12px; }
QPushButton#category:hover { background:#c3d7ec; }
QPushButton#category:checked { background:#0874e3; color:white; }
QWidget#paper QPushButton:disabled { color:#747c87; }
QWidget#footer { background:#373d45; border-top:1px solid #69707a; }
QWidget#footer QLabel { color:#f1f3f7; }
QWidget#footer QLabel#fps { color:#a1ec87; }
QPushButton#viewButton { color:#f4f5f8; background:#333941; padding:4px 16px; font-size:14px; border:1px solid #4b525c; border-radius:14px; }
QPushButton#viewButton:hover { background:#505e6e; }
)QSS");
    refreshLibrary();
}

void Editor::refreshLibrary() {
    library->clear();
    const QString category=categoryNames.value(categories->checkedId());
    const auto filter=search->text().trimmed();
    for(const auto &entry:catalog()) {
        if(category!=categoryNames.first() && category!=entry.category) continue;
        if(!entry.name.contains(filter,Qt::CaseInsensitive) && !entry.kind.contains(filter,Qt::CaseInsensitive)) continue;
        auto *item=new QListWidgetItem(QIcon(thumbnail(entry)),entry.name,library);
        item->setData(Qt::UserRole,entry.kind);
        item->setToolTip("Thêm "+entry.name);
        item->setTextAlignment(Qt::AlignCenter);
    }
}

void Editor::refreshProjectLabel() {
    const QString project=path.isEmpty()?"PhongKhach_01.int3d":QFileInfo(path).fileName();
    const bool clean=history.isClean() && scene.camera==savedCamera;
    projectLabel->setText("▱  Dự án: "+project+(clean?QString():" *"));
}

void Editor::refresh() {
    syncing=true;
    const auto *o=scene.find(viewport->selection());
    refreshProjectLabel();
    fields->setEnabled(o);
    duplicateAction->setEnabled(o && !isArchitecture(*o) && scene.objects.size()<500);
    deleteAction->setEnabled(o && !o->locked && !isArchitecture(*o));
    objectName->setText(o?o->name:QString());
    locked->setChecked(o && o->locked);
    if(o) {
        objectName->setReadOnly(o->locked);
        const QVector3D values[]{o->position,o->rotation,o->scale};
        for(int row=0;row<3;++row) for(int col=0;col<3;++col) {
            transform[row][col]->setValue(values[row][col]);
            transform[row][col]->setReadOnly(o->locked);
        }
        material->setCurrentText(o->material);
        material->setEnabled(!o->locked);
        objectColor->setEnabled(!o->locked);
        objectColor->setText(o->color.name()+"  ·  Chọn màu");
        objectColor->setStyleSheet(QString("background:%1;color:%2").arg(o->color.name(),o->color.lightness()>127?"#111820":"#f4f5fa"));
    }
    syncing=false;
    viewport->update();
}

void Editor::edit(const QString &label,const std::function<void()> &operation) {
    Scene before=scene;
    // Keep the snapshot independent even when the edit already holds an object pointer.
    before.objects.detach();
    const int selected=viewport->selection();
    operation();
    record(label,std::move(before),selected);
}

void Editor::record(const QString &label,Scene before,int oldSelection) {
    Scene after=scene;
    int newSelection=viewport->selection();
    history.push(new SceneEdit(label,[this,before=std::move(before),after=std::move(after),oldSelection,newSelection](bool redo){
        applyEdit(redo?after:before,redo?newSelection:oldSelection);
    }));
}

void Editor::applyEdit(const Scene &value,int selected) {
    const Camera camera=scene.camera;
    scene=value;
    scene.camera=camera;
    viewport->select(selected);
    refresh();
}

void Editor::resetScene(Scene next,const QString &file) {
    path=file;
    scene=std::move(next);
    viewport->resetRemote();
    savedCamera=scene.camera;
    history.clear();
    viewport->select(0);
    viewport->setMode(scene.camera.mode);
    refresh();
}

bool Editor::openProject(const QString &file) {
    Scene loaded;
    if(!Scene::load(file,loaded)) return false;
    resetScene(std::move(loaded),file);
    return true;
}

bool Editor::saveProject(const QString &file) {
    if(!scene.save(file)) return false;
    path=file;
    savedCamera=scene.camera;
    history.setClean();
    refresh();
    return true;
}

bool Editor::exportImage(const QString &file) {
    return !file.isEmpty() && viewport->isValid() && viewport->rendererError().isEmpty() &&
           viewport->grabFramebuffer().save(file);
}

bool Editor::confirmProjectChange() {
    if(history.isClean() && scene.camera==savedCamera) return true;
    const auto answer=QMessageBox::question(this,"Dự án chưa lưu","Lưu thay đổi của dự án hiện tại?",
                                           QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel,QMessageBox::Save);
    return answer==QMessageBox::Discard || (answer==QMessageBox::Save && chooseSave());
}

void Editor::closeEvent(QCloseEvent *event) {
    if(confirmProjectChange()) event->accept();
    else event->ignore();
}

void Editor::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    if(ratioLabel && viewport) ratioLabel->setText(QString("%1:1").arg(double(viewport->width())/std::max(1,viewport->height()),0,'f',2));
}

bool Editor::chooseSave(bool saveAs) {
    QString file=path;
    if(saveAs || file.isEmpty()) file=QFileDialog::getSaveFileName(this,"Lưu dự án",path,"Interior3D (*.int3d)");
    if(file.isEmpty()) return false;
    if(QFileInfo(file).suffix().isEmpty()) file+=".int3d";
    if(saveProject(file)) return true;
    QMessageBox::warning(this,"Lưu dự án","Không thể ghi tệp "+file);
    return false;
}

void Editor::chooseOpen() {
    if(!confirmProjectChange()) return;
    const auto file=QFileDialog::getOpenFileName(this,"Mở dự án",{},"Interior3D (*.int3d *.json)");
    if(!file.isEmpty() && !openProject(file)) QMessageBox::warning(this,"Mở dự án","Không thể đọc dự án "+file);
}

void Editor::chooseExport() {
    QString filter;
    auto file=QFileDialog::getSaveFileName(this,"Xuất ảnh","PhongKhach.png","PNG (*.png);;JPEG (*.jpg *.jpeg)",&filter);
    if(file.isEmpty()) return;
    if(QFileInfo(file).suffix().isEmpty()) file+=filter.startsWith("JPEG")?".jpg":".png";
    if(!exportImage(file)) QMessageBox::warning(this,"Xuất ảnh","Không thể xuất framebuffer OpenGL vào "+file);
}

namespace {
QRectF furnitureFootprint(const Object &object,const QVector3D &room) {
    const auto transform=object.matrix();
    QPointF minimum(1000,1000),maximum(-1000,-1000);
    for(const auto &part:geometry(object,room)) {
        const auto matrix=transform*part.matrix();
        for(float x:{-.5f,.5f}) for(float y:{-.5f,.5f}) for(float z:{-.5f,.5f}) {
            const auto p=matrix.map(QVector3D(x,y,z));
            minimum.setX(std::min(minimum.x(),double(p.x())));
            minimum.setY(std::min(minimum.y(),double(p.z())));
            maximum.setX(std::max(maximum.x(),double(p.x())));
            maximum.setY(std::max(maximum.y(),double(p.z())));
        }
    }
    return QRectF(minimum,maximum);
}

QVector3D findFurniturePlacement(const Scene &scene,const QString &kind) {
    Object item;
    item.kind=kind;
    item.color=Qt::white;
    const auto footprint=furnitureFootprint(item,scene.room).adjusted(-.07,-.07,.07,.07);
    QVector<QRectF> occupied;
    for(const auto &object:scene.objects)
        if(!isArchitecture(object) && object.kind!="rug" && object.kind!="tv")
            occupied.append(furnitureFootprint(object,scene.room));

    const double w=scene.room.x(),d=scene.room.z();
    const QRectF floor(-w/2+.10,-d/2+.10,w-.20,d-.20);
    const QPointF preferred(.15,1.20);
    QPointF best=preferred;
    double bestDistance=std::numeric_limits<double>::infinity();
    for(double z=floor.top();z<=floor.bottom();z+=.24) {
        for(double x=floor.left();x<=floor.right();x+=.24) {
            const QPointF candidate(x,z),offset=candidate-preferred;
            const double distance=QPointF::dotProduct(offset,offset);
            const auto bounds=footprint.translated(candidate);
            if(distance>=bestDistance || !floor.contains(bounds)) continue;
            if(std::any_of(occupied.cbegin(),occupied.cend(),[&](const QRectF &other){return other.intersects(bounds);}))
                continue;
            best=candidate;
            bestDistance=distance;
        }
    }
    return {float(best.x()),0,float(best.y())};
}
}

void Editor::addFurniture(const QString &kind) {
    if(scene.objects.size()>=500) {
        QMessageBox::information(this,"Thư viện nội thất","Dự án đã đạt giới hạn 500 đối tượng.");
        return;
    }
    edit("Thêm nội thất",[&]{
        viewport->select(scene.add(kind,findFurniturePlacement(scene,kind)));
    });
}

void Editor::showObjects() {
    QDialog dialog(this);
    dialog.setWindowTitle("Đối tượng trong dự án");
    dialog.resize(320,440);
    auto *layout=new QVBoxLayout(&dialog);
    auto *list=new QListWidget;
    for(const auto &o:scene.objects) {
        auto *item=new QListWidgetItem(o.name,list);
        item->setData(Qt::UserRole,o.id);
        if(o.id==viewport->selection()) list->setCurrentItem(item);
    }
    auto select=[&](QListWidgetItem *item) {
        if(item) viewport->select(item->data(Qt::UserRole).toInt());
    };
    connect(list,&QListWidget::currentItemChanged,&dialog,select);
    connect(list,&QListWidget::itemActivated,&dialog,&QDialog::accept);
    layout->addWidget(list);
    auto *b=new QDialogButtonBox(QDialogButtonBox::Close);
    connect(b,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    layout->addWidget(b);
    dialog.exec();
}

void Editor::showLights() {
    QDialog dialog(this);
    dialog.setWindowTitle("Ánh sáng");
    const auto before=scene;
    auto *form=new QFormLayout(&dialog);
    const QStringList captions{"Môi trường","Định hướng","Điểm"};
    float *values[]{&scene.light.ambient,&scene.light.directional,&scene.light.point};
    for(int i=0;i<3;++i) {
        auto *f=number(0,i==2?3:2,.05);
        f->setValue(*values[i]);
        form->addRow(captions[i],f);
        connect(f,&QDoubleSpinBox::valueChanged,&dialog,[this,value=values[i]](double v){*value=float(v);viewport->update();});
    }
    auto *lightColor=new QPushButton("Chọn màu ánh sáng…");
    form->addRow(lightColor);
    connect(lightColor,&QPushButton::clicked,&dialog,[this,&dialog]{
        auto c=QColorDialog::getColor(scene.light.color,&dialog);
        if(c.isValid()) {scene.light.color=c;viewport->update();}
    });
    auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
    connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    form->addRow(buttons);
    if(dialog.exec()==QDialog::Accepted) record("Ánh sáng",before,viewport->selection());
    else {scene.light=before.light;viewport->update();}
}

void Editor::showCamera() {
    QDialog dialog(this);
    dialog.setWindowTitle("Camera");
    auto *layout=new QVBoxLayout(&dialog);
    const QStringList captions{"Phối cảnh 3D","Mặt bằng 2D","Tham quan FPS","Đặt lại camera"};
    for(int i=0;i<captions.size();++i) {
        auto *b=new QPushButton(captions[i]);
        layout->addWidget(b);
        connect(b,&QPushButton::clicked,&dialog,[this,&dialog,i]{
            if(i<3) viewport->setMode(ViewMode(i));else viewport->resetCamera();
            dialog.accept();
        });
    }
    dialog.exec();
    viewport->setFocus();
}

void Editor::showPreview() {
    const QImage image=viewport->grabFramebuffer();
    if(image.isNull()) return;
    QDialog dialog(this);
    dialog.setWindowTitle("Xem trước ảnh render");
    auto *layout=new QVBoxLayout(&dialog);
    auto *picture=label("");
    picture->setPixmap(QPixmap::fromImage(image).scaled(900,620,Qt::KeepAspectRatio,Qt::SmoothTransformation));
    layout->addWidget(picture);
    auto *button=new QPushButton("Xuất ảnh…");
    connect(button,&QPushButton::clicked,this,&Editor::chooseExport);
    layout->addWidget(button);
    dialog.exec();
}

void Editor::showSettings() {
    QDialog dialog(this);
    dialog.setWindowTitle("Cài đặt");
    auto *layout=new QVBoxLayout(&dialog);
    auto *scale=new QCheckBox("Giữ tỷ lệ XYZ khi thay đổi kích thước");
    scale->setChecked(uniform->isChecked());
    connect(scale,&QCheckBox::toggled,uniform,&QCheckBox::setChecked);
    layout->addWidget(scale);
    auto *reset=new QPushButton("Đặt lại camera");
    connect(reset,&QPushButton::clicked,viewport,&Viewport::resetCamera);
    layout->addWidget(reset);
    auto *close=new QDialogButtonBox(QDialogButtonBox::Close);
    connect(close,&QDialogButtonBox::rejected,&dialog,&QDialog::accept);
    layout->addWidget(close);
    dialog.exec();
}

void Editor::showHelp() {
    QMessageBox::information(this,"Interior3D",
        "Chọn một món trong thư viện để thêm vào cảnh. Nhấn trái để chọn đối tượng.\n\n"
        "Kéo chuột phải để xoay camera. Kéo chuột giữa để pan. Lăn chuột để zoom.\n"
        "Trong Xem 3D, chọn vật rồi dùng phím mũi tên để xoay, kéo chuột trái để đặt. Nhấp phải để bỏ chọn. Nhập XYZ rồi nhấn Enter để chỉnh chính xác.\n"
        "Trong chế độ tham quan, WASD di chuyển, di chuột để nhìn và nhấn T để thoát.\n\n"
        "Ctrl+N tạo mới · Ctrl+O mở · Ctrl+S lưu · Ctrl+D sao chép · Delete xóa · Ctrl+E xuất ảnh · F11 toàn màn hình.");
}
