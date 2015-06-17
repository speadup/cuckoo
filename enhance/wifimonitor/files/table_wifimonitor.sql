/*==============================================================*/
/* Table: t_wifimonitor                                         */
/*==============================================================*/
create table if not exists t_wifimonitor (
  fid integer primary key autoincrement,
  fmac varchar(20) not null,
  fapmac varchar(20) default null ,
  fip varchar(20) default null,
  fphonetype varchar(50) default null,
  fregtime datetime not null,
  faction integer
);

/*==============================================================*/
/* Table: t_wifiaccesspoint    				                    */
/*==============================================================*/
create table if not exists t_wifiaccesspoint (
  fid integer primary key autoincrement,
  fmac varchar(20) not null,
  fssid varchar(20) default null,
  fsignal integer default null ,
  fregtime datetime not null
);

/*==============================================================*/
/* Table: t_wifi_info                                           */
/*==============================================================*/
create table if not exists t_wifi_info(
    fid integer primary key autoincrement,
    fmac varchar(20),
    fos varchar(20),
    ftype varchar(20),
    foem varchar(20)
);


