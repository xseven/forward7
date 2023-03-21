#pragma once

#include <QString>
#include <QVariant>

struct ForwardRule {
    QString name;

    QVariant sourceInterfaceName;
    QVariant sourceInterfaceDesc;
    QVariant sourceIp4;
    QVariant sourcePort;

    QVariant destinationInterfaceName;
    QVariant destinationInterfaceDesc;
    QVariant destinationIp4;
    QVariant destinationPort;

    bool isValid() const
    {
        return sourceInterfaceName.isValid() && destinationInterfaceName.isValid() && destinationIp4.isValid();
    }

    QString toString()
    {
        return QStringLiteral("Rule: [%1]\nsrc: [%2 %3:%4] dst: [%5 %6:%7]")
            .arg(name)
            .arg(sourceInterfaceDesc.toString())
            .arg(sourceIp4.isValid() ? sourceIp4.toString() : "*")
            .arg(sourcePort.isValid() ? sourcePort.toString() : "*")
            .arg(destinationInterfaceDesc.toString())
            .arg(destinationIp4.isValid() ? destinationIp4.toString() : "*")
            .arg(destinationPort.isValid() ? destinationPort.toString() : "*");
    }
};
