#pragma once

#include <QObject>

#include <set>

struct ForwardRule;

using InterfaceRules = std::map<QString, std::vector<ForwardRule>>;

namespace pcpp {
class RawPacket;
class PcapLiveDevice;
}

void onPacket(pcpp::RawPacket* rawPacket, pcpp::PcapLiveDevice* dev, void* cookie);

class ForwardWorker : public QObject {
    Q_OBJECT

    friend void onPacket(pcpp::RawPacket* rawPacket, pcpp::PcapLiveDevice* dev, void* cookie);

    enum class IFACE_STATUS {
        VALID = 1,
        INVALID
    };

public:
    ForwardWorker(QObject* parent = nullptr);

    void setupRules(const std::vector<ForwardRule>&);

    void startForwarding();
    void stopForwarding();

private:
    [[nodiscard]] InterfaceRules parseRules(const std::vector<ForwardRule>&);
    void openDevicesAndCheck();
    void createFiltersForDevices(const InterfaceRules&);
    void createRoutes(const InterfaceRules&);

signals:
    void setupFinished();

private:
    std::map<QString, IFACE_STATUS> ifaceStatuses;
    InterfaceRules interfaceRules;

    std::map<std::string, std::set<int16_t>> incomingPortFilters;
    std::map<std::pair<std::string, int16_t>, std::tuple<std::string, std::string, int16_t>> ifacePortRoutes;

    std::map<QString, std::string> macTable;
};
