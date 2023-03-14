#pragma once

#include "forward_rule.h"

#include <QDialog>

class QDialogButtonBox;
class QComboBox;
class QLineEdit;
class QCloseEvent;

class AddRuleDialog : public QDialog {
    Q_OBJECT

public:
    AddRuleDialog(QWidget* parent = nullptr);

    ForwardRule rule() const;

private:
    void setupUi();
    void setupUiConnections();

    void populateInterfaces();

    bool validateRule() const;

private:
    struct {
        QComboBox* sourceInterface;
        QLineEdit* sourceIp4;
        QLineEdit* sourcePort;

        QComboBox* destinationInterface;
        QLineEdit* destinationIp4;
        QLineEdit* destinationPort;

        QDialogButtonBox* buttonBox;
    } ui;
};
