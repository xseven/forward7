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

    QString toString()
    {
        return QStringLiteral("Rule: [%1] src: [%2 %3:%4] dst: [%5 %6:%7]")
            .arg(name)
            .arg(sourceInterfaceName.toString())
            .arg(sourceIp4.isValid() ? sourceIp4.toString() : "*")
            .arg(sourcePort.isValid() ? sourcePort.toString() : "*")
            .arg(destinationInterfaceName.toString())
            .arg(destinationIp4.isValid() ? destinationIp4.toString() : "*")
            .arg(destinationPort.isValid() ? destinationPort.toString() : "*");
    }
};
