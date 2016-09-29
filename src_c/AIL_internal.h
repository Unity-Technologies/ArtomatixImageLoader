#ifndef ARTOMATIX_AIL_INTERNAL_H
#define ARTOMATIX_AIL_INTERNAL_H

#define AIL_UNUSED_PARAM(name) (void)(name)

typedef struct CallbackData
{
    ReadCallback readCallback;
    TellCallback tellCallback;
    SeekCallback seekCallback;
    WriteCallback writeCallback;
    void * callbackData;

} CallbackData;


#endif // ARTOMATIX_AIL_INTERNAL_H
