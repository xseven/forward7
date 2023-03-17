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
    //[]
    ui.sourceInterface = new QComboBox(this);

    ui.sourceIp4 = new QLineEdit(this);
    ui.sourceIp4->setText("*");
    ui.sourcePort = new QLineEdit(this);
    ui.sourcePort->setText("*");

    //[]
    ui.destinationInterface = new QComboBox(this);

    ui.destinationIp4 = new QLineEdit(this);
    ui.destinationPort = new QLineEdit(this);
    ui.destinationPort->setText("*");

    //[]
    ui.buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    //[]
    auto formLayout = new QFormLayout;

    formLayout->addRow(tr("Source interface:"), ui.sourceInterface);
#if 0
    formLayout->addRow(tr("Source ip4:"), ui.sourceIp4);
#endif
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
#if 0
    if (ui.sourceIp4->text() != "*") {
        pcpp::IPv4Address addr(ui.sourceIp4->text().toStdString());
        if (!addr.isValid()) {
            QMessageBox::critical(const_cast<AddRuleDialog*>(this), QString("Source ip address error"), QString("Given source ip address [%1] is invalid").arg(ui.sourceIp4->text()));
            return false;
        }
    }
#endif

    {
        bool res { false };
        auto sourcePort = ui.sourcePort->text().toInt(&res);
        if (!(res && sourcePort > 0 && sourcePort <= 65535)) {
            QMessageBox::critical(const_cast<AddRuleDialog*>(this), QString("Source port error"), QString("Given source port [%1] is invalid").arg(ui.sourcePort->text()));
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
        if (!(res && destinationPort > 0 && destinationPort <= 65535)) {
            QMessageBox::critical(const_cast<AddRuleDialog*>(this), QString("Destination port error"), QString("Given destination port [%1] is invalid").arg(ui.destinationPort->text()));
            return false;
        }
    }

    return true;
}

ForwardRule AddRuleDialog::rule() const
{
    static int n { 0 };

    ForwardRule res;

    res.name = QString("rule #%1").arg(n++);

    //[]
    res.sourceInterfaceName = ui.sourceInterface->currentData();

    if (ui.sourceIp4->text() != "*")
        res.sourceInterfaceName = ui.sourceIp4->text();

    if (ui.sourcePort->text() != "*")
        res.sourcePort = ui.sourcePort->text();

    //[]
    res.destinationInterfaceName = ui.destinationInterface->currentData();
    res.destinationIp4 = ui.destinationIp4->text();

    if (ui.destinationPort->text() != "*")
        res.destinationPort = ui.destinationPort->text();

    return res;
}
