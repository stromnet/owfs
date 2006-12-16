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
#include "ow_2502.h"

/* ------- Prototypes ----------- */

/* DS2423 counter */
bREAD_FUNCTION(FS_r_page);
bWRITE_FUNCTION(FS_w_page);
bREAD_FUNCTION(FS_r_memory);
bWRITE_FUNCTION(FS_w_memory);
bREAD_FUNCTION(FS_r_param);

/* ------- Structures ----------- */

struct aggregate A2502 = { 4, ag_numbers, ag_separate, };
struct filetype DS2502[] = {
    F_STANDARD,
  {"memory", 128, NULL, ft_binary, fc_stable, {b: FS_r_memory}, {b: FS_w_memory}, {v:NULL},},
  {"pages", 0, NULL, ft_subdir, fc_volatile, {v: NULL}, {v: NULL}, {v:NULL},},
  {"pages/page", 32, &A2502, ft_binary, fc_stable, {b: FS_r_page}, {b: FS_w_page}, {v:NULL},},
};

DeviceEntry(09, DS2502);

struct filetype DS1982U[] = {
    F_STANDARD,
  {"mac_e", 6, NULL, ft_binary, fc_stable, {b: FS_r_param}, {v: NULL}, {i:4},},
  {"mac_fw", 8, NULL, ft_binary, fc_stable, {b: FS_r_param}, {v: NULL}, {i:4},},
  {"project", 4, NULL, ft_binary, fc_stable, {b: FS_r_param}, {v: NULL}, {i:0},},
  {"memory", 128, NULL, ft_binary, fc_stable, {b: FS_r_memory}, {b: FS_w_memory}, {v:NULL},},
  {"pages", 0, NULL, ft_subdir, fc_volatile, {v: NULL}, {v: NULL}, {v:NULL},},
  {"pages/page", 32, &A2502, ft_binary, fc_stable, {b: FS_r_page}, {b: FS_w_page}, {v:NULL},},
};

DeviceEntry(89, DS1982U);

/* ------- Functions ------------ */

/* DS2502 */
static int OW_w_mem(const BYTE * data, const size_t size,
		    const off_t offset, const struct parsedname *pn);
static int OW_r_mem(BYTE * data, const size_t size, const off_t offset,
		    const struct parsedname *pn);
static int OW_r_data(BYTE * data, const struct parsedname *pn);

/* 2502 memory */
static int FS_r_memory(BYTE * buf, const size_t size, const off_t offset,
		       const struct parsedname *pn)
{
//    if ( OW_r_mem( buf, size, (size_t) offset, pn) ) return -EINVAL ;
    if (OW_read_paged(buf, size, (size_t) offset, pn, 32, OW_r_mem))
	return -EINVAL;
    return size;
}

static int FS_r_page(BYTE * buf, const size_t size, const off_t offset,
		     const struct parsedname *pn)
{
    if (OW_r_mem(buf, size, (offset + (pn->extension << 5)), pn))
	return -EINVAL;
    return size;
}

static int FS_r_param(BYTE * buf, const size_t size, const off_t offset,
		      const struct parsedname *pn)
{
    BYTE data[32];
    if (OW_r_data(data, pn))
	return -EINVAL;
    return FS_output_ascii((ASCII *) buf, size, offset,
			   (ASCII *) & data[pn->ft->data.i],
			   (size_t) pn->ft->suglen);
}

static int FS_w_memory(const BYTE * buf, const size_t size,
		       const off_t offset, const struct parsedname *pn)
{
    if (OW_w_mem(buf, size, (size_t) offset, pn))
	return -EINVAL;
    return 0;
}

static int FS_w_page(const BYTE * buf, const size_t size,
		     const off_t offset, const struct parsedname *pn)
{
    if (OW_w_mem(buf, size, (size_t) (offset + (pn->extension << 5)), pn))
	return -EINVAL;
    return 0;
}

/* Byte-oriented write */
static int OW_w_mem(const BYTE * data, const size_t size,
		    const off_t offset, const struct parsedname *pn)
{
    BYTE p[5] = { 0x0F, offset & 0xFF, offset >> 8, data[0] };
    int ret;
    struct transaction_log tfirst[] = {
	TRXN_START,
	{p, NULL, 4, trxn_match,},
	{NULL, &p[4], 1, trxn_read,},
	{p, NULL, 5, trxn_crc8,},
	{NULL, NULL, 0, trxn_program,},
	{NULL, &p[3], 1, trxn_read,},
	TRXN_END,
    };

    if (size == 0)
	return 0;
    if (size == 1)
	return BUS_transaction(tfirst, pn) || (p[3] != data[0]);

    BUSLOCK(pn);
    if ((ret = BUS_transaction_nolock(tfirst, pn)
	 || (p[3] != data[0])) == 0) {
	size_t i;
	const BYTE *d = &data[1];
	UINT s = offset + 1;
	struct transaction_log trest[] = {
	    {d, NULL, 1, trxn_match,},
	    {NULL, p, 1, trxn_read,},
	    {p, (BYTE *) (&s), 1, trxn_crc8seeded,},
	    {NULL, NULL, 0, trxn_program,},
	    {NULL, p, 1, trxn_read,},
	    TRXN_END,
	};
	for (i = 1; i < size; ++i, ++s, ++d) {
	    if ((ret = BUS_transaction_nolock(trest, pn)
		 || (p[0] != d[0])))
		break;
	}
    }
    BUSUNLOCK(pn);

    return ret;
}

/* page-oriented read -- call will not span page boundaries */
static int OW_r_mem(BYTE * data, const size_t size, const off_t offset,
		    const struct parsedname *pn)
{
    BYTE p[4] = { 0xC3, offset & 0xFF, offset >> 8, };
    BYTE q[33];
    int rest = 33 - (offset & 0x1F);
    struct transaction_log t[] = {
	TRXN_START,
	{p, NULL, 3, trxn_match,},
	{NULL, &p[3], 1, trxn_read,},
	{p, NULL, 4, trxn_crc8,},
	{NULL, q, rest, trxn_read,},
	{q, NULL, rest, trxn_crc8,},
	TRXN_END,
    };
    if (BUS_transaction(t, pn))
	return 1;

    memcpy(data, q, size);
    return 0;
}

static int OW_r_data(BYTE * data, const struct parsedname *pn)
{
    BYTE p[32];

    if (OW_r_mem(p, 32, 0, pn) || CRC16(p, 3 + p[0]))
	return 1;
    memcpy(data, &p[1], p[0]);
    return 0;
}
