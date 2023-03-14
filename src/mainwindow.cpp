#include "mainwindow.h"

#include "addruledialog.h"

#include "IPv4Layer.h"
#include "PcapLiveDeviceList.h"

#include <QBoxLayout>
#include <QDockWidget>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QStateMachine>
#include <QToolBar>

namespace {
std::map<QString, ForwardRule> ifaceRulesMap;

void onPacket(pcpp::RawPacket* rawPacket, pcpp::PcapLiveDevice* dev, void* cookie)
{
    auto rules = (decltype(ifaceRulesMap)*)cookie;
    auto rule = rules->at(QString::fromStdString(dev->getName()));

    auto destIface = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByName(rule.destinationInterfaceName.toString().toStdString());

    pcpp::Packet packet(rawPacket);

    if (packet.isPacketOfType(pcpp::UDP)) {
        auto ip4Layer = packet.getLayerOfType<pcpp::IPv4Layer>();
        if (ip4Layer) {
            ip4Layer->setDstIPv4Address(pcpp::IPv4Address(rule.destinationIp4.toString().toStdString()));
            ip4Layer->setSrcIPv4Address(destIface->getIPv4Address());
            packet.computeCalculateFields();

            if (!destIface->isOpened())
                destIface->open();

            destIface->sendPacket(&packet);
        }
    }
}
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setupUi();
    setupUiConnections();

    setupStateMachine();
    setupStateMachineConnections();
}

void MainWindow::setupUi()
{
    //[]
    auto operationsToolBar = addToolBar(tr("Operations"));
    action.switchState = operationsToolBar->addAction(QIcon(":/sync.png"), tr("Start forwarding"));
    operationsToolBar->addSeparator();
    action.addRule = operationsToolBar->addAction(QIcon(":/add_circle.png"), tr("Add forwarding rules"));

    //[]
    {
        auto rulesDocker = new QDockWidget(this);

        {
            auto rulesWidget = new QWidget(rulesDocker);
            auto rulesLabel = new QLabel(tr("Forwarding rules"), rulesWidget);
            ui.rules = new QListWidget(rulesWidget);

            rulesWidget->setLayout(new QVBoxLayout);

            rulesWidget->layout()->addWidget(rulesLabel);
            rulesWidget->layout()->addWidget(ui.rules);

            rulesDocker->setWidget(rulesWidget);
        }

        rulesDocker->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        addDockWidget(Qt::LeftDockWidgetArea, rulesDocker);
    }
}

void MainWindow::setupUiConnections()
{
    connect(action.addRule, &QAction::triggered, this, &MainWindow::addRule);
}

void MainWindow::setupStateMachine()
{
    state.idle = new QState();

    state.idle->assignProperty(action.switchState, "text", "Click me");
    state.idle->assignProperty(action.addRule, "enabled", "true");

    state.running = new QState();
    state.running->assignProperty(action.addRule, "enabled", "false");

    state.idle->addTransition(action.switchState, &QAction::triggered, state.running);
    state.running->addTransition(action.switchState, &QAction::triggered, state.idle);

    stateMachine = new QStateMachine(this);

    stateMachine->addState(state.idle);
    stateMachine->addState(state.running);

    stateMachine->setInitialState(state.idle);

    stateMachine->start();
}

void MainWindow::setupStateMachineConnections()
{
    connect(state.running, &QState::entered, this, &MainWindow::runForwarding);
    connect(state.idle, &QState::entered, this, &MainWindow::stopForwarding);
}

void MainWindow::addRule()
{
    AddRuleDialog addRuleDialog;

    if (addRuleDialog.exec() == QDialog::Accepted) {
        auto rule = addRuleDialog.rule();

        auto item = new QListWidgetItem();
        item->setText(rule.name);
        item->setData(Qt::UserRole, QVariant::fromValue(rule));

        ui.rules->addItem(item);
    }
}

void MainWindow::runForwarding()
{
    for (int i = 0; i < ui.rules->count(); ++i) {
        auto rule = ui.rules->item(i)->data(Qt::UserRole).value<ForwardRule>();
        ifaceRulesMap.insert({ rule.sourceInterfaceName.toString(), rule });
    }

    for (const auto& i : ifaceRulesMap) {
        auto iface = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByName(i.first.toStdString());

        if (!iface->isOpened()) {
            if (!iface->open()) {
                QMessageBox::critical(this, tr("Device error"), tr("Unable to open device: [%1]").arg(i.first));
                continue;
            }

            pcpp::PortFilter portFilter(27480, pcpp::SRC_OR_DST);
            pcpp::ProtoFilter protocolFilter(pcpp::UDP);

            pcpp::AndFilter andFilter;

            andFilter.addFilter(&portFilter);
            andFilter.addFilter(&protocolFilter);

            iface->setFilter(andFilter);

            if (!iface->startCapture(onPacket, &ifaceRulesMap)) {
                QMessageBox::critical(this, tr("Device error"), tr("Unable to start capturing on device: [%1]").arg(i.first));
                continue;
            }
        }
    }
}

void MainWindow::stopForwarding()
{
    for (const auto& i : ifaceRulesMap) {
        auto iface = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByName(i.first.toStdString());
        iface->stopCapture();
        iface->close();
    }
}