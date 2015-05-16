/*
$Id$
    OWFS -- One-Wire filesystem
    Written 2015 by Matthias Urlichs
	email: <matthias@urlichs.de>
	Released under the GPL
	See the header file: ow.h for full attribution
	1wire/iButton system from Dallas Semiconductor
*/

/* 
    This code is a generic controller for the "MoaT" family of devices,
    which is miplemented as slave code on AVR Atmega and similar devices.

    These devices are not generic, but can be interrogates as to the
    features they contain.
*/

#include <config.h>
#include "owfs_config.h"
#define MOAT_NAMES
#include "ow_moat.h"

/* ------- Prototypes ----------- */

#define VISIBLE_FUNCTION(name)  static enum e_visibility name( const struct parsedname * pn );

READ_FUNCTION(FS_r_info_raw);
READ_FUNCTION(FS_r_name);
READ_FUNCTION(FS_r_types);
READ_FUNCTION(FS_r_console);
READ_FUNCTION(FS_r_raw);
READ_FUNCTION(FS_r_port);
READ_FUNCTION(FS_r_port_all);
READ_FUNCTION(FS_r_pwm);
READ_FUNCTION(FS_r_adc);
READ_FUNCTION(FS_r_count);
READ_FUNCTION(FS_r_raw_zero);
READ_FUNCTION(FS_r_alarm);
READ_FUNCTION(FS_r_alarm_two);
READ_FUNCTION(FS_r_alarm_sources);
WRITE_FUNCTION(FS_w_raw);
WRITE_FUNCTION(FS_w_port);
WRITE_FUNCTION(FS_w_pwm);
WRITE_FUNCTION(FS_w_adc);
WRITE_FUNCTION(FS_w_console);
VISIBLE_FUNCTION(FS_show_config);
VISIBLE_FUNCTION(FS_show_one_config);
VISIBLE_FUNCTION(FS_show_entry);
VISIBLE_FUNCTION(FS_show_s_entry);
VISIBLE_FUNCTION(FS_show_alarm);

static GOOD_OR_BAD OW_r_std(BYTE *buf, size_t *buflen, BYTE type, BYTE stype, const struct parsedname *pn);
static GOOD_OR_BAD OW_w_std(BYTE *buf, size_t size,    BYTE type, BYTE stype, const struct parsedname *pn);
//WRITE_FUNCTION(FS_w_raw);
//READ_FUNCTION(FS_r_status);
//WRITE_FUNCTION(FS_w_status);
//READ_FUNCTION(FS_r_mem);
//WRITE_FUNCTION(FS_w_mem);

#define _1W_READ_MOAT          0xF2
#define _1W_WRITE_MOAT         0xF4

/* Internal properties */
Make_SlaveSpecificTag(FEATURES, fc_stable);  // feature map: array of M_MAX BYTEs
Make_SlaveSpecificTag(CONFIGS, fc_stable);  // config list: array of CFG_MAX BYTEs
Make_SlaveSpecificTag(ALARMS, fc_stable);  // config list: array of CFG_MAX BYTEs
static GOOD_OR_BAD OW_r_features(BYTE *buf, const struct parsedname *pn);
static GOOD_OR_BAD OW_r_configs(BYTE *buf, const struct parsedname *pn);
static GOOD_OR_BAD OW_r_alarms(BYTE *buf, const struct parsedname *pn, char ignore);

/* ------- Structures ----------- */

static struct aggregate infotypes = { CFG_MAX, ag_numbers, ag_separate, };
static struct aggregate maxports = { 32, ag_numbers, ag_separate, };

static struct filetype MOAT[] = {
	F_STANDARD,
	{"config", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"config/name", 255, NON_AGGREGATE, ft_vascii, fc_static, FS_r_name, NO_WRITE_FUNCTION, FS_show_one_config, {.u=CFG_NAME,}, },
	{"config/types", 255, NON_AGGREGATE, ft_vascii, fc_static, FS_r_types, NO_WRITE_FUNCTION, FS_show_one_config, {.u=CFG_NUMS,}, },
	{"console", 255, NON_AGGREGATE, ft_vascii, fc_uncached, FS_r_console, FS_w_console, VISIBLE, NO_FILETYPE_DATA, },

	{"port", PROPERTY_LENGTH_YESNO, &maxports, ft_yesno, fc_volatile, FS_r_port, FS_w_port, FS_show_entry, {.u=M_PORT,}, },
	{"ports", 255, NON_AGGREGATE, ft_vascii, fc_volatile, FS_r_port_all, NO_WRITE_FUNCTION, FS_show_s_entry, {.u=M_PORT,}, },
	{"pwm", 20, &maxports, ft_binary, fc_stable, FS_r_pwm, FS_w_pwm, FS_show_entry, {.u=M_PWM,}, },
	{"adc", 20, &maxports, ft_binary, fc_stable, FS_r_adc, FS_w_adc, FS_show_entry, {.u=M_ADC,}, },
	{"count", PROPERTY_LENGTH_UNSIGNED, &maxports, ft_unsigned, fc_volatile, FS_r_count, NO_WRITE_FUNCTION, FS_show_entry, {.u=M_COUNT,}, },
	{"alarm", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"alarm/sources", 255, NON_AGGREGATE, ft_vascii, fc_volatile, FS_r_alarm_sources, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"alarm/config", 255, NON_AGGREGATE, ft_vascii, fc_volatile, FS_r_alarm, NO_WRITE_FUNCTION, FS_show_alarm, {.u=M_CONFIG,}, },
	{"alarm/alarm", 255, NON_AGGREGATE, ft_vascii, fc_volatile, FS_r_alarm, NO_WRITE_FUNCTION, FS_show_alarm, {.u=M_ALERT,}, },
	{"alarm/stats", 255, NON_AGGREGATE, ft_vascii, fc_volatile, FS_r_alarm, NO_WRITE_FUNCTION, FS_show_alarm, {.u=M_STATS,}, },
	{"alarm/console", 255, NON_AGGREGATE, ft_vascii, fc_volatile, FS_r_alarm, NO_WRITE_FUNCTION, FS_show_alarm, {.u=M_CONSOLE,}, },
	{"alarm/port", 255, NON_AGGREGATE, ft_vascii, fc_volatile, FS_r_alarm, NO_WRITE_FUNCTION, FS_show_alarm, {.u=M_PORT,}, },
	{"alarm/pwm", 255, NON_AGGREGATE, ft_vascii, fc_volatile, FS_r_alarm, NO_WRITE_FUNCTION, FS_show_alarm, {.u=M_PWM,}, },
	{"alarm/count", 255, NON_AGGREGATE, ft_vascii, fc_volatile, FS_r_alarm, NO_WRITE_FUNCTION, FS_show_alarm, {.u=M_COUNT,}, },
	{"alarm/adc", 255, NON_AGGREGATE, ft_vascii, fc_volatile, FS_r_alarm_two, NO_WRITE_FUNCTION, FS_show_alarm, {.u=M_ADC,}, },
	{"alarm/temp", 255, NON_AGGREGATE, ft_vascii, fc_volatile, FS_r_alarm, NO_WRITE_FUNCTION, FS_show_alarm, {.u=M_TEMP,}, },
	{"alarm/humid", 255, NON_AGGREGATE, ft_vascii, fc_volatile, FS_r_alarm, NO_WRITE_FUNCTION, FS_show_alarm, {.u=M_HUMID,}, },
	{"alarm/pid", 255, NON_AGGREGATE, ft_vascii, fc_volatile, FS_r_alarm, NO_WRITE_FUNCTION, FS_show_alarm, {.u=M_PID,}, },
	{"alarm/smoke", 255, NON_AGGREGATE, ft_vascii, fc_volatile, FS_r_alarm, NO_WRITE_FUNCTION, FS_show_alarm, {.u=M_SMOKE,}, },

	{"raw", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"raw/config", 255, &infotypes, ft_binary, fc_static, FS_r_raw, NO_WRITE_FUNCTION, FS_show_config, {.u=M_CONFIG,}, },
	{"raw/alarm", 255, &maxports, ft_binary, fc_uncached, FS_r_raw, NO_WRITE_FUNCTION, FS_show_entry, {.u=M_ALERT,}, },
	{"raw/alarms", 255, NON_AGGREGATE, ft_binary, fc_uncached, FS_r_raw_zero, NO_WRITE_FUNCTION, FS_show_s_entry, {.u=M_ALERT,}, },
	{"raw/stat", 255, &maxports, ft_binary, fc_uncached, FS_r_raw, FS_w_raw, FS_show_entry, {.u=M_STATS,}, },
	{"raw/stats", 255, NON_AGGREGATE, ft_binary, fc_uncached, FS_r_raw_zero, NO_WRITE_FUNCTION, FS_show_s_entry, {.u=M_STATS,}, },
	{"raw/port", 255, &maxports, ft_binary, fc_uncached, FS_r_raw, FS_w_raw, FS_show_entry, {.u=M_PORT,}, },
	{"raw/ports", 255, NON_AGGREGATE, ft_binary, fc_uncached, FS_r_raw_zero, NO_WRITE_FUNCTION, FS_show_s_entry, {.u=M_PORT,}, },
	{"raw/smoke", 255, &maxports, ft_binary, fc_uncached, FS_r_raw, FS_w_raw, FS_show_entry, {.u=M_SMOKE,}, },
	{"raw/temp", 255, &maxports, ft_binary, fc_uncached, FS_r_raw, FS_w_raw, FS_show_entry, {.u=M_TEMP,}, },
	{"raw/temps", 255, NON_AGGREGATE, ft_binary, fc_uncached, FS_r_raw_zero, NO_WRITE_FUNCTION, FS_show_s_entry, {.u=M_TEMP,}, },
	{"raw/humid", 255, &maxports, ft_binary, fc_uncached, FS_r_raw, FS_w_raw, FS_show_entry, {.u=M_HUMID,}, },
	{"raw/humids", 255, NON_AGGREGATE, ft_binary, fc_uncached, FS_r_raw_zero, NO_WRITE_FUNCTION, FS_show_s_entry, {.u=M_HUMID,}, },
	{"raw/adc", 255, &maxports, ft_binary, fc_uncached, FS_r_raw, FS_w_raw, FS_show_entry, {.u=M_ADC,}, },
	{"raw/adcs", 255, NON_AGGREGATE, ft_binary, fc_uncached, FS_r_raw_zero, NO_WRITE_FUNCTION, FS_show_s_entry, {.u=M_ADC,}, },
	{"raw/pid", 255, &maxports, ft_binary, fc_uncached, FS_r_raw, FS_w_raw, FS_show_entry, {.u=M_PID,}, },
	{"raw/pwm", 255, &maxports, ft_binary, fc_uncached, FS_r_raw, FS_w_raw, FS_show_entry, {.u=M_PWM,}, },
	{"raw/pwms", 255, NON_AGGREGATE, ft_binary, fc_uncached, FS_r_raw_zero, NO_WRITE_FUNCTION, FS_show_s_entry, {.u=M_PWM,}, },
	{"raw/count", 255, &maxports, ft_binary, fc_uncached, FS_r_raw, FS_w_raw, FS_show_entry, {.u=M_COUNT,}, },
	{"raw/counts", 255, NON_AGGREGATE, ft_binary, fc_uncached, FS_r_raw_zero, NO_WRITE_FUNCTION, FS_show_s_entry, {.u=M_COUNT,}, },
	{"raw/smoke", 255, &maxports, ft_binary, fc_uncached, FS_r_raw, FS_w_raw, FS_show_entry, {.u=M_SMOKE,}, },
	{"raw/smokes", 255, NON_AGGREGATE, ft_binary, fc_uncached, FS_r_raw_zero, NO_WRITE_FUNCTION, FS_show_s_entry, {.u=M_SMOKE,}, },
	{"raw/pid", 255, &maxports, ft_binary, fc_uncached, FS_r_raw, FS_w_raw, FS_show_entry, {.u=M_PID,}, },
	{"raw/pids", 255, NON_AGGREGATE, ft_binary, fc_uncached, FS_r_raw_zero, NO_WRITE_FUNCTION, FS_show_s_entry, {.u=M_PID,}, },

};

DeviceEntryExtended(F0, MOAT, DEV_alarm, NO_GENERIC_READ, NO_GENERIC_WRITE);

/* ------- Functions ------------ */

static ZERO_OR_ERROR FS_r_info_raw(struct one_wire_query *owq)
{
    BYTE buf[256];
    size_t len = OWQ_size(owq);
	if(len>sizeof(buf)) len=sizeof(buf);

	RETURN_ERROR_IF_BAD( OW_r_std(buf,&len, M_CONFIG, OWQ_pn(owq).extension, PN(owq)));

    return OWQ_format_output_offset_and_size((const char *)buf, len, owq);
}

static enum e_visibility FS_show_entry( const struct parsedname * pn )
{
	BYTE buf[M_MAX];
	if (pn->extension == EXTENSION_ALL)
		return visible_never;
	if (!pn->extension)
		return visible_never;
	if (pn->selected_filetype->data.u >= M_MAX)
		return visible_never;

	if(BAD(OW_r_features(buf, pn)))
		return visible_not_now;
	if (pn->extension > buf[pn->selected_filetype->data.u])
		return visible_not_now;

	return visible_now;
}

static enum e_visibility FS_show_alarm( const struct parsedname * pn )
{
	BYTE buf[(M_MAX+7)>>3];
	if (pn->selected_filetype->data.u >= M_MAX)
		return visible_never;

	if (BAD(OW_r_alarms(buf, pn, 0)))
		return visible_not_now;
	if (buf[pn->selected_filetype->data.u >> 3] & (1<<(pn->selected_filetype->data.u & 7)))
		return visible_now;

	return visible_not_now;
}

static enum e_visibility FS_show_config( const struct parsedname * pn )
{
	BYTE buf[(CFG_MAX+7)/8];
	if (pn->extension == EXTENSION_ALL)
		return visible_never;
	if (pn->extension >= CFG_MAX)
		return visible_never;

	if(BAD(OW_r_configs(buf, pn)))
		return visible_not_now;
	if (buf[pn->extension >> 3] & (1<<(pn->extension & 7)))
		return visible_now;

	return visible_not_now;
}

static enum e_visibility FS_show_one_config( const struct parsedname * pn )
{
	BYTE buf[(CFG_MAX+7)/8];
	if (pn->selected_filetype->data.u >= CFG_MAX)
		return visible_never;

	if(BAD(OW_r_configs(buf, pn)))
		return visible_not_now;
	if (buf[pn->selected_filetype->data.u >> 3] & (1<<(pn->selected_filetype->data.u & 7)))
		return visible_now;

	return visible_not_now;
}

static enum e_visibility FS_show_s_entry( const struct parsedname * pn )
{
	BYTE buf[M_MAX];
	if (pn->selected_filetype->data.u >= M_MAX)
		return visible_never;

	if(BAD(OW_r_features(buf, pn)))
		return visible_not_now;
	if (!buf[pn->selected_filetype->data.u])
		return visible_not_now;

	return visible_now;
}

static ZERO_OR_ERROR FS_r_raw(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
    BYTE buf[256];
    size_t len = OWQ_size(owq);
	if(len>sizeof(buf)) len=sizeof(buf);

	if (pn->extension == EXTENSION_ALL)
		RETURN_ERROR_IF_BAD( OW_r_std(buf,&len, pn->selected_filetype->data.u, 0, pn));
	else if(!pn->extension)
		return -EINVAL;
	else
		RETURN_ERROR_IF_BAD( OW_r_std(buf,&len, pn->selected_filetype->data.u, OWQ_pn(owq).extension, pn));

    return OWQ_format_output_offset_and_size((const char *)buf, len, owq);
}

static ZERO_OR_ERROR FS_r_port_all(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	BYTE b[16],*bp=b;
	BYTE buf[256];
	size_t len = 0;
	size_t lb = sizeof(b);
	int i;
	int max_port;

	if(BAD(OW_r_features(buf, pn)))
		return -EINVAL;
	max_port = buf[M_PORT];

	RETURN_ERROR_IF_BAD( OW_r_std(b,&lb, M_PORT, 0, pn));
	while(max_port && lb--) {
		BYTE m = 1;
		for(i=0;max_port && i<8;i++) {
			buf[len++] = (*bp & m) ? '1' : '0';
			buf[len++] = ',';
			max_port--;
			m <<= 1;
		}
		bp++;
	}

	return OWQ_format_output_offset_and_size((const char *)buf, len-1, owq);
}

static ZERO_OR_ERROR FS_r_port(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	BYTE buf[1];
	size_t len = sizeof(buf);

	if (!pn->extension) 
		return -EINVAL;
	
	RETURN_ERROR_IF_BAD( OW_r_std(buf,&len, M_PORT, pn->extension, pn));
	if (len != 1)
		return -EINVAL;

	OWQ_Y(owq) = !!(buf[0]&0x80);
    return 0;
}

static ZERO_OR_ERROR FS_r_raw_zero(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
    BYTE buf[256];
    size_t len = OWQ_size(owq);
	if(len>sizeof(buf)) len=sizeof(buf);

	RETURN_ERROR_IF_BAD( OW_r_std(buf,&len, pn->selected_filetype->data.u, 0, pn));

    return OWQ_format_output_offset_and_size((const char *)buf, len, owq);
}

static ZERO_OR_ERROR FS_r_name(struct one_wire_query *owq)
{
    BYTE buf[256];
    size_t len = OWQ_size(owq);

	RETURN_ERROR_IF_BAD( OW_r_std(buf,&len, M_CONFIG, CFG_NAME, PN(owq)));

    return OWQ_format_output_offset_and_size((const char *)buf, len, owq);
}

static ZERO_OR_ERROR FS_r_pwm(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
    BYTE buf[7];
    size_t len = sizeof(buf);
    char obuf[20];
    size_t olen;

	RETURN_ERROR_IF_BAD( OW_r_std(buf,&len, pn->selected_filetype->data.u, pn->extension, pn));
	if (len != sizeof(buf))
		return -EINVAL;

	olen = snprintf(obuf,sizeof(obuf), "%d,%d", buf[3]<<8|buf[4], buf[5]<<8|buf[6]);
    return OWQ_format_output_offset_and_size(obuf, olen, owq);
}

static ZERO_OR_ERROR FS_r_adc(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
    BYTE buf[7];
    size_t len = sizeof(buf);
    char obuf[30];
    size_t olen;

	RETURN_ERROR_IF_BAD( OW_r_std(buf,&len, pn->selected_filetype->data.u, pn->extension, pn));
	if (len != sizeof(buf))
		return -EINVAL;

	olen = snprintf(obuf,sizeof(obuf), "%d, %d,%d", buf[1]<<8|buf[2], buf[3]<<8|buf[4], buf[5]<<8|buf[6]);
    return OWQ_format_output_offset_and_size(obuf, olen, owq);
}

static ZERO_OR_ERROR FS_r_count(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
    BYTE buf[4], *bp = buf;
    size_t len = sizeof(buf);
	int res = 0;

	RETURN_ERROR_IF_BAD( OW_r_std(buf,&len, pn->selected_filetype->data.u, pn->extension, pn));

	while(len--)
		res = res<<8 | *bp++;

	OWQ_U(owq) = res;
    return 0;
}

static ZERO_OR_ERROR FS_r_alarm(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
    BYTE buf[4];
	char obuf[255],olen=0;
    size_t len = sizeof(buf);
	uint8_t m=1,i;

	RETURN_ERROR_IF_BAD( OW_r_std(buf,&len, M_ALERT, pn->selected_filetype->data.u, pn));

	for(i=0;i<len*8;i++) {
		if (buf[i>>3] & m)
			olen += snprintf(obuf+olen,sizeof(obuf)-olen-1,"%d,",i+1);
		if (m == 0x80)
			m = 1;
		else
			m <<= 1;
	}
    return OWQ_format_output_offset_and_size(obuf, olen ? olen-1 : 0, owq);
}

static ZERO_OR_ERROR FS_r_alarm_two(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
    BYTE buf[8];
	char obuf[255],olen=0;
    size_t len = sizeof(buf);
	uint8_t m=1,i,j=1;

	RETURN_ERROR_IF_BAD( OW_r_std(buf,&len, M_ALERT, pn->selected_filetype->data.u, pn));

	for(i=0;i<len;) {
		if (buf[i] & m)
			olen += snprintf(obuf+olen,sizeof(obuf)-olen-1,"-%d,",j);
		m <<= 1;
		if (buf[i] & m)
			olen += snprintf(obuf+olen,sizeof(obuf)-olen-1,"+%d,",j);
		if (m == 0x80) {
			m = 1;
			i++;
		} else
			m <<= 1;
		j += 1;
	}
    return OWQ_format_output_offset_and_size(obuf, olen ? olen-1 : 0, owq);
}

static ZERO_OR_ERROR FS_r_types(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
    unsigned char buf[256];
    size_t len = 0;
	BYTE b[M_MAX];
	int i;

	RETURN_ERROR_IF_BAD(OW_r_features(b, pn));

	for (i=0;i<M_MAX;i++) {
		if (b[i])
			len += snprintf((char *)buf+len,sizeof(buf)-len-1, "%s=%d\n", m_names[i],b[i]);
	}

    return OWQ_format_output_offset_and_size((const char *)buf, len, owq);
}

static ZERO_OR_ERROR FS_r_alarm_sources(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
    unsigned char buf[256];
    size_t len = 0;
	BYTE b[(M_MAX+7)>>3];
	int i;

	// Always get a new bitmap when reading this
	//Cache_Del_Internal(SlaveSpecificTag(ALARMS), pn);
	RETURN_ERROR_IF_BAD(OW_r_alarms(b, pn, 1));

	for (i=0;i<M_MAX;i++) {
		if (b[i>>3] & (1<<(i&7)))
			len += snprintf((char *)buf+len,sizeof(buf)-len-1, "%s,", m_names[i]);
	}

    return OWQ_format_output_offset_and_size((const char *)buf, len ? len-1 : 0, owq);
}

static ZERO_OR_ERROR FS_r_console(struct one_wire_query *owq)
{
    BYTE buf[256];
    size_t len = OWQ_size(owq);

	RETURN_ERROR_IF_BAD( OW_r_std(buf,&len, M_CONSOLE, 1, PN(owq)));

    return OWQ_format_output_offset_and_size((const char *)buf, len, owq);
}

static ZERO_OR_ERROR FS_w_console(struct one_wire_query *owq)
{
	if (OWQ_offset(owq) != 0)
		return -EINVAL; /* ignore? */
	return GB_to_Z_OR_E( OW_w_std( (BYTE *) OWQ_buffer(owq), OWQ_size(owq), M_CONSOLE,1, PN(owq)) ) ;
}

static ZERO_OR_ERROR FS_w_raw(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	BYTE *buf = (BYTE *) OWQ_buffer(owq);
    size_t len = OWQ_size(owq);
	if (OWQ_offset(owq) != 0)
		return -EINVAL; /* ignore? */

	if (pn->extension == EXTENSION_ALL || !pn->extension)
		return -EINVAL;

	return GB_to_Z_OR_E( OW_w_std( buf,len, pn->selected_filetype->data.u, pn->extension, pn) ) ;
}

static ZERO_OR_ERROR FS_w_port(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	BYTE buf[] = { OWQ_Y(owq) };
    size_t len = sizeof(buf);

	return GB_to_Z_OR_E( OW_w_std( buf,len, pn->selected_filetype->data.u, pn->extension, pn) ) ;
}

static ZERO_OR_ERROR FS_w_pwm(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	BYTE buf[4];
    size_t len = sizeof(buf);
	unsigned long t_on,t_off;

	char *ib = alloca(OWQ_size(owq)+1);
	char *ib2;

	strncpy(ib,OWQ_buffer(owq),OWQ_size(owq));
	ib[OWQ_size(owq)]=0;

	t_on = strtoul(ib,&ib2,0);
	if (ib2 == ib)
		return -EINVAL;
	if (*ib2++ != ',')
		return -EINVAL;
	t_off = strtoul(ib2,&ib,0);
	if (ib2 == ib)
		return -EINVAL;
	if (*ib == '\n')
		ib++;
	if (*ib)
		return -EINVAL;
	if (t_on >= 32768 || t_off >= 32768)
		return -EINVAL;
	
	buf[0] = t_on>>8;
	buf[1] = t_on;
	buf[2] = t_off>>8;
	buf[3] = t_off;
	return GB_to_Z_OR_E( OW_w_std( buf,len, pn->selected_filetype->data.u, pn->extension, pn) ) ;
}

static ZERO_OR_ERROR FS_w_adc(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	BYTE buf[4];
    size_t len = sizeof(buf);
	unsigned long lower,upper;

	char *ib = alloca(OWQ_size(owq)+1);
	char *ib2;

	strncpy(ib,OWQ_buffer(owq),OWQ_size(owq));
	ib[OWQ_size(owq)]=0;

	lower = strtoul(ib,&ib2,0);
	if (ib2 == ib)
		return -EINVAL;
	if (*ib2++ != ',')
		return -EINVAL;
	upper = strtoul(ib2,&ib,0);
	if (ib2 == ib)
		return -EINVAL;
	if (*ib == '\n')
		ib++;
	if (*ib)
		return -EINVAL;
	if (lower > 65535 || upper > 65535)
		return -EINVAL;
	
	buf[0] = lower>>8;
	buf[1] = lower;
	buf[2] = upper>>8;
	buf[3] = upper;
	return GB_to_Z_OR_E( OW_w_std( buf,len, pn->selected_filetype->data.u, pn->extension, pn) ) ;
}

static GOOD_OR_BAD OW_r_std(BYTE *buf, size_t *buflen, BYTE type, BYTE stype, const struct parsedname *pn)
{
	BYTE p[3] = { _1W_READ_MOAT, type,stype };

    size_t maxlen = *buflen;
    BYTE len;
	BYTE ovbuf[254];
	GOOD_OR_BAD ret = gbGOOD;

	struct transaction_log tfirst[] = {
		TRXN_START,
		TRXN_WRITE3(p),
		TRXN_READ1(&len),
		TRXN_END,
	};

	LEVEL_DEBUG( "read: read len for %d %d",type,stype) ;
	/* 0xFF means the device was too slow */
	if ( BAD(BUS_transaction(tfirst, pn)) || len == 0xFF) {
		goto out_bad;
	}
	LEVEL_DEBUG( "read: got len %d",len) ;
	{
		UINT crc;
		BYTE crcbuf[2];
		struct transaction_log recv_buf[] = {
			TRXN_READ(buf,len),
			TRXN_READ2(crcbuf),
			TRXN_END,
		};
		struct transaction_log recv_buf_sup[] = {
			TRXN_READ(buf,maxlen),
			TRXN_READ(ovbuf,len-maxlen),
			TRXN_READ2(crcbuf),
			TRXN_END,
		};
		struct transaction_log recv_crc[] = {
			TRXN_READ2(crcbuf),
			TRXN_END,
		};
		struct transaction_log recv_crc_sup[] = {
			TRXN_READ(ovbuf,len),
			TRXN_READ2(crcbuf),
			TRXN_END,
		};
		struct transaction_log xmit_crc[] = {
			TRXN_WRITE2(crcbuf),
			TRXN_END,
		};

		// if len=0, i.e. the remote sends no data at all, then just read the CRC.
		// Else if len<=max, all OK, read all into buffer.
		// Else if we want no data, read all into 
		if ( BAD(BUS_transaction(len ? (len>maxlen) ? maxlen ? recv_buf_sup : recv_crc_sup : recv_buf : recv_crc, pn)) ) {
			goto out_bad;
		}

		crc = CRC16compute(p,3,0);
		crc = CRC16compute(&len,1,crc);
		if (!len) {
			// no data
		} else if (len<=maxlen) {
			crc = CRC16compute(buf,len,crc);
		} else if (maxlen) {
			crc = CRC16compute(buf,maxlen,crc);
			crc = CRC16compute(ovbuf,len-maxlen,crc);
		} else {
			crc = CRC16compute(ovbuf,len,crc);
		}
		LEVEL_DEBUG( "read CRC: GOOD, got %02x%02x",crcbuf[0],crcbuf[1]) ;
		if ( CRC16seeded (crcbuf,2,crc) ) {
			LEVEL_DEBUG("CRC error");
			goto out_bad;
		}
		crcbuf[0] = ~crcbuf[0];
		crcbuf[1] = ~crcbuf[1];
		if ( BAD(BUS_transaction(xmit_crc, pn)) ) {
			goto out_bad;
		}
		*buflen = len;
	}

	LEVEL_DEBUG( "read: GOOD, got %d",*buflen) ;
	return ret;
out_bad:
	return gbBAD;
}

static GOOD_OR_BAD OW_w_std(BYTE *buf, size_t size, BYTE type, BYTE stype, const struct parsedname *pn)
{
	BYTE p[4] = { _1W_WRITE_MOAT, type,stype, size};
	BYTE crcbuf[2];
	UINT crc;

	struct transaction_log tfirst[] = {
		TRXN_START,
		TRXN_WRITE(p,4),
		TRXN_WRITE(buf,size),
		TRXN_READ2(crcbuf),
		TRXN_END,
	};
	struct transaction_log xmit_crc[] = {
		TRXN_WRITE2(crcbuf),
		TRXN_END,
	};

	if (size == 0) {
		return gbGOOD;
	}
	if (size > 255) {
		return gbBAD;
	}

	LEVEL_DEBUG( "write: %d for %d %d",size,type,stype) ;
	
	if ( BAD(BUS_transaction(tfirst, pn))) {
		goto out_bad;
	}

	crc = CRC16compute(p,4,0);
	crc = CRC16compute(buf,size,crc);
	if ( CRC16seeded (crcbuf,2,crc) ) {
		LEVEL_DEBUG("CRC error");
		goto out_bad;
	}
	LEVEL_DEBUG( "read CRC: GOOD, got %02x%02x",crcbuf[0],crcbuf[1]) ;
	crcbuf[0] = ~crcbuf[0];
	crcbuf[1] = ~crcbuf[1];
	if ( BAD(BUS_transaction(xmit_crc, pn)) ) {
		goto out_bad;
	}
	return gbGOOD;
out_bad:
	return gbBAD;
}

/**
 * This returns a cached copy of the "how many M_xxx does this device
 * have" array.
 */
static GOOD_OR_BAD OW_r_features(BYTE *buf, const struct parsedname *pn)
{
	if ( BAD( Cache_Get_SlaveSpecific(buf, M_MAX, SlaveSpecificTag(FEATURES), pn))) {
		size_t buflen = M_MAX;

		/* Wire format: byte array. */
		RETURN_BAD_IF_BAD( OW_r_std(buf, &buflen, M_CONFIG, CFG_NUMS, pn) );
		if (buflen < M_MAX) // older/smaller device?
			memset(buf+buflen,0,M_MAX-buflen);

		Cache_Add_SlaveSpecific(buf, M_MAX, SlaveSpecificTag(FEATURES), pn);
	}
	return gbGOOD;
}

/**
 * This returns a cached copy of the "which CFG_* entries does this device
 * have" bitmap.
 */
static GOOD_OR_BAD OW_r_configs(BYTE *buf, const struct parsedname *pn)
{
	if ( BAD( Cache_Get_SlaveSpecific(buf, (CFG_MAX+7)/8, SlaveSpecificTag(CONFIGS), pn))) {
		size_t buflen = (CFG_MAX+7)/8;
		memset(buf,0,buflen); // in case the remote's list is shorter

		RETURN_BAD_IF_BAD( OW_r_std(buf, &buflen, M_CONFIG, CFG_LIST, pn) );
		Cache_Add_SlaveSpecific(buf, (CFG_MAX+7)/8, SlaveSpecificTag(CONFIGS), pn);
	}
	return gbGOOD;
}

/**
 * This returns a cached copy of the "which M_* entries are triggering the
 * alarm" bitmap.
 */
static GOOD_OR_BAD OW_r_alarms(BYTE *buf, const struct parsedname *pn, char ignore)
{
	if ( ignore || BAD( Cache_Get_SlaveSpecific(buf, (M_MAX+7)/8, SlaveSpecificTag(ALARMS), pn))) {
		size_t buflen = (M_MAX+7)/8;
		memset(buf,0,buflen); // in case the remote's list is shorter

		RETURN_BAD_IF_BAD( OW_r_std(buf, &buflen, M_ALERT, 0, pn) );
		Cache_Add_SlaveSpecific(buf, (M_MAX+7)/8, SlaveSpecificTag(ALARMS), pn);
	}
	return gbGOOD;
}

#if 0
static ZERO_OR_ERROR FS_w_mem(struct one_wire_query *owq)
{
	return COMMON_write_eprom_mem_owq(owq) ;
}

static ZERO_OR_ERROR FS_w_status(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E(OW_w_status(OWQ_explode(owq))) ;
}

static ZERO_OR_ERROR FS_w_page(struct one_wire_query *owq)
{
	size_t pagesize = 32;
	return COMMON_offset_process( FS_w_mem, owq, OWQ_pn(owq).extension * pagesize) ;
}

static GOOD_OR_BAD OW_w_status(BYTE * data, size_t size, off_t offset, struct parsedname *pn)
{
	BYTE p[6] = { _1W_WRITE_STATUS, LOW_HIGH_ADDRESS(offset), data[0] };
	GOOD_OR_BAD ret = gbGOOD;
	struct transaction_log tfirst[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 4, 0),
		TRXN_PROGRAM,
		TRXN_READ1(p),
		TRXN_END,
	};

	if (size == 0) {
		return gbGOOD;
	}
	if (size == 1) {
		return BUS_transaction(tfirst, pn) || (p[0] & (~data[0]));
	}
	BUSLOCK(pn);
	if ( BAD(BUS_transaction(tfirst, pn)) || (p[0] & ~data[0])) {
		ret = gbBAD;
	} else {
		size_t i;
		const BYTE *d = &data[1];
		UINT s = offset + 1;
		struct transaction_log trest[] = {
			//TRXN_WR_CRC16_SEEDED( p, &s, 1, 0 ) ,
			TRXN_WR_CRC16_SEEDED(p, p, 1, 0),
			TRXN_PROGRAM,
			TRXN_READ1(p),
			TRXN_END,
		};
		for (i = 0; i < size; ++i, ++d, ++s) {
			if ( BAD(BUS_transaction(trest, pn)) || (p[0] & ~d[0])) {
				ret = gbBAD;
				break;
			}
		}
	}
	BUSUNLOCK(pn);
	return ret;
}
#endif

