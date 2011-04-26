#ifndef _INC_MD4
#define _INC_MD4

#if defined(__FreeBSD__) && __FreeBSD__ < 5 	/* for FreeBSD version <= 4 */
#include <inttypes.h> 
#else                 				/* otherwise */
#include <stdint.h>
#endif

typedef uint32_t Word32Type;

typedef struct {
  Word32Type buffer[4];
  unsigned char count[8];
  unsigned int done;
} MDstruct, *MDptr;

#ifdef __cplusplus
extern "C" {
#endif
extern void MDbegin(MDptr MDp) ;

extern void MDupdate(MDptr MDp, unsigned char *X, Word32Type count) ;

extern void MDprint(MDptr MDp) ;

#ifdef __cplusplus
}
#endif

#endif
