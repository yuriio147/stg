/*
 * Generated by asn1c-0.9.21 (http://lionet.info/asn1c)
 * From ASN.1 module "SMUX"
 * 	found in "SMUX.asn1"
 * 	`asn1c -fskeletons-copy`
 */

#ifndef	_ClosePDU_H_
#define	_ClosePDU_H_


#include <asn_application.h>

/* Including external dependencies */
#include <INTEGER.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum ClosePDU {
	ClosePDU_goingDown	= 0,
	ClosePDU_unsupportedVersion	= 1,
	ClosePDU_packetFormat	= 2,
	ClosePDU_protocolError	= 3,
	ClosePDU_internalError	= 4,
	ClosePDU_authenticationFailure	= 5
} e_ClosePDU;

/* ClosePDU */
typedef INTEGER_t	 ClosePDU_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_ClosePDU;
asn_struct_free_f ClosePDU_free;
asn_struct_print_f ClosePDU_print;
asn_constr_check_f ClosePDU_constraint;
ber_type_decoder_f ClosePDU_decode_ber;
der_type_encoder_f ClosePDU_encode_der;
xer_type_decoder_f ClosePDU_decode_xer;
xer_type_encoder_f ClosePDU_encode_xer;

#ifdef __cplusplus
}
#endif

#endif	/* _ClosePDU_H_ */
