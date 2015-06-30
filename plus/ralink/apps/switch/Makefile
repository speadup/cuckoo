#
# Copyright (C) 2010-2013 hua.shao@mediatek.com
#
# Ralink Property Software.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=switch
PKG_RELEASE:=4

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
PKG_KCONFIG:=RALINK_MT7620 
PKG_CONFIG_DEPENDS:=$(foreach c, $(PKG_KCONFIG),$(if $(CONFIG_$c),CONFIG_$(c)))


include $(INCLUDE_DIR)/package.mk

define Package/switch
  SECTION:=ralink
  CATEGORY:=Ralink
  TITLE:=Command to config switch
  DEPENDS:=@TARGET_ramips
endef

define Package/switch/description
  An program to config switch.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

TARGET_CFLAGS += \
	-I$(LINUX_DIR)/include \
	-I$(LINUX_DIR)/drivers/net/ethernet/raeth \
	$(foreach c, $(PKG_KCONFIG),$(if $(CONFIG_$c),-DCONFIG_$(c)))

MAKE_FLAGS += \
	CFLAGS="$(TARGET_CFLAGS)"

define Build/Configure
endef

define Package/switch/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/switch $(1)/sbin
	$(INSTALL_BIN) ./files/config-vlan.sh $(1)/sbin
endef


$(eval $(call BuildPackage,switch))

