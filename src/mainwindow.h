#pragma once

#include <QMainWindow>

class QAction;
class QListWidget;
class QState;
class QStateMachine;

namespace pcpp {
class PcapLiveDevice;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);

private:
    void setupUi();
    void setupUiConnections();

    void setupStateMachine();
    void setupStateMachineConnections();

    void addRule();

    void runForwarding();
    void stopForwarding();

private:
    struct {
        QListWidget* rules;
    } ui;

    struct {
        QAction* switchState;
        QAction* addRule;
    } action;

    QStateMachine* stateMachine;

    struct {
        QState* running;
        QState* idle;
    } state;

    std::vector<pcpp::PcapLiveDevice*> ifaces;
};
