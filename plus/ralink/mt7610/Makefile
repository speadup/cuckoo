# All rights reserved.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=mt7610
PKG_VERSION:=wifi_v3006_dpa_20130613

PKG_KCONFIG:= \
	RALINK_MT7620 MT7610_AP MT7610_AP_V24_DATA_STRUCTURE MT7610_AP_LED MT7610_AP_WSC MT7610_AP_WSC_V2 \
	MT7610_AP_LLTD MT7610_AP_WDS MT7610_AP_MBSS NEW_MBSSID_MODE MT7610_AP_APCLI MT7610_AP_MAC_REPEATER \
	MT7610_AP_IGMP_SNOOP MT7610_AP_DFS MT7610_AP_CARRIER M7610_CON_WPS_SUPPORT MT7610_AP_80211N_DRAFT3 \
	MT7610_AP_ATE MT7610_AP_QA MT7610_AP_FLASH MT7610_AP_BIG_ENDIAN MT7610_AP_TSSI_COMPENSATION \
	RTMP_TEMPERATURE_COMPENSATION MT7610_AP_SINGLE_SKU

PKG_CONFIG_DEPENDS:=$(foreach c, $(PKG_KCONFIG),$(if $(CONFIG_$c),CONFIG_$(c)))

include $(INCLUDE_DIR)/package.mk

TAR_CMD=$(HOST_TAR) -C $(1)/ $(TAR_OPTIONS)

define KernelPackage/mt7610
  SECTION:=kernel
  CATEGORY:=Kernel modules
  TITLE:=Ralink mt7610e wifi AP driver
  FILES:=$(PKG_BUILD_DIR)/MT7610_ap.ko
  DEPENDS:=@TARGET_ramips_mt7620 +wireless-tools +maccalc
  AUTOLOAD:=$(call AutoLoad,91,MT7610_ap)
  SUBMENU:=Wireless Drivers
  MENU:=1
endef

define KernelPackage/mt7610/config
	source "$(SOURCE)/Config.in"
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		CROSS_COMPILE="$(TARGET_CROSS)" \
		ARCH="$(LINUX_KARCH)" \
		SUBDIRS="$(PKG_BUILD_DIR)/" \
		$(foreach c, $(PKG_KCONFIG),$(if $(CONFIG_$c),CONFIG_$(c)=y))\
		modules
endef

define KernelPackage/mt7610/install
	$(CP) -r ./files/* $(1)/
	$(INSTALL_DIR) $(1)/etc/Wireless/RT2860/
	ln -s /tmp/MT7610.dat $(1)/etc/Wireless/RT2860/MT7610E.dat
endef

$(eval $(call KernelPackage,mt7610))
