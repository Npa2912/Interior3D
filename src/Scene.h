#pragma once

#include <QColor>
#include <QMatrix4x4>
#include <QString>
#include <QVector>
#include <QVector3D>

enum class ViewMode { Orbit, Plan, Walk };

struct Camera {
    QVector3D target{.16f, 1.32f, -1.04f};
    QVector3D eye{.2f, 1.65f, 2.3f};
    float yaw = -7, pitch = 3, distance = 4.51f;
    float planDistance = 7.8f;
    QVector3D planTarget;
    float walkYaw = -90, walkPitch = 0;
    ViewMode mode = ViewMode::Orbit;

    bool operator==(const Camera &other) const;
    QVector3D forward() const;
    QVector3D position() const;
    QMatrix4x4 view() const;
    QMatrix4x4 projection(float aspect) const;
};

struct Light {
    QColor color{"#fff5e8"};
    float ambient = .46f, directional = .52f, point = .7f;
};

struct Object {
    int id = 0;
    QString kind, name;
    QVector3D position, rotation, scale{1, 1, 1};
    QColor color;
    QString material = "Mặc định";
    bool locked = false;
    QMatrix4x4 matrix() const;
};

struct Part {
    QVector3D center, size;
    QColor color;
    QString material;
    bool tint = true;
    float yaw = 0;
    bool cylinder = false;
    float roll = 0;
    QMatrix4x4 matrix() const;
};

struct CatalogItem {
    QString kind, name, category;
    QColor color;
};

struct Scene {
    QVector<Object> objects;
    Light light;
    Camera camera;
    QVector3D room{6.8f, 3.15f, 6.4f};
    int nextId = 1;

    static Scene livingRoom();
    Object *find(int id);
    const Object *find(int id) const;
    int add(const QString &kind, QVector3D position = {});
    int duplicate(int id);
    bool remove(int id);
    bool save(const QString &file, QString *error = nullptr) const;
    static bool load(const QString &file, Scene &result, QString *error = nullptr);
};

const QVector<CatalogItem> &catalog();
QVector<Part> geometry(const Object &object, const QVector3D &room);
bool isArchitecture(const Object &object);
