#include "forwardworker.h"

#include "forward_rule.h"

#include "EthLayer.h"
#include "IPv4Layer.h"
#include "NetworkUtils.h"
#include "PcapLiveDeviceList.h"
#include "UdpLayer.h"

namespace {
void onPacket(pcpp::RawPacket* rawPacket, pcpp::PcapLiveDevice* dev, void* cookie)
{
    pcpp::Packet packet(rawPacket);

    if (packet.isPacketOfType(pcpp::UDP)) {
        auto ip4Layer = packet.getLayerOfType<pcpp::IPv4Layer>();
        if (ip4Layer) {
            qDebug() << "[ip layer] src: " << QString::fromStdString(ip4Layer->getSrcIPv4Address().toString())
                     << " dst: " << QString::fromStdString(ip4Layer->getDstIPv4Address().toString());
        }

        auto udpLayer = packet.getLayerOfType<pcpp::UdpLayer>();
        if (udpLayer) {
            qDebug() << "[udp layer] port: " << udpLayer->getDstPort();
        }
    }
}
}

ForwardWorker::ForwardWorker(QObject* parent)
    : QObject(parent)
{
}

void ForwardWorker::setupRules(const std::vector<ForwardRule>& rules)
{
    const auto& interfaceRules = parseRules(rules);
    openDevicesAndCheck();
    createFiltersForDevices(interfaceRules);

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

        if (!device->isOpened())
            device->open();

        ifaceStatuses[ifaceStatus.first] = device->isOpened() && device->getMtu() > 0 ? IFACE_STATUS::VALID : IFACE_STATUS::INVALID;
    }
}

void ForwardWorker::createFiltersForDevices(const InterfaceRules& interfaceRules)
{
    std::vector<pcpp::GeneralFilter*> filterPointers;

    pcpp::AndFilter andFilter;

    //[]
    pcpp::ProtoFilter protocolFilter(pcpp::UDP);
    andFilter.addFilter(&protocolFilter);

    for (const auto& i : interfaceRules) {
        auto iface = i.first;
        auto rules = i.second;

        //[]
        auto device = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByName(iface.toStdString());

        auto localDestinationIpOrFilter = new pcpp::OrFilter;
        filterPointers.push_back(localDestinationIpOrFilter);

        //[Local dest]
        auto deviceIp = device->getIPv4Address();

        auto localIpFilter = new pcpp::IPFilter(deviceIp.toString(), pcpp::DST);
        filterPointers.push_back(localIpFilter);
        localDestinationIpOrFilter->addFilter(localIpFilter);

        //[Local broadcast dest]
        QString localBroadcastIp;
        {
            auto s = QString::fromStdString(deviceIp.toString()).split('.', Qt::SkipEmptyParts);
            s[3] = "255";
            localBroadcastIp = s.join('.');
        }

        auto localBroadcastIpFilter = new pcpp::IPFilter(localBroadcastIp.toStdString(), pcpp::DST);
        filterPointers.push_back(localBroadcastIpFilter);
        localDestinationIpOrFilter->addFilter(localBroadcastIpFilter);

        andFilter.addFilter(localDestinationIpOrFilter);

        //[]
        auto rulesOrFilter = new pcpp::OrFilter;
        filterPointers.push_back(rulesOrFilter);

        for (const auto& rule : rules) {
            auto ruleAndFilter = new pcpp::AndFilter;
            filterPointers.push_back(ruleAndFilter);

            if (rule.sourceIp4.isValid()) {
                auto ipFilter = new pcpp::IPFilter(rule.sourceIp4.toString().toStdString(), pcpp::SRC);
                filterPointers.push_back(ipFilter);

                ruleAndFilter->addFilter(ipFilter);
            }

            if (rule.sourcePort.isValid()) {
                auto portFilter = new pcpp::PortFilter(rule.sourcePort.toInt(), pcpp::DST);
                filterPointers.push_back(portFilter);

                ruleAndFilter->addFilter(portFilter);
            }

            rulesOrFilter->addFilter(ruleAndFilter);
        }

        andFilter.addFilter(rulesOrFilter);

        if (ifaceStatuses.at(iface) == IFACE_STATUS::VALID) {
            auto device = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByName(iface.toStdString());
            device->setFilter(andFilter);
        }

        for (auto p : filterPointers) {
            delete p;
        }

        filterPointers.clear();
    }
}

void ForwardWorker::startForwarding()
{
    for (const auto ifaceStatus : ifaceStatuses) {
        auto iface = ifaceStatus.first;
        auto status = ifaceStatus.second;

        if (status == IFACE_STATUS::VALID) {
            auto device = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByName(iface.toStdString());
            device->startCapture(onPacket, nullptr);
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
