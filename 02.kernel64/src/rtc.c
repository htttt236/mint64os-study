#include "rtc.h"
#include "types.h"
#include "assembly_utility.h"


// CMOS 메모리에서 RTC 컨트롤러가 저장한 현재 시간을 읽음
void kReadRTCTime(byte* pbHour, byte* pbMinute, byte* pbSecond){
    byte bData;

    // CMOS 메모리 어드레스 레지스터(포트 0x70)에 시간을 저장하는 레지스터 저장
    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_HOUR);
    // CMOS 데이터 레지스터(포트 0x71)에서 시간을 읽음
    bData = kInPortByte(RTC_CMOSDATA);
    *pbHour = RTC_BCDTOBINARY(bData);

    // 분
    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_MINUTE);
    bData = kInPortByte(RTC_CMOSDATA);
    *pbMinute = RTC_BCDTOBINARY(bData);

    // 초
    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_SECOND);
    bData = kInPortByte(RTC_CMOSDATA);
    pbSecond = RTC_BCDTOBINARY(bData);
}

// CMOS 메모리에서 RTC 컨트롤러가 저장한 현재 일자를 읽음
void kReadRTCDate(word* pwYear, byte* pbMonth, byte* pbDayOfMonth, byte* pbDayOfWeek){
    byte bData;

    // CMOS 메모리 어드레스 레지스터(포트 0x70)에 연도를 저장하는 레지스터 지정
    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_YEAR);
    // CMOS 데이터 레지스터(포트 0x71)에서 연도를 읽음
    bData = kInPortByte(RTC_CMOSDATA);
    *pwYear = RTC_BCDTOBINARY(bData) + 2000;

    // 월
    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_MONTH);
    bData = kInPortByte(RTC_CMOSDATA);
    *pbMonth = RTC_BCDTOBINARY(bData);
    
    // 일
    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_DAYOFMONTH);
    bData = kInPortByte(RTC_CMOSDATA);
    *pbDayOfMonth = RTC_BCDTOBINARY(bData);
    
    // 요일
    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_DAYOFWEEK);
    bData = kInPortByte(RTC_CMOSDATA);
    *pbDayOfWeek = RTC_BCDTOBINARY(bData);
}

// 요일 값을 이용해서 해당 요일의 문자열을 반환
char* kConvertDayOfWeekToString(byte bDayOfWeek){
    static char* vpcDayOfWeekString[8] = {"Error", "Sunday", "Monday", 
        "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    
    // 요일 범위가 넘어가면 에러를 반환
    if(bDayOfWeek >= 8){
        return vpcDayOfWeekString[0];
    }

    // 요일을 반환
    return vpcDayOfWeekString[bDayOfWeek];
}