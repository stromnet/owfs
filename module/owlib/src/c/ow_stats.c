/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: palfille@earthlink.net
    Released under the GPL
    See the header file: ow.h for full attribution
    1wire/iButton system from Dallas Semiconductor
*/

/* General Device File format:
    This device file corresponds to a specific 1wire/iButton chip type
    ( or a closely related family of chips )

    The connection to the larger program is through the "device" data structure,
      which must be declared in the acompanying header file.

    The device structure holds the
      family code,
      name,
      device type (chip, interface or pseudo)
      number of properties,
      list of property structures, called "filetype".

    Each filetype structure holds the
      name,
      estimated length (in bytes),
      aggregate structure pointer,
      data format,
      read function,
      write funtion,
      generic data pointer

    The aggregate structure, is present for properties that several members
    (e.g. pages of memory or entries in a temperature log. It holds:
      number of elements
      whether the members are lettered or numbered
      whether the elements are stored together and split, or separately and joined
*/

/* Stats are a pseudo-device -- they are a file-system entry and handled as such,
     but have a different caching type to distiguish their handling */

#include "owfs_config.h"
#include "ow_stats.h"

/* ----------------- */
/* ---- Globals ---- */
/* ----------------- */
unsigned int cache_flips = 0 ;
unsigned int cache_adds = 0 ;
struct average old_avg = {0L,0L,0L,0L,} ;
struct average new_avg = {0L,0L,0L,0L,} ;
struct average store_avg = {0L,0L,0L,0L,} ;
struct cache cache_ext = {0L,0L,0L,0L,0L,} ;
struct cache cache_int = {0L,0L,0L,0L,0L,} ;
struct cache cache_dir = {0L,0L,0L,0L,0L,} ;
struct cache cache_sto = {0L,0L,0L,0L,0L,} ;

unsigned int read_calls = 0 ;
unsigned int read_cache = 0 ;
unsigned int read_bytes = 0 ;
unsigned int read_cachebytes = 0 ;
unsigned int read_array = 0 ;
unsigned int read_tries[3] = {0,0,0,} ;
unsigned int read_success = 0 ;
struct average read_avg = {0L,0L,0L,0L,} ;

unsigned int write_calls = 0 ;
unsigned int write_bytes = 0 ;
unsigned int write_array = 0 ;
unsigned int write_tries[3] = {0,0,0,} ;
unsigned int write_success = 0 ;
struct average write_avg = {0L,0L,0L,0L,} ;

struct directory dir_main = { 0L, 0L, } ;
struct directory dir_dev = { 0L, 0L, } ;
unsigned int dir_depth = 0 ;
struct average dir_avg = {0L,0L,0L,0L,} ;

/* max delay between a write and when reading first char */
struct timeval max_delay = {0, 0, } ;

// ow_locks.c
//struct timeval bus_pause = {0, 0, } ;
struct timeval total_bus_time = {0, 0, } ;
unsigned int total_bus_locks = 0 ;
unsigned int total_bus_unlocks = 0 ;

// ow_crc.c
unsigned int CRC8_tries = 0 ;
unsigned int CRC8_errors = 0 ;
unsigned int CRC16_tries = 0 ;
unsigned int CRC16_errors = 0 ;

// ow_bus.c
unsigned int BUS_send_data_errors = 0 ;
unsigned int BUS_send_data_memcmp_errors = 0 ;
unsigned int BUS_readin_data_errors = 0 ;
unsigned int BUS_send_and_get_timeout = 0 ;
unsigned int BUS_send_and_get_select_errors = 0 ;
unsigned int BUS_send_and_get_errors = 0 ;
unsigned int BUS_send_and_get_interrupted = 0 ;
unsigned int BUS_select_low_errors = 0 ;
unsigned int BUS_select_low_branch_errors = 0 ;

// ow_ds9490.c
unsigned int DS9490_wait_errors = 0 ;
unsigned int DS9490_reset_errors = 0 ;
unsigned int DS9490_sendback_data_errors = 0 ;
unsigned int DS9490_next_both_errors = 0 ;
unsigned int DS9490_PowerByte_errors = 0 ;
unsigned int DS9490_level_errors = 0 ;
unsigned int DS9490_select_low_errors = 0 ;

// ow_ds9097.c
unsigned int DS9097_detect_errors = 0 ;
unsigned int DS9097_PowerByte_errors = 0 ;
unsigned int DS9097_level_errors = 0 ;
unsigned int DS9097_next_both_errors = 0 ;
unsigned int DS9097_read_bits_errors = 0 ;
unsigned int DS9097_sendback_data_errors = 0 ;
unsigned int DS9097_sendback_bits_errors = 0 ;
unsigned int DS9097_read_errors = 0 ;
unsigned int DS9097_write_errors = 0 ;
unsigned int DS9097_reset_errors = 0 ;
unsigned int DS9097_reset_tcsetattr_errors = 0 ;

// ow_ds9097U.c
unsigned int DS2480_reset_errors = 0 ;
unsigned int DS2480_send_cmd_errors = 0 ;
unsigned int DS2480_send_cmd_memcmp_errors = 0 ;
unsigned int DS2480_sendout_data_errors = 0 ;
unsigned int DS2480_sendout_cmd_errors = 0 ;
unsigned int DS2480_sendback_data_errors = 0 ;
unsigned int DS2480_sendback_cmd_errors = 0 ;
unsigned int DS2480_write_errors = 0 ;
unsigned int DS2480_write_interrupted = 0 ;
unsigned int DS2480_read_errors = 0 ;
unsigned int DS2480_read_interrupted = 0 ;
unsigned int DS2480_read_select_errors = 0 ;
unsigned int DS2480_read_timeout = 0 ;
unsigned int DS2480_PowerByte_errors = 0 ;
unsigned int DS2480_level_errors = 0 ;
unsigned int DS2480_databit_errors = 0 ;
unsigned int DS2480_next_both_errors = 0 ;
unsigned int DS2480_ProgramPulse_errors = 0 ;


struct average all_avg = {0L,0L,0L,0L,} ;

/* ------- Prototypes ----------- */
/* Statistics reporting */
 uREAD_FUNCTION( FS_stat ) ;
 uREAD_FUNCTION( FS_stat_p ) ;
 fREAD_FUNCTION( FS_time ) ;
 fREAD_FUNCTION( FS_time_p ) ;
 uREAD_FUNCTION( FS_elapsed ) ;

/* -------- Structures ---------- */
struct filetype stats_cache[] = {
    {"flips"           , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & cache_flips      , } ,
    {"additions"       , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & cache_adds       , } ,
    {"primary"         ,  0, NULL  , ft_subdir  , ft_statistic, {v:NULL}   , {v:NULL}, NULL               , } ,
    {"primary/now"     , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & new_avg.current  , } ,
    {"primary/sum"     , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & new_avg.sum      , } ,
    {"primary/num"     , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & new_avg.count    , } ,
    {"primary/max"     , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & new_avg.max      , } ,
    {"secondary"       ,  0, NULL  , ft_subdir  , ft_statistic, {v:NULL}   , {v:NULL}, NULL               , } ,
    {"secondary/now"   , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & old_avg.current  , } ,
    {"secondary/sum"   , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & old_avg.sum      , } ,
    {"secondary/num"   , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & old_avg.count    , } ,
    {"secondary/max"   , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & old_avg.max      , } ,
    {"persistent"      ,  0, NULL  , ft_subdir  , ft_statistic, {v:NULL}   , {v:NULL}, NULL               , } ,
    {"persistent/now"  , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & store_avg.current, } ,
    {"persistent/sum"  , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & store_avg.sum    , } ,
    {"persistent/num"  , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & store_avg.count  , } ,
    {"persistent/max"  , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & store_avg.max    , } ,
    {"external"        ,  0, NULL  , ft_subdir  , ft_statistic, {v:NULL}   , {v:NULL}, NULL               , } ,
    {"external/tries"  , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & cache_ext.tries  , } ,
    {"external/hits"   , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & cache_ext.hits   , } ,
    {"external/added"  , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & cache_ext.adds, } ,
    {"external/expired", 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & cache_ext.expires, } ,
    {"external/deleted", 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & cache_ext.deletes, } ,
    {"internal"        ,  0, NULL  , ft_subdir  , ft_statistic, {v:NULL}   , {v:NULL}, NULL               , } ,
    {"internal/tries"  , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & cache_int.tries  , } ,
    {"internal/hits"   , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & cache_int.hits   , } ,
    {"internal/added"  , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & cache_int.adds, } ,
    {"internal/expired", 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & cache_int.expires, } ,
    {"internal/deleted", 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & cache_int.deletes, } ,
    {"directory"        , 0, NULL  , ft_subdir  , ft_statistic, {v:NULL}   , {v:NULL}, NULL               , } ,
    {"directory/tries"  ,15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & cache_dir.tries  , } ,
    {"directory/hits"   ,15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & cache_dir.hits   , } ,
    {"directory/added"  ,15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & cache_dir.adds   , } ,
    {"directory/expired",15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & cache_dir.expires, } ,
    {"directory/deleted",15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & cache_dir.deletes, } ,
} ;

struct device d_stats_cache = { "cache", "cache", 0, NFT(stats_cache), stats_cache } ;
    // Note, the store hit rate and deletions are not shown -- too much information!

struct aggregate Aread = { 3 , ag_numbers, ag_separate, } ;
struct filetype stats_read[] = {
    {"calls"           , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & read_calls       , } ,
    {"cachesuccess"    , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & read_cache       , } ,
    {"cachebytes"      , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & read_cachebytes  , } ,
    {"success"         , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & read_success     , } ,
    {"bytes"           , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & read_bytes       , } ,
    {"tries"           , 15, &Aread, ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, read_tries         , } ,
}
 ;
struct device d_stats_read = { "read", "read", 0, NFT(stats_read), stats_read } ;

struct filetype stats_write[] = {
    {"calls"           , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & write_calls      , } ,
    {"success"         , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & write_success    , } ,
    {"bytes"           , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & write_bytes      , } ,
    {"tries"           , 15, &Aread, ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, write_tries        , } ,
}
 ;
struct device d_stats_write = { "write", "write", 0, NFT(stats_write), stats_write } ;

struct filetype stats_directory[] = {
    {"maxdepth"        , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & dir_depth        , } ,
    {"bus"             ,  0, NULL  , ft_subdir  , ft_statistic, {v:NULL}   , {v:NULL}, NULL               , } ,
    {"bus/calls"       , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & dir_main.calls   , } ,
    {"bus/entries"     , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & dir_main.entries , } ,
    {"device"          ,  0, NULL  , ft_subdir  , ft_statistic, {v:NULL}   , {v:NULL}, NULL               , } ,
    {"device/calls"    , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & dir_dev.calls    , } ,
    {"device/entries"  , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & dir_dev.entries  , } ,
}
 ;
struct device d_stats_directory = { "directory", "directory", 0, NFT(stats_directory), stats_directory } ;

struct filetype stats_thread[] = {
    {"multithreading"  , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & multithreading   , } ,
    {"device_slots"    , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & maxslots         , } ,
    {"directory"       ,  0, NULL  , ft_subdir  , ft_statistic, {v:NULL}   , {v:NULL}, NULL               , } ,
    {"directory/now"   , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & dir_avg.current  , } ,
    {"directory/sum"   , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & dir_avg.sum      , } ,
    {"directory/num"   , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & dir_avg.count    , } ,
    {"directory/max"   , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & dir_avg.max      , } ,
    {"overall"         ,  0, NULL  , ft_subdir  , ft_statistic, {v:NULL}   , {v:NULL}, NULL               , } ,
    {"overall/now"     , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & all_avg.current  , } ,
    {"overall/sum"     , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & all_avg.sum      , } ,
    {"overall/num"     , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & all_avg.count    , } ,
    {"overall/max"     , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & all_avg.max      , } ,
    {"read"            ,  0, NULL  , ft_subdir  , ft_statistic, {v:NULL}   , {v:NULL}, NULL               , } ,
    {"read/now"        , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & read_avg.current , } ,
    {"read/sum"        , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & read_avg.sum     , } ,
    {"read/num"        , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & read_avg.count   , } ,
    {"read/max"        , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & read_avg.max     , } ,
    {"write"           ,  0, NULL  , ft_subdir  , ft_statistic, {v:NULL}   , {v:NULL}, NULL               , } ,
    {"write/now"       , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & write_avg.current, } ,
    {"write/sum"       , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & write_avg.sum    , } ,
    {"write/num"       , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & write_avg.count  , } ,
    {"write/max"       , 15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & write_avg.max    , } ,
}
 ;
struct device d_stats_thread = { "threads", "threads", 0, NFT(stats_thread), stats_thread } ;

extern struct aggregate Asystem;

struct filetype stats_bus[] = {
    {"elapsed_time"    , 15, NULL    , ft_unsigned, ft_statistic, {u:FS_elapsed},{v:NULL}, NULL          , } ,
    {"bus_time"        , 12, &Asystem, ft_float,    ft_statistic, {u:FS_time_p}, {v:NULL}, (void *)0  , } ,
    /* bus_idle_time should be the same is elapsed_time - bus_time
     * Not any big idea to implement it */
  //{"bus_idle_time"   , 12, &Asystem, ft_float,    ft_statistic, {u:FS_time_p}, {v:NULL}, bus_idle_time , } ,
    {"bus_locks"       , 15, &Asystem, ft_unsigned, ft_statistic, {u:FS_stat_p}, {v:NULL}, (void *)0  , } ,
    {"bus_unlocks"     , 15, &Asystem, ft_unsigned, ft_statistic, {u:FS_stat_p}, {v:NULL}, (void *)1  , } ,

    /* bus_pause_time is not very useful... Look at bus_time to see if the bus
     * has been used much instead */
  //{"bus_pause_time"  , 12, NULL , ft_float,    ft_statistic, {f:FS_time}, {v:NULL}, & bus_pause        , } ,
    {"total_bus_time"  , 12, NULL , ft_float,    ft_statistic, {f:FS_time}, {v:NULL}, & total_bus_time   , } ,
    {"total_bus_unlocks",15, NULL , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & total_bus_unlocks, } ,
    {"total_bus_locks" , 15, NULL , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & total_bus_locks  , } ,
};
struct device d_stats_bus = { "bus", "bus", 0, NFT(stats_bus), stats_bus } ;

#define FS_stat_ROW(var) {"" #var "",      15, NULL  , ft_unsigned, ft_statistic, {u:FS_stat}, {v:NULL}, & var, }

struct filetype stats_errors[] = {
    {"max_delay"        , 12, NULL  , ft_float, ft_statistic, {f:FS_time}, {v:NULL}, & max_delay         , } ,

// ow_bus.c
FS_stat_ROW(BUS_send_data_errors),
FS_stat_ROW(BUS_send_data_memcmp_errors),
FS_stat_ROW(BUS_readin_data_errors),
FS_stat_ROW(BUS_send_and_get_timeout),
FS_stat_ROW(BUS_send_and_get_select_errors),
FS_stat_ROW(BUS_send_and_get_errors),
FS_stat_ROW(BUS_send_and_get_interrupted),
FS_stat_ROW(BUS_select_low_errors),
FS_stat_ROW(BUS_select_low_branch_errors),

// ow_ds9490.c
FS_stat_ROW(DS9490_wait_errors),
FS_stat_ROW(DS9490_reset_errors),
FS_stat_ROW(DS9490_sendback_data_errors),
FS_stat_ROW(DS9490_next_both_errors),
FS_stat_ROW(DS9490_PowerByte_errors),
FS_stat_ROW(DS9490_level_errors),
FS_stat_ROW(DS9490_select_low_errors),

// ow_ds9097.c
FS_stat_ROW(DS9097_detect_errors),
FS_stat_ROW(DS9097_PowerByte_errors),
FS_stat_ROW(DS9097_level_errors),
FS_stat_ROW(DS9097_next_both_errors),
FS_stat_ROW(DS9097_read_bits_errors),
FS_stat_ROW(DS9097_sendback_data_errors),
FS_stat_ROW(DS9097_sendback_bits_errors),
FS_stat_ROW(DS9097_read_errors),
FS_stat_ROW(DS9097_write_errors),
FS_stat_ROW(DS9097_reset_errors),
FS_stat_ROW(DS9097_reset_tcsetattr_errors),

// ow_ds9097U.c
FS_stat_ROW(DS2480_reset_errors),
FS_stat_ROW(DS2480_send_cmd_errors),
FS_stat_ROW(DS2480_send_cmd_memcmp_errors),
FS_stat_ROW(DS2480_sendout_data_errors),
FS_stat_ROW(DS2480_sendout_cmd_errors),
FS_stat_ROW(DS2480_sendback_data_errors),
FS_stat_ROW(DS2480_sendback_cmd_errors),
FS_stat_ROW(DS2480_write_errors),
FS_stat_ROW(DS2480_write_interrupted),
FS_stat_ROW(DS2480_read_errors),
FS_stat_ROW(DS2480_read_interrupted),
FS_stat_ROW(DS2480_read_select_errors),
FS_stat_ROW(DS2480_read_timeout),
FS_stat_ROW(DS2480_PowerByte_errors),
FS_stat_ROW(DS2480_level_errors),
FS_stat_ROW(DS2480_databit_errors),
FS_stat_ROW(DS2480_next_both_errors),
FS_stat_ROW(DS2480_ProgramPulse_errors),

FS_stat_ROW(CRC8_errors),
FS_stat_ROW(CRC8_tries),
FS_stat_ROW(CRC16_errors),
FS_stat_ROW(CRC16_tries),

}
 ;

struct device d_stats_errors = { "errors", "errors", 0, NFT(stats_errors), stats_errors } ;


/* ------- Functions ------------ */

static int FS_stat(unsigned int * u , const struct parsedname * pn) {
    int dindex = pn->extension ;
    if (dindex<0) dindex = 0 ;
    if (pn->ft->data == NULL) return -ENOENT ;
    STATLOCK
        u[0] =  ((unsigned int *)pn->ft->data)[dindex] ;
    STATUNLOCK
    return 0 ;
}

static int FS_stat_p(unsigned int * u , const struct parsedname * pn) {
    int dindex = pn->extension ;
    unsigned int *ptr;
    struct connection_in *c;
    if (dindex<0) dindex = 0 ;
    c = find_connection_in(dindex+1);
    if(!c) return -ENOENT ;
    switch((unsigned int)pn->ft->data) {
    case 0:
      ptr = &c->bus_locks;
      break;
    case 1:
      ptr = &c->bus_unlocks;
      break;
    default:
      return -ENOENT;
    }
    STATLOCK
        u[0] =  *ptr;
    STATUNLOCK
    return 0 ;
}

static int FS_time_p(FLOAT *u , const struct parsedname * pn) {
    FLOAT f;
    int dindex = pn->extension ;
    struct timeval * tv;
    struct connection_in *c;
    if (dindex<0) dindex = 0 ;
    c = find_connection_in(dindex+1);
    if(!c) return -ENOENT ;
    
    switch((unsigned int)pn->ft->data) {
    case 0:
      tv = &c->bus_time;
      break;
    default:
      return -ENOENT;
    }
    STATLOCK /* to prevent simultaneous changes to bus timing variables */
    f = (FLOAT)tv->tv_sec + ((FLOAT)(tv->tv_usec/1000))/1000.0;
    STATUNLOCK
//printf("FS_time sec=%ld usec=%ld f=%7.3f\n",tv[dindex].tv_sec,tv[dindex].tv_usec, f) ;
    u[0] = f;
    return 0 ;
}

static int FS_time(FLOAT *u , const struct parsedname * pn) {
    FLOAT f;
    int dindex = pn->extension ;
    struct timeval * tv = (struct timeval *) pn->ft->data ;
    if (dindex<0) dindex = 0 ;
    if (tv == NULL) return -ENOENT ;

    STATLOCK /* to prevent simultaneous changes to bus timing variables */
    f = (FLOAT)tv[dindex].tv_sec + ((FLOAT)(tv[dindex].tv_usec/1000))/1000.0;
    STATUNLOCK
//printf("FS_time sec=%ld usec=%ld f=%7.3f\n",tv[dindex].tv_sec,tv[dindex].tv_usec, f) ;
    u[0] = f;
    return 0 ;
}

static int FS_elapsed(unsigned int * u , const struct parsedname * pn) {
//printf("ELAPSE start=%u, now=%u, diff=%u\n",start_time,time(NULL),time(NULL)-start_time) ;
    (void) pn ;
    STATLOCK
        u[0] = time(NULL)-start_time ;
    STATUNLOCK
    return 0 ;
}
