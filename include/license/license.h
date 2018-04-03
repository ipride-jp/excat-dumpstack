/*
 * @(#)license.h
 * $Id$
 */

#if !defined(___LICENSE_H___)
#define ___LICENSE_H___
#if defined(__cplusplus)
extern "C" {
#endif /* defined(__cplusplus) */

/**
 * initialize license library
 */
void license_initialize(void);


/**
 * check if the licence file is valid or not
 * @param certfile the name of license file
 * @param appverstr application name and version
 * @return nonzero if valid
 */
int license_is_valid_file(const char* certfile, const char* appverstr);


/**
 * extract an application information from the license file.
 * the return value is newly allocated, so you MUST free it.
 * @param certfile the name of license file
 * @return nul-terminated information string
 * TODO: document the format.
 */
char* license_get_info(const char* certfile);

/**
 * extract a date from when the license is valid.
 * the return value is newly allocated, so you MUST free it.
 * @param certfile the name of license file
 * @return nul-terminated date string
 * TODO: document the format.
 */
char* license_get_begin_date(const char* certfile);

/**
 * extract a date to when the license is valid.
 * the return value is newly allocated, so you MUST free it.
 * @param certfile the name of license file
 * @return nul-terminated date string
 * TODO: document the format.
 */
char* license_get_end_date(const char* certfile);

/**
 * extract a name of a licensee from the license.
 * the return value is newly allocated, so you MUST free it.
 * @param certfile the name of license file
 * @return nul-terminated name of licensee
 * TODO: document the format.
 */
char* license_get_subject(const char* certfile);

/**
 * extract a public key of the license file.
 * the return value is newly allocated, so you MUST free it.
 * @param certfile the name of license file
 * @return DON'T KNOW WHAT THE FUNCTION RETURNS!
 * TODO: document the format.
 */
char* license_get_public_key(const char* certfile);

/* add by Qiu Song on 20090929 for MACアドレスのチェック */
char* license_get_mac_address(const char* certfile);
/* end of add by Qiu Song on 20090929 for MACアドレスのチェック */
	
#if defined(__cplusplus)
}
#endif /* defined(__cplusplus) */
#endif /* !defined(___LICENSE_H___) */

/* the end of file */
