/*
 * Generated by asn1c-0.9.21 (http://lionet.info/asn1c)
 * From ASN.1 module "RFC1155-SMI"
 * 	found in "RFC1155-SMI.asn1"
 * 	`asn1c -fskeletons-copy`
 */

#ifndef	_Gauge_H_
#define	_Gauge_H_


#include <asn_application.h>

/* Including external dependencies */
#include <INTEGER.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Gauge */
typedef INTEGER_t	 Gauge_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_Gauge;
asn_struct_free_f Gauge_free;
asn_struct_print_f Gauge_print;
asn_constr_check_f Gauge_constraint;
ber_type_decoder_f Gauge_decode_ber;
der_type_encoder_f Gauge_encode_der;
xer_type_decoder_f Gauge_decode_xer;
xer_type_encoder_f Gauge_encode_xer;

#ifdef __cplusplus
}
#endif

#endif	/* _Gauge_H_ */