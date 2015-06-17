#
#Makefile for the plugin_work
#

CFLAGS += -Wall -O2 -shared -fPIC

OUR_OBJS += plugin_public.o plugin_encode.o plugin_bbs.o plugin_search.o plugin_url.o plugin_pop3.o plugin_smtp.o plugin_im_qq.o  plugin_im_weixin.o plugin_im_momo.o plugin_im_yy.o plugin_im_bilin.o plugin_im_yixin.o plugin_im_fetion.o rand_str.o plugin_game.o plugin_mail_wangyi.o plugin_weibo.o plugin_mail_qq.o plugin_mail_139.o plugin_mail_189.o plugin_mail_sina.o plugin_weibo_qq.o plugin_weibo_sohu.o plugin_weibo_wangyi.o plugin_imap.o plugin_im_webqq.o plugin_ftp.o plugin_stock_tonghuashun.o plugin_game_qq.o plugin_game_haofang.o plugin_proxy_socks4.o plugin_proxy_socks5.o plugin_stock_dazhihui.o plugin_im_mi.o plugin_im_laiwang.o

# -DNEDBUG 关闭assert判断
#PLUGIN_PRINT_DEBUG 普通打印
#RESULT_DEBUG 结果打印
#PLUGIN_MEM_DEBUG 调试内存

DEBUG_FLAG += -DNEDBUG

MK_PATH=../

include $(MK_PATH)mk/hpanalysis.mk

PLUGIN_BASE += plugin_public.o plugin_encode.o rand_str.o

PLUGIN_SO += plugin_bbs.so plugin_search.so plugin_url.so plugin_pop3.so plugin_smtp.so plugin_im_qq.so plugin_im_weixin.so plugin_im_momo.so plugin_im_yy.so plugin_im_bilin.so plugin_im_yixin.so plugin_im_fetion.so plugin_game.so plugin_mail_wangyi.so plugin_weibo.so plugin_mail_qq.so plugin_mail_139.so plugin_mail_189.so plugin_mail_sina.so plugin_weibo_qq.so plugin_weibo_sohu.so plugin_weibo_wangyi.so plugin_imap.so plugin_im_webqq.so plugin_ftp.so plugin_stock_tonghuashun.so plugin_game_qq.so plugin_game_haofang.so plugin_proxy_socks4.so plugin_proxy_socks5.so plugin_stock_dazhihui.so plugin_im_mi.so plugin_im_laiwang.so

all_objs:$(PLUGIN_SO)

$(PLUGIN_SO):%.so:%.o $(PLUGIN_BASE)

	$(CC) $(CPU_DIGITS) $(CFLAGS) $(LDFLAGS) $(DEBUG_FLAG) $(PLUGIN_BASE) $< -o $@					   

clean:
	rm -f *.o *.so

