#include "addruledialog.h"

#include <PcapLiveDeviceList.h>

#include <QAbstractItemView>
#include <QBoxLayout>
#include <QComboBox>
#include <QDebug>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>

AddRuleDialog::AddRuleDialog(QWidget* parent)
    : QDialog(parent)
{
    setupUi();
    setupUiConnections();

    populateInterfaces();
}

void AddRuleDialog::setupUi()
{
    setWindowTitle(tr("Add forward rule"));

    //[Source]
    ui.sourceInterface = new QComboBox(this);

    ui.sourceIp4 = new QLineEdit(this);
    ui.sourceIp4->setText("*");
    ui.sourceIp4->setReadOnly(true);

    ui.sourcePort = new QLineEdit(this);
    ui.sourcePort->setText("*");

    //[Destination]
    ui.destinationInterface = new QComboBox(this);

    ui.destinationIp4 = new QLineEdit(this);

    ui.destinationPort = new QLineEdit(this);
    ui.destinationPort->setText("*");

    //[Standard buttons]
    ui.buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    //[Layout]
    auto formLayout = new QFormLayout;

    formLayout->addRow(tr("Source interface:"), ui.sourceInterface);
    formLayout->addRow(tr("Source ip4:"), ui.sourceIp4);
    formLayout->addRow(tr("Source port:"), ui.sourcePort);

    formLayout->addRow(tr("Destination interface:"), ui.destinationInterface);
    formLayout->addRow(tr("Destination ip4:"), ui.destinationIp4);
    formLayout->addRow(tr("Destination port:"), ui.destinationPort);

    formLayout->addRow(ui.buttonBox);

    setLayout(formLayout);
}

void AddRuleDialog::setupUiConnections()
{
    connect(ui.buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        if (validateRule())
            accept();
    });

    connect(ui.buttonBox, &QDialogButtonBox::rejected, this, [this]() {
        reject();
    });

    connect(ui.sourceInterface, &QComboBox::currentIndexChanged, this, [this]() {
        auto ifaceName = ui.sourceInterface->currentData().toString().toStdString();

        auto device = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByName(ifaceName);

        if (device) {
            ui.sourceIp4->setText(QString::fromStdString(device->getIPv4Address().toString()));
        }
    });

    connect(ui.destinationInterface, &QComboBox::currentIndexChanged, this, [this]() {
        auto ifaceName = ui.destinationInterface->currentData().toString().toStdString();

        auto device = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByName(ifaceName);

        if (device) {
            ui.destinationIp4->setText(QString::fromStdString(device->getIPv4Address().toString()));
        }
    });
}

void AddRuleDialog::populateInterfaces()
{
    auto devices = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDevicesList();

    for (const auto& device : devices) {
        ui.sourceInterface->addItem(QString::fromStdString(device->getDesc()), QString::fromStdString(device->getName()));
        ui.destinationInterface->addItem(QString::fromStdString(device->getDesc()), QString::fromStdString(device->getName()));
    }
}

bool AddRuleDialog::validateRule() const
{
    {
        if (ui.sourceIp4->text() != "*") {
            pcpp::IPv4Address addr(ui.sourceIp4->text().toStdString());
            if (!addr.isValid()) {
                QMessageBox::critical(const_cast<AddRuleDialog*>(this), QString("Source ip address error"), QString("Given source ip address [%1] is invalid").arg(ui.sourceIp4->text()));
                return false;
            }
        }
    }

    {
        bool res { false };

        auto sourcePort = ui.sourcePort->text().toInt(&res);
        if (!(res && sourcePort > std::numeric_limits<uint16_t>::min() && sourcePort <= std::numeric_limits<uint16_t>::max())) {
            QMessageBox::critical(const_cast<AddRuleDialog*>(this), QString("Source port error"), QString("Given source port [%1] is invalid").arg(ui.sourcePort->text()));
            return false;
        }
    }

    {
        pcpp::IPv4Address addr(ui.destinationIp4->text().toStdString());
        if (!addr.isValid()) {
            QMessageBox::critical(const_cast<AddRuleDialog*>(this), QString("Destination ip address error"), QString("Given Destination ip address [%1] is invalid").arg(ui.destinationIp4->text()));
            return false;
        }
    }

    {
        bool res { false };

        auto destinationPort = ui.destinationPort->text().toInt(&res);
        if (!(res && destinationPort > std::numeric_limits<uint16_t>::min() && destinationPort <= std::numeric_limits<uint16_t>::max())) {
            QMessageBox::critical(const_cast<AddRuleDialog*>(this), QString("Destination port error"), QString("Given destination port [%1] is invalid").arg(ui.destinationPort->text()));
            return false;
        }
    }

    return true;
}

ForwardRule AddRuleDialog::rule() const
{
    ForwardRule res;

    res.name = QStringLiteral("[%1:%2] -> [%3:%4]")
                   .arg(ui.sourceIp4->text())
                   .arg(ui.sourcePort->text())
                   .arg(ui.destinationIp4->text())
                   .arg(ui.destinationPort->text());

    //[]
    res.sourceInterfaceName = ui.sourceInterface->currentData();
    res.sourceInterfaceDesc = ui.sourceInterface->currentText();

    if (ui.sourceIp4->text() != "*")
        res.sourceIp4 = ui.sourceIp4->text();

    if (ui.sourcePort->text() != "*")
        res.sourcePort = ui.sourcePort->text();

    //[]
    res.destinationInterfaceName = ui.destinationInterface->currentData();
    res.destinationInterfaceDesc = ui.destinationInterface->currentText();

    res.destinationIp4 = ui.destinationIp4->text();

    if (ui.destinationPort->text() != "*")
        res.destinationPort = ui.destinationPort->text();

    return res;
}
