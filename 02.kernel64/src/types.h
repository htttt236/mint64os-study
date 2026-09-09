#ifndef __TYPES_H__
#define __TYPES_H__

#define byte unsigned char
#define word unsigned short
#define dword unsigned int
#define qword unsigned long
#define bool unsigned char

#define true 1
#define false 0
#define null 0

// stddef.h 헤더에 포함된 offsetof() 매크로의 내용
#define offsetof(TYPE, MEMBER) __builtin_offsetof (TYPE, MEMBER)


#pragma pack(push, 1)

typedef struct kCharacterStruct{
    byte bCharacter;
    byte bAttribute;
}CHARACTER;

#pragma pack(pop)
#endif /*__TYPES_H__*/