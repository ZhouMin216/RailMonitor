#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include "./protocol/DeviceParser.h"

class IconShoe
{
public:
    IconShoe();

    const ShoeData& GetShoeData() { return shoeData_;}

private:
    ShoeData shoeData_;
};



class DeviceManager
{
public:
    DeviceManager();

private:
    // QMap<quint16, IconShoe> shoeMap_;
};

#endif // DEVICEMANAGER_H
