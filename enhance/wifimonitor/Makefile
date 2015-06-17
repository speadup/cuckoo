#
# Copyright (C) 2008-2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=wifimonitor
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

define Package/wifimonitor
  SECTION:=base
  CATEGORY:=Yeetec
  DEPENDS:=+libsqlite3 +libpcap
  TITLE:=WiFi MAC collection
endef

TARGET_CPPFLAGS :=

TARGET_LDFLAGS := 
define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Configure/Default

endef

Build/Configure = $(Build/Configure/Default)

define Build/Compile
	CFLAGS="$(TARGET_CPPFLAGS) $(TARGET_CFLAGS)" \
	LDFLAGS="$(TARGET_LDFLAGS)" \
	$(MAKE) -f Makefile -C $(PKG_BUILD_DIR) \
		$(TARGET_CONFIGURE_OPTS) 
endef

define Package/wifimonitor/install
	$(INSTALL_DIR) $(1)/usr/sbin $(1)/etc/init.d/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/wifimonitor $(1)/usr/sbin
	$(INSTALL_BIN) ./files/wifimonitor.init $(1)/etc/init.d/wifimonitor
	$(INSTALL_DIR) $(1)/usr/share/sql
	$(INSTALL_DATA) ./files/table_wifimonitor.sql $(1)/usr/share/sql/
endef

$(eval $(call BuildPackage,wifimonitor))
