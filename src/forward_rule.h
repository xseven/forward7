#pragma once

#include <QString>
#include <QVariant>

struct ForwardRule {
    QString name;

    QVariant sourceInterfaceName;
    QVariant sourceIp4;
    QVariant sourcePort;

    QVariant destinationInterfaceName;
    QVariant destinationIp4;
    QVariant destinationPort;

    bool isValid() const
    {
        return sourceInterfaceName.isValid() && destinationInterfaceName.isValid() && destinationIp4.isValid();
    }
};
