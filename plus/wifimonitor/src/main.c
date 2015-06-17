#define _GNU_SOURCE
#include <pcap.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include "ieee802_11.h"
#include "ieee802_11_radio.h"
#include "llc.h"
#ifdef _WITH_MYSQL_DB
#include "mysql/mysql_database.h"
#else
#include "sqlite/sqlite_database.h"
#endif
#include "uthash.h"
#include <arpa/inet.h>
#include "cpack.h"

#define TAG_PAD         ((u_int8_t)   0)
#define TAG_SUBNET_MASK     ((u_int8_t)   1)
#define TAG_TIME_OFFSET     ((u_int8_t)   2)
#define TAG_GATEWAY     ((u_int8_t)   3)
#define TAG_TIME_SERVER     ((u_int8_t)   4)
#define TAG_NAME_SERVER     ((u_int8_t)   5)
#define TAG_DOMAIN_SERVER   ((u_int8_t)   6)
#define TAG_LOG_SERVER      ((u_int8_t)   7)
#define TAG_COOKIE_SERVER   ((u_int8_t)   8)
#define TAG_LPR_SERVER      ((u_int8_t)   9)
#define TAG_IMPRESS_SERVER  ((u_int8_t)  10)
#define TAG_RLP_SERVER      ((u_int8_t)  11)
#define TAG_HOSTNAME        ((u_int8_t)  12)
#define TAG_BOOTSIZE        ((u_int8_t)  13)
#define TAG_END         ((u_int8_t) 255)

#define IPPORT_BOOTPS       67
#define IPPORT_BOOTPC       68
#define VM_RFC1048  { 99, 130, 83, 99 }

#define ETHERTYPE_IP        0x0800  /* IP protocol */

#define OUI_ENCAP_ETHER 0x000000        /* encapsulated Ethernet */
#define OUI_CISCO       0x00000c        /* Cisco protocols */
#define OUI_NORTEL      0x000081        /* Nortel SONMP */
#define OUI_CISCO_90    0x0000f8        /* Cisco bridging */
#define OUI_RFC2684     0x0080c2        /* RFC 2427/2684 bridged Ethernet */
#define OUI_ATM_FORUM   0x00A03E        /* ATM Forum */
#define OUI_CABLE_BPDU  0x00E02F        /* DOCSIS spanning tree BPDU */
#define OUI_APPLETALK   0x080007        /* Appletalk */
#define OUI_JUNIPER     0x009069        /* Juniper */
#define OUI_HP          0x080009        /* Hewlett-Packard */
#define OUI_IEEE_8021_PRIVATE 0x0080c2      /* IEEE 802.1 Organisation Specific - Annex F */
#define OUI_IEEE_8023_PRIVATE 0x00120f      /* IEEE 802.3 Organisation Specific - Annex G */
#define OUI_TIA         0x0012bb        /* TIA - Telecommunications Industry Association - ANSI/TIA-1057- 2006 */
#define OUI_DCBX        0x001B21        /* DCBX */

#define BUFSIZE 128
#define DEFAULT_SNAPLEN 96

#define EXTRACT_16BITS(p) \
    ((u_int16_t)((u_int16_t)*((const u_int8_t *)(p) + 0) << 8 | \
                              (u_int16_t)*((const u_int8_t *)(p) + 1)))

#define EXTRACT_24BITS(p) \
    ((u_int32_t)((u_int32_t)*((const u_int8_t *)(p) + 0) << 16 | \
                 (u_int32_t)*((const u_int8_t *)(p) + 1) << 8 | \
                 (u_int32_t)*((const u_int8_t *)(p) + 2)))

#define EXTRACT_LE_16BITS(p) \
	((u_int16_t)((u_int16_t)*((const u_int8_t *)(p) + 1) << 8 | \
		(u_int16_t)*((const u_int8_t *)(p) + 0)))

#define EXTRACT_LE_32BITS(p) \
	    ((u_int32_t)((u_int32_t)*((const u_int8_t *)(p) + 3) << 24 | \
			             (u_int32_t)*((const u_int8_t *)(p) + 2) << 16 | \
			             (u_int32_t)*((const u_int8_t *)(p) + 1) << 8 | \
			             (u_int32_t)*((const u_int8_t *)(p) + 0)))
 
#define roundup2(x, y)  (((x)+((y)-1))&(~((y)-1))) 

#define TIMEOUT_CLEAN 30
#define TIMEOUT_COUNT 20

long g_curtime = 0, g_pretime = 0;
char g_smac[20] = {0}, g_dmac[20] = {0};
int g_signal = 0;
#ifndef _WITH_MYSQL_DB
#define SQLITE_COMMIT_COUNT 5
extern char *g_database;
//sqlite3 *g_sqlite = NULL; 
#endif

struct ip {
    u_int8_t    ip_vhl;     /* header length, version */
#define IP_V(ip)    (((ip)->ip_vhl & 0xf0) >> 4)
#define IP_HL(ip)   ((ip)->ip_vhl & 0x0f)
    u_int8_t    ip_tos;     /* type of service */
    u_int16_t   ip_len;     /* total length */
    u_int16_t   ip_id;      /* identification */
    u_int16_t   ip_off;     /* fragment offset field */
#define IP_DF 0x4000            /* dont fragment flag */
#define IP_MF 0x2000            /* more fragments flag */
#define IP_OFFMASK 0x1fff       /* mask for fragmenting bits */
    u_int8_t    ip_ttl;     /* time to live */
    u_int8_t    ip_p;       /* protocol */
    u_int16_t   ip_sum;     /* checksum */
    struct  in_addr ip_src,ip_dst;  /* source and dest address */
} UNALIGNED;

struct udphdr {
    u_int16_t   uh_sport;       /* source port */
    u_int16_t   uh_dport;       /* destination port */
    u_int16_t   uh_ulen;        /* udp length */
    u_int16_t   uh_sum;         /* udp checksum */
};

struct bootp {
    u_int8_t    bp_op;      /* packet opcode type */
    u_int8_t    bp_htype;   /* hardware addr type */
    u_int8_t    bp_hlen;    /* hardware addr length */
    u_int8_t    bp_hops;    /* gateway hops */
    u_int32_t   bp_xid;     /* transaction ID */
    u_int16_t   bp_secs;    /* seconds since boot began */
    u_int16_t   bp_flags;   /* flags - see bootp_flag_values[]
                               in print-bootp.c */
    struct in_addr  bp_ciaddr;  /* client IP address */
    struct in_addr  bp_yiaddr;  /* 'your' IP address */
    struct in_addr  bp_siaddr;  /* server IP address */
    struct in_addr  bp_giaddr;  /* gateway IP address */
    u_int8_t    bp_chaddr[16];  /* client hardware address */
    u_int8_t    bp_sname[64];   /* server host name */
    u_int8_t    bp_file[128];   /* boot file name */
    u_int8_t    bp_vend[64];    /* vendor-specific area */
};

struct ip_print_demux_state {
    const struct ip *ip;
    const u_char *cp;
    u_int   len, off;
    u_char  nh;
    int     advance;
};

struct hash_data
{
    int cnt;
    char ssid[33];
    char mac[20];
    int signal;
    UT_hash_handle hh; 
};

struct hash_data *ssid_head = NULL;

struct radiotap_state
{
    u_int32_t   present;
    u_int8_t    rate;
};

struct data_info
{
    char *mac;
    char *ssid;
    int signal;
    char *time;
    int len;
};

#ifdef _WITH_MYSQL_DB
int query_ssid(MYSQL_ROW row_data, void *callback_pram)
{
    char sql[300] = {0};
    struct data_info *p;
    p = (struct data_info *)callback_pram;
    if(!row_data){
        sprintf(sql, "insert into t_wifiaccesspoint (fmac, fssid, fsignal, fregtime) values('%s', '%s', %d, '%s');",
                p->mac, p->ssid, p->signal, p->time);
        mysql_database_exec_sql(sql);
        return -1;
    }
    else{
        sprintf(sql, "update t_wifiaccesspoint set fregtime='%s',fsignal=%d where fid=%s;",
                p->time, p->signal, row_data[0]);
        mysql_database_exec_sql(sql);
        return -1;
    }

    return 0;
}
#else
int query_ssid(sqlite3 *db, char **row_data, int row_num, int field_num, void *callback_pram)
{
    char sql[300] = {0};
    struct data_info *p;

    p = (struct data_info *)callback_pram;
    //printf("rownum=====================%d\n", row_num);
    if(!row_num){
        sprintf(sql, "insert into t_wifiaccesspoint (fmac, fssid, fsignal, fregtime) values('%s', '%s', %d, '%s');",
                p->mac, p->ssid, p->signal, p->time);
        //printf("sql=%s\n", sql);
        sqlite_database_insert(db, sql);
        return -1;
    }
    else{
        sprintf(sql, "update t_wifiaccesspoint set fregtime='%s',fsignal=%d where fid=%s;",
                p->time, p->signal, row_data[field_num]);
        sqlite_database_insert(db, sql);
        return -1;
    }

    return 0;
}
#endif

static int extract_header_length(u_int16_t fc)
{
	int len;

	switch (FC_TYPE(fc)) {
		case T_MGMT:
			return MGMT_HDRLEN;
		case T_CTRL:
			switch (FC_SUBTYPE(fc)) {
				case CTRL_PS_POLL:
					return CTRL_PS_POLL_HDRLEN;
				case CTRL_RTS:
					return CTRL_RTS_HDRLEN;
				case CTRL_CTS:
					return CTRL_CTS_HDRLEN;
				case CTRL_ACK:
					return CTRL_ACK_HDRLEN;
				case CTRL_CF_END:
					return CTRL_END_HDRLEN;
				case CTRL_END_ACK:
					return CTRL_END_ACK_HDRLEN;
				default:
					return 0;
			}
		case T_DATA:
			len = (FC_TO_DS(fc) && FC_FROM_DS(fc)) ? 30 : 24;
			if (DATA_FRAME_IS_QOS(FC_SUBTYPE(fc)))
				len += 2;
			return len;
		default:
			return 0;
	}
}

static void data_header_print(u_int16_t fc, const u_char *p)
{
	char smac[20] = {0}, dmac[20] = {0};

#define ADDR1  (p + 4)
#define ADDR2  (p + 10)
#define ADDR3  (p + 16)       
#define ADDR4  (p + 24) 
    memset(g_smac, 0, 20);
    memset(g_dmac, 0, 20);
    if (!FC_TO_DS(fc) && !FC_FROM_DS(fc)) {
	    sprintf(smac, "%02x:%02x:%02x:%02x:%02x:%02x", 
                *(ADDR2), *(ADDR2+1), *(ADDR2+2), *(ADDR2+3), *(ADDR2+4), *(ADDR2+5));
	    sprintf(dmac, "%02x:%02x:%02x:%02x:%02x:%02x", 
                *(ADDR1), *(ADDR1+1), *(ADDR1+2), *(ADDR1+3), *(ADDR1+4), *(ADDR1+5));
    } else if (!FC_TO_DS(fc) && FC_FROM_DS(fc)) {
	    sprintf(smac, "%02x:%02x:%02x:%02x:%02x:%02x", 
                *(ADDR3), *(ADDR3+1), *(ADDR3+2), *(ADDR3+3), *(ADDR3+4), *(ADDR3+5));
	    sprintf(dmac, "%02x:%02x:%02x:%02x:%02x:%02x", 
                *(ADDR1), *(ADDR1+1), *(ADDR1+2), *(ADDR1+3), *(ADDR1+4), *(ADDR1+5));
    } else if (FC_TO_DS(fc) && !FC_FROM_DS(fc)) {
	    sprintf(smac, "%02x:%02x:%02x:%02x:%02x:%02x", 
                *(ADDR2), *(ADDR2+1), *(ADDR2+2), *(ADDR2+3), *(ADDR2+4), *(ADDR2+5));
	    sprintf(dmac, "%02x:%02x:%02x:%02x:%02x:%02x", 
                *(ADDR3), *(ADDR3+1), *(ADDR3+2), *(ADDR3+3), *(ADDR3+4), *(ADDR3+5));
    } else if (FC_TO_DS(fc) && FC_FROM_DS(fc)) {
	    sprintf(smac, "%02x:%02x:%02x:%02x:%02x:%02x", 
                *(ADDR4), *(ADDR4+1), *(ADDR4+2), *(ADDR4+3), *(ADDR4+4), *(ADDR4+5));
	    sprintf(dmac, "%02x:%02x:%02x:%02x:%02x:%02x", 
                *(ADDR3), *(ADDR3+1), *(ADDR3+2), *(ADDR3+3), *(ADDR3+4), *(ADDR3+5));
    }
    memcpy(g_smac, smac, strlen(smac));
    memcpy(g_dmac, dmac, strlen(dmac));

#undef ADDR1
#undef ADDR2
#undef ADDR3
#undef ADDR4
}

static void mgmt_header_print(u_int16_t fc, const u_char *p)
{
	const struct mgmt_header_t *hp = (const struct mgmt_header_t *) p;
	char smac[20] = {0}, dmac[20] = {0};
	char sql[300] = {0}, cur_time[20], phonetype[20] = {0};
	int action = -1;
	time_t now;
	struct tm *tm_now; 
	 
	time(&now);
	tm_now = localtime(&now);

	sprintf(cur_time, "%04d-%02d-%02d %02d:%02d:%02d", 
			tm_now->tm_year + 1900, 
			tm_now->tm_mon + 1,
			tm_now->tm_mday,
			tm_now->tm_hour,
			tm_now->tm_min,
			tm_now->tm_sec);

	sprintf(smac, "%02x:%02x:%02x:%02x:%02x:%02x", 
			hp->sa[0],
			hp->sa[1],
			hp->sa[2],
			hp->sa[3],
			hp->sa[4],
			hp->sa[5]);

	sprintf(dmac, "%02x:%02x:%02x:%02x:%02x:%02x", 
			hp->da[0],
			hp->da[1],
			hp->da[2],
			hp->da[3],
			hp->da[4],
			hp->da[5]);

    sprintf(phonetype,"%s", "NULL");

	switch (FC_SUBTYPE(fc)) {
		case ST_ASSOC_REQUEST:
			action = 2;
			sprintf(sql, "insert into t_wifimonitor (fmac, fapmac, fphonetype, fregtime, faction) values('%s', '%s', '%s', '%s', %d);", smac, dmac, phonetype, cur_time, action);
			break;
		case ST_ASSOC_RESPONSE:
			break;
		case ST_PROBE_REQUEST:
			action = 0;
			sprintf(sql, "insert into t_wifimonitor (fmac, fregtime, faction) values('%s', '%s', %d);",
					smac, cur_time, action);
			break;
		case ST_PROBE_RESPONSE:
			break;
		case ST_BEACON:
			break;
		case ST_ATIM:
			break;
		case ST_DISASSOC:
			action = 3;
			sprintf(sql, "insert into t_wifimonitor (fmac, fapmac, fphonetype, fregtime, faction) values('%s', '%s', '%s', '%s', %d);", smac, dmac, phonetype, cur_time, action);
			break;
		case ST_AUTH:
			action = 1;
			sprintf(sql, "insert into t_wifimonitor (fmac, fapmac, fphonetype, fregtime, faction) values('%s', '%s', '%s', '%s', %d);", smac, dmac, phonetype, cur_time, action);
			break;
		case ST_DEAUTH:
			break;
		default:
			break;
	}
	if(action >= 0)
    {
#ifdef _WITH_MYSQL_DB
		mysql_database_exec_sql(sql);
#else
       /* static int count = 0;
        if(count == 0){
            sqlite_database_open(&g_sqlite);
            sqlite_database_begin_transaction(g_sqlite);
        }
        sqlite_database_insert(g_sqlite, sql);
        count++;
        if(count >= SQLITE_COMMIT_COUNT){
            sqlite_database_commit_transaction(g_sqlite);
            sqlite_database_close(g_sqlite);
            count = 0;
        }*/
        
        sqlite3 *db = NULL; 
        sqlite_database_open(&db);
        if(db){
            sqlite_database_insert(db, sql);
            sqlite_database_close(db);
        }

#endif
    }
}

static void handle_probe_response(const struct mgmt_header_t *pmh, const u_char *p, u_int length)
{
    struct ssid_t ssid;
    struct hash_data *tmp = NULL, *h_data = NULL;
    int offset = 0;
    char smac[20] = {0};

    if (length < IEEE802_11_TSTAMP_LEN + IEEE802_11_BCNINT_LEN + IEEE802_11_CAPINFO_LEN)
        return;

    offset += IEEE802_11_TSTAMP_LEN;
    length -= IEEE802_11_TSTAMP_LEN;
    offset += IEEE802_11_BCNINT_LEN;
    length -= IEEE802_11_BCNINT_LEN;
    offset += IEEE802_11_CAPINFO_LEN;
    length -= IEEE802_11_CAPINFO_LEN;

    if(length < 2)  
        return;

    sprintf(smac, "%02x:%02x:%02x:%02x:%02x:%02x", 
			pmh->sa[0],
			pmh->sa[1],
			pmh->sa[2],
			pmh->sa[3],
			pmh->sa[4],
			pmh->sa[5]);

    memcpy(&ssid, p + offset, 2); 
    offset += 2;
    length -= 2;

    if (ssid.length != 0) {
        if (ssid.length > sizeof(ssid.ssid) - 1)
            return;
        if (length < ssid.length)
            return;
        memset(&ssid.ssid, 0, sizeof(ssid.ssid));
        memcpy(&ssid.ssid, p + offset, ssid.length);
        offset += ssid.length;
        length -= ssid.length;
        ssid.ssid[ssid.length] = '\0';

        HASH_FIND_STR(ssid_head, (char *)ssid.ssid, tmp);
        if(tmp){
            tmp->cnt++;
            tmp->signal = g_signal;
        }
        else{
            h_data = (struct hash_data *)calloc(1, sizeof(struct hash_data));
            h_data->cnt = 0;
            h_data->signal = g_signal;
            sprintf(h_data->ssid, "%s", ssid.ssid);
            sprintf(h_data->mac, "%s", smac);
            HASH_ADD_STR(ssid_head, ssid, h_data);
        }
    }
}

static int handle_beacon(const struct mgmt_header_t *pmh, const u_char *p, u_int length)
{
    struct mgmt_body_t pbody;
    int offset = 0;
    int ret;
    struct ssid_t ssid;
    struct data_info data;
    char smac[20] = {0}, sql[300] = {0}, cur_time[20];
    time_t now;
	struct tm *tm_now; 
    struct hash_data *tmp = NULL, *h_data = NULL;
    struct timeval tm;

	time(&now);
	tm_now = localtime(&now);

	sprintf(cur_time, "%04d-%02d-%02d %02d:%02d:%02d", 
			tm_now->tm_year + 1900, 
			tm_now->tm_mon + 1,
			tm_now->tm_mday,
			tm_now->tm_hour,
			tm_now->tm_min,
			tm_now->tm_sec);

    sprintf(smac, "%02x:%02x:%02x:%02x:%02x:%02x", 
			pmh->sa[0],
			pmh->sa[1],
			pmh->sa[2],
			pmh->sa[3],
			pmh->sa[4],
			pmh->sa[5]);


    memset(&pbody, 0, sizeof(pbody));

    if (length < IEEE802_11_TSTAMP_LEN + IEEE802_11_BCNINT_LEN +
        IEEE802_11_CAPINFO_LEN)
        return 0;
    memcpy(&pbody.timestamp, p, IEEE802_11_TSTAMP_LEN);
    offset += IEEE802_11_TSTAMP_LEN;
    length -= IEEE802_11_TSTAMP_LEN;
    pbody.beacon_interval = EXTRACT_LE_16BITS(p+offset);
    offset += IEEE802_11_BCNINT_LEN;
    length -= IEEE802_11_BCNINT_LEN;
    pbody.capability_info = EXTRACT_LE_16BITS(p+offset);
    offset += IEEE802_11_CAPINFO_LEN;
    length -= IEEE802_11_CAPINFO_LEN;

    if(*(p + offset) == E_SSID){
        memcpy(&ssid, p + offset, 2);  
        offset += 2;
        if (ssid.length != 0) {        
            if (ssid.length > sizeof(ssid.ssid) - 1)
                return 0; 
            if (length < ssid.length)
                return 0;
            memcpy(&ssid.ssid, p + offset, ssid.length);
            ssid.ssid[ssid.length] = '\0';

            HASH_FIND_STR(ssid_head, (char *)ssid.ssid, tmp);
            if(tmp){
                tmp->cnt++;
                tmp->signal = g_signal;
                gettimeofday(&tm, NULL);
                g_curtime = tm.tv_sec;
                if((g_curtime - g_pretime) > TIMEOUT_CLEAN){
                    g_pretime = g_curtime;
                    for(h_data = ssid_head; h_data != NULL; h_data = h_data->hh.next){
                        if(h_data->cnt > TIMEOUT_COUNT){
                            sprintf(sql, "select fid from  t_wifiaccesspoint where fssid='%s';",
                                    h_data->ssid);
                            data.mac = h_data->mac;
                            data.ssid = h_data->ssid;
                            data.time = cur_time;
                            data.signal = h_data->signal;
                            data.len = length;
#ifdef _WITH_MYSQL_DB
                            mysql_database_query_row(sql, query_ssid, &data);
#else
                            sqlite3 *db = NULL;
                            sqlite_database_open(&db);
                            if(db)
                            {
                                sqlite_database_select(db, sql, query_ssid, &data);
                                sqlite_database_close(db);
                            }                            
#endif
                            //printf("ssid=%s,cnt=%d\n", h_data->ssid, h_data->cnt);
                        }
                    }
                    //printf("===============hash count=%d==================\n",HASH_COUNT(ssid_head));
                    for(h_data = ssid_head; h_data != NULL; h_data = h_data->hh.next){
                        HASH_DEL(ssid_head, h_data);
                        if(h_data)
                            free(h_data);
                    }
                    //printf("==========after clear hash count=%d==================\n",HASH_COUNT(ssid_head));
                }
            }
            else{
                h_data = (struct hash_data *)calloc(1, sizeof(struct hash_data));
                h_data->cnt = 0;
                h_data->signal = g_signal;
                sprintf(h_data->ssid, "%s", ssid.ssid);
                sprintf(h_data->mac, "%s", smac);
                HASH_ADD_STR(ssid_head, ssid, h_data);
            }
        }
    }
    return ret;
}

void bootp_print(register const u_char *cp, u_int length)
{
    struct bootp *bp;
    u_int8_t tag, len;
    u_char *p;
    u_char vm_rfc1048[4] = VM_RFC1048;
    char hostname[255] = {0}, sql[300] = {0};

    bp = (struct bootp *)cp;
    if(bp->bp_op == 0 || bp->bp_op == 1){
        if (memcmp((const char *)bp->bp_vend, vm_rfc1048, sizeof(u_int32_t)) == 0){
            p = bp->bp_vend;
            p += sizeof(int32_t);
            /*for(i = 0; i < 50; i++)
                printf("%x ", *(p + i));
            printf("\n");
            */
            while(tag < TAG_END){
                tag = *p;
                if(tag <= 0)
                    break;
                len = *(p + 1);
                if(len <= 0 || len > 254)
                    break;
                //printf("tag=%d, len=%d,host=%d\n", tag, len, TAG_HOSTNAME);
                if(tag == TAG_HOSTNAME){
                    memcpy(hostname, (char*)(p + 2), len); 
                    //printf("hostname:%s\n", hostname);
                    if(bp->bp_op == 0)
                        sprintf(sql, "update t_wifimonitor set fphonetype='%s' where fmac='%s';", hostname, g_smac);
                    else if(bp->bp_op == 1)
                        sprintf(sql, "update t_wifimonitor set fphonetype='%s' where fmac='%s';", hostname, g_smac);
                    //printf("sql:%s\n", sql);
#ifdef _WITH_MYSQL_DB
                    mysql_database_exec_sql(sql);
#else

                    sqlite3 *db = NULL; 
                    sqlite_database_open(&db);
                    if(db)
                    {
                        sqlite_database_insert(db, sql);
                        sqlite_database_close(db);
                    }
#endif
                    break;
                }
                p += len + 2;
            }
        }
    }
}

void ip_print(const u_char *bp, int length)
{
    struct ip_print_demux_state  ipd;
    struct ip_print_demux_state *ipds=&ipd;
    struct udphdr *up;
    struct ip *ip;
    u_int16_t sport, dport;
    const u_char *ipend;
    u_int hlen;

    ipds->ip = (const struct ip *)bp;
    if (IP_V(ipds->ip) != 4) { 
        printf("IP%u ", IP_V(ipds->ip));
        if (IP_V(ipds->ip) == 6)
            printf(", wrong link-layer encapsulation");
    }       

    if (length < sizeof (struct ip)) {
        return;
    }
    hlen = IP_HL(ipds->ip) * 4;
    if (hlen < sizeof (struct ip)) {
        return;
    }

    ipds->len = EXTRACT_16BITS(&ipds->ip->ip_len);
    if (ipds->len < hlen) {
#ifdef GUESS_TSO
        if (ipds->len) {
            return;
        }   
        else {
            ipds->len = length;
        }   
#else
        return;
#endif 
    }   

    ipend = bp + ipds->len;

    ipds->len -= hlen;

    ipds->off = EXTRACT_16BITS(&ipds->ip->ip_off);
    if ((ipds->off & 0x1fff) == 0) {
        ipds->cp = (const u_char *)ipds->ip + hlen;
        ipds->nh = ipds->ip->ip_p;

        if(ipds->nh == IPPROTO_UDP){
            up = (struct udphdr *)ipds->cp;
            ip = (struct ip *)ipds->ip;
            //sport = ntohs(up->uh_sport);
            //dport = ntohs(up->uh_dport);
            sport = EXTRACT_16BITS(&up->uh_sport);
            dport = EXTRACT_16BITS(&up->uh_dport);
#define ISPORT(p) (dport == (p) || sport == (p))
            if (ISPORT(IPPORT_BOOTPC) || ISPORT(IPPORT_BOOTPS)){
                bootp_print((const u_char *)(up + 1), length); 
            }
                    
        }
    }
}

void llc_print(const u_char *p, u_int length)
{
    u_int8_t dsap_field, dsap, ssap_field, ssap;
    u_int16_t control, et;
    u_int32_t orgcode;

    if (length < 3) {
        return;
    }

    dsap_field = *p;
    ssap_field = *(p + 1);

    control = *(p + 2);
    if ((control & LLC_U_FMT) != LLC_U_FMT) {
        if (length < 4)
            return;
        control = EXTRACT_LE_16BITS(p + 2);
    }

    dsap = dsap_field & ~LLC_IG;
    ssap = ssap_field & ~LLC_GSAP;

    if (ssap == LLCSAP_SNAP && dsap == LLCSAP_SNAP
        && control == LLC_UI) {
        p += 3;
        length -= 3;

        orgcode = EXTRACT_24BITS(p);
        et = EXTRACT_16BITS(p + 3);
        p += 5;
        length -= 5;
        if(orgcode == OUI_CISCO_90 || orgcode == OUI_ENCAP_ETHER){
             if(et == ETHERTYPE_IP){
                ip_print(p, length);
             }  
        }
    }  
}

static void mgmt_body_print(u_int16_t fc, const struct mgmt_header_t *pmh, const u_char *p, u_int length)
{
	time_t now;
	struct tm *tm_now; 
    char smac[20] = {0}, cur_time[20];

	time(&now);
	tm_now = localtime(&now);

	sprintf(cur_time, "%04d-%02d-%02d %02d:%02d:%02d", 
			tm_now->tm_year + 1900, 
			tm_now->tm_mon + 1,
			tm_now->tm_mday,
			tm_now->tm_hour,
			tm_now->tm_min,
			tm_now->tm_sec);

    sprintf(smac, "%02x:%02x:%02x:%02x:%02x:%02x", 
			pmh->sa[0],
			pmh->sa[1],
			pmh->sa[2],
			pmh->sa[3],
			pmh->sa[4],
			pmh->sa[5]);

    switch (FC_SUBTYPE(fc)) {
    case ST_BEACON:
        handle_beacon(pmh, p, length);
       break;
    case ST_PROBE_RESPONSE:
        handle_probe_response(pmh, p, length);
       break;
    default:
       break;
    }
}

static void ieee_802_11_hdr_print(u_int16_t fc, const u_char *p, u_int hdrlen, u_int meshdrlen, const u_int8_t **srcp, const u_int8_t **dstp) 
{
    switch (FC_TYPE(fc)) {
    case T_MGMT:
        mgmt_header_print(fc, p);
        break;
    case T_DATA:
        data_header_print(fc, p);
        break;
    }
}

static u_int ieee802_11_print(const u_char *p, u_int length, u_int orig_caplen, int pad, u_int fcslen)
{
    u_int16_t fc;
    u_int caplen, hdrlen, meshdrlen;
    const u_int8_t *src, *dst;

    caplen = orig_caplen;

    if (length < fcslen) {
        return caplen;
    }

    length -= fcslen;
    if (caplen > length) {
        fcslen = caplen - length;
        caplen -= fcslen;
    }

    if (caplen < IEEE802_11_FC_LEN) {
        return orig_caplen;
    }

    fc = EXTRACT_LE_16BITS(p);
    hdrlen = extract_header_length(fc);
    if (pad)
        hdrlen = roundup2(hdrlen, 4);

    meshdrlen = 0;

    if (caplen < hdrlen) {
        return hdrlen;
    }

    ieee_802_11_hdr_print(fc, p, hdrlen, meshdrlen, &src, &dst);

    length -= hdrlen;
    caplen -= hdrlen;
    p += hdrlen;

    switch (FC_TYPE(fc)) {
    case T_MGMT:
        mgmt_body_print(fc, (const struct mgmt_header_t *)(p - hdrlen), p, length);
        break;
    case T_DATA:
        if (DATA_FRAME_IS_NULL(FC_SUBTYPE(fc)))
            return 0;
        if (!FC_WEP(fc))
            llc_print(p, length);
        break;
    default:
        break;
    }
    return hdrlen;
}

static void print_radiotap_field(struct cpack_state *s, u_int32_t bit, u_int8_t *flags, struct radiotap_state *state, u_int32_t presentflags)
{
    union {
        int8_t      i8;
        u_int8_t    u8;
        int16_t     i16;
        u_int16_t   u16;
        u_int32_t   u32;
        u_int64_t   u64;
    } u, u2, u3, u4;
    int rc;

    switch (bit) {
    case IEEE80211_RADIOTAP_FLAGS:
        rc = cpack_uint8(s, &u.u8);
        if (rc != 0)
            break;
        *flags = u.u8;
        break;
    case IEEE80211_RADIOTAP_RATE:
        rc = cpack_uint8(s, &u.u8);
        if (rc != 0)
            break;
        state->rate = u.u8;
        break;
    case IEEE80211_RADIOTAP_DB_ANTSIGNAL:
    case IEEE80211_RADIOTAP_DB_ANTNOISE:
    case IEEE80211_RADIOTAP_ANTENNA:
        rc = cpack_uint8(s, &u.u8);
        break;
    case IEEE80211_RADIOTAP_DBM_ANTSIGNAL:
    case IEEE80211_RADIOTAP_DBM_ANTNOISE:
        rc = cpack_int8(s, &u.i8);
        break;
    case IEEE80211_RADIOTAP_CHANNEL:
        rc = cpack_uint16(s, &u.u16);
        if (rc != 0)
            break;
        rc = cpack_uint16(s, &u2.u16);
        break;
    case IEEE80211_RADIOTAP_FHSS:
    case IEEE80211_RADIOTAP_LOCK_QUALITY:
    case IEEE80211_RADIOTAP_TX_ATTENUATION:
    case IEEE80211_RADIOTAP_RX_FLAGS:
        rc = cpack_uint16(s, &u.u16);
        break;
    case IEEE80211_RADIOTAP_DB_TX_ATTENUATION:
        rc = cpack_uint8(s, &u.u8);
        break;
    case IEEE80211_RADIOTAP_DBM_TX_POWER:
        rc = cpack_int8(s, &u.i8);
        break;
    case IEEE80211_RADIOTAP_TSFT:
        rc = cpack_uint64(s, &u.u64);
        break;
    case IEEE80211_RADIOTAP_XCHANNEL:
        rc = cpack_uint32(s, &u.u32);
        if (rc != 0)
            break;
        rc = cpack_uint16(s, &u2.u16);
        if (rc != 0)
            break;
        rc = cpack_uint8(s, &u3.u8);
        if (rc != 0)
            break;
        rc = cpack_uint8(s, &u4.u8);
        break;
    case IEEE80211_RADIOTAP_MCS:
        rc = cpack_uint8(s, &u.u8);
        if (rc != 0)
            break;
        rc = cpack_uint8(s, &u2.u8);
        if (rc != 0)
            break;
        rc = cpack_uint8(s, &u3.u8);
        break;
    case IEEE80211_RADIOTAP_VENDOR_NAMESPACE: 
        {
            u_int8_t vns[3];
            u_int16_t length;
            u_int8_t subspace;

            if ((cpack_align_and_reserve(s, 2)) == NULL) {
                rc = -1;
                break;
            }

            rc = cpack_uint8(s, &vns[0]);
            if (rc != 0)
                break;
            rc = cpack_uint8(s, &vns[1]);
            if (rc != 0)
                break;
            rc = cpack_uint8(s, &vns[2]);
            if (rc != 0)
                break;
            rc = cpack_uint8(s, &subspace);
            if (rc != 0)
                break;
            rc = cpack_uint16(s, &length);
            if (rc != 0)
                break;
            s->c_next += length;
            break;
        }
    default:
        return;
    }

    if (rc != 0) {
        return;
    }

    state->present = presentflags;

    switch (bit) {
    case IEEE80211_RADIOTAP_DBM_ANTSIGNAL:
        //printf("%ddB signal\n", u.i8);
        g_signal = u.i8;
        break;
    }
}

static u_int ieee802_11_radio_print(const u_char *p, u_int length, u_int caplen)
{
#define BITNO_32(x) (((x) >> 16) ? 16 + BITNO_16((x) >> 16) : BITNO_16((x)))
#define BITNO_16(x) (((x) >> 8) ? 8 + BITNO_8((x) >> 8) : BITNO_8((x)))
#define BITNO_8(x) (((x) >> 4) ? 4 + BITNO_4((x) >> 4) : BITNO_4((x)))
#define BITNO_4(x) (((x) >> 2) ? 2 + BITNO_2((x) >> 2) : BITNO_2((x)))
#define BITNO_2(x) (((x) & 2) ? 1 : 0)
#define BIT(n)  (1U << n)
#define IS_EXTENDED(__p)    \
    (EXTRACT_LE_32BITS(__p) & BIT(IEEE80211_RADIOTAP_EXT)) != 0

    struct cpack_state cpacker;
    struct ieee80211_radiotap_header *hdr;
    u_int32_t present, next_present;
    u_int32_t presentflags = 0;
    u_int32_t *presentp, *last_presentp;
    enum ieee80211_radiotap_type bit;
    int bit0;
    u_int len;
    u_int8_t flags;
    int pad;
    u_int fcslen;
    struct radiotap_state state;

    if (caplen < sizeof(*hdr)) {
        return caplen;
    }

    hdr = (struct ieee80211_radiotap_header *)p;

    len = EXTRACT_LE_16BITS(&hdr->it_len);

    if (caplen < len) {
        return caplen;
    }
    cpack_init(&cpacker, (u_int8_t *)hdr, len);
    cpack_advance(&cpacker, sizeof(*hdr));
    for (last_presentp = &hdr->it_present;
         IS_EXTENDED(last_presentp) &&
         (u_char*)(last_presentp + 1) <= p + len;
         last_presentp++)
         cpack_advance(&cpacker, sizeof(hdr->it_present));

    if (IS_EXTENDED(last_presentp)) {
        return caplen;
    }

    flags = 0;
    pad = 0;
    fcslen = 0;
    g_signal = 0;
    for (bit0 = 0, presentp = &hdr->it_present; presentp <= last_presentp;
         presentp++, bit0 += 32) {
        presentflags = EXTRACT_LE_32BITS(presentp);

        memset(&state, 0, sizeof(state));

        for (present = EXTRACT_LE_32BITS(presentp); present;
             present = next_present) {
            next_present = present & (present - 1);

            bit = (enum ieee80211_radiotap_type)
                (bit0 + BITNO_32(present ^ next_present));
            print_radiotap_field(&cpacker, bit, &flags, &state, presentflags);
        }
    }

    if (flags & IEEE80211_RADIOTAP_F_DATAPAD)
        pad = 1;
    if (flags & IEEE80211_RADIOTAP_F_FCS)
        fcslen = 4;
    return len + ieee802_11_print(p + len, length - len, caplen - len, pad,
                                  fcslen);
#undef BITNO_32
#undef BITNO_16
#undef BITNO_8
#undef BITNO_4
#undef BITNO_2
#undef BIT
}

static void print_packet(u_char *user, const struct pcap_pkthdr *h, const u_char *sp) 
{
    ieee802_11_radio_print(sp, h->len, h->caplen);
}


int main(int argc, char **argv)
{
	char *device = NULL;
	char ebuf[PCAP_ERRBUF_SIZE];
	int opt, snaplen = 65535;
	pcap_t *pd;

	while((opt = getopt(argc, argv, "i:d:")) != -1){
		switch(opt){
			case 'i':
				device = optarg;
				break;
#ifndef _WITH_MYSQL_DB
            case 'd':
                g_database = optarg;
                break;
#endif
		}
	}

	if(!device){
		printf("Usage %s -i wlan[x]\n", argv[0]);
		return -1;
	}

#ifdef _WITH_MYSQL_DB
	if(mysql_database_init() < 0){
		printf("mysql init error!\n");
		return -1;
	}
#else
    if(!g_database){
		printf("Usage %s -d database\n", argv[0]);
		return -1;
    }
#endif

	if((pd = pcap_open_live(device, snaplen, 1, 1000, ebuf))== NULL){
		printf("%s",ebuf);
		return -1;
	}

	if(pcap_loop(pd, -1, print_packet, NULL)<0){
		printf("pcap_loop:%s\n", pcap_geterr(pd));
		return -1;
	}

#ifndef _WITH_MYSQL_DB
    //if(g_sqlite)
    //   sqlite_database_close(g_sqlite);
#endif
	pcap_close(pd);
	return 0;
	/*
    bpf_u_int32 localnet, netmask;
	pcap_handler callback;
	int snaplen = 65535;
	int status;
	int cnt, i;
	int type;
	int dlt;
	const char *dlt_name;
	struct bpf_program fcode;
	cnt = -1;

	pd = pcap_create(device, ebuf); 
	if(pd == NULL){
		printf("%s\n", ebuf);
		return -1;
	}

	status = pcap_set_snaplen(pd, snaplen);
	if(status != 0){
	 	printf("set_snaplen error!\n");
		return -1;
	}	

	status = pcap_set_promisc(pd, 1);
	if(status != 0){
	 	printf("pcap_set_promisc error!\n");
		return -1;
	}
	  
	status = pcap_set_timeout(pd, 1000);
	if(status != 0){
	 	printf("pcap_set_timeout error!\n");
		return -1;
	}

	status = pcap_activate(pd);
	if(status < 0){
	
	}

	i = pcap_snapshot(pd);
	if(snaplen < i){
		printf("snaplen over\n");
		snaplen = i;
	}

	if(pcap_lookupnet(device, &localnet, &netmask, ebuf) < 0){
		localnet = 0;
		netmask = 0;
		printf("%s", ebuf);
	}

	if(pcap_compile(pd, &fcode, NULL, 0, netmask) < 0){
		printf("%s", pcap_geterr(pd));
	}

	if(pcap_setfilter(pd, &fcode) < 0){
		printf("%s", pcap_geterr(pd));
	}

	type = pcap_datalink(pd);
	callback = print_packet;

	dlt = pcap_datalink(pd);
	dlt_name = pcap_datalink_val_to_name(dlt);

	if(dlt_name == NULL){
		printf("listening on %s, link-type %u, capture size %u bytes\n", 
				device,
				dlt,
				snaplen);
	} else {
		printf("listening on %s, link-type %s (%s), capture size %u bytes\n", 
				device,
				dlt_name,
				pcap_datalink_val_to_description(dlt),
				snaplen);
	}
	
	status = pcap_loop(pd, cnt, callback, NULL);

	pcap_freecode(&fcode);
	pcap_close(pd);
	*/
}
