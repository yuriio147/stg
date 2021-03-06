/*
 * Generated by asn1c-0.9.21 (http://lionet.info/asn1c)
 * From ASN.1 module "SMUX"
 * 	found in "SMUX.asn1"
 * 	`asn1c -fskeletons-copy`
 */

#include <asn_internal.h>

#include "SMUX-PDUs.h"

static asn_TYPE_member_t asn_MBR_SMUX_PDUs_1[] = {
	{ ATF_NOFLAGS, 0, offsetof(struct SMUX_PDUs, choice.open),
		-1 /* Ambiguous tag (CHOICE?) */,
		0,
		&asn_DEF_OpenPDU,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"open"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct SMUX_PDUs, choice.close),
		(ASN_TAG_CLASS_APPLICATION | (1 << 2)),
		0,
		&asn_DEF_ClosePDU,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"close"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct SMUX_PDUs, choice.registerRequest),
		(ASN_TAG_CLASS_APPLICATION | (2 << 2)),
		0,
		&asn_DEF_RReqPDU,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"registerRequest"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct SMUX_PDUs, choice.registerResponse),
		(ASN_TAG_CLASS_APPLICATION | (3 << 2)),
		0,
		&asn_DEF_RRspPDU,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"registerResponse"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct SMUX_PDUs, choice.pdus),
		-1 /* Ambiguous tag (CHOICE?) */,
		0,
		&asn_DEF_PDUs,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"pdus"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct SMUX_PDUs, choice.commitOrRollback),
		(ASN_TAG_CLASS_APPLICATION | (4 << 2)),
		0,
		&asn_DEF_SOutPDU,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"commitOrRollback"
		},
};
static asn_TYPE_tag2member_t asn_MAP_SMUX_PDUs_tag2el_1[] = {
    { (ASN_TAG_CLASS_APPLICATION | (0 << 2)), 0, 0, 0 }, /* simple at 52 */
    { (ASN_TAG_CLASS_APPLICATION | (1 << 2)), 1, 0, 0 }, /* close at 27 */
    { (ASN_TAG_CLASS_APPLICATION | (2 << 2)), 2, 0, 0 }, /* registerRequest at 30 */
    { (ASN_TAG_CLASS_APPLICATION | (3 << 2)), 3, 0, 0 }, /* registerResponse at 33 */
    { (ASN_TAG_CLASS_APPLICATION | (4 << 2)), 5, 0, 0 }, /* commitOrRollback at 41 */
    { (ASN_TAG_CLASS_CONTEXT | (0 << 2)), 4, 0, 0 }, /* get-request at 34 */
    { (ASN_TAG_CLASS_CONTEXT | (1 << 2)), 4, 0, 0 }, /* get-next-request at 37 */
    { (ASN_TAG_CLASS_CONTEXT | (2 << 2)), 4, 0, 0 }, /* get-response at 40 */
    { (ASN_TAG_CLASS_CONTEXT | (3 << 2)), 4, 0, 0 }, /* set-request at 43 */
    { (ASN_TAG_CLASS_CONTEXT | (4 << 2)), 4, 0, 0 } /* trap at 47 */
};
static asn_CHOICE_specifics_t asn_SPC_SMUX_PDUs_specs_1 = {
	sizeof(struct SMUX_PDUs),
	offsetof(struct SMUX_PDUs, _asn_ctx),
	offsetof(struct SMUX_PDUs, present),
	sizeof(((struct SMUX_PDUs *)0)->present),
	asn_MAP_SMUX_PDUs_tag2el_1,
	10,	/* Count of tags in the map */
	0,
	-1	/* Extensions start */
};
asn_TYPE_descriptor_t asn_DEF_SMUX_PDUs = {
	"SMUX-PDUs",
	"SMUX-PDUs",
	CHOICE_free,
	CHOICE_print,
	CHOICE_constraint,
	CHOICE_decode_ber,
	CHOICE_encode_der,
	CHOICE_decode_xer,
	CHOICE_encode_xer,
	0, 0,	/* No PER support, use "-gen-PER" to enable */
	CHOICE_outmost_tag,
	0,	/* No effective tags (pointer) */
	0,	/* No effective tags (count) */
	0,	/* No tags (pointer) */
	0,	/* No tags (count) */
	0,	/* No PER visible constraints */
	asn_MBR_SMUX_PDUs_1,
	6,	/* Elements count */
	&asn_SPC_SMUX_PDUs_specs_1	/* Additional specs */
};

