include $(TOPDIR)/rules.mk

PKG_NAME:=lora-mqtt
PKG_VERSION:=2.1.0
PKG_RELEASE:=1

# PKG_SOURCE_PROTO:=git
# PKG_SOURCE_URL:=git://github.com/unwireddevices/lora-mqtt.git
# PKG_SOURCE:=$(PKG_NAME).tar.gz
# PKG_SOURCE_VERSION:=HEAD
# PKG_SOURCE_SUBDIR:=$(PKG_NAME)

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
include $(INCLUDE_DIR)/package.mk

define Package/lora-mqtt
	SECTION:=base
	CATEGORY:=Utilities
	DEFAULT:=n
	DEPENDS:=+libmosquitto +libstdcpp
	TITLE:=LoRa-MQTT bridge for Unwired Range modules
	URL:=http://www.unwireddevices.com
endef

define Package/lora-mqtt/description
	LoRa-MQTT bridge for Unwired Range modules
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Configure
	$(call Build/Configure/Default,--with-linux-headers=$(LINUX_DIR))
endef

define Package/lora-mqtt/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/lora-mqtt
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bin/mqtt $(1)/usr/bin/
	$(INSTALL_BIN) ./files/mqtt.lora.init $(1)/etc/init.d/mqtt.lora
	$(INSTALL_CONF) ./files/mqtt.conf $(1)/etc/lora-mqtt/mqtt.conf
endef

$(eval $(call BuildPackage,lora-mqtt))
