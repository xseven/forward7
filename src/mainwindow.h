#pragma once

#include <QMainWindow>

class QAction;
class QListWidget;
class QState;
class QStateMachine;
class QPlainTextEdit;

struct ForwardRule;
class ForwardWorker;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);

protected:
    bool event(QEvent* event) override final;

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

    void addLogMessage(const QString& message);

private:
    struct {
        QListWidget* rules;
        QPlainTextEdit* log;
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
