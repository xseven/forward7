#include "mainwindow.h"

#include "addruledialog.h"
#include "forwardworker.h"
#include "logeventfilter.h"

#if 0
#include "EthLayer.h"
#include "IPv4Layer.h"
#include "NetworkUtils.h"
#include "PcapLiveDeviceList.h"
#endif

#include <QBoxLayout>
#include <QCoreApplication>
#include <QDockWidget>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QStateMachine>
#include <QToolBar>

#if 0
namespace {
std::map<QString, ForwardRule> ifaceRulesMap;

void onPacket(pcpp::RawPacket* rawPacket, pcpp::PcapLiveDevice* dev, void* cookie)
{
    auto rules = (decltype(ifaceRulesMap)*)cookie;
    auto rule = rules->at(QString::fromStdString(dev->getName()));

    auto destIface = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByName(rule.destinationInterfaceName.toString().toStdString());

    pcpp::Packet packet(rawPacket);

    if (packet.isPacketOfType(pcpp::UDP)) {

        pcpp::MacAddress destMac;

        auto ip4Layer = packet.getLayerOfType<pcpp::IPv4Layer>();
        if (ip4Layer) {
            ip4Layer->setDstIPv4Address(pcpp::IPv4Address(rule.destinationIp4.toString().toStdString()));
            ip4Layer->setSrcIPv4Address(destIface->getIPv4Address());

            double arpRespTime { 0 };

            destMac = pcpp::NetworkUtils::getInstance().getMacAddress(
                pcpp::IPv4Address(rule.destinationIp4.toString().toStdString()),
                destIface,
                arpRespTime,
                destIface->getMacAddress(),
                destIface->getIPv4Address(),
                5);
        }

        auto ethLayer = packet.getLayerOfType<pcpp::EthLayer>();
        if (ethLayer) {
            ethLayer->setSourceMac(destIface->getMacAddress());

            ethLayer->setDestMac(destMac);
        }

        packet.computeCalculateFields();

        if (!destIface->isOpened())
            destIface->open();

        destIface->sendPacket(&packet);
    }
}
}
#endif

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , forwardWorker(new ForwardWorker(this))
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
            ui.rules->setContextMenuPolicy(Qt::CustomContextMenu);

            rulesWidget->setLayout(new QVBoxLayout);

            rulesWidget->layout()->addWidget(rulesLabel);
            rulesWidget->layout()->addWidget(ui.rules);

            rulesDocker->setWidget(rulesWidget);
        }

        rulesDocker->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        addDockWidget(Qt::LeftDockWidgetArea, rulesDocker);
    }

    //[]
    ui.log = new QPlainTextEdit(this);
    ui.log->setReadOnly(true);
    setCentralWidget(ui.log);
}

void MainWindow::setupUiConnections()
{
    connect(action.addRule, &QAction::triggered, this, &MainWindow::addRule);
    connect(ui.rules, &QListWidget::customContextMenuRequested, this, &MainWindow::rulesContextMenu);
}

void MainWindow::setupStateMachine()
{
    //[]
    state.idle = new QState();

    state.idle->assignProperty(action.addRule, "enabled", true);
    state.idle->assignProperty(action.switchState, "icon", QIcon(":/sync.png"));
    state.idle->assignProperty(action.switchState, "text", tr("Start forwarding"));

    //[]
    state.setup = new QState();

    //[]
    state.running = new QState();

    state.running->assignProperty(action.addRule, "enabled", false);
    state.running->assignProperty(action.switchState, "icon", QIcon(":/sync_disabled.png"));
    state.running->assignProperty(action.switchState, "text", tr("Stop forwarding"));

    //[]
    state.idle->addTransition(action.switchState, &QAction::triggered, state.setup);
    state.setup->addTransition(forwardWorker, &ForwardWorker::setupFinished, state.running);
    state.running->addTransition(action.switchState, &QAction::triggered, state.idle);

    //[]
    stateMachine = new QStateMachine(this);

    stateMachine->addState(state.idle);
    stateMachine->addState(state.setup);
    stateMachine->addState(state.running);

    stateMachine->setInitialState(state.idle);

    stateMachine->start();
}

void MainWindow::setupStateMachineConnections()
{
    connect(state.idle, &QState::entered, this, &MainWindow::stopForwarding);
    connect(state.setup, &QState::entered, this, [this]() {
        qDebug() << "Setup state";

        forwardWorker->setupRules(rules());
    });
    connect(state.running, &QState::entered, this, &MainWindow::runForwarding);
}

void MainWindow::addRule()
{
    AddRuleDialog addRuleDialog;

    if (addRuleDialog.exec() == QDialog::Accepted) {
        auto rule = addRuleDialog.rule();
        qDebug().noquote() << rule.toString();

        auto item = new QListWidgetItem();
        item->setText(rule.name);
        item->setData(Qt::UserRole, QVariant::fromValue(rule));

        ui.rules->addItem(item);
    }
}

void MainWindow::runForwarding()
{
    qDebug() << "Forwarding state";

    forwardWorker->startForwarding();

#if 0
    for (const auto& i : ifaceRulesMap) {
        auto iface = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByName(i.first.toStdString());

        if (!iface->isOpened()) {
            if (!iface->open()) {
                QMessageBox::critical(this, tr("Device error"), tr("Unable to open device: [%1]").arg(i.first));
                continue;
            }

            // pcpp::PortFilter portFilter(27480, pcpp::SRC_OR_DST);
            pcpp::ProtoFilter protocolFilter(pcpp::UDP);

            pcpp::AndFilter andFilter;

            // andFilter.addFilter(&portFilter);
            andFilter.addFilter(&protocolFilter);

            iface->setFilter(andFilter);

            if (!iface->startCapture(onPacket, &ifaceRulesMap)) {
                QMessageBox::critical(this, tr("Device error"), tr("Unable to start capturing on device: [%1]").arg(i.first));
                continue;
            }
        }
    }
#endif
}

void MainWindow::stopForwarding()
{
    qDebug() << "Idle state";

    forwardWorker->stopForwarding();
#if 0
    for (const auto& i : ifaceRulesMap) {
        auto iface = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByName(i.first.toStdString());
        iface->stopCapture();
        iface->close();
    }
#endif
}

std::vector<ForwardRule> MainWindow::rules() const
{
    std::vector<ForwardRule> res;
    res.reserve(ui.rules->count());

    for (int i = 0; i < ui.rules->count(); ++i) {
        res.push_back(ui.rules->item(i)->data(Qt::UserRole).value<ForwardRule>());
    }

    return res;
}

void MainWindow::rulesContextMenu(const QPoint& pt)
{
    auto globalPos = ui.rules->mapToGlobal(pt);

    QMenu rulesMenu;

    auto addRuleAction = rulesMenu.addAction("Add", this, &MainWindow::addRule);

    auto deleteRuleAction = rulesMenu.addAction("Remove", this, [this]() {
        auto index = ui.rules->currentIndex();
        ui.rules->takeItem(index.row());
    });

    addRuleAction->setEnabled(state.idle->active());
    deleteRuleAction->setEnabled(state.idle->active() && ui.rules->count() > 0);

    rulesMenu.exec(globalPos);
}

void MainWindow::addLogMessage(const QString& message)
{
    ui.log->appendPlainText(message);
    // ui.log->verticalScrollBar()->setValue(ui.log->verticalScrollBar()->maximum());

    QCoreApplication::processEvents();
}

bool MainWindow::event(QEvent* event)
{
    if (event->type() == LogEvent::logEventType) {
        auto logEvent = static_cast<LogEvent*>(event);
        addLogMessage(logEvent->message());
        return true;
    }

    return QWidget::event(event);
}
