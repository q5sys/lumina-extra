/*
# Copyright (c) 2018, Ole-André Rodlie <ole.andre.rodlie@gmail.com> All rights reserved.
#
# Available under the 3-clause BSD license
# See the LICENSE file for full details
*/

#include "manager.h"
#include "udisks2.h"
#include <QDebug>
#include <QTimer>

Device::Device(const QString block, QObject *parent)
    : QObject(parent)
    , path(block)
    , isOptical(false)
    , isRemovable(false)
    , hasMedia(false)
    , opticalDataTracks(0)
    , opticalAudioTracks(0)
    , isBlankDisc(false)
    , hasPartition(false)
    , dbus(0)
{
    QDBusConnection system = QDBusConnection::systemBus();
    dbus = new QDBusInterface(DBUS_SERVICE, path, QString("%1.Block").arg(DBUS_SERVICE), system, parent);
    system.connect(dbus->service(), dbus->path(), DBUS_PROPERTIES, "PropertiesChanged", this, SLOT(handlePropertiesChanged(QString,QMap<QString,QVariant>)));
    updateDeviceProperties();
}

void Device::mount()
{
    if (!dbus->isValid() || !mountpoint.isEmpty()) { return; }
    QString reply = uDisks2::mountDevice(path);
    if (!reply.isEmpty()) {
        emit errorMessage(path, reply);
        return;
    }
    updateDeviceProperties();
}

void Device::unmount()
{
    if (!dbus->isValid() || mountpoint.isEmpty()) { return; }
    QString reply = uDisks2::unmountDevice(path);
    updateDeviceProperties();
    if (!reply.isEmpty() || !mountpoint.isEmpty()) {
        if (reply.isEmpty()) { reply = QObject::tr("Failed to umount %1").arg(name); }
        emit errorMessage(path, reply);
        return;
    }
    if (isOptical) { eject(); }
}

void Device::eject()
{
    if (!dbus->isValid()) { return; }
    QString reply = uDisks2::ejectDevice(drive);
    updateDeviceProperties();
    if (!reply.isEmpty()/* || hasMedia*/) {
        if (reply.isEmpty()) { reply = QObject::tr("Failed to eject %1").arg(name); }
        emit errorMessage(path, reply);
    }
}

void Device::updateDeviceProperties()
{
    if (!dbus->isValid()) { return; }

    bool hadMedia =  hasMedia;
    QString lastMountpoint = mountpoint;
    QString lastName = name;

    drive = uDisks2::getDrivePath(path);
    name = uDisks2::getDeviceName(drive);
    dev = path.split("/").takeLast();
    isRemovable = uDisks2::isRemovable(drive);
    mountpoint = uDisks2::getMountPoint(path);
    filesystem = uDisks2::getFileSystem(path);
    isOptical = uDisks2::isOptical(drive);
    hasMedia = uDisks2::hasMedia(drive);
    opticalDataTracks = uDisks2::opticalDataTracks(drive);
    opticalAudioTracks = uDisks2::opticalAudioTracks(drive);
    isBlankDisc = uDisks2::opticalMediaIsBlank(drive);
    hasPartition = uDisks2::hasPartition(path);

    if (hadMedia != hasMedia) {
        emit mediaChanged(path, hasMedia);
    }
    if (lastMountpoint != mountpoint) {
        emit mountpointChanged(path, mountpoint);
    }
    if (lastName != name) {
        emit nameChanged(path, name);
    }
}

void Device::handlePropertiesChanged(const QString &interfaceType, const QMap<QString, QVariant> &changedProperties)
{
    Q_UNUSED(interfaceType)
    Q_UNUSED(changedProperties)
    updateDeviceProperties();
}

Manager::Manager(QObject *parent)
    : QObject(parent)
    , dbus(0)
{
    setupDBus();
    timer.setInterval(60000);
    connect(&timer, SIGNAL(timeout()), this, SLOT(checkUDisks()));
    timer.start();
}

void Manager::setupDBus()
{
    QDBusConnection system = QDBusConnection::systemBus();
    if (system.isConnected()) {
        system.connect(DBUS_SERVICE, DBUS_PATH, DBUS_OBJMANAGER, DBUS_DEVICE_ADDED, this, SLOT(deviceAdded(const QDBusObjectPath&)));
        system.connect(DBUS_SERVICE, DBUS_PATH, DBUS_OBJMANAGER, DBUS_DEVICE_REMOVED, this, SLOT(deviceRemoved(const QDBusObjectPath&)));
        if (dbus==NULL) { dbus = new QDBusInterface(DBUS_SERVICE, DBUS_PATH, DBUS_OBJMANAGER, system); } // only used to verify the UDisks is running
        scanDevices();
    }
}

void Manager::scanDevices()
{
    QStringList foundDevices = uDisks2::getDevices();
    for (int i=0;i<foundDevices.size();i++) {
        QString foundDevicePath = foundDevices.at(i);
        bool hasDevice = devices.contains(foundDevicePath);
        if (hasDevice) { continue; }
        Device *newDevice = new Device(foundDevicePath, this);
        connect(newDevice, SIGNAL(mediaChanged(QString,bool)), this, SLOT(handleDeviceMediaChanged(QString,bool)));
        connect(newDevice, SIGNAL(mountpointChanged(QString,QString)), this, SLOT(handleDeviceMountpointChanged(QString,QString)));
        connect(newDevice, SIGNAL(errorMessage(QString,QString)), this, SLOT(handleDeviceErrorMessage(QString,QString)));
        devices[foundDevicePath] = newDevice;
    }
    emit updatedDevices();
}

void Manager::deviceAdded(const QDBusObjectPath &obj)
{
    if (!dbus->isValid()) { return; }
    QString path = obj.path();
    if (path.startsWith(QString("%1/jobs").arg(DBUS_PATH))) { return; }

    scanDevices();
    emit foundNewDevice(path);
}

void Manager::deviceRemoved(const QDBusObjectPath &obj)
{
    if (!dbus->isValid()) { return; }
    QString path = obj.path();
    bool deviceExists = devices.contains(path);
    if (path.startsWith(QString("%1/jobs").arg(DBUS_PATH))) { return; }

    if (deviceExists) {
        if (uDisks2::getDevices().contains(path)) { return; }
        delete devices.take(path);
    }
    scanDevices();
}

void Manager::handleDeviceMediaChanged(QString devicePath, bool mediaPresent)
{
    emit mediaChanged(devicePath, mediaPresent);
}

void Manager::handleDeviceMountpointChanged(QString devicePath, QString deviceMountpoint)
{
    emit mountpointChanged(devicePath, deviceMountpoint);
}

void Manager::handleDeviceErrorMessage(QString devicePath, QString deviceError)
{
    emit deviceErrorMessage(devicePath, deviceError);
}

void Manager::checkUDisks()
{
    if (!QDBusConnection::systemBus().isConnected()) {
        setupDBus();
        return;
    }
    if (!dbus->isValid()) { scanDevices(); }
}
