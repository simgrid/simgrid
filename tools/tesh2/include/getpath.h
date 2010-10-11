#ifndef __GETPATH_H
#define __GETPATH_H
 
#ifdef __cplusplus
extern "C" {
  
#endif  /*  */
  
/* getpath th 	  -- 	get the path of the file name specified by the first parameter
 * 						of the function and store the path in its second parmater.
 *						the function returns the length of the path of the file.				 
 *
 * param file_name		The file name to get the path
 * param path			The address of the path of the file
 * 
 * return				If successful, the function returns the len of the path.
 *						Otherwise the function returns -1 and sets errno to indicate
 *						the error.
 *
 * errors
 *
 *						[ENOENT]	the file name specified as parameter does not exist.
 *				
 *						[ENOMEM]	because this function use calloc, errno can be set with
 *									this error code.				
 *
 */ 
  int  getpath(const char *file_name, char **path);
  
/* translatepath   -- 	path translation
 *
 * param totranslate	The path to translate.
 * param transled		The address of the translated path.
 *
 * return				If successful the function returns the len of the translated path.
 *						0therwise the function returns -1 and sets the global variable errno
 *						to indicate the error.
 *
 * errors				[ENOTDIR]	the path to translate is not a directory.
 
 *						[ENOMEM]	because this function use calloc, errno can be set with
 *									this error code.				
 */ 
  int  translatepath(const char *totranslate, char **translated);
  
#ifdef __cplusplus
} 
#endif  /*  */

#endif  /* !__GETPATH_H */
