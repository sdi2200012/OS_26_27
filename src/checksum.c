#include <stdio.h>
#include <string.h>
#include "header/common.h"
#include "header/errexit.h"




/* djb33a: h = 33*h ^ c, ξεκινώντας από 5381 */
uint32_t djb33_hash(const char *s, size_t len)
{
    uint32_t h = 5381;
    while (len--) {
        h += (h << 5);
        h ^= (unsigned char) *s++;
    }
    return h;
}