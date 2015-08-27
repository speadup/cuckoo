include $(TOPDIR)/rules.mk

PKG_NAME:=inotify-tools
PKG_VERSION:=3.14
PKG_RELEASE:=1

PKG_SOURCE_URL:=http://github.com/downloads/rvoicilas/inotify-tools/
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_MD5SUM:=b43d95a0fa8c45f8bab3aec9672cf30c

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/inotify-tools
  TITLE:=inotify-tools
  SECTION:=utils
  CATEGORY:=Utilities
  DEPENDS:=
endef

define Package/inotify-tools/install
	$(INSTALL_DIR) $(1)/usr/bin 
	$(INSTALL_DIR) $(1)/usr/lib 
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/inotify* $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib*.so* $(1)/usr/lib/
endef

define Package/inotify-tools/description
inotify-tools is a C library and a set of command-line programs 
for Linux providing a simple interface to inotify. These programs 
can be used to monitor and act upon filesystem events. 
endef

$(eval $(call BuildPackage,inotify-tools))
