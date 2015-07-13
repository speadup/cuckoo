include $(TOPDIR)/rules.mk

PKG_NAME:=openvpn-radius
PKG_VERSION:=2.1
PKG_RELEASE:=1

PKG_SOURCE_NAME:=radiusplugin
PKG_SOURCE_URL:=http://www.nongnu.org/radiusplugin/
PKG_SOURCE:=$(PKG_SOURCE_NAME)_v$(PKG_VERSION).tar.gz
PKG_MD5SUM:=c4715d9cc220a8f9f708408a84cf89bc

PKG_INSTALL:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/openvpn-radius/$(PKG_SOURCE_NAME)

include $(INCLUDE_DIR)/package.mk

TARGET_CFLAGS := \
	-I$(STAGING_DIR)/usr/include

define Package/openvpn-radius
  TITLE:=Open source VPN radius plugin
  SECTION:=net
  CATEGORY:=Network
  DEPENDS:=+libgcrypt +libpthread +openvpn 
  SUBMENU:=VPN
endef

define Package/openvpn-radius/install
	$(INSTALL_DIR) $(1)/usr/lib/openvpn $(1)/etc/openvp
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/lib/openvpn/radiusplugin.so $(1)/usr/lib/openvpn
	$(INSTALL_CONF) ./files/radius.cnf $(1)/etc/openvpn
endef
$(eval $(call BuildPackage,openvpn-radius))
