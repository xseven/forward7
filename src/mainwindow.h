#pragma once

#include <QMainWindow>

class QAction;
class QListWidget;
class QState;
class QStateMachine;

struct ForwardRule;
class ForwardWorker;

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

    void rulesContextMenu(const QPoint& pt);

    [[nodiscrad]] std::vector<ForwardRule> rules() const;

private:
    struct {
        QListWidget* rules;
    } ui;

    struct {
        QAction* switchState;
        QAction* addRule;
    } action;

    QStateMachine* stateMachine { nullptr };

    struct {
        QState* idle;
        QState* setup;
        QState* running;
    } state;

    ForwardWorker* forwardWorker { nullptr };
};
