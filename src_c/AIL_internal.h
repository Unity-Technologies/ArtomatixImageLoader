#ifndef ARTOMATIX_AIL_INTERNAL_H
#define ARTOMATIX_AIL_INTERNAL_H

#define AIL_UNUSED_PARAM(name) (void)(name)

// don't take this to mean we support big-endian. Everything will probably break on big-endian
// taken from SDL: https://www.libsdl.org/release/SDL-1.2.15/include/SDL_endian.h
#define AIL_LIL_ENDIAN	1234
#define AIL_BIG_ENDIAN	4321
#ifndef AIL_BYTEORDER
    #ifdef __linux__
        #include <endian.h>
        #define AIL_BYTEORDER  __BYTE_ORDER
    #else
        #if defined(__hppa__) || \
            defined(__m68k__) || defined(mc68000) || defined(_M_M68K) || \
            (defined(__MIPS__) && defined(__MISPEB__)) || \
            defined(__ppc__) || defined(__POWERPC__) || defined(_M_PPC) || \
            defined(__sparc__)
            #define AIL_BYTEORDER	AIL_BIG_ENDIAN
        #else
            #define AIL_BYTEORDER	AIL_LIL_ENDIAN
        #endif
    #endif
#endif

typedef struct CallbackData
{
    ReadCallback readCallback;
    TellCallback tellCallback;
    SeekCallback seekCallback;
    WriteCallback writeCallback;
    void * callbackData;

} CallbackData;


#endif // ARTOMATIX_AIL_INTERNAL_H
