/*
# Copyright (c) 2018, Ole-André Rodlie <ole.andre.rodlie@gmail.com> All rights reserved.
#
# Available under the 3-clause BSD license
# See the LICENSE file for full details
*/

#ifndef MANAGER_H
#define MANAGER_H

#include <QObject>
#include <QMap>
#include <QtDBus>

class Device : public QObject
{
    Q_OBJECT
public:
    explicit Device(const QString block, QObject *parent = NULL);
    QString name;
    QString path;
    QString dev;
    QString drive;
    QString mountpoint;
    QString filesystem;
    bool isOptical;
    bool isRemovable;
    bool hasMedia;
    int opticalDataTracks;
    int opticalAudioTracks;
    bool isBlankDisc;
    bool hasPartition;
private:
    QDBusInterface *dbus;
signals:
    void mediaChanged(QString devicePath, bool mediaPresent);
    void mountpointChanged(QString devicePath, QString deviceMountpoint);
    void nameChanged(QString devicePath, QString deviceName);
    void errorMessage(QString devicePath, QString deviceError);
public slots:
    void mount();
    void unmount();
    void eject();
private slots:
    void updateDeviceProperties();
    void handlePropertiesChanged(const QString &interfaceType, const QMap<QString, QVariant> &changedProperties);
};

class Manager : public QObject
{
    Q_OBJECT
public:
    explicit Manager(QObject *parent = NULL);
    QMap<QString,Device*> devices;
private:
    QDBusInterface *dbus;
    QTimer timer;
signals:
    void updatedDevices();
    void mediaChanged(QString path, bool media);
    void mountpointChanged(QString path, QString mountpoint);
    void deviceErrorMessage(QString path, QString error);
    void foundNewDevice(QString path);
private slots:
    void setupDBus();
    void scanDevices();
    void deviceAdded(const QDBusObjectPath &obj);
    void deviceRemoved(const QDBusObjectPath &obj);
    void handleDeviceMediaChanged(QString devicePath, bool mediaPresent);
    void handleDeviceMountpointChanged(QString devicePath, QString deviceMountpoint);
    void handleDeviceErrorMessage(QString devicePath, QString deviceError);
    void checkUDisks();
};

#endif // MANAGER_H
