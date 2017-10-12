Name:           lora-mqtt
Version:        2.2.0
Release:        0
Summary:        Translator between LoRa application layer and MQTT broker
URL:            https://github.com/unwireddevices/lora-mqtt
License:        GPL-2.0
Group:          Network & Connectivity/Other

Source0:        %{name}-%{version}.tar.gz
Source1001:     dist/openwrt/files/mqtt.conf
#Source1002:     lora-mqtt.service

BuildRequires:  mosquitto-devel
BuildRequires:  libopenssl-devel
BuildRequires:  libcares-devel

Requires:       libopenssl
Requires:       libcares
Requires:       libmosquitto1

%description
Translator between binary LoRa application layer and MQTT broker.
lora-mqtt is used to provide an interface between LoRa star network
and network layer and high-level protocols and applications


%prep
%setup -q

%build
make all

%install
install -D -m 644 bin/mqtt  %{buildroot}%{_bindir}/lora-mqtt
install -D -m 644 %{S:1001} %{buildroot}/etc/lora-mqtt/mqtt.conf

%files
%defattr(-,root,root,-)
%config(noreplace) %attr(-,root,%{name}) /etc/lora-mqtt/mqtt.conf
%{_bindir}/lora-mqtt
%{_unitdir}/%{name}.service
