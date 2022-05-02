#ifndef _SSD_ODTINFO_H_
#define _SSD_ODTINFO_H_

#define CSV_START_KEY_WORD  "FlashType"
#define ALEMENT_KEY  "@MICRON_CLK@"

class MicronTimingInfo
{
public:
    static MicronTimingInfo* Instance(const char*);
    static void Free();
    unsigned char GetParam(const char* ctrl, const char* flash, const char* flh_interface, int flh_clk );
private:
    MicronTimingInfo();
    ~MicronTimingInfo();
    static MicronTimingInfo* _inst;
    char** _keys;
    unsigned char** _datas;
    int _colCnt = 0;
    int _rowCnt = 0;
};
#endif /*_SSD_ODTINFO_H_*/