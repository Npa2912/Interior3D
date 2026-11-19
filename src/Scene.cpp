#include "Scene.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>
#include <QtMath>
#include <algorithm>
#include <cmath>

bool Camera::operator==(const Camera &other) const {
    return target==other.target && eye==other.eye && yaw==other.yaw && pitch==other.pitch &&
           distance==other.distance && planDistance==other.planDistance && planTarget==other.planTarget &&
           walkYaw==other.walkYaw && walkPitch==other.walkPitch && mode==other.mode;
}

QVector3D Camera::forward() const {
    const float y = qDegreesToRadians(walkYaw), p = qDegreesToRadians(walkPitch);
    return {std::cos(y) * std::cos(p), std::sin(p), std::sin(y) * std::cos(p)};
}

QVector3D Camera::position() const {
    if (mode == ViewMode::Walk) return eye;
    if (mode == ViewMode::Plan) return planTarget + QVector3D(0, 26, 0);
    const float y = qDegreesToRadians(yaw), p = qDegreesToRadians(pitch);
    return target + distance * QVector3D(std::sin(y) * std::cos(p), std::sin(p), std::cos(y) * std::cos(p));
}

QMatrix4x4 Camera::view() const {
    QMatrix4x4 result;
    if (mode == ViewMode::Plan) result.lookAt(position(), planTarget, {0, 0, -1});
    else if (mode == ViewMode::Walk) result.lookAt(eye, eye + forward(), {0, 1, 0});
    else result.lookAt(position(), target, {0, 1, 0});
    return result;
}

QMatrix4x4 Camera::projection(float aspect) const {
    QMatrix4x4 result;
    if (mode == ViewMode::Plan) {
        const float half = planDistance * .53f;
        result.ortho(-half * aspect, half * aspect, -half, half, .05f, 60);
    } else {
        result.perspective(mode == ViewMode::Walk ? 66 : 64, aspect, .06f, 80);
    }
    return result;
}

QMatrix4x4 Object::matrix() const {
    QMatrix4x4 m;
    m.translate(position);
    m.rotate(rotation.y(), 0, 1, 0);
    m.rotate(rotation.x(), 1, 0, 0);
    m.rotate(rotation.z(), 0, 0, 1);
    m.scale(scale);
    return m;
}

QMatrix4x4 Part::matrix() const {
    QMatrix4x4 m;
    m.translate(center);
    m.rotate(yaw, 0, 1, 0);
    m.rotate(roll, 0, 0, 1);
    m.scale(size);
    return m;
}

const QVector<CatalogItem> &catalog() {
    static const QVector<CatalogItem> entries{
        {"sofa", "Sofa", "Phòng khách", QColor("#d8c6ad")},
        {"chair", "Ghế bành", "Phòng khách", QColor("#d2b79b")},
        {"table", "Bàn trà", "Phòng khách", QColor("#a46f45")},
        {"bed", "Giường", "Phòng ngủ", QColor("#d7c2a6")},
        {"wardrobe", "Tủ quần áo", "Phòng ngủ", QColor("#bb9572")},
        {"dining", "Bàn ăn", "Phòng bếp", QColor("#a9784e")},
        {"plant", "Cây xanh", "Trang trí", QColor("#5f8d36")},
        {"lamp", "Đèn sàn", "Trang trí", QColor("#ead1a0")},
        {"shelf", "Kệ sách", "Trang trí", QColor("#a07451")},
        {"console", "Kệ tivi", "Phòng khách", QColor("#bda991")},
        {"tv", "Tivi", "Phòng khách", QColor("#172a39")},
        {"rug", "Thảm", "Trang trí", QColor("#bdaba0")},
        {"vanity", "Tủ lavabo", "Phòng tắm", QColor("#c2a387")},
        {"ottoman", "Ghế đôn", "Phòng khách", QColor("#d2bea9")}
    };
    return entries;
}

bool isArchitecture(const Object &o) {
    return o.kind == "floor" || o.kind == "wall" || o.kind == "ceiling" ||
           o.kind == "window" || o.kind == "slats" || o.kind == "pendant" || o.kind == "art";
}

Scene Scene::livingRoom() {
    Scene s;
    auto shell = [&](const QString &kind, const QString &name, const char *color, QVector3D at = {}) {
        Object o;
        o.id = s.nextId++;
        o.kind = kind;
        o.name = name;
        o.color = QColor(color);
        o.position = at;
        s.objects.append(o);
    };
    shell("floor", "Sàn gỗ", "#ae7950");
    shell("wall", "Tường", "#d6ccc0");
    shell("ceiling", "Trần", "#e0d8ce");
    shell("window", "Cửa sổ", "#a7d9f0", {0, 0, -3.12f});
    shell("slats", "Vách ốp gỗ", "#765037", {3.22f, 0, -.7f});
    shell("pendant", "Đèn trần", "#e5bb77", {0, 2.59f, -.35f});
    shell("art", "Tranh treo tường", "#337088", {-3.29f, 1.89f, -.9f});
    auto add = [&](const QString &kind, QVector3D p, float yaw = 0) {
        s.find(s.add(kind, p))->rotation.setY(yaw);
    };
    add("rug", {-.15f, 0, .25f});
    add("sofa", {-2.05f, 0, -.18f}, 90);
    add("table", {-.1f, 0, .28f});
    add("ottoman", {1.04f, 0, 1.32f});
    add("console", {2.89f, 0, -.55f}, -90);
    add("tv", {3.18f, 1.02f, -.55f}, -90);
    add("shelf", {2.92f, 0, -2.27f}, -90);
    const int frontPlant=s.add("plant",{-1.89f,0,1.50f});
    s.find(frontPlant)->scale={1.08f,1.35f,1.08f};
    add("plant", {2.25f, 0, -2.59f});
    add("lamp", {-2.45f, 0, -2.37f});
    return s;
}

Object *Scene::find(int id) {
    for (auto &object : objects) if (object.id == id) return &object;
    return nullptr;
}

const Object *Scene::find(int id) const {
    for (const auto &object : objects) if (object.id == id) return &object;
    return nullptr;
}

int Scene::add(const QString &kind, QVector3D position) {
    if(objects.size()>=500) return 0;
    for (const auto &item : catalog()) {
        if (item.kind != kind) continue;
        Object object;
        object.id = nextId++;
        object.kind = kind;
        object.name = item.name;
        object.position = position;
        object.color = item.color;
        objects.append(object);
        return object.id;
    }
    return 0;
}

int Scene::duplicate(int id) {
    const auto *original = find(id);
    if (!original || objects.size()>=500 || isArchitecture(*original)) return 0;
    Object copy = *original;
    copy.id = nextId++;
    copy.name += " (bản sao)";
    copy.position += QVector3D(.45f, 0, .45f);
    copy.locked = false;
    objects.append(copy);
    return copy.id;
}

bool Scene::remove(int id) {
    for (qsizetype i = 0; i < objects.size(); ++i) {
        const auto &o = objects[i];
        if (o.id != id) continue;
        if (o.locked || isArchitecture(o)) return false;
        objects.removeAt(i);
        return true;
    }
    return false;
}

QVector<Part> geometry(const Object &o, const QVector3D &room) {
    QVector<Part> parts;
    auto box = [&](QVector3D p, QVector3D size, const QColor &color,
                   QString material = {}, bool tint = true, float yaw = 0,
                   bool cylinder = false, float roll = 0) {
        parts.append({p, size, color, material, tint, yaw, cylinder, roll});
    };
    auto legs = [&](float x, float z, float height, float width) {
        for (int sx : {-1, 1}) for (int sz : {-1, 1})
            box({sx*x, height/2, sz*z}, {width, height, width}, QColor("#5d4536"), "Gỗ", false);
    };
    const QString &type = o.kind;
    if (type == "floor") {
        const int rows = 24;
        for (int r = 0; r < rows; ++r) {
            const float depth = room.z()/rows;
            const float z = -room.z()/2 + depth*(r+.5f);
            const float seam = (r%3 - 1)*.8f;
            for (int i = 0; i < 3; ++i) {
                const float left = i == 0 ? -room.x()/2 : (i-1.5f)*room.x()/3 + seam;
                const float right = i == 2 ? room.x()/2 : (i-.5f)*room.x()/3 + seam;
                box({(left+right)/2, -.07f, z}, {right-left-.012f, .13f, depth-.013f},
                    o.color.lighter(87 + (r*19+i*13)%28), "Gỗ");
            }
        }
        box({0,-.095f,0},{room.x(),.13f,room.z()},o.color.darker(155),"Gỗ");
    } else if (type == "wall") {
        const float w=room.x(), h=room.y(), d=room.z();
        box({-w/2,h/2,0},{.12f,h,d},o.color);
        box({w/2,h/2,0},{.12f,h,d},o.color);
        for (float x : {-1.f,1.f})
            box({x*w*.405f,h/2,-d/2},{w*.19f,h,.12f},o.color);
        box({0,.18f,-d/2},{w*.65f,.36f,.12f},o.color);
        box({0,h-.20f,-d/2},{w*.65f,.4f,.12f},o.color);
    } else if (type == "ceiling") {
        box({0,room.y(),0},{room.x(),.11f,room.z()},o.color);
        for (float x : {-2.65f,2.65f}) {
            box({x,room.y()-.085f,0},{1.48f,.16f,room.z()},o.color);
            box({x*.72f,room.y()-.155f,0},{.044f,.022f,room.z()},QColor("#ffe1a4"),"Phát sáng",false);
            for (float z : {-2.6f,-.8f,1.f})
                box({x,room.y()-.175f,z},{.13f,.028f,.13f},QColor("#fff4d7"),"Phát sáng",false,0,true);
        }
    } else if (type == "window") {
        box({0,1.57f,-.025f},{4.3f,2.42f,.045f},o.color,"Bầu trời");
        for (int i=0;i<17;++i) {
            const float h=.14f+(i*17%21)*.028f;
            box({-2.02f+i*.248f,.67f+h/2,.005f},{.19f,h,.023f},QColor::fromHsv(193+i%10,80,170+i%5*11),"Phát sáng",false);
        }
        for (float x : {-2.15f,-.72f,.72f,2.15f})
            box({x,1.57f,.055f},{.057f,2.46f,.07f},QColor("#313b44"),"Kim loại",false);
        for (float y : {.36f,1.15f,2.78f})
            box({0,y,.055f},{4.34f,.057f,.07f},QColor("#313b44"),"Kim loại",false);
        for(int side : {-1,1}) for(int i=0;i<8;++i)
            box({side*(1.78f+i*.074f),1.50f,.16f},{.094f,2.88f,.13f+(i%2)*.055f},
                QColor("#918886").lighter(96+i%3*7),"Vải",false);
    } else if (type == "slats") {
        box({.018f,1.38f,0},{.03f,2.75f,3.4f},o.color.darker(155),"Gỗ");
        for(int i=0;i<44;++i)
            box({0,1.38f,-1.66f+i*.077f},{.044f,2.75f,.047f},o.color.lighter(91+i%5*5),"Gỗ");
    } else if (type == "pendant") {
        // Each ring uses short straight segments so it shares the cube mesh.
        for (int ring=0;ring<2;++ring) {
            const float radius=ring ? .55f : .78f, y=ring ? -.28f : 0;
            for(int i=0;i<40;++i) {
                float a=i*6.2831853f/40;
                box({radius*std::cos(a),y,radius*std::sin(a)},{.038f,.044f,radius*.161f},o.color,"Phát sáng",true,-qRadiansToDegrees(a));
            }
        }
        for(float x : {-.48f,0.f,.48f})
            box({x,.19f,0},{.016f,.43f,.016f},QColor("#83664a"),"Kim loại",false);
    } else if (type == "art") {
        for(int i=0;i<3;++i) {
            float z=(i-1)*.53f;
            box({0,0,z},{.038f,.91f,.51f},QColor("#544637"),"Gỗ",false);
            box({.023f,0,z},{.014f,.85f,.45f},QColor("#ede0cc"),{},false);
            box({.037f,-.12f+i*.08f,z},{.015f,.40f,.32f},o.color.lighter(90+i*20),{},true,0,true);
        }
    } else if(type=="ottoman") {
        box({0,.115f,0},{.81f,.22f,.72f},o.color.darker(109),"Vải",true,0,true);
        box({0,.26f,0},{.87f,.25f,.77f},o.color,"Vải",true,0,true);
        box({0,.383f,0},{.83f,.027f,.735f},o.color.lighter(106),"Vải",true,0,true);
    } else if (type == "sofa" || type == "chair") {
        const bool couch=type=="sofa";
        const int count=couch?3:1;
        const float width=couch?2.72f:.94f;
        legs(width/2-.14f,.35f,.14f,.065f);
        box({0,.27f,0},{width,.30f,.94f},o.color.darker(106),"Vải");
        box({0,.70f,-.37f},{width,.61f,.22f},o.color,"Vải");
        for(int i=0;i<count;++i) {
            float x=(i-(count-1)*.5f)*.80f;
            box({x,.45f,.055f},{.77f,.17f,.70f},o.color.lighter(109),"Vải");
            box({x,.78f,-.235f},{.76f,.53f,.19f},o.color.lighter(107),"Vải");
        }
        for(float x : {-width/2+.09f,width/2-.09f})
            box({x,.545f,.025f},{.18f,.46f,.87f},o.color,"Vải");
        if(couch) {
            box({-.84f,.34f,.53f},{.77f,.34f,.95f},o.color,"Vải");
            box({-.84f,.46f,.58f},{.75f,.16f,.86f},o.color.lighter(110),"Vải");
            box({.66f,.75f,-.105f},{.37f,.34f,.15f},QColor("#285575"),"Vải",false,0,false,-13);
            box({-.53f,.75f,-.105f},{.35f,.33f,.15f},QColor("#e8ddd2"),"Vải",false,0,false,11);
        }
    } else if (type == "table" || type == "dining") {
        bool dining=type=="dining";
        float h=dining?.75f:.43f, w=dining?1.75f:1.46f, d=dining?.91f:.84f;
        legs(w/2-.13f,d/2-.11f,h-.05f,.075f);
        box({0,h,0},{w,.105f,d},o.color,"Gỗ");
        if(!dining) {
            box({-.37f,h+.067f,.07f},{.29f,.029f,.19f},QColor("#ebe0d0"),{},false,12);
            box({-.37f,h+.094f,.065f},{.25f,.024f,.17f},QColor("#466177"),{},false,-8);
            box({.20f,h+.15f,-.12f},{.19f,.23f,.19f},QColor("#dbcab6"),{},false,0,true);
            for(int i=0;i<7;++i) {
                const float a=i*.90f;
                box({.2f+.10f*std::cos(a),h+.29f+(i%3)*.04f,-.12f+.09f*std::sin(a)},
                    {.095f,.09f,.095f},QColor("#fff0d2"),{},false,0,true);
            }
        }
    } else if (type == "rug") {
        box({0,.025f,0},{3.64f,.035f,2.74f},o.color,"Vải");
        for(int i=0;i<18;++i)
            box({0,.046f,-1.29f+i*.15f},{3.49f,.008f,.019f},o.color.lighter(i%2?119:91),"Vải");
        for(float x : {-1.68f,1.68f}) box({x,.048f,0},{.025f,.009f,2.6f},o.color.lighter(122),"Vải");
    } else if (type == "tv") {
        box({0,.51f,0},{1.98f,1.07f,.079f},QColor("#14191c"),"Kim loại",false);
        box({0,.51f,.043f},{1.91f,1.0f,.015f},o.color,"Kính");
    } else if(type=="plant") {
        box({0,.18f,0},{.37f,.36f,.37f},QColor("#dfd5ca"),{},false,0,true);
        box({0,.37f,0},{.31f,.025f,.31f},QColor("#62503d"),{},false,0,true);
        box({0,.65f,0},{.035f,.59f,.035f},QColor("#626438"),{},false);
        for(int i=0;i<12;++i) {
            float a=i*2.39f, r=.22f+(i%3)*.06f;
            box({r*std::cos(a),.63f+i*.047f,r*std::sin(a)},{.44f,.061f,.15f},
                o.color.lighter(88+i%4*11),{},true,-qRadiansToDegrees(a),true,24.f);
        }
    } else if(type=="lamp") {
        box({0,.035f,0},{.38f,.07f,.38f},QColor("#726044"),"Kim loại",false,0,true);
        box({0,.76f,0},{.032f,1.51f,.032f},QColor("#93754f"),"Kim loại",false,0,true);
        box({0,1.53f,0},{.43f,.3f,.43f},o.color,"Phát sáng",true,0,true);
    } else if(type=="bed") {
        legs(.74f,.89f,.18f,.11f);
        box({0,.25f,0},{1.78f,.26f,2.04f},o.color,"Gỗ");
        box({0,.59f,-.99f},{1.78f,1.1f,.18f},o.color,"Gỗ");
        box({0,.43f,0},{1.69f,.18f,1.89f},QColor("#eee2d7"),"Vải",false);
        box({0,.55f,.35f},{1.70f,.09f,1.17f},QColor("#899faa"),"Vải",false);
        for(float x : {-.42f,.42f})
            box({x,.61f,-.61f},{.70f,.16f,.40f},QColor("#f4e8d8"),"Vải",false);
    } else {
        bool shelf=type=="shelf", tall=type=="wardrobe"||shelf;
        float w=shelf?.82f:(type=="console"?2.55f:1.30f);
        float h=tall?2.13f:(type=="vanity"?.81f:.57f), d=.43f;
        box({0,h/2,-d*.45f},{w,h,.07f},o.color.darker(125),"Gỗ");
        for(float x : {-w/2+.035f,w/2-.035f})
            box({x,h/2,0},{.07f,h,d},o.color,"Gỗ");
        box({0,h-.04f,0},{w,.08f,d},o.color,"Gỗ");
        box({0,.066f,0},{w,.08f,d},o.color,"Gỗ");
        if(shelf) {
            for(int i=1;i<4;++i) {
                float y=i*h/4;
                box({0,y,0},{w,.054f,d},o.color,"Gỗ");
                box({.07f,y+.14f,0},{.12f,.23f,.17f},QColor("#ded1ac"),{},false);
                box({-.13f,y+.13f,0},{.085f,.20f,.16f},QColor("#547281"),{},false);
            }
        } else {
            for(float x : {-w/4,w/4}) {
                box({x,h/2,d/2},{w/2-.022f,h-.16f,.045f},o.color.lighter(118),"Gỗ");
                box({x,h*.58f,d/2+.03f},{.12f,.027f,.032f},QColor("#725942"),"Kim loại",false);
            }
        }
        if(type=="vanity") box({0,h+.06f,0},{.56f,.15f,.37f},QColor("#f1ede5"),{},false);
    }
    return parts;
}

namespace {
QJsonArray vectorJson(QVector3D v) { return {v.x(),v.y(),v.z()}; }

bool vectorValue(const QJsonValue &value, QVector3D &result, double min, double max) {
    const auto a=value.toArray();
    if(a.size()!=3) return false;
    for(int i=0;i<3;++i) {
        const double number=a.at(i).toDouble(qQNaN());
        if(!std::isfinite(number)||number<min||number>max) return false;
        result[i]=float(number);
    }
    return true;
}

bool scalar(const QJsonObject &o, const QString &key, float &result, double min, double max) {
    const double v=o.value(key).toDouble(qQNaN());
    if(!std::isfinite(v)||v<min||v>max) return false;
    result=float(v);
    return true;
}
}

bool Scene::save(const QString &file, QString *error) const {
    QJsonArray items;
    for(const auto &o:objects)
        items.append(QJsonObject{{"id",o.id},{"kind",o.kind},{"name",o.name},{"position",vectorJson(o.position)},
            {"rotation",vectorJson(o.rotation)},{"scale",vectorJson(o.scale)},{"color",o.color.name()},
            {"material",o.material},{"locked",o.locked}});
    QJsonObject lighting{{"color",light.color.name()},{"ambient",light.ambient},{"directional",light.directional},{"point",light.point}};
    QJsonObject viewing{{"target",vectorJson(camera.target)},{"eye",vectorJson(camera.eye)},{"yaw",camera.yaw},
        {"pitch",camera.pitch},{"distance",camera.distance},{"planDistance",camera.planDistance},{"planTarget",vectorJson(camera.planTarget)},{"walkYaw",camera.walkYaw},{"walkPitch",camera.walkPitch},
        {"mode",int(camera.mode)}};
    QByteArray bytes=QJsonDocument(QJsonObject{{"version",1},{"room",vectorJson(room)},{"objects",items},
        {"light",lighting},{"camera",viewing}}).toJson();
    QSaveFile output(file);
    if(output.open(QIODevice::WriteOnly) && output.write(bytes)==bytes.size() && output.commit()) return true;
    if(error) *error=output.errorString();
    return false;
}

bool Scene::load(const QString &file, Scene &result, QString *error) {
    QFile input(file);
    auto fail=[&](const QString &message) { if(error) *error=message; return false; };
    if(!input.open(QIODevice::ReadOnly)) return fail(input.errorString());
    if(input.size()>16*1024*1024) return fail("Tệp dự án quá lớn.");
    QJsonParseError parse;
    const auto root=QJsonDocument::fromJson(input.readAll(),&parse).object();
    if(parse.error!=QJsonParseError::NoError) return fail(parse.errorString());
    if(root.value("version").toInt()!=1 || !root.value("objects").isArray())
        return fail("Định dạng dự án không hợp lệ.");
    Scene loaded;
    if(!vectorValue(root.value("room"),loaded.room,3,20)) {
        // The room may have a low ceiling, while width and depth still need clearance.
        if(!vectorValue(root.value("room"),loaded.room,2.2,20) || loaded.room.x()<3 || loaded.room.z()<3)
            return fail("Kích thước phòng không hợp lệ.");
    }
    const auto list=root.value("objects").toArray();
    if(list.size()>500) return fail("Dự án vượt giới hạn 500 đối tượng.");
    QSet<int> ids;
    for(const auto &item:list) {
        const auto data=item.toObject();
        Object o;
        o.id=data.value("id").toInt();
        o.kind=data.value("kind").toString();
        o.name=data.value("name").toString();
        o.color=QColor(data.value("color").toString());
        o.material=data.value("material").toString();
        o.locked=data.value("locked").toBool();
        bool known=isArchitecture(o);
        for(const auto &entry:catalog()) known|=o.kind==entry.kind;
        if(o.id<1||o.id>=1000000000||ids.contains(o.id)||!known||o.name.trimmed().isEmpty()||!o.color.isValid()||
           !vectorValue(data.value("position"),o.position,-100,100)||
           !vectorValue(data.value("rotation"),o.rotation,-360,360)||
           !vectorValue(data.value("scale"),o.scale,.0499,20))
            return fail("Dữ liệu đối tượng không hợp lệ.");
        ids.insert(o.id);
        loaded.nextId=std::max(loaded.nextId,o.id+1);
        loaded.objects.append(o);
    }
    const auto lighting=root.value("light").toObject();
    loaded.light.color=QColor(lighting.value("color").toString());
    if(!loaded.light.color.isValid()||!scalar(lighting,"ambient",loaded.light.ambient,0,2)||
       !scalar(lighting,"directional",loaded.light.directional,0,2)||
       !scalar(lighting,"point",loaded.light.point,0,3)) return fail("Dữ liệu ánh sáng không hợp lệ.");
    const auto viewing=root.value("camera").toObject();
    auto &camera=loaded.camera;
    const int mode=viewing.value("mode").toInt(-1);
    if(mode<0||mode>2||!vectorValue(viewing.value("target"),camera.target,-100,100)||
       !vectorValue(viewing.value("eye"),camera.eye,-100,100)||
       !scalar(viewing,"yaw",camera.yaw,-360,360)||!scalar(viewing,"pitch",camera.pitch,-85,85)||
       !scalar(viewing,"distance",camera.distance,1,35)||!scalar(viewing,"walkYaw",camera.walkYaw,-360,360)||
       !scalar(viewing,"walkPitch",camera.walkPitch,-85,85)) return fail("Dữ liệu camera không hợp lệ.");
    if((viewing.contains("planDistance") && !scalar(viewing,"planDistance",camera.planDistance,1,35)) ||
       (viewing.contains("planTarget") && !vectorValue(viewing.value("planTarget"),camera.planTarget,-100,100)))
        return fail("Dữ liệu mặt bằng không hợp lệ.");
    camera.mode=ViewMode(mode);
    camera.eye.setY(std::clamp(camera.eye.y(),.5f,loaded.room.y()-.15f));
    result=std::move(loaded);
    return true;
}
