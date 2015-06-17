#ifndef BASE_ENCODE_H
#define BASE_ENCODE_H


enum TEXT_ENCODE
{
	ENCODE_GB18030 = 0,
	ENCODE_URL_GB18030,
	ENCODE_UTF8,
	ENCODE_URL_UTF8,
	ENCODE_UTF16,
	ENCODE_URL_UTF16,
	ENCODE_CUSTOM_UTF16,
	ENCODE_URL_CUSTOM_UTF16,
	ENCODE_BASE64,
};


void convert_urlcharset_to_utf8(char *paddr, char *charset);
void convert_custom_utf16_to_utf8(char *pmsg);
void reset_http_esc(char *psrc, int length, char *pneedle, char c_reset);
void convert_gb18030_to_utf8(char *str, int len);
void mail_conver_head_acount(char *p_mail_acount);
void base64_decode(char *str, char *dst);
void decode_base64(char *str, int len);
int  code_convert(char *from_charset, char *to_charset, char **ppin, size_t src_len, char **ppout, size_t des_len );
void convert_encode(char *str, int len, enum TEXT_ENCODE encode, char *extra);

#endif
