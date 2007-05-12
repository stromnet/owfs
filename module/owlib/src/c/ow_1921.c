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

#include <config.h>
#include "owfs_config.h"
#include "ow_1921.h"

/* ------- Prototypes ----------- */
/* DS1921 Temperature */
READ_FUNCTION(FS_r_histotemp);
READ_FUNCTION(FS_r_histogap);
READ_FUNCTION(FS_r_histoelem);
READ_FUNCTION(FS_r_resolution);
READ_FUNCTION(FS_r_version);
READ_FUNCTION(FS_r_rangelow);
READ_FUNCTION(FS_r_rangehigh);
READ_FUNCTION(FS_r_histogram);
READ_FUNCTION(FS_r_logtemp);
READ_FUNCTION(FS_r_logdate);
READ_FUNCTION(FS_r_logudate);
READ_FUNCTION(FS_logelements);
READ_FUNCTION(FS_r_temperature);
READ_FUNCTION(FS_bitread);
WRITE_FUNCTION(FS_bitwrite);
READ_FUNCTION(FS_rbitread);
WRITE_FUNCTION(FS_rbitwrite);
WRITE_FUNCTION(FS_easystart);

READ_FUNCTION(FS_alarmudate);
READ_FUNCTION(FS_alarmstart);
READ_FUNCTION(FS_alarmend);
READ_FUNCTION(FS_alarmcnt);
READ_FUNCTION(FS_alarmelems);
READ_FUNCTION(FS_r_alarmtemp);
WRITE_FUNCTION(FS_w_alarmtemp);

READ_FUNCTION(FS_mdate);
READ_FUNCTION(FS_umdate);
READ_FUNCTION(FS_r_date);
WRITE_FUNCTION(FS_w_date);
READ_FUNCTION(FS_r_counter);
WRITE_FUNCTION(FS_w_counter);
READ_FUNCTION(FS_r_delay);
WRITE_FUNCTION(FS_w_delay);
READ_FUNCTION(FS_r_mem);
WRITE_FUNCTION(FS_w_mem);
READ_FUNCTION(FS_r_page);
WRITE_FUNCTION(FS_w_page);
READ_FUNCTION(FS_r_samplerate);
WRITE_FUNCTION(FS_w_samplerate);
READ_FUNCTION(FS_r_run);
WRITE_FUNCTION(FS_w_run);
READ_FUNCTION(FS_r_3byte);
READ_FUNCTION(FS_r_atime);
WRITE_FUNCTION(FS_w_atime);
READ_FUNCTION(FS_r_atrig);
WRITE_FUNCTION(FS_w_atrig);
WRITE_FUNCTION(FS_w_mip);

/* ------- Structures ----------- */
#define HISTOGRAM_DATA_ELEMENTS 63
#define LOG_DATA_ELEMENTS 2048

struct BitRead {
	size_t location;
	int bit;
};
static struct BitRead BitReads[] = {
	{0x0214, 7,},				//temperature in progress
	{0x0214, 5,},				// Mission in progress
	{0x0214, 4,},				//sample in progress
	{0x020E, 3,},				// rollover
	{0x020E, 7,},				// clock running (reversed)
};

struct Mission {
	_DATE start;
	int rollover;
	int interval;
	int samples;
};

struct aggregate A1921p = { 16, ag_numbers, ag_separate, };
struct aggregate A1921l = { LOG_DATA_ELEMENTS, ag_numbers, ag_mixed, };
struct aggregate A1921h = { HISTOGRAM_DATA_ELEMENTS, ag_numbers, ag_mixed, };
struct aggregate A1921m = { 12, ag_numbers, ag_aggregate, };
struct filetype DS1921[] = {
	F_STANDARD,
  {"memory", 512, NULL, ft_binary, fc_stable, {o: FS_r_mem}, {o: FS_w_mem}, {v:NULL},},

  {"pages",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {o: NULL}, {o: NULL}, {v:NULL},},
  {"pages/page", 32, &A1921p, ft_binary, fc_stable, {o: FS_r_page}, {o: FS_w_page}, {v:NULL},},

  {"histogram",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {o: NULL}, {o: NULL}, {v:NULL},},
  {"histogram/counts",PROPERTY_LENGTH_UNSIGNED, &A1921h, ft_unsigned, fc_volatile, {o: FS_r_histogram}, {o: NULL}, {v:NULL},},
  {"histogram/elements",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_static, {o: FS_r_histoelem}, {o: NULL}, {v:NULL},},
  {"histogram/gap",PROPERTY_LENGTH_TEMPGAP, NULL, ft_tempgap, fc_static, {o: FS_r_histogap}, {o: NULL}, {v:NULL},},
  {"histogram/temperature",PROPERTY_LENGTH_TEMP, &A1921h, ft_temperature, fc_static, {o: FS_r_histotemp}, {o: NULL}, {v:NULL},},

  {"clock",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {o: NULL}, {o: NULL}, {v:NULL},},
  {"clock/date",PROPERTY_LENGTH_DATE, NULL, ft_date, fc_second, {o: FS_r_date}, {o: FS_w_date}, {v:NULL},},
  {"clock/udate",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_second, {o: FS_r_counter}, {o: FS_w_counter}, {v:NULL},},
  {"clock/running",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, {o: FS_rbitread}, {o: FS_rbitwrite}, {v:&BitReads[4]},},

  {"about",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {o: NULL}, {o: NULL}, {v:NULL},},
  {"about/resolution",PROPERTY_LENGTH_TEMPGAP, NULL, ft_tempgap, fc_static, {o: FS_r_resolution}, {o: NULL}, {v:NULL},},
  {"about/templow",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_static, {o: FS_r_rangelow}, {o: NULL}, {v:NULL},},
  {"about/temphigh",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_static, {o: FS_r_rangehigh}, {o: NULL}, {v:NULL},},
  {"about/version", 11, NULL, ft_ascii, fc_stable, {o: FS_r_version}, {o: NULL}, {v:NULL},},
  {"about/samples",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile, {o: FS_r_3byte}, {o: NULL}, {s:0x021D},},
  {"about/measuring",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_rbitread}, {o: NULL}, {v:&BitReads[0]},},

  {"temperature",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_r_temperature}, {o: NULL}, {v:NULL},},

  {"mission",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {o: NULL}, {o: NULL}, {v:NULL},},
  {"mission/running",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_bitread}, {o: FS_w_mip}, {v:&BitReads[1]},},
  {"mission/frequency",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_samplerate}, {o: FS_w_samplerate}, {v:NULL},},
  {"mission/samples",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile, {o: FS_r_3byte}, {o: NULL}, {s:0x021A},},
  {"mission/delay",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile, {o: FS_r_delay}, {o: FS_w_delay}, {v:NULL},},
  {"mission/rollover",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, {o: FS_bitread}, {o: FS_bitwrite}, {v:&BitReads[3]},},
  {"mission/date",PROPERTY_LENGTH_DATE, NULL, ft_date, fc_volatile, {o: FS_mdate}, {o: NULL}, {v:NULL},},
  {"mission/udate",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile, {o: FS_umdate}, {o: NULL}, {v:NULL},},
  {"mission/sampling",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_bitread}, {o: NULL}, {v:&BitReads[2]},},
  {"mission/easystart",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_stable, {o: NULL}, {o: FS_easystart}, {v:NULL},},

  {"overtemp",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {o: NULL}, {o: NULL}, {v:NULL},},
  {"overtemp/date",PROPERTY_LENGTH_DATE, &A1921m, ft_date, fc_volatile, {o: FS_alarmstart}, {o: NULL}, {s:0x0250},},
  {"overtemp/udate",PROPERTY_LENGTH_UNSIGNED, &A1921m, ft_unsigned, fc_volatile, {o: FS_alarmudate}, {o: NULL}, {s:0x0250},},
  {"overtemp/end",PROPERTY_LENGTH_DATE, &A1921m, ft_date, fc_volatile, {o: FS_alarmend}, {o: NULL}, {s:0x0250},},
  {"overtemp/count",PROPERTY_LENGTH_UNSIGNED, &A1921m, ft_unsigned, fc_volatile, {o: FS_alarmcnt}, {o: NULL}, {s:0x0250},},
  {"overtemp/elements",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile, {o: FS_alarmelems}, {o: NULL}, {s:0x0250},},
  {"overtemp/temperature",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_stable, {o: FS_r_alarmtemp}, {o: FS_w_alarmtemp}, {s:0x020C},},

  {"undertemp",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {o: NULL}, {o: NULL}, {v:NULL},},
  {"undertemp/date",PROPERTY_LENGTH_DATE, &A1921m, ft_date, fc_volatile, {o: FS_alarmstart}, {o: NULL}, {s:0x0220},},
  {"undertemp/udate",PROPERTY_LENGTH_UNSIGNED, &A1921m, ft_unsigned, fc_volatile, {o: FS_alarmudate}, {o: NULL}, {s:0x0220},},
  {"undertemp/end",PROPERTY_LENGTH_DATE, &A1921m, ft_date, fc_volatile, {o: FS_alarmend}, {o: NULL}, {s:0x0220},},
  {"undertemp/count",PROPERTY_LENGTH_UNSIGNED, &A1921m, ft_unsigned, fc_volatile, {o: FS_alarmcnt}, {o: NULL}, {s:0x0220},},
  {"undertemp/elements",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile, {o: FS_alarmelems}, {o: NULL}, {s:0x0220},},
  {"undertemp/temperature",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_stable, {o: FS_r_alarmtemp}, {o: FS_w_alarmtemp}, {s:0x020C},},

  {"log",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {o: NULL}, {o: NULL}, {v:NULL},},
  {"log/temperature",PROPERTY_LENGTH_TEMP, &A1921l, ft_temperature, fc_volatile, {o: FS_r_logtemp}, {o: NULL}, {v:NULL},},
  {"log/date",PROPERTY_LENGTH_DATE, &A1921l, ft_date, fc_volatile, {o: FS_r_logdate}, {o: NULL}, {v:NULL},},
  {"log/udate",PROPERTY_LENGTH_UNSIGNED, &A1921l, ft_unsigned, fc_volatile, {o: FS_r_logudate}, {o: NULL}, {v:NULL},},
  {"log/elements",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile, {o: FS_logelements}, {o: NULL}, {v:NULL},},

  // no entries in these directories yet
  {"set_alarm",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {o: NULL}, {o: NULL}, {v:NULL},},
  {"set_alarm/trigger",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {o: NULL}, {o: NULL}, {v:NULL},},
  {"set_alarm/templow",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {o: NULL}, {o: NULL}, {v:NULL},},
  {"set_alarm/temphigh",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {o: NULL}, {o: NULL}, {v:NULL},},
  {"set_alarm/date",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {o: NULL}, {o: NULL}, {v:NULL},},

  {"alarm_state",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_stable, {o: FS_r_samplerate}, {o: FS_w_samplerate}, {v:NULL},},

  {"running",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, {o: FS_r_run}, {o: FS_w_run}, {v:NULL},},
  {"alarm_second",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_stable, {o: FS_r_atime}, {o: FS_w_atime}, {s:0x0207},},
  {"alarm_minute",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_stable, {o: FS_r_atime}, {o: FS_w_atime}, {s:0x0208},},
  {"alarm_hour",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_stable, {o: FS_r_atime}, {o: FS_w_atime}, {s:0x0209},},
  {"alarm_dow",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_stable, {o: FS_r_atime}, {o: FS_w_atime}, {s:0x020A},},
  {"alarm_trigger",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_stable, {o: FS_r_atrig}, {o: FS_w_atrig}, {v:NULL},},
};

DeviceEntryExtended(21, DS1921, DEV_alarm | DEV_temp | DEV_ovdr);

#define _1W_WRITE_SCRATCHPAD 0x0F
#define _1W_READ_SCRATCHPAD 0xAA
#define _1W_COPY_SCRATCHPAD 0x55
#define _1W_READ_MEMORY 0xF0
#define _1W_READ_MEMORY_WITH_CRC 0xA5
#define _1W_CLEAR_MEMORY 0x3C
#define _1W_CONVERT_TEMPERATURE 0x44

/* Different version of the Thermocron, sorted by ID[11,12] of name. Keep in sorted order */
struct Version {
	UINT ID;
	char *name;
	_FLOAT histolow;
	_FLOAT resolution;
	_FLOAT rangelow;
	_FLOAT rangehigh;
	UINT delay;
};
static struct Version Versions[] = {
	{0x000, "DS1921G-F5", -40.0, 0.500, -40., +85., 90,},
	{0x064, "DS1921L-F50", -40.0, 0.500, -40., +85., 300,},
	{0x15C, "DS1921L-F53", -40.0, 0.500, -30., +85., 300,},
	{0x254, "DS1921L-F52", -40.0, 0.500, -20., +85., 300,},
	{0x34C, "DS1921L-F51", -40.0, 0.500, -10., +85., 300,},
	{0x3B2, "DS1921Z-F5", -5.5, 0.125, -5., +26., 360,},
	{0x4F2, "DS1921H-F5", +14.5, 0.125, +15., +46., 360,},
};

	/* AM/PM for hours field */
const int ampm[8] = { 0, 10, 20, 30, 0, 10, 12, 22 };

#define VersionElements ( sizeof(Versions) / sizeof(struct Version) )
static int VersionCmp(const void *pn, const void *version)
{
	return (((((const struct parsedname *) pn)->
			  sn[5]) >> 4) | (((UINT) ((const struct parsedname *) pn)->
							   sn[6]) << 4)) -
		((const struct Version *) version)->ID;
}

/* ------- Functions ------------ */

/* DS1921 */
static int OW_w_mem( BYTE * data,  size_t length,
					 off_t offset,  struct parsedname *pn);
static int OW_temperature(int *T, const UINT delay, struct parsedname *pn);
static int OW_clearmemory( struct parsedname *pn);
static int OW_2date(_DATE * d, const BYTE * data);
static int OW_2mdate(_DATE * d, const BYTE * data);
static void OW_date(const _DATE * d, BYTE * data);
static int OW_MIP(struct parsedname *pn);
static int OW_FillMission(struct Mission *m, struct parsedname *pn);
static int OW_alarmlog(int *t, int *c, off_t offset, struct parsedname *pn);
static int OW_stopmission( struct parsedname *pn);
static int OW_startmission(UINT freq, struct parsedname *pn);
static int OW_w_date(_DATE * D, struct parsedname * pn ) ;
static int OW_w_run(int state, struct parsedname * pn ) ;
static int OW_small_read( BYTE * buffer, size_t size, off_t location, struct parsedname * pn ) ;
static int OW_r_histogram_single(struct one_wire_query * owq) ;
static int OW_r_histogram_all(struct one_wire_query * owq) ;
static int OW_r_logtemp_single(struct Version *v, struct Mission * mission, struct one_wire_query * owq) ;
static int OW_r_logtemp_all(struct Version *v, struct Mission * mission, struct one_wire_query * owq) ;
static int OW_r_logdate_all(struct Mission * mission, struct one_wire_query * owq) ;
static int OW_r_logdate_single(struct Mission * mission, struct one_wire_query * owq) ;
static int OW_r_logudate_all(struct Mission * mission, struct one_wire_query * owq) ;
static int OW_r_logudate_single(struct Mission * mission, struct one_wire_query * owq) ;

static int FS_bitread(struct one_wire_query * owq)
{
	BYTE d;
    struct parsedname * pn = PN(owq) ;
	struct BitRead *br;
	if (pn->ft->data.v == NULL)
		return -EINVAL;
	br = ((struct BitRead *) (pn->ft->data.v));
	if (OW_small_read(&d, 1, br->location, pn ))
		return -EINVAL;
    OWQ_Y(owq) = UT_getbit(&d, br->bit);
	return 0;
}

static int FS_bitwrite(struct one_wire_query * owq)
{
	BYTE d;
    struct parsedname * pn = PN(owq) ;
    struct BitRead *br;
	if (pn->ft->data.v == NULL)
		return -EINVAL;
	br = ((struct BitRead *) (pn->ft->data.v));
	if (OW_small_read(&d, 1, br->location, pn ))
		return -EINVAL;
    UT_setbit(&d, br->bit, OWQ_Y(owq));
	if (OW_w_mem(&d, 1, br->location, pn))
		return -EINVAL;
	return 0;
}

static int FS_rbitread(struct one_wire_query * owq)
{
	int ret = FS_bitread(owq);
    OWQ_Y(owq) = !OWQ_Y(owq);
	return ret;
}

static int FS_rbitwrite(struct one_wire_query * owq)
{
    OWQ_Y(owq) = !OWQ_Y(owq) ;
	return FS_bitwrite(owq);
}

/* histogram counts */
static int FS_r_histogram(struct one_wire_query * owq)
{
    switch ( OWQ_pn(owq).extension ) {
        case EXTENSION_ALL:
            return OW_r_histogram_all(owq) ;
        default:
            return OW_r_histogram_single(owq) ;
    }
}

static int FS_r_histotemp(struct one_wire_query * owq)
{
    int extension = OWQ_pn(owq).extension ;
    struct Version *v =
            (struct Version *) bsearch(PN(owq), Versions, VersionElements,
								   sizeof(struct Version), VersionCmp);
	if (v == NULL)
		return -EINVAL;
	if (extension == EXTENSION_ALL) {	/* ALL */
		int i;
        for (i = 0; i < HISTOGRAM_DATA_ELEMENTS; ++i)
            OWQ_array_F(owq,i) = v->histolow + 4 * i * v->resolution;
	} else {					/* element */
        OWQ_F(owq) = v->histolow + 4 * extension * v->resolution;
	}
	return 0;
}

static int FS_r_histogap(struct one_wire_query * owq)
{
	struct Version *v =
            (struct Version *) bsearch(PN(owq), Versions, VersionElements,
								   sizeof(struct Version), VersionCmp);
	if (v == NULL)
		return -EINVAL;
    OWQ_F(owq) = v->resolution * 4;
	return 0;
}
static int FS_r_histoelem(struct one_wire_query * owq)
{
    OWQ_U(owq) = HISTOGRAM_DATA_ELEMENTS;
	return 0;
}


static int FS_r_version(struct one_wire_query * owq)
{
	struct Version *v =
            (struct Version *) bsearch(PN(owq), Versions, VersionElements,
								   sizeof(struct Version), VersionCmp);
    return v ? Fowq_output_offset_and_size_z(v->name, owq) : -ENOENT;
}

static int FS_r_resolution(struct one_wire_query * owq)
{
	struct Version *v =
            (struct Version *) bsearch(PN(owq), Versions, VersionElements,
								   sizeof(struct Version), VersionCmp);
	if (v == NULL)
		return -EINVAL;
    OWQ_F(owq) = v->resolution;
	return 0;
}

static int FS_r_rangelow(struct one_wire_query * owq)
{
	struct Version *v =
            (struct Version *) bsearch(PN(owq), Versions, VersionElements,
								   sizeof(struct Version), VersionCmp);
	if (v == NULL)
		return -EINVAL;
    OWQ_F(owq) = v->rangelow;
	return 0;
}

static int FS_r_rangehigh(struct one_wire_query * owq)
{
	struct Version *v =
            (struct Version *) bsearch(PN(owq), Versions, VersionElements,
								   sizeof(struct Version), VersionCmp);
	if (v == NULL)
		return -EINVAL;
    OWQ_F(owq) = v->rangehigh;
	return 0;
}

/* Temperature -- force if not in progress */
static int FS_r_temperature(struct one_wire_query * owq)
{
	int temp;
    struct parsedname * pn = PN(owq) ;
	struct Version *v =
		(struct Version *) bsearch(pn, Versions, VersionElements,
								   sizeof(struct Version), VersionCmp);
	if (v == NULL)
		return -EINVAL;
	if (OW_MIP(pn))
		return -EBUSY;			/* Mission in progress */
	if (OW_temperature(&temp, v->delay, pn))
		return -EINVAL;
    OWQ_F(owq) = (_FLOAT) temp *v->resolution + v->histolow;
	return 0;
}

/* read counter */
/* Save a function by shoving address in data field */
static int FS_r_3byte(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
	size_t addr = pn->ft->data.s;
	BYTE data[3];
	if (OW_small_read(data, 3, addr, pn ))
		return -EINVAL;
    OWQ_U(owq) = (((((UINT) data[2]) << 8) | data[1]) << 8) | data[0];
	return 0;
}

/* mission start date */
static int FS_mdate(struct one_wire_query * owq)
{
	struct Mission mission;

    if (OW_FillMission(&mission, PN(owq)))
		return -EINVAL;
	/* Get date from chip */
    OWQ_D(owq) = mission.start;
	return 0;
}

/* mission start date */
static int FS_umdate(struct one_wire_query * owq)
{
	struct Mission mission;

    if (OW_FillMission(&mission, PN(owq)))
		return -EINVAL;
	/* Get date from chip */
    OWQ_U(owq) = mission.start;
	return 0;
}

static int FS_alarmelems(struct one_wire_query * owq)
{
	struct Mission mission;
    struct parsedname * pn = PN(owq) ;
    int t[12];
	int c[12];
	int i;

	if (OW_FillMission(&mission, pn))
		return -EINVAL;
	if (OW_alarmlog(t, c, pn->ft->data.s, pn))
		return -EINVAL;
	for (i = 0; i < 12; ++i)
		if (c[i] == 0)
			break;
    OWQ_U(owq) = i;
	return 0;
}

/* Temperature -- force if not in progress */
static int FS_r_alarmtemp(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    BYTE data[1];
	struct Version *v =
		(struct Version *) bsearch(pn, Versions, VersionElements,
								   sizeof(struct Version), VersionCmp);
	if (v == NULL)
		return -EINVAL;
	if (OW_small_read(data, 1, pn->ft->data.s, pn ))
		return -EINVAL;
    OWQ_F(owq) = (_FLOAT) data[0] * v->resolution + v->histolow;
	return 0;
}

/* Temperature -- force if not in progress */
static int FS_w_alarmtemp(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    BYTE data[1];
	struct Version *v =
		(struct Version *) bsearch(pn, Versions, VersionElements,
								   sizeof(struct Version), VersionCmp);
	if (v == NULL)
		return -EINVAL;
	if (OW_MIP(pn))
		return -EBUSY;
    data[0] = (OWQ_F(owq) - v->histolow) / v->resolution;
	return (OW_w_mem(data, 1, pn->ft->data.s, pn)) ? -EINVAL : 0;
}

static int FS_alarmudate(struct one_wire_query * owq)
{
	struct Mission mission;
    struct parsedname * pn = PN(owq) ;
	int t[12];
	int c[12];
	int i;

	if (OW_FillMission(&mission, pn))
		return -EINVAL;
	if (OW_alarmlog(t, c, pn->ft->data.s, pn))
		return -EINVAL;
	for (i = 0; i < 12; ++i)
        OWQ_array_U(owq,i) = mission.start + t[i] * mission.interval;
	return 0;
}

static int FS_alarmstart(struct one_wire_query * owq)
{
	struct Mission mission;
    struct parsedname * pn = PN(owq) ;
    int t[12];
	int c[12];
	int i;

	if (OW_FillMission(&mission, pn))
		return -EINVAL;
	if (OW_alarmlog(t, c, pn->ft->data.s, pn))
		return -EINVAL;
	for (i = 0; i < 12; ++i)
        OWQ_array_D(owq,i) = mission.start + t[i] * mission.interval;
	return 0;
}

static int FS_alarmend(struct one_wire_query * owq)
{
	struct Mission mission;
    struct parsedname * pn = PN(owq) ;
    int t[12];
	int c[12];
	int i;

	if (OW_FillMission(&mission, pn))
		return -EINVAL;
	if (OW_alarmlog(t, c, pn->ft->data.s, pn))
		return -EINVAL;
	for (i = 0; i < 12; ++i)
        OWQ_array_D(owq,i) = mission.start + (t[i] + c[i]) * mission.interval;
	return 0;
}

static int FS_alarmcnt(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    int t[12];
	int c[12];
	int i;

	if (OW_alarmlog(t, c, pn->ft->data.s, pn))
		return -EINVAL;
	for (i = 0; i < 12; ++i)
        OWQ_array_U(owq,i) = c[i];
	return 0;
}

/* read clock */
static int FS_r_date(struct one_wire_query * owq)
{
	BYTE data[7];

	/* Get date from chip */
    if (OW_small_read(data, 7, 0x0200, PN(owq) ))
		return -EINVAL;
    return OW_2date(&OWQ_D(owq), data);
}

/* read clock */
static int FS_r_counter(struct one_wire_query * owq)
{
	BYTE data[7];
	_DATE d;
	int ret;

	/* Get date from chip */
    if (OW_small_read(data, 7, 0x0200, PN(owq) ))
		return -EINVAL;
	if ((ret = OW_2date(&d, data)))
		return ret;
    OWQ_U(owq) = (UINT) d;
	return 0;
}

/* set clock */
static int FS_w_date(struct one_wire_query * owq)
{
    return OW_w_date(&OWQ_D(owq), PN(owq));
}

static int FS_w_counter(struct one_wire_query * owq)
{
	BYTE data[7];
    struct parsedname * pn = PN(owq) ;
    _DATE d = (_DATE) OWQ_U(owq);

	/* Busy if in mission */
	if (OW_MIP(pn))
		return -EBUSY;

	OW_date(&d, data);
	return OW_w_mem(data, 7, 0x0200, pn) ? -EINVAL : 0;
}

/* stop/start clock running */
static int FS_w_run(struct one_wire_query * owq)
{
    return OW_w_run(OWQ_Y(owq),PN(owq) ) ;
}

/* clock running? */
static int FS_r_run(struct one_wire_query * owq)
{
	BYTE cr;
    if (OW_small_read(&cr, 1, 0x020E, PN(owq) ))
		return -EINVAL;
    OWQ_Y(owq) = ((cr & 0x80) == 0);
	return 0;
}

/* start/stop mission */
static int FS_w_mip(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    if (OWQ_Y(owq)) {					/* start a mission! */
		BYTE data;
		if (OW_small_read(&data, 1, 0x020D, pn ))
			return -EINVAL;
		return OW_startmission((UINT) data, pn);
	} else {
		return OW_stopmission(pn);
	}
}

/* read the interval between samples during a mission */
static int FS_r_samplerate(struct one_wire_query * owq)
{
	BYTE data;
    if (OW_small_read(&data, 1, 0x020D, PN(owq) ))
		return -EINVAL;
    OWQ_U(owq) = data;
	return 0;
}

/* write the interval between samples during a mission */
static int FS_w_samplerate(struct one_wire_query * owq)
{
    if (OWQ_U(owq) > 0) {
        return OW_startmission(OWQ_U(owq), PN(owq));
	} else {
        return OW_stopmission(PN(owq));
	}
}

/* read the alarm time field (not bit 7, though) */
static int FS_r_atime(struct one_wire_query * owq)
{
	BYTE data;
    if (OW_small_read(&data, 1, OWQ_pn(owq).ft->data.s, PN(owq) ))
		return -EFAULT;
    OWQ_U(owq) = data & 0x7F;
	return 0;
}

/* write one of the alarm fields */
/* NOTE: keep first bit */
static int FS_w_atime(struct one_wire_query * owq)
{
	BYTE data;
    struct parsedname * pn = PN(owq) ;
	if (OW_small_read(&data, 1, pn->ft->data.s, pn ))
		return -EFAULT;
    data = ((BYTE) OWQ_U(owq)) | (data & 0x80);	/* EM on */
	if (OW_w_mem(&data, 1, pn->ft->data.s, pn))
		return -EFAULT;
	return 0;
}

/* read the alarm time field (not bit 7, though) */
static int FS_r_atrig(struct one_wire_query * owq)
{
	BYTE data[4];
    if (OW_small_read(data, 4, 0x0207, PN(owq) ))
		return -EFAULT;
	if (data[3] & 0x80) {
        OWQ_U(owq) = 4;
	} else if (data[2] & 0x80) {
        OWQ_U(owq) = 3;
	} else if (data[1] & 0x80) {
        OWQ_U(owq) = 2;
	} else if (data[0] & 0x80) {
        OWQ_U(owq) = 1;
	} else {
        OWQ_U(owq) = 0;
	}
	return 0;
}

static int FS_r_mem(struct one_wire_query * owq)
{
	size_t pagesize = 32 ;
	if (OWQ_readwrite_paged(owq, 0, pagesize, OW_r_mem_crc16_A5 ))
		return -EINVAL;
    return 0;
}

static int FS_w_mem(struct one_wire_query * owq)
{
	size_t pagesize = 32 ;
	if (OW_readwrite_paged(owq, 0, pagesize, OW_w_mem))
		return -EINVAL;
	return 0;
}

static int FS_w_atrig(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    BYTE data[4];
	if (OW_small_read(data, 4, 0x0207, pn ))
		return -EFAULT;
	data[0] &= 0x7F;
	data[1] &= 0x7F;
	data[2] &= 0x7F;
	data[3] &= 0x7F;
    switch (OWQ_U(owq)) {				/* Intentional fall-throughs in cases */
	case 1:
		data[0] |= 0x80;
	case 2:
		data[1] |= 0x80;
	case 3:
		data[2] |= 0x80;
	case 4:
		data[3] |= 0x80;
	}
	if (OW_w_mem(data, 4, 0x0207, pn))
		return -EFAULT;
	return 0;
}

static int FS_r_page(struct one_wire_query * owq)
{
    size_t pagesize = 32 ;
	if (OW_r_mem_crc16_A5(owq, OWQ_pn(owq).extension, pagesize ))
		return -EINVAL;
    return OWQ_size(owq);
}

static int FS_w_page(struct one_wire_query * owq)
{
	if (OW_w_mem
           ((BYTE *) OWQ_buffer(owq), OWQ_size(owq), (size_t) (OWQ_offset(owq) + ((OWQ_pn(owq).extension) << 5)), PN(owq)))
		return -EINVAL;
	return 0;
}

/* temperature log */
static int FS_logelements(struct one_wire_query * owq)
{
	struct Mission mission;

    if (OW_FillMission(&mission, PN(owq)))
		return -EINVAL;

    OWQ_U(owq) = (mission.samples > LOG_DATA_ELEMENTS) ? LOG_DATA_ELEMENTS : mission.samples;
	return 0;
}

/* temperature log */
static int FS_r_logdate(struct one_wire_query * owq)
{
    struct Mission mission;

    if (OW_FillMission(&mission, PN(owq)))
        return -EINVAL;
    
    switch( OWQ_pn(owq).extension ) {
        case EXTENSION_ALL:
            return OW_r_logdate_all( &mission, owq ) ;
        default:
            return OW_r_logdate_single( &mission, owq ) ;
    }
}

/* temperature log */
static int FS_r_logudate(struct one_wire_query * owq)
{
    struct Mission mission;

    if (OW_FillMission(&mission, PN(owq)))
        return -EINVAL;
    
    switch( OWQ_pn(owq).extension ) {
        case EXTENSION_ALL:
            return OW_r_logudate_all( &mission, owq ) ;
        default:
            return OW_r_logudate_single( &mission, owq ) ;
    }
}

/* mission delay */
static int FS_r_delay(struct one_wire_query * owq)
{
	BYTE data[2];
    if (OW_small_read(data, 2, (size_t) 0x0212, PN(owq) ))
		return -EINVAL;
    OWQ_U(owq) = (((UINT) data[1]) << 8) | data[0];
	return 0;
}

/* mission delay */
static int FS_w_delay(struct one_wire_query * owq)
{
    BYTE data[] = { OWQ_U(owq) & 0xFF, (OWQ_U(owq) >> 8) & 0xFF, };
    if (OW_MIP(PN(owq)))
		return -EBUSY;
    if (OW_w_mem(data, 2, (size_t) 0x0212, PN(owq)))
		return -EINVAL;
	return 0;
}

/* temperature log */
static int FS_r_logtemp(struct one_wire_query * owq)
{
	struct Mission mission;
    struct parsedname * pn = PN(owq) ;
	struct Version *v =
		(struct Version *) bsearch(pn, Versions, VersionElements,
								   sizeof(struct Version), VersionCmp);

	if (v == NULL)
		return -EINVAL;

	if (OW_FillMission(&mission, pn))
		return -EINVAL;

    switch ( pn->extension ) {
        case EXTENSION_ALL:
            return OW_r_logtemp_all( v, &mission, owq ) ;
        default:
            return OW_r_logtemp_single( v, &mission, owq ) ;
    }
}

static int FS_easystart(struct one_wire_query * owq)
{
	/* write 0x020E -- 0x0214 */
	BYTE data[] = { 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, };

	/* Stop clock, no rollover, no delay, temp alarms on, alarms cleared */
    if (OW_w_mem(data, 7, 0x020E, PN(owq)))
		return -EINVAL;

    return OW_startmission(OWQ_U(owq), PN(owq));
}

static int OW_w_mem( BYTE * data,  size_t size,
					 off_t offset,  struct parsedname *pn)
{
	BYTE p[3 + 1 + 32 + 2] =
    { _1W_WRITE_SCRATCHPAD, LOW_HIGH_ADDRESS(offset), };
	size_t rest = 32 - (offset & 0x1F);
	int ret;

	/* Copy to scratchpad -- use CRC16 if write to end of page, but don't force it */
	memcpy(&p[3], data, size);
	if ((offset + size) & 0x1F) {	/* to end of page */
		BUSLOCK(pn);
		ret = BUS_select(pn) || BUS_send_data(p, 3 + size, pn);
		BUSUNLOCK(pn);
	} else {
		BUSLOCK(pn);
		ret = BUS_select(pn) || BUS_send_data(p, 3 + size, pn)
			|| BUS_readin_data(&p[3 + size], 2, pn)
			|| CRC16(p, 3 + size + 2);
		BUSUNLOCK(pn);
	}
	if (ret)
		return 1;

	/* Re-read scratchpad and compare */
	/* Note: location of data has now shifted down a byte for E/S register */
    p[0] = _1W_READ_SCRATCHPAD;
	BUSLOCK(pn);
	ret = BUS_select(pn) || BUS_send_data(p, 3, pn)
		|| BUS_readin_data(&p[3], 1 + rest + 2, pn)
		|| CRC16(p, 4 + rest + 2) || memcmp(&p[4], data, size);
	BUSUNLOCK(pn);
	if (ret)
		return 1;

	/* Copy Scratchpad to SRAM */
    p[0] = _1W_COPY_SCRATCHPAD;
	BUSLOCK(pn);
	ret = BUS_select(pn) || BUS_send_data(p, 4, pn);
	BUSUNLOCK(pn);
	if (ret)
		return 1;

	UT_delay(1);				/* 1 msec >> 2 usec per byte */
	return 0;
}

static int OW_temperature(int *T, const UINT delay,
						  struct parsedname *pn)
{
    BYTE data = _1W_CONVERT_TEMPERATURE;
	int ret;

	/* Mission not progress, force conversion */
	BUSLOCK(pn);
	ret = BUS_select(pn) || BUS_send_data(&data, 1, pn);
	BUSUNLOCK(pn);
	if (ret)
		return 1;

	/* Thermochron is powered (internally by battery) -- no reason to hold bus */
	UT_delay(delay);

	ret = OW_small_read(&data, 1, 0x0211, pn );	/* read temp register */
	*T = (int) data;
	return ret;
}

static int OW_clearmemory( struct parsedname *pn)
{
	BYTE cr;
	int ret;
	/* Clear memory flag */
	if (OW_small_read(&cr, 1, 0x020E, pn ))
		return -EINVAL;
	cr = (cr & 0x3F) | 0x40;
	if (OW_w_mem(&cr, 1, 0x020E, pn))
		return -EINVAL;

	/* Clear memory command */
    cr = _1W_CLEAR_MEMORY;
	BUSLOCK(pn);
	ret = BUS_select(pn) || BUS_send_data(&cr, 1, pn);
	BUSUNLOCK(pn);

	UT_delay(1);				/* wait 500 usec */
	return ret;
}

/* translate 7 byte field to a Unix-style date (number) */
static int OW_2date(_DATE * d, const BYTE * data)
{
	struct tm t;

	/* Prefill entries */
	d[0] = time(NULL);
	if (gmtime_r(d, &t) == NULL)
		return -EINVAL;

	/* Get date from chip */
	t.tm_sec = (data[0] & 0x0F) + 10 * (data[0] >> 4);	/* BCD->dec */
	t.tm_min = (data[1] & 0x0F) + 10 * (data[1] >> 4);	/* BCD->dec */
	t.tm_hour = (data[2] & 0x0F) + ampm[data[2] >> 4];	/* BCD->dec */
	t.tm_mday = (data[4] & 0x0F) + 10 * (data[4] >> 4);	/* BCD->dec */
	t.tm_mon = (data[5] & 0x0F) + 10 * ((data[5] & 0x10) >> 4);	/* BCD->dec */
	t.tm_year = (data[6] & 0x0F) + 10 * (data[6] >> 4) + 100 * (2 - (data[5] >> 7));	/* BCD->dec */
//printf("_DATE_READ data=%2X, %2X, %2X, %2X, %2X, %2X, %2X\n",data[0],data[1],data[2],data[3],data[4],data[5],data[6]);
//printf("_DATE: sec=%d, min=%d, hour=%d, mday=%d, mon=%d, year=%d, wday=%d, isdst=%d\n",tm.tm_sec,tm.tm_min,tm.tm_hour,tm.tm_mday,tm.tm_mon,tm.tm_year,tm.tm_wday,tm.tm_isdst) ;

	/* Pass through time_t again to validate */
	if ((d[0] = mktime(&t)) == (time_t) - 1)
		return -EINVAL;
	return 0;
}

/* translate m byte field to a Unix-style date (number) */
static int OW_2mdate(_DATE * d, const BYTE * data)
{
	struct tm t;
	int year;
//printf("M_DATE data=%.2X %.2X %.2X %.2X %.2X\n",data[0],data[1],data[2],data[3],data[4]);
	/* Prefill entries */
	d[0] = time(NULL);
	if (gmtime_r(d, &t) == NULL)
		return -EINVAL;
	year = t.tm_year;
//printf("M_DATE year=%d\n",year);

	/* Get date from chip */
	t.tm_sec = 0;				/* BCD->dec */
	t.tm_min = (data[0] & 0x0F) + 10 * (data[0] >> 4);	/* BCD->dec */
	t.tm_hour = (data[1] & 0x0F) + ampm[data[1] >> 4];	/* BCD->dec */
	t.tm_mday = (data[2] & 0x0F) + 10 * (data[2] >> 4);	/* BCD->dec */
	t.tm_mon = (data[3] & 0x0F) + 10 * ((data[3] & 0x10) >> 4);	/* BCD->dec */
	t.tm_year = (data[4] & 0x0F) + 10 * (data[4] >> 4);	/* BCD->dec */
//printf("M_DATE tm=(%d-%d-%d %d:%d:%d)\n",t.tm_year,t.tm_mon,t.tm_mday,t.tm_hour,t.tm_min,t.tm_sec);
	/* Adjust the century -- should be within 50 years of current */
	while (t.tm_year + 50 < year)
		t.tm_year += 100;
//printf("M_DATE tm=(%d-%d-%d %d:%d:%d)\n",t.tm_year,t.tm_mon,t.tm_mday,t.tm_hour,t.tm_min,t.tm_sec);

	/* Pass through time_t again to validate */
	if ((d[0] = mktime(&t)) == (time_t) - 1)
		return -EINVAL;
	return 0;
}

/* set clock */
static void OW_date(const _DATE * d, BYTE * data)
{
	struct tm tm;
	int year;

	/* Convert time format */
	gmtime_r(d, &tm);

	data[0] = tm.tm_sec + 6 * (tm.tm_sec / 10);	/* dec->bcd */
	data[1] = tm.tm_min + 6 * (tm.tm_min / 10);	/* dec->bcd */
	data[2] = tm.tm_hour + 6 * (tm.tm_hour / 10);	/* dec->bcd */
	data[3] = tm.tm_wday;		/* dec->bcd */
	data[4] = tm.tm_mday + 6 * (tm.tm_mday / 10);	/* dec->bcd */
	data[5] = tm.tm_mon + 6 * (tm.tm_mon / 10);	/* dec->bcd */
	year = tm.tm_year % 100;
	data[6] = year + 6 * (year / 10);	/* dec->bcd */
	if (tm.tm_year > 99 && tm.tm_year < 200)
		data[5] |= 0x80;
//printf("_DATE_WRITE data=%2X, %2X, %2X, %2X, %2X, %2X, %2X\n",data[0],data[1],data[2],data[3],data[4],data[5],data[6]);
//printf("_DATE: sec=%d, min=%d, hour=%d, mday=%d, mon=%d, year=%d, wday=%d, isdst=%d\n",tm.tm_sec,tm.tm_min,tm.tm_hour,tm.tm_mday,tm.tm_mon,tm.tm_year,tm.tm_wday,tm.tm_isdst) ;
}

/* many things are disallowed if mission in progress */
/* returns 1 if MIP, 0 if not, <0 if error */
static int OW_MIP(struct parsedname *pn)
{
	BYTE data;
	int ret = OW_small_read(&data, 1, 0x0214, pn );	/* read status register */

	if (ret)
		return -EINVAL;
	return UT_getbit(&data, 5);
}

static int OW_FillMission(struct Mission *mission, struct parsedname *pn)
{
	BYTE data[16];

	/* Get date from chip */
	if (OW_small_read(data, 16, 0x020D, pn ))
		return -EINVAL;
	mission->interval = 60 * (int) data[0];
	mission->rollover = UT_getbit(&data[1], 3);
	mission->samples =
		(((((UINT) data[15]) << 8) | data[14]) << 8) | data[13];
	return OW_2mdate(&(mission->start), &data[8]);
}

static int OW_alarmlog(int *t, int *c, off_t offset, struct parsedname *pn)
{
	BYTE data[48];
	int i, j = 0;
	OWQ_make( owq_alog ) ;

	OWQ_create_temporary( owq_alog, (char *) data, sizeof(data), offset, pn ) ;

	if (OWQ_readwrite_paged(owq_alog, 0, 32, OW_r_mem_crc16_A5))
		return -EINVAL;

	for (i = 0; i < 12; ++i) {
		t[i] =
			(((((UINT) data[j + 2]) << 8) | data[j + 1]) << 8) | data[j];
		c[i] = data[j + 3];
		j += 4;
	}
	return 0;
}

static int OW_stopmission(struct parsedname *pn)
{
	BYTE data = 0x00;			/* dummy */
	return OW_small_read(&data, 1, 0x0210, pn );
}

static int OW_startmission(UINT freq, struct parsedname *pn)
{
	BYTE data;

	/* stop the mission */
	if (OW_stopmission(pn))
		return -EINVAL;			/* stop */

	if (freq == 0)
		return 0;				/* stay stopped */

	if (freq > 255)
		return -ERANGE;			/* Bad interval */

	if (OW_small_read(&data, 1, 0x020E, pn ))
		return -EINVAL;
	if ((data & 0x80)) {		/* clock stopped */
		_DATE d = time(NULL);
		/* start clock */
		if (OW_w_date(&d, pn))
			return -EINVAL;		/* set the clock to current time */
		UT_delay(1000);			/* wait for the clock to count a second */
	}

	/* clear memory */
	if (OW_clearmemory(pn))
		return -EINVAL;

	/* finally, set the sample interval (to start the mission) */
	data = freq & 0xFF;
	return OW_w_mem(&data, 1, 0x020D, pn);
}

/* set clock */
static int OW_w_date(_DATE * D, struct parsedname * pn )
{
    BYTE data[7];

    /* Busy if in mission */
    if (OW_MIP(pn))
        return -EBUSY;

    OW_date(D, data);
    if (OW_w_mem(data, 7, 0x0200, pn))
        return -EINVAL;
    return OW_w_run(1, pn);
}

/* stop/start clock running */
static int OW_w_run(int state, struct parsedname * pn )
{
    BYTE cr;

    if (OW_small_read(&cr, 1, 0x020E, pn))
        return -EINVAL;
    cr = state ? cr & 0x7F : cr | 0x80;
    if (OW_w_mem(&cr, 1, 0x020E, pn))
        return -EINVAL;
    return 0;
}

static int OW_small_read( BYTE * buffer, size_t size, off_t location, struct parsedname * pn )
{
    OWQ_make( owq_small ) ;
    OWQ_create_temporary( owq_small, (char *) buffer, size, location, pn ) ;
    return OW_r_mem_crc16_A5( owq_small, 0, 32 ) ;
}

#define HISTOGRAM_DATA_SIZE 2
static int OW_r_histogram_all(struct one_wire_query * owq)
{
    size_t pagesize = 32 ;
    OWQ_make( owq_histo ) ;
    int i;
    BYTE data[HISTOGRAM_DATA_ELEMENTS * HISTOGRAM_DATA_SIZE];

    OWQ_create_temporary( owq_histo, (char *) data, sizeof(data), 0x0800, PN(owq) ) ;

    if (OWQ_readwrite_paged(owq_histo, 0, pagesize, OW_r_mem_crc16_A5))
        return -EINVAL;
    for (i = 0; i < HISTOGRAM_DATA_ELEMENTS; ++i) {
        OWQ_array_U(owq,i) = (((UINT) data[i * HISTOGRAM_DATA_SIZE + 1]) << 8) | data[i * HISTOGRAM_DATA_SIZE];
    }
    return 0;
}

static int OW_r_histogram_single(struct one_wire_query * owq)
{
    BYTE data[HISTOGRAM_DATA_SIZE];
    if (OW_small_read(data, 2, (size_t) 0x800 + OWQ_pn(owq).extension * HISTOGRAM_DATA_SIZE, PN(owq) ))
        return -EINVAL;
    OWQ_U(owq) = (((UINT) data[1]) << 8) | data[0];
    return 0;
}

/* temperature log */
static int OW_r_logtemp_single(struct Version *v, struct Mission * mission, struct one_wire_query * owq)
{
    int pass = 0;
    int off = 0;
    BYTE data[1];
    struct parsedname * pn = PN(owq) ;
    
    if (mission->rollover) {
        pass = mission->samples / LOG_DATA_ELEMENTS; // samples/2048
        off = mission->samples % LOG_DATA_ELEMENTS;  // samples%2048
    }

    if (pass) {
        if (OW_small_read(data, 1, (size_t) 0x1000 + ((pn->extension + off) % LOG_DATA_ELEMENTS), pn ))
            return -EINVAL;
    } else {
        if (OW_small_read(data, 1, (size_t) 0x1000 + pn->extension, pn ))
            return -EINVAL;
    }
    OWQ_F(owq) = (_FLOAT) data[0] * v->resolution + v->histolow;
    
        return 0;
}

/* temperature log */
static int OW_r_logtemp_all(struct Version *v, struct Mission * mission, struct one_wire_query * owq)
{
    int pass = 0;
    int off = 0;
    size_t pagesize = 32 ;
    int i;
    BYTE data[LOG_DATA_ELEMENTS];
    OWQ_make( owq_log ) ;
    
    if (mission->rollover) {
        pass = mission->samples / LOG_DATA_ELEMENTS; // samples/2048
        off = mission->samples % LOG_DATA_ELEMENTS;  // samples%2048
    }

    OWQ_create_temporary( owq_log, (char *) data, sizeof(data), 0x1000, PN(owq) ) ;
    if (OWQ_readwrite_paged(owq_log, 0, pagesize, OW_r_mem_crc16_A5 ))
        return -EINVAL;
    if (pass) {
        for (i = 0; i < LOG_DATA_ELEMENTS; ++i)
            OWQ_array_F(owq,i) = (_FLOAT) data[(i + off) % LOG_DATA_ELEMENTS] * v->resolution + v->histolow;
    } else {
        for (i = 0; i < LOG_DATA_ELEMENTS; ++i)
            OWQ_array_F(owq,i) = (_FLOAT) data[i] * v->resolution + v->histolow;
    }
    
    return 0;
}

static int OW_r_logdate_single(struct Mission * mission, struct one_wire_query * owq)
{
    int extension = OWQ_pn(owq).extension ;
    int pass = 0;

    if (mission->rollover)
        pass = mission->samples / LOG_DATA_ELEMENTS; // samples/2048

    if (pass) {
        OWQ_D(owq) =
                mission->start + (mission->samples - LOG_DATA_ELEMENTS -
                extension) * mission->interval;
    } else {
        OWQ_D(owq) = mission->start + extension * mission->interval;
    }
    return 0;
}

static int OW_r_logdate_all(struct Mission * mission, struct one_wire_query * owq)
{
    int pass = 0;
    int i;

    if (mission->rollover)
        pass = mission->samples / LOG_DATA_ELEMENTS; // samples/2048

    if (pass) {
        for (i = 0; i < LOG_DATA_ELEMENTS; ++i)
            OWQ_array_D(owq,i) =
                    mission->start + (mission->samples - LOG_DATA_ELEMENTS -
                            i) * mission->interval;
    } else {
        for (i = 0; i < LOG_DATA_ELEMENTS; ++i)
            OWQ_array_D(owq,i) = mission->start + i * mission->interval;
    }
    return 0;
}

static int OW_r_logudate_single(struct Mission * mission, struct one_wire_query * owq)
{
    int extension = OWQ_pn(owq).extension ;
    int pass = 0;

    if (mission->rollover)
        pass = mission->samples / LOG_DATA_ELEMENTS; // samples/2048

    if (pass) {
        OWQ_U(owq) =
                mission->start + (mission->samples - LOG_DATA_ELEMENTS -
                extension) * mission->interval;
    } else {
        OWQ_U(owq) = mission->start + extension * mission->interval;
    }
    return 0;
}

static int OW_r_logudate_all(struct Mission * mission, struct one_wire_query * owq)
{
    int pass = 0;
    int i;

    if (mission->rollover)
        pass = mission->samples / LOG_DATA_ELEMENTS; // samples/2048

    if (pass) {
        for (i = 0; i < LOG_DATA_ELEMENTS; ++i)
            OWQ_array_U(owq,i) =
                    mission->start + (mission->samples - LOG_DATA_ELEMENTS -
                            i) * mission->interval;
    } else {
        for (i = 0; i < LOG_DATA_ELEMENTS; ++i)
            OWQ_array_U(owq,i) = mission->start + i * mission->interval;
    }
    return 0;
}


#undef HISTOGRAM_DATA_SIZE

#undef LOG_DATA_ELEMENTS
#undef HISTOGRAM_DATA_ELEMENTS

