/***************************************************************************
**                                                                        **
**  Controlling interface for K8090 8-Channel Relay Card from Velleman    **
**  through usb using virtual serial port in Qt.                          **
**  Copyright (C) 2018 Jakub Klener                                       **
**                                                                        **
**  This file is part of SpRelay application.                             **
**                                                                        **
**  You can redistribute it and/or modify it under the terms of the       **
**  3-Clause BSD License as published by the Open Source Initiative.      **
**                                                                        **
**  This program is distributed in the hope that it will be useful,       **
**  but WITHOUT ANY WARRANTY; without even the implied warranty of        **
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the          **
**  3-Clause BSD License for more details.                                **
**                                                                        **
**  You should have received a copy of the 3-Clause BSD License along     **
**  with this program.                                                    **
**  If not, see https://opensource.org/licenses/                          **
**                                                                        **
****************************************************************************/

#include "central_widget.h"

#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalMapper>
#include <QSpinBox>
#include <QStringBuilder>
#include <QTimer>

#include <limits>
#include <cstdint>

#include "indicator_button.h"
#include "sprelay/core/k8090.h"


namespace sprelay {
namespace gui {

CentralWidget::CentralWidget(core::K8090 *k8090, const QString &com_port_name, QWidget *parent)
    : QWidget{parent},
      com_port_name_{com_port_name},
      refresh_delay_timer_{new QTimer}
{
    if (k8090) {
        k8090_ = k8090;
    } else {
        k8090_ = new core::K8090(this);
    }
    refresh_delay_timer_->setSingleShot(true);
    connect(refresh_delay_timer_.get(), &QTimer::timeout, this, &CentralWidget::onRefreshTimersDelay);
    connected_ = k8090_->isConnected();

    constructGui();
    connectionStatusChanged();
    if (k8090_->isConnected()) {
        onRefreshRelaysButtonClicked();
    }
}


CentralWidget::~CentralWidget()
{
}


void CentralWidget::onConnectButtonClicked()
{
    k8090_->setComPortName(ports_combo_box_->currentText());
    k8090_->connectK8090();
}


void CentralWidget::onPortsComboBoxCurrentIndexChanged(const QString &portName)
{
    Q_UNUSED(portName)
    // TODO(lumik): probably disconnect if port name changes.
}


void CentralWidget::onRefreshPortsButtonClicked()
{
    if (!k8090_->availablePorts().isEmpty()) {
        QString msg;
        QString currPort = ports_combo_box_->currentText();
        QStringList comPortNames;
        foreach (const core::serial_utils::ComPortParams &comPortParams,  // NOLINT(whitespace/parens)
                 core::K8090::availablePorts()) {
            msg.append(tr(
                "Port name: %1\n"
                "Description: %2\n"
                "Manufacturer: %3\n"
                "Product Identifier: %4\n"
                "Vendor Identifier: %5\n")
                .arg(comPortParams.port_name)
                .arg(comPortParams.description)
                .arg(comPortParams.manufacturer)
                .arg(comPortParams.product_identifier)
                .arg(comPortParams.vendor_identifier));
            comPortNames.append(comPortParams.port_name);
        }
        QMessageBox::information(this, tr("Serial ports information:"), msg, QMessageBox::Ok);
        bool ok = true;
        bool counteq = false;
        if ((counteq = (ports_combo_box_->count() == comPortNames.count()))) {
            for (int ii = 0; ii < comPortNames.count(); ++ii) {
                if (ports_combo_box_->findText(comPortNames.at(ii)) < 0) {
                    ok = false;
                }
            }
        }
        if (!counteq || !ok) {
            ports_combo_box_->clear();
            ports_combo_box_->insertItems(1, comPortNames);
            int index;
            if ((index = ports_combo_box_->findText(currPort)) >= 0)
                ports_combo_box_->setCurrentIndex(index);
            else if ((index = ports_combo_box_->findText(com_port_name_)) >= 0)
                ports_combo_box_->setCurrentIndex(index);
            if (!connected_ && index == -1) {
                k8090_->setComPortName(ports_combo_box_->currentText());
            }
            com_port_name_ = ports_combo_box_->currentText();
        }
    } else {
        // TODO(lumik): there is no port, probably exception, because there should be virtual one
    }
}


void CentralWidget::onRefreshRelaysButtonClicked()
{
    k8090_->refreshRelaysInfo();
}


void CentralWidget::resetFactoryDefaultsButtonClicked()
{
    k8090_->resetFactoryDefaults();
    k8090_->refreshRelaysInfo();
}


void CentralWidget::onRelayOnButtonClicked(int relay)
{
    k8090_->switchRelayOn(core::K8090Traits::from_number(relay));
}


void CentralWidget::onRelayOffButtonClicked(int relay)
{
    k8090_->switchRelayOff(core::K8090Traits::from_number(relay));
}


void CentralWidget::onToggleRelayButtonClicked(int relay)
{
    k8090_->toggleRelay(core::K8090Traits::from_number(relay));
}


void CentralWidget::onMomentaryButtonClicked(int relay)
{
    // TODO(lumik): make method for computing button modes, probably store them inside some struct. Make general
    // mechanism of their synchronisation with states saved in indicator buttons.
    // get current button modes
    core::K8090Traits::RelayID momentary = core::K8090Traits::RelayID::NONE;
    core::K8090Traits::RelayID toggle = core::K8090Traits::RelayID::NONE;
    core::K8090Traits::RelayID timed = core::K8090Traits::RelayID::NONE;
    for (int i = 0; i < kNRelays; ++i) {
        if (momentary_buttons_arr_[i]->state()) {
            momentary |= core::K8090Traits::from_number(i);
        }
        if (toggle_mode_buttons_arr_[i]->state()) {
            toggle |= core::K8090Traits::from_number(i);
        }
        if (timed_buttons_arr_[i]->state()) {
            timed |= core::K8090Traits::from_number(i);
        }
    }

    // set the required mode
    momentary |= core::K8090Traits::from_number(relay);
    toggle &= ~core::K8090Traits::from_number(relay);
    timed &= ~core::K8090Traits::from_number(relay);
    k8090_->setButtonMode(momentary, toggle, timed);
}


void CentralWidget::onToggleModeButtonClicked(int relay)
{
    // TODO(lumik): make method for computing button modes, probably store them inside some struct. Make general
    // mechanism of their synchronisation with states saved in indicator buttons.
    // get current button modes
    core::K8090Traits::RelayID momentary = core::K8090Traits::RelayID::NONE;
    core::K8090Traits::RelayID toggle = core::K8090Traits::RelayID::NONE;
    core::K8090Traits::RelayID timed = core::K8090Traits::RelayID::NONE;
    for (int i = 0; i < kNRelays; ++i) {
        if (momentary_buttons_arr_[i]->state()) {
            momentary |= core::K8090Traits::from_number(i);
        }
        if (toggle_mode_buttons_arr_[i]->state()) {
            toggle |= core::K8090Traits::from_number(i);
        }
        if (timed_buttons_arr_[i]->state()) {
            timed |= core::K8090Traits::from_number(i);
        }
    }

    // set the required mode
    momentary &= ~core::K8090Traits::from_number(relay);
    toggle |= core::K8090Traits::from_number(relay);
    timed &= ~core::K8090Traits::from_number(relay);
    k8090_->setButtonMode(momentary, toggle, timed);
}


void CentralWidget::onTimedButtonClicked(int relay)
{
    // TODO(lumik): make method for computing button modes, probably store them inside some struct. Make general
    // mechanism of their synchronisation with states saved in indicator buttons.
    // get current button modes
    core::K8090Traits::RelayID momentary = core::K8090Traits::RelayID::NONE;
    core::K8090Traits::RelayID toggle = core::K8090Traits::RelayID::NONE;
    core::K8090Traits::RelayID timed = core::K8090Traits::RelayID::NONE;
    for (int i = 0; i < kNRelays; ++i) {
        if (momentary_buttons_arr_[i]->state()) {
            momentary |= core::K8090Traits::from_number(i);
        }
        if (toggle_mode_buttons_arr_[i]->state()) {
            toggle |= core::K8090Traits::from_number(i);
        }
        if (timed_buttons_arr_[i]->state()) {
            timed |= core::K8090Traits::from_number(i);
        }
    }

    // set the required mode
    momentary &= ~core::K8090Traits::from_number(relay);
    toggle &= ~core::K8090Traits::from_number(relay);
    timed |= core::K8090Traits::from_number(relay);
    k8090_->setButtonMode(momentary, toggle, timed);
}


void CentralWidget::onSetDefaultTimerButtonClicked(int relay)
{
    k8090_->setRelayTimerDelay(core::K8090Traits::from_number(relay), timer_spin_box_arr_[relay]->value());
}


void CentralWidget::onStartTimerButtonClicked(int relay)
{
    k8090_->startRelayTimer(core::K8090Traits::from_number(relay), timer_spin_box_arr_[relay]->value());
}


void CentralWidget::onTimerSpinBoxValueChanged(int relay)
{
    // TODO(lumik): probably remove this method.
    Q_UNUSED(relay)
}


void CentralWidget::onRelayStatus(core::K8090Traits::RelayID previous, core::K8090Traits::RelayID current,
    core::K8090Traits::RelayID timed)
{
    Q_UNUSED(previous)
    bool is_timed = false;
    for (int i = 0; i < kNRelays; ++i) {
        if (static_cast<bool>(core::K8090Traits::from_number(i) & current)) {
            relay_on_buttons_arr_[i]->setState(true);
        } else {
            relay_on_buttons_arr_[i]->setState(false);
            onRemainingTimerDelay(core::K8090Traits::from_number(i), 0);
        }
        if (static_cast<bool>(core::K8090Traits::from_number(i) & timed)) {
            is_timed = true;
            start_timer_buttons_arr_[i]->setState(true);
        } else {
            start_timer_buttons_arr_[i]->setState(false);
        }
    }
    if (is_timed && !refresh_delay_timer_->isActive()) {
        onRefreshTimersDelay();
    }
}


void CentralWidget::onButtonStatus(core::K8090Traits::RelayID state, core::K8090Traits::RelayID pressed,
    core::K8090Traits::RelayID released)
{
    Q_UNUSED(pressed)
    Q_UNUSED(released)
    for (int i = 0; i < kNRelays; ++i) {
        if (static_cast<bool>(core::K8090Traits::from_number(i) & state)) {
            pushed_indicators_arr_[i]->setState(true);
        } else {
            pushed_indicators_arr_[i]->setState(false);
        }
    }
}


void CentralWidget::onTotalTimerDelay(core::K8090Traits::RelayID relay, quint16 delay)
{
    for (int i = 0; i < kNRelays; ++i) {
        if (static_cast<bool>(core::K8090Traits::from_number(i) & relay)) {
            default_timer_labels_arr_[i]->setText(tr("%1").arg(delay));
        }
    }
}


void CentralWidget::onRemainingTimerDelay(core::K8090Traits::RelayID relay, quint16 delay)
{
    for (int i = 0; i < kNRelays; ++i) {
        if (static_cast<bool>(core::K8090Traits::from_number(i) & relay)) {
            if (start_timer_buttons_arr_[i]->state()) {
                remaining_time_labels_arr_[i]->setText(tr("%1").arg(delay));
            } else {
                remaining_time_labels_arr_[i]->setText(tr("%1").arg(0));
            }
        }
    }
}


void CentralWidget::onButtonModes(core::K8090Traits::RelayID momentary, core::K8090Traits::RelayID toggle,
    core::K8090Traits::RelayID timed)
{
    for (int i = 0; i < kNRelays; ++i) {
        if (static_cast<bool>(core::K8090Traits::from_number(i) & momentary)) {
            momentary_buttons_arr_[i]->setState(true);
        } else {
            momentary_buttons_arr_[i]->setState(false);
        }
        if (static_cast<bool>(core::K8090Traits::from_number(i) & toggle)) {
            toggle_mode_buttons_arr_[i]->setState(true);
        } else {
            toggle_mode_buttons_arr_[i]->setState(false);
        }
        if (static_cast<bool>(core::K8090Traits::from_number(i) & timed)) {
            timed_buttons_arr_[i]->setState(true);
        } else {
            timed_buttons_arr_[i]->setState(false);
        }
    }
}


void CentralWidget::onJumperStatus(bool on)
{
    jumper_status_light_->setState(on);
}


void CentralWidget::onFirmwareVersion(int year, int week)
{
    firmware_version_label_->setText(tr("Firmware version: %1.%2").arg(year).arg(week));
}


void CentralWidget::onConnected()
{
    if (!connected_) {
        connected_ = true;
        connectionStatusChanged();
    }
}


void CentralWidget::onConnectionFailed()
{
    connected_ = false;
    connectionStatusChanged();
    QMessageBox::critical(this, tr("Communication failed!"), tr("The communication with the relay failed."));
}


void CentralWidget::onNotConnected()
{
    connected_ = false;
    QMessageBox msgBox;
    msgBox.setText(tr("Connect the relay before you use it."));
    msgBox.exec();
}


void CentralWidget::onDisconnected()
{
    if (connected_) {
        connected_ = false;
        connect_button_->setState(false);
        connectionStatusChanged();
    }
}


void CentralWidget::onRefreshTimersDelay()
{
    bool is_timed;
    for (int i = 0; i < kNRelays; ++i) {
        if (start_timer_buttons_arr_[i]->state()) {
            is_timed = true;
            k8090_->queryRemainingTimerDelay(core::K8090Traits::from_number(i));
        }
    }
    if (is_timed & !refresh_delay_timer_->isActive()) {
        refresh_delay_timer_->start(kRefreshTimersRateMs_);
    }
}


void CentralWidget::constructGui()
{
    createUiElements();
    connectGui();
    makeLayout();
}


void CentralWidget::createUiElements()
{
    // COM port
    connect_button_.reset(new IndicatorButton{tr("Connect"), this});
    refresh_ports_button_.reset(new QPushButton{tr("Refresh Ports"), this});
    ports_combo_box_.reset(new QComboBox(this));
    initializePortsCombobox();

    // relays
    // globals
    refresh_relays_button_.reset(new QPushButton{tr("Refresh"), this});
    reset_factory_defaults_button_.reset(new QPushButton{tr("Factory Reset"), this});
    // TODO(lumik): version label causes the gui width change. Consider setting reasonable minimal width.
    firmware_version_label_.reset(new QLabel{tr("Firmware version: 1.0.0"), this});
    jumper_status_light_.reset(new IndicatorLight{this});
    for (int i = 0; i < kNRelays; ++i) {
        pushed_indicators_arr_[i].reset(new IndicatorLight{this});
        relay_on_buttons_arr_[i].reset(new IndicatorButton{this});
        relay_off_buttons_arr_[i].reset(new QPushButton{this});
        toggle_relay_buttons_arr_[i].reset(new QPushButton{this});
        momentary_buttons_arr_[i].reset(new IndicatorButton{this});
        toggle_mode_buttons_arr_[i].reset(new IndicatorButton{this});
        timed_buttons_arr_[i].reset(new IndicatorButton{this});
        default_timer_labels_arr_[i].reset(new QLabel{"0", this});
        remaining_time_labels_arr_[i].reset(new QLabel{"0", this});
        set_default_timer_buttons_arr_[i].reset(new QPushButton{this});
        start_timer_buttons_arr_[i].reset(new IndicatorButton{this});
        timer_spin_box_arr_[i].reset(new QSpinBox{this});
        timer_spin_box_arr_[i]->setMinimum(0);
        timer_spin_box_arr_[i]->setMaximum(std::numeric_limits<std::uint16_t>::max());
    }
}


void CentralWidget::initializePortsCombobox()
{
    int index = 0;
    QList<core::serial_utils::ComPortParams> com_port_params_list = core::K8090::availablePorts();
    bool current_port_found = false;
    // fill combo box
    for (const core::serial_utils::ComPortParams &com_port_params : com_port_params_list) {
        ports_combo_box_->insertItem(index++, com_port_params.port_name);
        if (com_port_name_ == com_port_params.port_name) {
            current_port_found = true;
        }
    }

    // if com_port_name_ doesn't match any port, try to find card.
    if (!current_port_found) {
        for (const core::serial_utils::ComPortParams &com_port_params : com_port_params_list) {
            if (com_port_params.product_identifier == core::K8090::kProductID
                    && com_port_params.vendor_identifier == core::K8090::kVendorID) {
                com_port_name_ = com_port_params.port_name;
                break;
            }
        }
    }
    if ((index = ports_combo_box_->findText(com_port_name_)) != -1)
        ports_combo_box_->setCurrentIndex(index);
    com_port_name_ = ports_combo_box_->currentText();
}


void CentralWidget::connectGui()
{
    // reactions on user interaction with gui
    connect(connect_button_.get(), &QPushButton::clicked, this, &CentralWidget::onConnectButtonClicked);
    connect(refresh_ports_button_.get(), &QPushButton::clicked, this, &CentralWidget::onRefreshPortsButtonClicked);
    connect(ports_combo_box_.get(), static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::currentIndexChanged),
            this, &CentralWidget::onPortsComboBoxCurrentIndexChanged);

    connect(refresh_relays_button_.get(), &QPushButton::clicked, this, &CentralWidget::onRefreshRelaysButtonClicked);
    connect(reset_factory_defaults_button_.get(), &QPushButton::clicked,
            this, &CentralWidget::resetFactoryDefaultsButtonClicked);

    relay_on_mapper_ = std::unique_ptr<QSignalMapper>{new QSignalMapper};
    relay_off_mapper_ = std::unique_ptr<QSignalMapper>{new QSignalMapper};
    toggle_relay_mapper_ = std::unique_ptr<QSignalMapper>{new QSignalMapper};
    momentary_mapper_ = std::unique_ptr<QSignalMapper>{new QSignalMapper};
    toggle_mode_mapper_ = std::unique_ptr<QSignalMapper>{new QSignalMapper};
    timed_mapper_ = std::unique_ptr<QSignalMapper>{new QSignalMapper};
    set_default_timer_mapper_ = std::unique_ptr<QSignalMapper>{new QSignalMapper};
    start_timer_mapper_ = std::unique_ptr<QSignalMapper>{new QSignalMapper};
    timer_spin_box_mapper_ = std::unique_ptr<QSignalMapper>{new QSignalMapper};
    for (int i = 0; i < 8; ++i) {
        connect(relay_on_buttons_arr_[i].get(), &IndicatorButton::clicked,
                relay_on_mapper_.get(), static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
        relay_on_mapper_->setMapping(relay_on_buttons_arr_[i].get(), i);
        connect(relay_off_buttons_arr_[i].get(), &IndicatorButton::clicked,
                relay_off_mapper_.get(), static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
        relay_off_mapper_->setMapping(relay_off_buttons_arr_[i].get(), i);
        connect(toggle_relay_buttons_arr_[i].get(), &QPushButton::clicked,
                toggle_relay_mapper_.get(), static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
        toggle_relay_mapper_->setMapping(toggle_relay_buttons_arr_[i].get(), i);
        connect(momentary_buttons_arr_[i].get(), &QPushButton::clicked,
                momentary_mapper_.get(), static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
        momentary_mapper_->setMapping(momentary_buttons_arr_[i].get(), i);
        connect(timed_buttons_arr_[i].get(), &IndicatorButton::clicked,
                timed_mapper_.get(), static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
        toggle_mode_mapper_->setMapping(toggle_mode_buttons_arr_[i].get(), i);
        connect(set_default_timer_buttons_arr_[i].get(), &IndicatorButton::clicked,
                set_default_timer_mapper_.get(), static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
        timed_mapper_->setMapping(timed_buttons_arr_[i].get(), i);
        connect(toggle_mode_buttons_arr_[i].get(), &IndicatorButton::clicked,
                toggle_mode_mapper_.get(), static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
        set_default_timer_mapper_->setMapping(set_default_timer_buttons_arr_[i].get(), i);
        connect(start_timer_buttons_arr_[i].get(), &IndicatorButton::clicked,
                start_timer_mapper_.get(), static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
        start_timer_mapper_->setMapping(start_timer_buttons_arr_[i].get(), i);
        connect(timer_spin_box_arr_[i].get(), static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                timer_spin_box_mapper_.get(), static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
        timer_spin_box_mapper_->setMapping(timer_spin_box_arr_[i].get(), i);
    }
    connect(relay_on_mapper_.get(), static_cast<void (QSignalMapper::*)(int)>(&QSignalMapper::mapped),
            this, &CentralWidget::onRelayOnButtonClicked);
    connect(relay_off_mapper_.get(), static_cast<void (QSignalMapper::*)(int)>(&QSignalMapper::mapped),
            this, &CentralWidget::onRelayOffButtonClicked);
    connect(toggle_relay_mapper_.get(), static_cast<void (QSignalMapper::*)(int)>(&QSignalMapper::mapped),
            this, &CentralWidget::onToggleRelayButtonClicked);
    connect(momentary_mapper_.get(), static_cast<void (QSignalMapper::*)(int)>(&QSignalMapper::mapped),
            this, &CentralWidget::onMomentaryButtonClicked);
    connect(toggle_mode_mapper_.get(), static_cast<void (QSignalMapper::*)(int)>(&QSignalMapper::mapped),
            this, &CentralWidget::onToggleModeButtonClicked);
    connect(timed_mapper_.get(), static_cast<void (QSignalMapper::*)(int)>(&QSignalMapper::mapped),
            this, &CentralWidget::onTimedButtonClicked);
    connect(set_default_timer_mapper_.get(), static_cast<void (QSignalMapper::*)(int)>(&QSignalMapper::mapped),
            this, &CentralWidget::onSetDefaultTimerButtonClicked);
    connect(start_timer_mapper_.get(), static_cast<void (QSignalMapper::*)(int)>(&QSignalMapper::mapped),
            this, &CentralWidget::onStartTimerButtonClicked);
    connect(timer_spin_box_mapper_.get(), static_cast<void (QSignalMapper::*)(int)>(&QSignalMapper::mapped),
            this, &CentralWidget::onTimerSpinBoxValueChanged);

    // reactions to signals from the relay
    connect(k8090_, &core::K8090::relayStatus, this, &CentralWidget::onRelayStatus);
    connect(k8090_, &core::K8090::buttonStatus, this, &CentralWidget::onButtonStatus);
    connect(k8090_, &core::K8090::totalTimerDelay, this, &CentralWidget::onTotalTimerDelay);
    connect(k8090_, &core::K8090::remainingTimerDelay, this, &CentralWidget::onRemainingTimerDelay);
    connect(k8090_, &core::K8090::buttonModes, this, &CentralWidget::onButtonModes);
    connect(k8090_, &core::K8090::jumperStatus, this, &CentralWidget::onJumperStatus);
    connect(k8090_, &core::K8090::firmwareVersion, this, &CentralWidget::onFirmwareVersion);
    connect(k8090_, &core::K8090::connected, this, &CentralWidget::onConnected);
    connect(k8090_, &core::K8090::connectionFailed, this, &CentralWidget::onConnectionFailed);
    connect(k8090_, &core::K8090::notConnected, this, &CentralWidget::onNotConnected);
    connect(k8090_, &core::K8090::disconnected, this, &CentralWidget::onDisconnected);
}


void CentralWidget::makeLayout()
{
    QHBoxLayout *main_layout = new QHBoxLayout;
    setLayout(main_layout);
    main_layout->setSizeConstraint(QLayout::SetFixedSize);

    // COM port settings
    QVBoxLayout *port_v_layout = new QVBoxLayout;
    main_layout->addLayout(port_v_layout, 0);
    port_v_layout->addWidget(connect_button_.get());
    QLabel *ports_label = new QLabel(tr("Select port:"), this);
    ports_label->setBuddy(ports_combo_box_.get());  // buddy accepts focus instead of label (for editing)
    port_v_layout->addWidget(ports_label);
    port_v_layout->addWidget(ports_combo_box_.get());
    port_v_layout->addWidget(refresh_ports_button_.get());

    // Relays globals
    relays_globals_box_.reset(new QGroupBox{tr("Relays globals")});
    port_v_layout->addWidget(relays_globals_box_.get());
    QVBoxLayout *relays_globals_layout = new QVBoxLayout;
    relays_globals_box_->setLayout(relays_globals_layout);
    relays_globals_layout->addWidget(refresh_relays_button_.get());
    relays_globals_layout->addWidget(firmware_version_label_.get());
    QHBoxLayout *jumper_status_layout = new QHBoxLayout;
    relays_globals_layout->addLayout(jumper_status_layout);
    jumper_status_layout->addWidget(new QLabel{tr("Jumper Status:"), this});
    jumper_status_layout->addWidget(jumper_status_light_.get());
    relays_globals_layout->addWidget(reset_factory_defaults_button_.get());

    port_v_layout->addStretch();

    // relay buttons
    QVBoxLayout *relays_grid_v_layout = new QVBoxLayout;
    main_layout->addLayout(relays_grid_v_layout);

    // relay numbers
    QGroupBox *relay_number_box = new QGroupBox{tr("Relays")};
    relays_grid_v_layout->addWidget(relay_number_box);
    QGridLayout *relay_number_grid_layout = new QGridLayout;
    relay_number_box->setLayout(relay_number_grid_layout);
    relay_number_grid_layout->addWidget(new QLabel{tr("Number:"), this}, 0, 0);
    for (int i = 0; i < kNRelays; ++i) {
        relay_number_grid_layout->addWidget(new QLabel{QString::number(i + 1), this}, 0, i + 1, Qt::AlignHCenter);
    }

    // relays button status settings
    relay_button_status_settings_box_.reset(new QGroupBox{tr("Relay button status")});
    relays_grid_v_layout->addWidget(relay_button_status_settings_box_.get());
    QGridLayout *button_status_grid_layout = new QGridLayout;
    relay_button_status_settings_box_->setLayout(button_status_grid_layout);
    button_status_grid_layout->addWidget(new QLabel{tr("pushed:"), this}, 0, 0);
    for (int i = 0; i < kNRelays; ++i) {
        button_status_grid_layout->addWidget(pushed_indicators_arr_[i].get(), 0, i + 1, Qt::AlignHCenter);
    }

    // relays power settings
    relay_power_settings_box_.reset(new QGroupBox{tr("Relays power settings")});
    relays_grid_v_layout->addWidget(relay_power_settings_box_.get());
    QGridLayout *power_grid_layout = new QGridLayout;
    relay_power_settings_box_->setLayout(power_grid_layout);
    power_grid_layout->addWidget(new QLabel{tr("Switch on:"), this}, 0, 0);
    power_grid_layout->addWidget(new QLabel{tr("Switch off:"), this}, 1, 0);
    power_grid_layout->addWidget(new QLabel{tr("Toggle:"), this}, 2, 0);
    for (int i = 0; i < kNRelays; ++i) {
        power_grid_layout->addWidget(relay_on_buttons_arr_[i].get(), 0, i + 1, Qt::AlignHCenter);
        power_grid_layout->addWidget(relay_off_buttons_arr_[i].get(), 1, i + 1, Qt::AlignHCenter);
        power_grid_layout->addWidget(toggle_relay_buttons_arr_[i].get(), 2, i + 1, Qt::AlignHCenter);
    }

    // relays mode settings
    relay_mode_settings_box_.reset(new QGroupBox{tr("Relays mode settings")});
    relays_grid_v_layout->addWidget(relay_mode_settings_box_.get());
    QGridLayout *mode_grid_layout = new QGridLayout;
    relay_mode_settings_box_->setLayout(mode_grid_layout);
    mode_grid_layout->addWidget(new QLabel(tr("Momentary:"), this), 0, 0);
    mode_grid_layout->addWidget(new QLabel(tr("Toggle:"), this), 1, 0);
    mode_grid_layout->addWidget(new QLabel(tr("Timed:"), this), 2, 0);
    for (int i = 0; i < kNRelays; ++i) {
        mode_grid_layout->addWidget(momentary_buttons_arr_[i].get(), 0, i + 1, Qt::AlignHCenter);
        mode_grid_layout->addWidget(toggle_mode_buttons_arr_[i].get(), 1, i + 1, Qt::AlignHCenter);
        mode_grid_layout->addWidget(timed_buttons_arr_[i].get(), 2, i + 1, Qt::AlignHCenter);
    }

    // relay timers settings
    relay_timers_settings_box_.reset(new QGroupBox{tr("Relay timers settings")});
    relays_grid_v_layout->addWidget(relay_timers_settings_box_.get());
    QGridLayout *timer_grid_layout = new QGridLayout;
    relay_timers_settings_box_->setLayout(timer_grid_layout);
    timer_grid_layout->addWidget(new QLabel(tr("Default timer (s):"), this), 0, 0);
    timer_grid_layout->addWidget(new QLabel(tr("Remaining time (s):"), this), 1, 0);
    timer_grid_layout->addWidget(new QLabel(tr("Default:"), this), 2, 0);
    timer_grid_layout->addWidget(new QLabel(tr("Start:"), this), 3, 0);
    timer_grid_layout->addWidget(new QLabel(tr("Timer (s):"), this), 4, 0);
    for (int i = 0; i < kNRelays; ++i) {
        timer_grid_layout->addWidget(default_timer_labels_arr_[i].get(), 0, i + 1, Qt::AlignHCenter);
        timer_grid_layout->addWidget(remaining_time_labels_arr_[i].get(), 1, i + 1, Qt::AlignHCenter);
        timer_grid_layout->addWidget(set_default_timer_buttons_arr_[i].get(), 2, i + 1, Qt::AlignHCenter);
        timer_grid_layout->addWidget(start_timer_buttons_arr_[i].get(), 3, i + 1, Qt::AlignHCenter);
        timer_grid_layout->addWidget(timer_spin_box_arr_[i].get(), 4, i + 1, Qt::AlignHCenter);
    }

    const int layout_no = 5;
    QGridLayout * grid_layouts[layout_no] = {button_status_grid_layout, relay_number_grid_layout, power_grid_layout,
        mode_grid_layout, timer_grid_layout};
    int relay_label_min_width = std::numeric_limits<int>::min();
    int relay_label_width;
    for (int i = 0; i < layout_no; ++i) {
        for (int j = 0; j < grid_layouts[i]->rowCount(); ++j) {
            relay_label_width = grid_layouts[i]->itemAtPosition(j, 0)->sizeHint().width();
            if (relay_label_width > relay_label_min_width) {
                relay_label_min_width = relay_label_width;
            }
        }
    }
    for (int i = 0; i < layout_no; ++i) {
        grid_layouts[i]->setColumnMinimumWidth(0, relay_label_min_width);
    }

    relays_grid_v_layout->addStretch();
    main_layout->addStretch();
}


void CentralWidget::connectionStatusChanged()
{
    connect_button_->setState(connected_);

    relays_globals_box_->setEnabled(connected_);
    relay_button_status_settings_box_->setEnabled(connected_);
    relay_power_settings_box_->setEnabled(connected_);
    relay_mode_settings_box_->setEnabled(connected_);
    relay_timers_settings_box_->setEnabled(connected_);
}


}  // namespace gui
}  // namespace sprelay
