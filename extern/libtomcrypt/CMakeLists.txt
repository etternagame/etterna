
list(APPEND TOMCRYPT_CIPHERS
	"src/ciphers/anubis.c"
	"src/ciphers/blowfish.c"
	"src/ciphers/camellia.c"
	"src/ciphers/cast5.c"
	"src/ciphers/des.c"
	"src/ciphers/kasumi.c"
	"src/ciphers/khazad.c"
	"src/ciphers/kseed.c"
	"src/ciphers/multi2.c"
	"src/ciphers/noekeon.c"
	"src/ciphers/rc2.c"
	"src/ciphers/rc5.c"
	"src/ciphers/rc6.c"
	"src/ciphers/skipjack.c"
	"src/ciphers/xtea.c"
)

source_group("ciphers" FILES ${TOMCRYPT_CIPHERS})

list(APPEND TOMCRYPT_CIPHERS_AES
	"src/ciphers/aes/aes.c"
)

source_group("ciphers\\\\aes" FILES ${TOMCRYPT_CIPHERS_AES})

list(APPEND TOMCRYPT_CIPHERS_SAFER
	"src/ciphers/safer/safer.c"
	"src/ciphers/safer/saferp.c"
)

source_group("ciphers\\\\safer" FILES ${TOMCRYPT_CIPHERS_SAFER})

list(APPEND TOMCRYPT_CIPHERS_TWOFISH
	"src/ciphers/twofish/twofish.c"
)

source_group("ciphers\\\\twofish" FILES ${TOMCRYPT_CIPHERS_TWOFISH})

list(APPEND TOMCRYPT_ENCAUTH_CCM
	"src/encauth/ccm/ccm_add_aad.c"
	"src/encauth/ccm/ccm_add_nonce.c"
	"src/encauth/ccm/ccm_done.c"
	"src/encauth/ccm/ccm_init.c"
	"src/encauth/ccm/ccm_memory.c"
	"src/encauth/ccm/ccm_process.c"
	"src/encauth/ccm/ccm_reset.c"
	"src/encauth/ccm/ccm_test.c"
)

source_group("encauth\\\\ccm" FILES ${TOMCRYPT_ENCAUTH_CCM})

list(APPEND TOMCRYPT_ENCAUTH_CHACHAPOLY
	"src/encauth/chachapoly/chacha20poly1305_add_aad.c"
	"src/encauth/chachapoly/chacha20poly1305_decrypt.c"
	"src/encauth/chachapoly/chacha20poly1305_done.c"
	"src/encauth/chachapoly/chacha20poly1305_encrypt.c"
	"src/encauth/chachapoly/chacha20poly1305_init.c"
	"src/encauth/chachapoly/chacha20poly1305_memory.c"
	"src/encauth/chachapoly/chacha20poly1305_setiv.c"
	"src/encauth/chachapoly/chacha20poly1305_setiv_rfc7905.c"
	"src/encauth/chachapoly/chacha20poly1305_test.c"
)

source_group("encauth\\\\chachapoly" FILES ${TOMCRYPT_ENCAUTH_CHACHAPOLY})

list(APPEND TOMCRYPT_ENCAUTH_EAX
	"src/encauth/eax/eax_addheader.c"
	"src/encauth/eax/eax_decrypt.c"
	"src/encauth/eax/eax_decrypt_verify_memory.c"
	"src/encauth/eax/eax_done.c"
	"src/encauth/eax/eax_encrypt.c"
	"src/encauth/eax/eax_encrypt_authenticate_memory.c"
	"src/encauth/eax/eax_init.c"
	"src/encauth/eax/eax_test.c"
)

source_group("encauth\\\\eax" FILES ${TOMCRYPT_ENCAUTH_EAX})

list(APPEND TOMCRYPT_ENCAUTH_GCM
	"src/encauth/gcm/gcm_add_aad.c"
	"src/encauth/gcm/gcm_add_iv.c"
	"src/encauth/gcm/gcm_done.c"
	"src/encauth/gcm/gcm_gf_mult.c"
	"src/encauth/gcm/gcm_init.c"
	"src/encauth/gcm/gcm_memory.c"
	"src/encauth/gcm/gcm_mult_h.c"
	"src/encauth/gcm/gcm_process.c"
	"src/encauth/gcm/gcm_reset.c"
	"src/encauth/gcm/gcm_test.c"
)

source_group("encauth\\\\gcm" FILES ${TOMCRYPT_ENCAUTH_GCM})

list(APPEND TOMCRYPT_ENCAUTH_OCB
	"src/encauth/ocb/ocb_decrypt.c"
	"src/encauth/ocb/ocb_decrypt_verify_memory.c"
	"src/encauth/ocb/ocb_done_decrypt.c"
	"src/encauth/ocb/ocb_done_encrypt.c"
	"src/encauth/ocb/ocb_encrypt.c"
	"src/encauth/ocb/ocb_encrypt_authenticate_memory.c"
	"src/encauth/ocb/ocb_init.c"
	"src/encauth/ocb/ocb_ntz.c"
	"src/encauth/ocb/ocb_shift_xor.c"
	"src/encauth/ocb/ocb_test.c"
	"src/encauth/ocb/s_ocb_done.c"
)

source_group("encauth\\\\ocb" FILES ${TOMCRYPT_ENCAUTH_OCB})

list(APPEND TOMCRYPT_ENCAUTH_OCB3
	"src/encauth/ocb3/ocb3_add_aad.c"
	"src/encauth/ocb3/ocb3_decrypt.c"
	"src/encauth/ocb3/ocb3_decrypt_last.c"
	"src/encauth/ocb3/ocb3_decrypt_verify_memory.c"
	"src/encauth/ocb3/ocb3_done.c"
	"src/encauth/ocb3/ocb3_encrypt.c"
	"src/encauth/ocb3/ocb3_encrypt_authenticate_memory.c"
	"src/encauth/ocb3/ocb3_encrypt_last.c"
	"src/encauth/ocb3/ocb3_init.c"
	"src/encauth/ocb3/ocb3_int_ntz.c"
	"src/encauth/ocb3/ocb3_int_xor_blocks.c"
	"src/encauth/ocb3/ocb3_test.c"
)

source_group("encauth\\\\ocb3" FILES ${TOMCRYPT_ENCAUTH_OCB3})

list(APPEND TOMCRYPT_HASHES
	"src/hashes/blake2b.c"
	"src/hashes/blake2s.c"
	"src/hashes/md2.c"
	"src/hashes/md4.c"
	"src/hashes/md5.c"
	"src/hashes/rmd128.c"
	"src/hashes/rmd160.c"
	"src/hashes/rmd256.c"
	"src/hashes/rmd320.c"
	"src/hashes/sha1.c"
	"src/hashes/sha3.c"
	"src/hashes/sha3_test.c"
	"src/hashes/tiger.c"
)

source_group("hashes" FILES ${TOMCRYPT_HASHES})

list(APPEND TOMCRYPT_HASHES_CHC
	"src/hashes/chc/chc.c"
)

source_group("hashes\\\\chc" FILES ${TOMCRYPT_HASHES_CHC})

list(APPEND TOMCRYPT_HASHES_HELPER
	"src/hashes/helper/hash_file.c"
	"src/hashes/helper/hash_filehandle.c"
	"src/hashes/helper/hash_memory.c"
	"src/hashes/helper/hash_memory_multi.c"
)

source_group("hashes\\\\helper" FILES ${TOMCRYPT_HASHES_HELPER})

list(APPEND TOMCRYPT_HASHES_SHA2
	"src/hashes/sha2/sha224.c"
	"src/hashes/sha2/sha256.c"
	"src/hashes/sha2/sha384.c"
	"src/hashes/sha2/sha512.c"
	"src/hashes/sha2/sha512_224.c"
	"src/hashes/sha2/sha512_256.c"
)

source_group("hashes\\\\sha2" FILES ${TOMCRYPT_HASHES_SHA2})

list(APPEND TOMCRYPT_HASHES_WHIRL
	"src/hashes/whirl/whirl.c"
)

source_group("hashes\\\\whirl" FILES ${TOMCRYPT_HASHES_WHIRL})

list(APPEND TOMCRYPT_MAC_BLAKE2
	"src/mac/blake2/blake2bmac.c"
	"src/mac/blake2/blake2bmac_file.c"
	"src/mac/blake2/blake2bmac_memory.c"
	"src/mac/blake2/blake2bmac_memory_multi.c"
	"src/mac/blake2/blake2bmac_test.c"
	"src/mac/blake2/blake2smac.c"
	"src/mac/blake2/blake2smac_file.c"
	"src/mac/blake2/blake2smac_memory.c"
	"src/mac/blake2/blake2smac_memory_multi.c"
	"src/mac/blake2/blake2smac_test.c"
)

source_group("mac\\\\blake2" FILES ${TOMCRYPT_MAC_BLAKE2})

list(APPEND TOMCRYPT_MAC_F9
	"src/mac/f9/f9_done.c"
	"src/mac/f9/f9_file.c"
	"src/mac/f9/f9_init.c"
	"src/mac/f9/f9_memory.c"
	"src/mac/f9/f9_memory_multi.c"
	"src/mac/f9/f9_process.c"
	"src/mac/f9/f9_test.c"
)

source_group("mac\\\\f9" FILES ${TOMCRYPT_MAC_F9})

list(APPEND TOMCRYPT_MAC_HMAC
	"src/mac/hmac/hmac_done.c"
	"src/mac/hmac/hmac_file.c"
	"src/mac/hmac/hmac_init.c"
	"src/mac/hmac/hmac_memory.c"
	"src/mac/hmac/hmac_memory_multi.c"
	"src/mac/hmac/hmac_process.c"
	"src/mac/hmac/hmac_test.c"
)

source_group("mac\\\\hmac" FILES ${TOMCRYPT_MAC_HMAC})

list(APPEND TOMCRYPT_MAC_OMAC
	"src/mac/omac/omac_done.c"
	"src/mac/omac/omac_file.c"
	"src/mac/omac/omac_init.c"
	"src/mac/omac/omac_memory.c"
	"src/mac/omac/omac_memory_multi.c"
	"src/mac/omac/omac_process.c"
	"src/mac/omac/omac_test.c"
)

source_group("mac\\\\omac" FILES ${TOMCRYPT_MAC_OMAC})

list(APPEND TOMCRYPT_MAC_PELICAN
	"src/mac/pelican/pelican.c"
	"src/mac/pelican/pelican_memory.c"
	"src/mac/pelican/pelican_test.c"
)

source_group("mac\\\\pelican" FILES ${TOMCRYPT_MAC_PELICAN})

list(APPEND TOMCRYPT_MAC_PMAC
	"src/mac/pmac/pmac_done.c"
	"src/mac/pmac/pmac_file.c"
	"src/mac/pmac/pmac_init.c"
	"src/mac/pmac/pmac_memory.c"
	"src/mac/pmac/pmac_memory_multi.c"
	"src/mac/pmac/pmac_ntz.c"
	"src/mac/pmac/pmac_process.c"
	"src/mac/pmac/pmac_shift_xor.c"
	"src/mac/pmac/pmac_test.c"
)

source_group("mac\\\\pmac" FILES ${TOMCRYPT_MAC_PMAC})

list(APPEND TOMCRYPT_MAC_POLY1305
	"src/mac/poly1305/poly1305.c"
	"src/mac/poly1305/poly1305_file.c"
	"src/mac/poly1305/poly1305_memory.c"
	"src/mac/poly1305/poly1305_memory_multi.c"
	"src/mac/poly1305/poly1305_test.c"
)

source_group("mac\\\\poly1305" FILES ${TOMCRYPT_MAC_POLY1305})

list(APPEND TOMCRYPT_MAC_XCBC
	"src/mac/xcbc/xcbc_done.c"
	"src/mac/xcbc/xcbc_file.c"
	"src/mac/xcbc/xcbc_init.c"
	"src/mac/xcbc/xcbc_memory.c"
	"src/mac/xcbc/xcbc_memory_multi.c"
	"src/mac/xcbc/xcbc_process.c"
	"src/mac/xcbc/xcbc_test.c"
)

source_group("mac\\\\xcbc" FILES ${TOMCRYPT_MAC_XCBC})

list(APPEND TOMCRYPT_MATH
	"src/math/gmp_desc.c"
	"src/math/ltm_desc.c"
	"src/math/multi.c"
	"src/math/radix_to_bin.c"
	"src/math/rand_bn.c"
	"src/math/rand_prime.c"
	"src/math/tfm_desc.c"
)

source_group("math" FILES ${TOMCRYPT_MATH})

list(APPEND TOMCRYPT_MATH_FP
	"src/math/fp/ltc_ecc_fp_mulmod.c"
)

source_group("math\\\\fp" FILES ${TOMCRYPT_MATH_FP})

list(APPEND TOMCRYPT_MISC
	"src/misc/adler32.c"
	"src/misc/burn_stack.c"
	"src/misc/compare_testvector.c"
	"src/misc/crc32.c"
	"src/misc/error_to_string.c"
	"src/misc/mem_neq.c"
	"src/misc/pk_get_oid.c"
	"src/misc/zeromem.c"
)

source_group("misc" FILES ${TOMCRYPT_MISC})

list(APPEND TOMCRYPT_MISC_BASE64
	"src/misc/base64/base64_decode.c"
	"src/misc/base64/base64_encode.c"
)

source_group("misc\\\\base64" FILES ${TOMCRYPT_MISC_BASE64})

list(APPEND TOMCRYPT_MISC_CRYPT
	"src/misc/crypt/crypt.c"
	"src/misc/crypt/crypt_argchk.c"
	"src/misc/crypt/crypt_cipher_descriptor.c"
	"src/misc/crypt/crypt_cipher_is_valid.c"
	"src/misc/crypt/crypt_constants.c"
	"src/misc/crypt/crypt_find_cipher.c"
	"src/misc/crypt/crypt_find_cipher_any.c"
	"src/misc/crypt/crypt_find_cipher_id.c"
	"src/misc/crypt/crypt_find_hash.c"
	"src/misc/crypt/crypt_find_hash_any.c"
	"src/misc/crypt/crypt_find_hash_id.c"
	"src/misc/crypt/crypt_find_hash_oid.c"
	"src/misc/crypt/crypt_find_prng.c"
	"src/misc/crypt/crypt_fsa.c"
	"src/misc/crypt/crypt_hash_descriptor.c"
	"src/misc/crypt/crypt_hash_is_valid.c"
	"src/misc/crypt/crypt_inits.c"
	"src/misc/crypt/crypt_ltc_mp_descriptor.c"
	"src/misc/crypt/crypt_prng_descriptor.c"
	"src/misc/crypt/crypt_prng_is_valid.c"
	"src/misc/crypt/crypt_prng_rng_descriptor.c"
	"src/misc/crypt/crypt_register_all_ciphers.c"
	"src/misc/crypt/crypt_register_all_hashes.c"
	"src/misc/crypt/crypt_register_all_prngs.c"
	"src/misc/crypt/crypt_register_cipher.c"
	"src/misc/crypt/crypt_register_hash.c"
	"src/misc/crypt/crypt_register_prng.c"
	"src/misc/crypt/crypt_sizes.c"
	"src/misc/crypt/crypt_unregister_cipher.c"
	"src/misc/crypt/crypt_unregister_hash.c"
	"src/misc/crypt/crypt_unregister_prng.c"
)

source_group("misc\\\\crypt" FILES ${TOMCRYPT_MISC_CRYPT})

list(APPEND TOMCRYPT_MISC_HKDF
	"src/misc/hkdf/hkdf.c"
	"src/misc/hkdf/hkdf_test.c"
)

source_group("misc\\\\hkdf" FILES ${TOMCRYPT_MISC_HKDF})

list(APPEND TOMCRYPT_MISC_PKCS5
	"src/misc/pkcs5/pkcs_5_1.c"
	"src/misc/pkcs5/pkcs_5_2.c"
	"src/misc/pkcs5/pkcs_5_test.c"
)

source_group("misc\\\\pkcs5" FILES ${TOMCRYPT_MISC_PKCS5})

list(APPEND TOMCRYPT_MODES_CBC
	"src/modes/cbc/cbc_decrypt.c"
	"src/modes/cbc/cbc_done.c"
	"src/modes/cbc/cbc_encrypt.c"
	"src/modes/cbc/cbc_getiv.c"
	"src/modes/cbc/cbc_setiv.c"
	"src/modes/cbc/cbc_start.c"
)

source_group("modes\\\\cbc" FILES ${TOMCRYPT_MODES_CBC})

list(APPEND TOMCRYPT_MODES_CFB
	"src/modes/cfb/cfb_decrypt.c"
	"src/modes/cfb/cfb_done.c"
	"src/modes/cfb/cfb_encrypt.c"
	"src/modes/cfb/cfb_getiv.c"
	"src/modes/cfb/cfb_setiv.c"
	"src/modes/cfb/cfb_start.c"
)
source_group("modes\\\\cfb" FILES ${TOMCRYPT_MODES_CFB})

list(APPEND TOMCRYPT_MODES_CTR
	"src/modes/ctr/ctr_decrypt.c"
	"src/modes/ctr/ctr_done.c"
	"src/modes/ctr/ctr_encrypt.c"
	"src/modes/ctr/ctr_getiv.c"
	"src/modes/ctr/ctr_setiv.c"
	"src/modes/ctr/ctr_start.c"
	"src/modes/ctr/ctr_test.c"
)

source_group("modes\\\\ctr" FILES ${TOMCRYPT_MODES_CTR})

list(APPEND TOMCRYPT_MODES_ECB
	"src/modes/ecb/ecb_decrypt.c"
	"src/modes/ecb/ecb_done.c"
	"src/modes/ecb/ecb_encrypt.c"
	"src/modes/ecb/ecb_start.c"
)

source_group("modes\\\\ecb" FILES ${TOMCRYPT_MODES_ECB})

list(APPEND TOMCRYPT_MODES_F8
	"src/modes/f8/f8_decrypt.c"
	"src/modes/f8/f8_done.c"
	"src/modes/f8/f8_encrypt.c"
	"src/modes/f8/f8_getiv.c"
	"src/modes/f8/f8_setiv.c"
	"src/modes/f8/f8_start.c"
	"src/modes/f8/f8_test_mode.c"
)

source_group("modes\\\\f8" FILES ${TOMCRYPT_MODES_F8})

list(APPEND TOMCRYPT_MODES_LRW
	"src/modes/lrw/lrw_decrypt.c"
	"src/modes/lrw/lrw_done.c"
	"src/modes/lrw/lrw_encrypt.c"
	"src/modes/lrw/lrw_getiv.c"
	"src/modes/lrw/lrw_process.c"
	"src/modes/lrw/lrw_setiv.c"
	"src/modes/lrw/lrw_start.c"
	"src/modes/lrw/lrw_test.c"
)

source_group("modes\\\\lrw" FILES ${TOMCRYPT_MODES_LRW})

list(APPEND TOMCRYPT_MODES_OFB
	"src/modes/ofb/ofb_decrypt.c"
	"src/modes/ofb/ofb_done.c"
	"src/modes/ofb/ofb_encrypt.c"
	"src/modes/ofb/ofb_getiv.c"
	"src/modes/ofb/ofb_setiv.c"
	"src/modes/ofb/ofb_start.c"
)

source_group("modes\\\\ofb" FILES ${TOMCRYPT_MODES_OFB})

list(APPEND TOMCRYPT_MODES_XTS
	"src/modes/xts/xts_decrypt.c"
	"src/modes/xts/xts_done.c"
	"src/modes/xts/xts_encrypt.c"
	"src/modes/xts/xts_init.c"
	"src/modes/xts/xts_mult_x.c"
	"src/modes/xts/xts_test.c"
)

source_group("modes\\\\xts" FILES ${TOMCRYPT_MODES_XTS})

list(APPEND TOMCRYPT_PK_ASN1_DER_BIT
	"src/pk/asn1/der/bit/der_decode_bit_string.c"
	"src/pk/asn1/der/bit/der_decode_raw_bit_string.c"
	"src/pk/asn1/der/bit/der_encode_bit_string.c"
	"src/pk/asn1/der/bit/der_encode_raw_bit_string.c"
	"src/pk/asn1/der/bit/der_length_bit_string.c"
)

source_group("pk\\\\asn1\\\\der\\\\bit" FILES ${TOMCRYPT_PK_ASN1_DER_BIT})

list(APPEND TOMCRYPT_PK_ASN1_DER_BOOLEAN
	"src/pk/asn1/der/boolean/der_decode_boolean.c"
	"src/pk/asn1/der/boolean/der_encode_boolean.c"
	"src/pk/asn1/der/boolean/der_length_boolean.c"
)

source_group("pk\\\\asn1\\\\der\\\\boolean" FILES ${TOMCRYPT_PK_ASN1_DER_BOOLEAN})

list(APPEND TOMCRYPT_PK_ASN1_DER_CHOICE
	"src/pk/asn1/der/choice/der_decode_choice.c"
)

source_group("pk\\\\asn1\\\\der\\\\choice" FILES ${TOMCRYPT_PK_ASN1_DER_CHOICE})

list(APPEND TOMCRYPT_PK_ASN1_DER_GENERALIZEDTIME
	"src/pk/asn1/der/generalizedtime/der_decode_generalizedtime.c"
	"src/pk/asn1/der/generalizedtime/der_encode_generalizedtime.c"
	"src/pk/asn1/der/generalizedtime/der_length_generalizedtime.c"
)

source_group("pk\\\\asn1\\\\der\\\\generalizedtime" FILES ${TOMCRYPT_PK_ASN1_DER_GENERALIZEDTIME})

list(APPEND TOMCRYPT_PK_ASN1_DER_IA5
	"src/pk/asn1/der/ia5/der_decode_ia5_string.c"
	"src/pk/asn1/der/ia5/der_encode_ia5_string.c"
	"src/pk/asn1/der/ia5/der_length_ia5_string.c"
)

source_group("pk\\\\asn1\\\\der\\\\ia5" FILES ${TOMCRYPT_PK_ASN1_DER_IA5})

list(APPEND TOMCRYPT_PK_ASN1_DER_INTEGER
	"src/pk/asn1/der/integer/der_decode_integer.c"
	"src/pk/asn1/der/integer/der_encode_integer.c"
	"src/pk/asn1/der/integer/der_length_integer.c"
)

source_group("pk\\\\asn1\\\\der\\\\integer" FILES ${TOMCRYPT_PK_ASN1_DER_INTEGER})

list(APPEND TOMCRYPT_PK_ASN1_DER_OBJECT_IDENTIFIER
	"src/pk/asn1/der/object_identifier/der_decode_object_identifier.c"
	"src/pk/asn1/der/object_identifier/der_encode_object_identifier.c"
	"src/pk/asn1/der/object_identifier/der_length_object_identifier.c"
)

source_group("pk\\\\asn1\\\\der\\\\object_identifier" FILES ${TOMCRYPT_PK_ASN1_DER_OBJECT_IDENTIFIER})

list(APPEND TOMCRYPT_PK_ASN1_DER_OCTET
	"src/pk/asn1/der/octet/der_decode_octet_string.c"
	"src/pk/asn1/der/octet/der_encode_octet_string.c"
	"src/pk/asn1/der/octet/der_length_octet_string.c"
)

source_group("pk\\\\asn1\\\\der\\\\octet" FILES ${TOMCRYPT_PK_ASN1_DER_OCTET})

list(APPEND TOMCRYPT_PK_ASN1_DER_PRINTABLE_STRING
	"src/pk/asn1/der/printable_string/der_decode_printable_string.c"
	"src/pk/asn1/der/printable_string/der_encode_printable_string.c"
	"src/pk/asn1/der/printable_string/der_length_printable_string.c"
)

source_group("pk\\\\asn1\\\\der\\\\printable_string" FILES ${TOMCRYPT_PK_ASN1_DER_PRINTABLE_STRING})

list(APPEND TOMCRYPT_PK_ASN1_DER_SEQUENCE
	"src/pk/asn1/der/sequence/der_decode_sequence_ex.c"
	"src/pk/asn1/der/sequence/der_decode_sequence_flexi.c"
	"src/pk/asn1/der/sequence/der_decode_sequence_multi.c"
	"src/pk/asn1/der/sequence/der_decode_subject_public_key_info.c"
	"src/pk/asn1/der/sequence/der_encode_sequence_ex.c"
	"src/pk/asn1/der/sequence/der_encode_sequence_multi.c"
	"src/pk/asn1/der/sequence/der_encode_subject_public_key_info.c"
	"src/pk/asn1/der/sequence/der_length_sequence.c"
	"src/pk/asn1/der/sequence/der_sequence_free.c"
	"src/pk/asn1/der/sequence/der_sequence_shrink.c"
)

source_group("pk\\\\asn1\\\\der\\\\sequence" FILES ${TOMCRYPT_PK_ASN1_DER_SEQUENCE})

list(APPEND TOMCRYPT_PK_ASN1_DER_SET
	"src/pk/asn1/der/set/der_encode_set.c"
	"src/pk/asn1/der/set/der_encode_setof.c"
)

source_group("pk\\\\asn1\\\\der\\\\set" FILES ${TOMCRYPT_PK_ASN1_DER_SET})

list(APPEND TOMCRYPT_PK_ASN1_DER_SHORT_INTEGER
	"src/pk/asn1/der/short_integer/der_decode_short_integer.c"
	"src/pk/asn1/der/short_integer/der_encode_short_integer.c"
	"src/pk/asn1/der/short_integer/der_length_short_integer.c"
)

source_group("pk\\\\asn1\\\\der\\\\short_integer" FILES ${TOMCRYPT_PK_ASN1_DER_SHORT_INTEGER})

list(APPEND TOMCRYPT_PK_ASN1_DER_TELETEX_STRING
	"src/pk/asn1/der/teletex_string/der_decode_teletex_string.c"
	"src/pk/asn1/der/teletex_string/der_length_teletex_string.c"
)

source_group("pk\\\\asn1\\\\der\\\\teletex_string" FILES ${TOMCRYPT_PK_ASN1_DER_TELETEX_STRING})

list(APPEND TOMCRYPT_PK_ASN1_DER_UTCTIME
	"src/pk/asn1/der/utctime/der_decode_utctime.c"
	"src/pk/asn1/der/utctime/der_encode_utctime.c"
	"src/pk/asn1/der/utctime/der_length_utctime.c"
)

source_group("pk\\\\asn1\\\\der\\\\utctime" FILES ${TOMCRYPT_PK_ASN1_DER_UTCTIME})

list(APPEND TOMCRYPT_PK_ASN1_DER_UTF8
	"src/pk/asn1/der/utf8/der_decode_utf8_string.c"
	"src/pk/asn1/der/utf8/der_encode_utf8_string.c"
	"src/pk/asn1/der/utf8/der_length_utf8_string.c"
)

source_group("pk\\\\asn1\\\\der\\\\utf8" FILES ${TOMCRYPT_PK_ASN1_DER_UTF8})

list(APPEND TOMCRYPT_PK_DH
	"src/pk/dh/dh.c"
	"src/pk/dh/dh_check_pubkey.c"
	"src/pk/dh/dh_export.c"
	"src/pk/dh/dh_export_key.c"
	"src/pk/dh/dh_free.c"
	"src/pk/dh/dh_generate_key.c"
	"src/pk/dh/dh_import.c"
	"src/pk/dh/dh_set.c"
	"src/pk/dh/dh_set_pg_dhparam.c"
	"src/pk/dh/dh_shared_secret.c"
)

source_group("pk\\\\dh" FILES ${TOMCRYPT_PK_DH})

list(APPEND TOMCRYPT_PK_DSA
	"src/pk/dsa/dsa_decrypt_key.c"
	"src/pk/dsa/dsa_encrypt_key.c"
	"src/pk/dsa/dsa_export.c"
	"src/pk/dsa/dsa_free.c"
	"src/pk/dsa/dsa_generate_key.c"
	"src/pk/dsa/dsa_generate_pqg.c"
	"src/pk/dsa/dsa_import.c"
	"src/pk/dsa/dsa_make_key.c"
	"src/pk/dsa/dsa_set.c"
	"src/pk/dsa/dsa_set_pqg_dsaparam.c"
	"src/pk/dsa/dsa_shared_secret.c"
	"src/pk/dsa/dsa_sign_hash.c"
	"src/pk/dsa/dsa_verify_hash.c"
	"src/pk/dsa/dsa_verify_key.c"
)

source_group("pk\\\\dsa" FILES ${TOMCRYPT_PK_DSA})

list(APPEND TOMCRYPT_PK_ECC
	"src/pk/ecc/ecc.c"
	"src/pk/ecc/ecc_ansi_x963_export.c"
	"src/pk/ecc/ecc_ansi_x963_import.c"
	"src/pk/ecc/ecc_decrypt_key.c"
	"src/pk/ecc/ecc_encrypt_key.c"
	"src/pk/ecc/ecc_export.c"
	"src/pk/ecc/ecc_free.c"
	"src/pk/ecc/ecc_get_size.c"
	"src/pk/ecc/ecc_import.c"
	"src/pk/ecc/ecc_make_key.c"
	"src/pk/ecc/ecc_shared_secret.c"
	"src/pk/ecc/ecc_sign_hash.c"
	"src/pk/ecc/ecc_sizes.c"
	"src/pk/ecc/ecc_test.c"
	"src/pk/ecc/ecc_verify_hash.c"
	"src/pk/ecc/ltc_ecc_is_valid_idx.c"
	"src/pk/ecc/ltc_ecc_map.c"
	"src/pk/ecc/ltc_ecc_mul2add.c"
	"src/pk/ecc/ltc_ecc_mulmod.c"
	"src/pk/ecc/ltc_ecc_mulmod_timing.c"
	"src/pk/ecc/ltc_ecc_points.c"
	"src/pk/ecc/ltc_ecc_projective_add_point.c"
	"src/pk/ecc/ltc_ecc_projective_dbl_point.c"
)

source_group("pk\\\\ecc" FILES ${TOMCRYPT_PK_ECC})

list(APPEND TOMCRYPT_PK_KATJA
	"src/pk/katja/katja_decrypt_key.c"
	"src/pk/katja/katja_encrypt_key.c"
	"src/pk/katja/katja_export.c"
	"src/pk/katja/katja_exptmod.c"
	"src/pk/katja/katja_free.c"
	"src/pk/katja/katja_import.c"
	"src/pk/katja/katja_make_key.c"
)

source_group("pk\\\\katja" FILES ${TOMCRYPT_PK_KATJA})

list(APPEND TOMCRYPT_PK_PKCS1
	"src/pk/pkcs1/pkcs_1_i2osp.c"
	"src/pk/pkcs1/pkcs_1_mgf1.c"
	"src/pk/pkcs1/pkcs_1_oaep_decode.c"
	"src/pk/pkcs1/pkcs_1_oaep_encode.c"
	"src/pk/pkcs1/pkcs_1_os2ip.c"
	"src/pk/pkcs1/pkcs_1_pss_decode.c"
	"src/pk/pkcs1/pkcs_1_pss_encode.c"
	"src/pk/pkcs1/pkcs_1_v1_5_decode.c"
	"src/pk/pkcs1/pkcs_1_v1_5_encode.c"
)

source_group("pk\\\\pkcs1" FILES ${TOMCRYPT_PK_PKCS1})

list(APPEND TOMCRYPT_PK_RSA
	"src/pk/rsa/rsa_decrypt_key.c"
	"src/pk/rsa/rsa_encrypt_key.c"
	"src/pk/rsa/rsa_export.c"
	"src/pk/rsa/rsa_exptmod.c"
	"src/pk/rsa/rsa_free.c"
	"src/pk/rsa/rsa_get_size.c"
	"src/pk/rsa/rsa_import.c"
	"src/pk/rsa/rsa_import_pkcs8.c"
	"src/pk/rsa/rsa_import_x509.c"
	"src/pk/rsa/rsa_make_key.c"
	"src/pk/rsa/rsa_set.c"
	"src/pk/rsa/rsa_sign_hash.c"
	"src/pk/rsa/rsa_sign_saltlen_get.c"
	"src/pk/rsa/rsa_verify_hash.c"
)

source_group("pk\\\\rsa" FILES ${TOMCRYPT_PK_RSA})

list(APPEND TOMCRYPT_PRNGS
	"src/prngs/chacha20.c"
	"src/prngs/fortuna.c"
	"src/prngs/rc4.c"
	"src/prngs/rng_get_bytes.c"
	"src/prngs/rng_make_prng.c"
	"src/prngs/sober128.c"
	"src/prngs/sprng.c"
	"src/prngs/yarrow.c"
)

source_group("prngs" FILES ${TOMCRYPT_PRNGS})

list(APPEND TOMCRYPT_STREAM_CHACHA
	"src/stream/chacha/chacha_crypt.c"
	"src/stream/chacha/chacha_done.c"
	"src/stream/chacha/chacha_ivctr32.c"
	"src/stream/chacha/chacha_ivctr64.c"
	"src/stream/chacha/chacha_keystream.c"
	"src/stream/chacha/chacha_setup.c"
	"src/stream/chacha/chacha_test.c"
)

source_group("stream\\\\chacha" FILES ${TOMCRYPT_STREAM_CHACHA})

list(APPEND TOMCRYPT_STREAM_RC4
	"src/stream/rc4/rc4_stream.c"
	"src/stream/rc4/rc4_test.c"
)

source_group("stream\\\\rc4" FILES ${TOMCRYPT_STREAM_RC4})

list(APPEND TOMCRYPT_STREAM_SOBER128
	"src/stream/sober128/sober128_stream.c"
	"src/stream/sober128/sober128_test.c"
)

source_group("stream\\\\sober128" FILES ${TOMCRYPT_STREAM_SOBER128})

list(APPEND TOMCRYPT_SRC
	${TOMCRYPT_CIPHERS}
    ${TOMCRYPT_CIPHERS_AES}
    ${TOMCRYPT_CIPHERS_SAFER}
    ${TOMCRYPT_CIPHERS_TWOFISH}
    ${TOMCRYPT_ENCAUTH_CCM}
    ${TOMCRYPT_ENCAUTH_CHACHAPOLY}
    ${TOMCRYPT_ENCAUTH_EAX}
    ${TOMCRYPT_ENCAUTH_GCM}
    ${TOMCRYPT_ENCAUTH_OCB}
    ${TOMCRYPT_ENCAUTH_OCB3}
    ${TOMCRYPT_HASHES}
    ${TOMCRYPT_HASHES_CHC}
    ${TOMCRYPT_HASHES_HELPER}
    ${TOMCRYPT_HASHES_SHA2}
    ${TOMCRYPT_HASHES_WHIRL}
    ${TOMCRYPT_MAC_BLAKE2}
    ${TOMCRYPT_MAC_F9}
    ${TOMCRYPT_MAC_HMAC}
    ${TOMCRYPT_MAC_OMAC}
    ${TOMCRYPT_MAC_PELICAN}
    ${TOMCRYPT_MAC_PMAC}
    ${TOMCRYPT_MAC_POLY1305}
    ${TOMCRYPT_MAC_XCBC}
    ${TOMCRYPT_MATH}
    ${TOMCRYPT_MATH_FP}
    ${TOMCRYPT_MISC}
    ${TOMCRYPT_MISC_BASE64}
    ${TOMCRYPT_MISC_CRYPT}
    ${TOMCRYPT_MISC_HKDF}
    ${TOMCRYPT_MISC_PKCS5}
    ${TOMCRYPT_MODES_CBC}
    ${TOMCRYPT_MODES_CFB}
    ${TOMCRYPT_MODES_CTR}
    ${TOMCRYPT_MODES_ECB}
    ${TOMCRYPT_MODES_F8}
    ${TOMCRYPT_MODES_LRW}
    ${TOMCRYPT_MODES_OFB}
    ${TOMCRYPT_MODES_XTS}
    ${TOMCRYPT_PK_ASN1_DER_BIT}
    ${TOMCRYPT_PK_ASN1_DER_BOOLEAN}
    ${TOMCRYPT_PK_ASN1_DER_CHOICE}
    ${TOMCRYPT_PK_ASN1_DER_GENERALIZEDTIME}
    ${TOMCRYPT_PK_ASN1_DER_IA5}
    ${TOMCRYPT_PK_ASN1_DER_INTEGER}
    ${TOMCRYPT_PK_ASN1_DER_OBJECT_IDENTIFIER}
    ${TOMCRYPT_PK_ASN1_DER_OCTET}
    ${TOMCRYPT_PK_ASN1_DER_PRINTABLE_STRING}
    ${TOMCRYPT_PK_ASN1_DER_SEQUENCE}
    ${TOMCRYPT_PK_ASN1_DER_SET}
    ${TOMCRYPT_PK_ASN1_DER_SHORT_INTEGER}
    ${TOMCRYPT_PK_ASN1_DER_TELETEX_STRING}
    ${TOMCRYPT_PK_ASN1_DER_UTCTIME}
    ${TOMCRYPT_PK_ASN1_DER_UTF8}
    ${TOMCRYPT_PK_DH}
    ${TOMCRYPT_PK_DSA}
    ${TOMCRYPT_PK_ECC}
    ${TOMCRYPT_PK_KATJA}
    ${TOMCRYPT_PK_PKCS1}
    ${TOMCRYPT_PK_RSA}
    ${TOMCRYPT_PRNGS}
    ${TOMCRYPT_STREAM_CHACHA}
    ${TOMCRYPT_STREAM_RC4}
    ${TOMCRYPT_STREAM_SOBER128}
)

list(APPEND TOMCRYPT_HPP
	"src/headers/tomcrypt.h"
	"src/headers/tomcrypt_argchk.h"
	"src/headers/tomcrypt_cfg.h"
	"src/headers/tomcrypt_cipher.h"
	"src/headers/tomcrypt_custom.h"
	"src/headers/tomcrypt_hash.h"
	"src/headers/tomcrypt_mac.h"
	"src/headers/tomcrypt_macros.h"
	"src/headers/tomcrypt_math.h"
	"src/headers/tomcrypt_misc.h"
	"src/headers/tomcrypt_pk.h"
	"src/headers/tomcrypt_pkcs.h"
	"src/headers/tomcrypt_prng.h"
)

source_group("headers" FILES ${TOMCRYPT_HPP})

add_library("tomcrypt" STATIC ${TOMCRYPT_SRC} ${TOMCRYPT_HPP})

set_target_properties(tomcrypt PROPERTIES COMPILE_DEFINITIONS LTC_NO_ASM)
set_property(TARGET "tomcrypt" APPEND PROPERTY COMPILE_DEFINITIONS LTC_DESC)
set_property(TARGET "tomcrypt" APPEND PROPERTY COMPILE_DEFINITIONS LTC_SOURCE)
set_property(TARGET "tomcrypt" APPEND PROPERTY COMPILE_DEFINITIONS LTC_DEVRANDOM)



#if (WITH_PORTABLE_TOMCRYPT)
#  sm_add_compile_definition("tomcrypt" LTC_NO_ASM)
#elseif(WITH_NO_ROLC_TOMCRYPT AND NOT APPLE)
#  sm_add_compile_definition("tomcrypt" LTC_NO_ROLC)
#endif()
#
#if (APPLE)
#  sm_add_compile_definition("tomcrypt" LTC_NO_ROLC)
#  sm_append_simple_target_property("tomcrypt" XCODE_ATTRIBUTE_GCC_NO_COMMON_BLOCKS "YES")
#  sm_append_simple_target_property("tomcrypt" XCODE_ATTRIBUTE_GCC_PREPROCESSOR_DEFINITIONS[variant=Debug]
#  "'CMAKE_INTDIR=\"Debug\"' LTC_NO_ROLC DEBUG=1")
#elseif (MSVC)
#  sm_add_compile_definition("tomcrypt" _CRT_SECURE_NO_WARNINGS)
#endif()

#disable_project_warnings("tomcrypt")

target_include_directories("tomcrypt" PUBLIC "src/headers")