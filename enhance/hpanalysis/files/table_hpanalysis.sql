/*==============================================================*/
/* table: t_protocol_all                                        */
/*==============================================================*/
create table if not exists t_protocol_all ( 
    fid            integer( 11 )  not null,
    fparentid      text( 30 ),
    fname          text( 30 ),
    fprotocolid    text( 4 ),
    fprotocolsubid text( 12 ),
    fprotocolclass text( 30 ),
    ftype          integer( 11 ),
    fisenable      integer( 1 ),
    fisshow        integer( 1 ),
    primary key ( fid ) 
);

/*==============================================================*/
/* table: t_protocol_main                                       */
/*==============================================================*/
create table if not exists t_protocol_main ( 
    id             integer( 11 )  not null,
    group_id       integer( 11 )  not null,
    fprotocolsubid text( 64 ),
    faction        integer( 11 )  not null,
    fcode          text( 20 ),
    fstatus        integer( 11 )  not null,
    primary key ( id ) 
);

/*==============================================================*/
/* table: t_feature_mark                                        */
/*==============================================================*/
create table if not exists t_feature_mark ( 
    id              integer( 11 )  not null,
    fcode           text( 20 ),
    forder_flag     integer( 11 ),
    mflag           integer( 11 ),
    moffset_or_head text( 255 )    not null,
    mfeature        text( 255 )    not null,
    primary key ( id ) 
);
/*==============================================================*/
/* table: t_feature_extract                                     */
/*==============================================================*/
create table if not exists t_feature_extract ( 
    id              integer( 11 )  not null,
    fcode           text( 20 ),
    eflag           integer( 11 ),
    eencode         integer( 11 ),
    eoffset_or_head text( 255 ),
    eoffset_or_tail text( 255 ),
    efield_name     text( 64 ),
    extra           text( 255 ),
    primary key ( id ) 
);

/*==============================================================*/
/* table: t_place old_phpsoap                                   */
/*==============================================================*/
create table if not exists t_place ( 
    fid           integer( 11 )  not null,
    fplacecode    text( 14 ),
    fplacename    text( 100 ),
    faddress      text( 100 ),
    fmailcode     text( 6 ),
    flegalman     text( 50 ),
    fphone        text( 50 ),
    fsafeman      text( 50 ),
    fsafephone    text( 50 ),
    fsafeemail    text( 50 ),
    fterminalnum  integer( 11 ),
    fmemo         text( 1024 ),
    fmac          text( 255 ),
    fsendtime     text,
    freqtime      text,
    factip        text( 15 ),
    facttime      text,
    fcheckip      text( 15 ),
    fchecktime    text,
    fagentno      text( 30 ),
    fsyntag       integer( 1 ),
    finservercode text( 6 ),
    fnetstatecode text( 1 ),
    fserversnum   integer( 11 ),
    foutip        text( 39 ),
    finkindcode   text( 2 ),
    fstaffnum     integer( 11 ),
    fonarea       text( 100 ),
    fonareaman    text( 50 ),
    fonareaphone  text( 50 ),
    fremark       integer( 1 )   not null,
    primary key ( fid ) 
);

