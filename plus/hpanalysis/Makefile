#
# Copyright (C) 2008-2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=hpanalysis
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

define Package/hpanalysis
  SECTION:=base
  CATEGORY:=Yeetec
  DEPENDS:=+libpthread +libsqlite3 +libiconv-full 
  TITLE:=High-performance protocol analyzer
endef

TARGET_CPPFLAGS := \
	-D_GNU_SOURCE \
	-I$(STAGING_DIR)/usr/lib/libiconv-full/include

TARGET_LDFLAGS := \
	-L$(STAGING_DIR)/usr/lib/libiconv-full/lib \
	-liconv
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
	$(MAKE) -f Makefile.build -C $(PKG_BUILD_DIR) \
		$(TARGET_CONFIGURE_OPTS) 
endef

define Package/hpanalysis/install
	$(INSTALL_DIR) $(1)/sbin $(1)/lib/plugins
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/hpanalysis $(1)/sbin/hpanalysis
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/plugin_work/plugin_bbs.so $(1)/lib/plugins
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/plugin_work/plugin_pop3.so $(1)/lib/plugins
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/plugin_work/plugin_search.so $(1)/lib/plugins
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/plugin_work/plugin_smtp.so $(1)/lib/plugins
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/plugin_work/plugin_url.so $(1)/lib/plugins
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/plugin_work/plugin_im_bilin.so $(1)/lib/plugins
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/plugin_work/plugin_im_fetion.so $(1)/lib/plugins
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/plugin_work/plugin_im_momo.so $(1)/lib/plugins
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/plugin_work/plugin_im_qq.so $(1)/lib/plugins
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/plugin_work/plugin_im_weixin.so $(1)/lib/plugins
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/plugin_work/plugin_im_yixin.so $(1)/lib/plugins
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/plugin_work/plugin_im_yy.so $(1)/lib/plugins
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/plugin_work/plugin_game.so $(1)/lib/plugins
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/plugin_work/plugin_mail_wangyi.so $(1)/lib/plugins
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/plugin_work/plugin_weibo.so $(1)/lib/plugins
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/plugin_work/plugin_mail_qq.so $(1)/lib/plugins
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/plugin_work/plugin_mail_139.so $(1)/lib/plugins
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/plugin_work/plugin_mail_189.so $(1)/lib/plugins
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/plugin_work/plugin_mail_sina.so $(1)/lib/plugins
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/plugin_work/plugin_weibo_sohu.so $(1)/lib/plugins
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/plugin_work/plugin_weibo_qq.so $(1)/lib/plugins
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/plugin_work/plugin_weibo_wangyi.so $(1)/lib/plugins
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/plugin_work/plugin_imap.so $(1)/lib/plugins
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/plugin_work/plugin_ftp.so $(1)/lib/plugins
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/plugin_work/plugin_stock_tonghuashun.so $(1)/lib/plugins
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/plugin_work/plugin_game_qq.so $(1)/lib/plugins
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/plugin_work/plugin_game_haofang.so $(1)/lib/plugins
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/plugin_work/plugin_proxy_socks4.so $(1)/lib/plugins
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/plugin_work/plugin_proxy_socks5.so $(1)/lib/plugins
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/plugin_work/plugin_stock_dazhihui.so $(1)/lib/plugins
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_DIR) $(1)/disk/mail_download
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) ./files/hpanalysis.init $(1)/etc/init.d/hpanalysis
	$(INSTALL_DIR) $(1)/usr/share/sql
	$(INSTALL_DATA) ./files/table_hpanalysis.sql $(1)/usr/share/sql/
	$(INSTALL_DATA) ./files/data_hpanalysis.sql $(1)/usr/share/sql/
endef

$(eval $(call BuildPackage,hpanalysis))
