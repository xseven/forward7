#include "forwardworker.h"

#include "forward_rule.h"

#include "EthLayer.h"
#include "IPv4Layer.h"
#include "NetworkUtils.h"
#include "PcapLiveDeviceList.h"
#include "UdpLayer.h"

// namespace {
void onPacket(pcpp::RawPacket* rawPacket, pcpp::PcapLiveDevice* device, void* cookie)
{
    auto forwardWorker = reinterpret_cast<ForwardWorker*>(cookie);

    pcpp::Packet packet(rawPacket);

    if (packet.isPacketOfType(pcpp::UDP)) {

        auto iface = QString::fromStdString(device->getName());

        const auto& macTable = forwardWorker->macTable;
        const auto& ifaceRules = forwardWorker->interfaceRules;

        auto ip4Layer = packet.getLayerOfType<pcpp::IPv4Layer>();
        if (ip4Layer) {
#if 0
            qDebug() << "[ip layer] src: " << QString::fromStdString(ip4Layer->getSrcIPv4Address().toString())
                     << " dst: " << QString::fromStdString(ip4Layer->getDstIPv4Address().toString());
#endif
            //[Outgoing packet. Skip it]
            if (device->getIPv4Address() == ip4Layer->getSrcIPAddress()) {
                return;
            }

            auto udpLayer = packet.getLayerOfType<pcpp::UdpLayer>();
            if (udpLayer) {
                if (forwardWorker->incomingPortFilters[device->getName()].count(udpLayer->getDstPort()) == 0) {
                    return;
                }
                auto routeKey = std::make_pair(device->getName(), udpLayer->getDstPort());

                if (forwardWorker->ifacePortRoutes.contains(routeKey)) {
                    auto [destIface, destIp, destPort] = forwardWorker->ifacePortRoutes.at(routeKey);

                    auto destDevice = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByName(destIface);
                    auto destMac = macTable.at(QString("%1-%2").arg(QString::fromStdString(destDevice->getName())).arg(QString::fromStdString(destIp)));

                    {
                        pcpp::EthLayer newEthLayer(destDevice->getMacAddress(), destMac);
                        pcpp::IPv4Layer newIpLayer(destDevice->getIPv4Address(), pcpp::IPv4Address(destIp));
                        pcpp::UdpLayer newUdpLayer(udpLayer->getDstPort(), destPort);

                        pcpp::Packet newPacket(128);

                        newPacket.addLayer(&newEthLayer);
                        newPacket.addLayer(&newIpLayer);
                        newPacket.addLayer(&newUdpLayer);

                        newPacket.computeCalculateFields();

                        destDevice->sendPacket(&newPacket);
                    }
                }
            }
        }
    }
}
//}

ForwardWorker::ForwardWorker(QObject* parent)
    : QObject(parent)
{
}

void ForwardWorker::setupRules(const std::vector<ForwardRule>& rules)
{
    interfaceRules = parseRules(rules);
    openDevicesAndCheck();
    createFiltersForDevices(interfaceRules);
    createRoutes(interfaceRules);

    emit setupFinished();
}

InterfaceRules ForwardWorker::parseRules(const std::vector<ForwardRule>& rules)
{
    InterfaceRules interfaceRules;

    for (const auto& rule : rules) {
        auto sourceIfaceName = rule.sourceInterfaceName.toString();
        auto destinationIfaceName = rule.destinationInterfaceName.toString();

        ifaceStatuses[sourceIfaceName] = IFACE_STATUS::INVALID;
        ifaceStatuses[destinationIfaceName] = IFACE_STATUS::INVALID;

        interfaceRules[sourceIfaceName].push_back(rule);
    }

    return interfaceRules;
}

void ForwardWorker::openDevicesAndCheck()
{
    for (const auto& ifaceStatus : ifaceStatuses) {
        auto device = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByName(ifaceStatus.first.toStdString());

        if (!device->isOpened()) {
            device->open();

            qDebug() << "Opened device " << QString::fromStdString(device->getDesc()) << " mac: " << QString::fromStdString(device->getMacAddress().toString());
        }

        ifaceStatuses[ifaceStatus.first] = device->isOpened() && device->getMtu() > 0 ? IFACE_STATUS::VALID : IFACE_STATUS::INVALID;
    }
}

void ForwardWorker::createFiltersForDevices(const InterfaceRules& interfaceRules)
{
    for (const auto& i : interfaceRules) {
        auto iface = i.first;
        auto rules = i.second;

        auto device = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByName(iface.toStdString());
        auto deviceIp = device->getIPv4Address();

        for (const auto& rule : rules) {
            incomingPortFilters[device->getName()].insert(rule.sourcePort.toInt());

            ifacePortRoutes[std::make_pair(device->getName(), rule.sourcePort.toInt())] = std::make_tuple(
                rule.destinationInterfaceName.toString().toStdString(),
                rule.destinationIp4.toString().toStdString(),
                rule.destinationPort.toInt());
        }
    }
}

void ForwardWorker::createRoutes(const InterfaceRules& interfaceRules)
{
    for (const auto& interfaceRule : interfaceRules) {
        auto iface = interfaceRule.first;
        auto rules = interfaceRule.second;

        for (const auto& rule : rules) {
            double arpRespTime { 0 };

            auto destIface = rule.destinationInterfaceName.toString();
            auto destIp = rule.destinationIp4.toString();
            auto tableKey = QString("%1-%2").arg(destIface).arg(destIp);
            auto destDevice = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByName(destIface.toStdString());

            if (!macTable.contains(tableKey)) {
                auto destMac = pcpp::NetworkUtils::getInstance().getMacAddress(
                    pcpp::IPv4Address(destIp.toStdString()),
                    destDevice,
                    arpRespTime,
                    destDevice->getMacAddress(),
                    destDevice->getIPv4Address(),
                    5);

                if (destMac.isValid()) {
                    macTable[tableKey] = destMac.toString();
                }
            }
        }
    }
}

void ForwardWorker::startForwarding()
{
    for (const auto ifaceStatus : ifaceStatuses) {
        auto iface = ifaceStatus.first;
        auto status = ifaceStatus.second;

        if (status == IFACE_STATUS::VALID) {
            auto device = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByName(iface.toStdString());
            device->startCapture(onPacket, this);
        }
    }
}

void ForwardWorker::stopForwarding()
{
    for (const auto ifaceStatus : ifaceStatuses) {
        auto iface = ifaceStatus.first;
        auto status = ifaceStatus.second;

        if (status == IFACE_STATUS::VALID) {
            auto device = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByName(iface.toStdString());
            device->stopCapture();
            device->clearFilter();
            device->close();

            ifaceStatuses[iface] = IFACE_STATUS::INVALID;
        }
    }
}
