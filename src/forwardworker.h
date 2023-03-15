#pragma once

#include <QObject>

struct ForwardRule;

using InterfaceRules = std::map<QString, std::vector<ForwardRule>>;

class ForwardWorker : public QObject {
    Q_OBJECT

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

signals:
    void setupFinished();

private:
    std::map<QString, IFACE_STATUS> ifaceStatuses;
};
