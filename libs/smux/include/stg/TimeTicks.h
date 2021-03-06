/*
 * Generated by asn1c-0.9.21 (http://lionet.info/asn1c)
 * From ASN.1 module "RFC1155-SMI"
 * 	found in "RFC1155-SMI.asn1"
 * 	`asn1c -fskeletons-copy`
 */

#ifndef	_TimeTicks_H_
#define	_TimeTicks_H_


#include <asn_application.h>

/* Including external dependencies */
#include <INTEGER.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TimeTicks */
typedef INTEGER_t	 TimeTicks_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_TimeTicks;
asn_struct_free_f TimeTicks_free;
asn_struct_print_f TimeTicks_print;
asn_constr_check_f TimeTicks_constraint;
ber_type_decoder_f TimeTicks_decode_ber;
der_type_encoder_f TimeTicks_encode_der;
xer_type_decoder_f TimeTicks_decode_xer;
xer_type_encoder_f TimeTicks_encode_xer;

#ifdef __cplusplus
}
#endif

#endif	/* _TimeTicks_H_ */
