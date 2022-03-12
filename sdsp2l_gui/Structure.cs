using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace Xpacer
{
    [StructLayout(LayoutKind.Sequential, Pack = 0)]
    public struct sPowerCycleTestStatus
    {
        public int iLOOP;
        public int iLASTLOOP;
        public ePowerCycleState eSTATE;
        public ePowerCycleState eLASTSTATE;
        public eFailCode eFAILCODE;
        public int iTESTTIME;
        public bool bSTOP;
    }

    public enum eFailCode
    { 
        E_MIS_COMPARE,
        E_DEVICE_LOST,
        E_DEVICE_EXIST,
        E_WRITE_FAIL,
        E_READ_FAIL,
        E_OTHERS
    }

    public enum ePowerCycleState
    {
        E_IDLE,
        E_TEST_FAIL,
        E_TEST_PASS,
        E_INIT,
        E_SCAN_DEVICE,
        E_WRITE,
        E_VERIFY,
        E_POWER_OFF,
        E_POWER_ON,
    }

    [StructLayout(LayoutKind.Sequential, Pack = 0)]
    public struct sPowerCycleTestConfig
    {
        public int iWRITE_SIZE;
        public int iWORK_LOAD;
        public int iTEST_LOOP;
        public int iPOWOFF_IDLE_TIME;
        public int iREMOVE_PWR_TIME;
        public int iQUEUE_DEPTH;
        public int iREBUILD_TO;
        public int iTEST_TYPE;

        public bool bPREFILL;
        public bool bOUTPUT_LOG;
        public bool bCOM_EXIST;

    }

    public enum eWriteSize
    {
        E_512BYTES,
        E_4KB,
        E_16KB,
        E_32KB,
        E_64KB,
        E_128KB,
        E_MIX_WRITESIZE,
    }

    public enum eFEATURE_ID_ORDER
    { 
        E_ARBITRATION,
        E_POWER_MANAGMENT,
        E_LBA_RANGE_TYPE,
        E_TEMP_THRESHOLD,
        E_ERROR_RECOVERY,
        E_VOLITILE_WRITE_CACHE,
        E_NUMBER_OF_QUEUE,
        E_INTERRUPT_COALESCING,
        E_INTERRUPT_VEC_CONFIG,
        E_WRITE_AUTOMICITY_NORMAL,
        E_ASYNC_EVENT_CONFIG,
        E_APST,
        E_HOST_MEM_BUFFER,
        E_TIMESTAMP,
        E_KEEP_ALIVE_TIMER,
        E_HOST_CONTROL_THERMAL_MANG,
        E_NON_OPERATION_PWR_STATE_CFG
    }

    [StructLayout(LayoutKind.Sequential, Pack = 0)]
    public struct sIdentifyControllerData {
        
        //
        // byte 0 : 255, Controller Capabilities and Features
        //
        public UInt16 VID;                   // byte [   1:   0] M - PCI Vendor ID (VID)
        public short SSVID;                 // byte [   3:   2] M - PCI Subsystem Vendor ID (SSVID)
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 20)]
        public char[] SN;   //20                // byte [  23:   4] M - Serial Number (SN)
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 40)]
        public char[] MN;   //40                // byte [  63:  24] M - Model Number (MN)
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 8)]
        public char[] FR;   //8                // byte [  71:  64] M - Firmware Revision (FR)
        public char RAB;                    // byte [       72] M - Recommended Arbitration Burst (RAB)
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
        public char[] IEEE;   //3              // byte [  75:  73] M - IEEE OUI Identifier (IEEE). Controller Vendor code.
        public char CMIC;                   // byte [       76] O - Controller Multi-Path I/O and Namespace Sharing Capabilities (CMIC)
        public char MDTS;                   // byte [       77] M - Maximum Data Transfer Size (MDTS)
        public short CNTLID;                // byte [  79:  78] M - Controller ID (CNTLID)
        public int VER;                     // byte [  83:  80] M - Version (VER)
        public int RTD3R;                   // byte [  87:  84] M - RTD3 Resume Latency (RTD3R)
        public int RTD3E;                   // byte [  91:  88] M - RTD3 Entry Latency (RTD3E)

        public int OAES;                         // byte [  95:  92] M - Optional Asynchronous Events Supported (OAES)
        public int CTRATT;                       // byte [  99:  96] M - Controller Attributes (CTRATT)
        public short RRLS;               // byte [ 101: 100] O - Read Recovery Levels Supported (RRLS) <rev1.4>
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 9)]
        public char[] Reserved0;//[9];       // byte [ 110: 102]

        public char CNTRLTYPE;          // byte [      111] M - Controller Type (CNTRLTYPE)             <rev1.4>
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
        public char[] FGUID;//[16];          // byte [ 127: 112] O - FRU Globally Unique Identifier (FGUID)  <rev1.3>
        public short CRDT1;              // byte [ 129: 128] O - Command Retry Delay Time 1 (CRDT1)      <rev1.4>
        public short CRDT2;              // byte [ 131: 130] O - Command Retry Delay Time 2 (CRDT2)      <rev1.4>
        public short CRDT3;              // byte [ 133: 132] O - Command Retry Delay Time 3 (CRDT3)      <rev1.4>
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 106)]
        public char[] Reserved1;//[106];     // byte [ 239: 134]
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
        public char[] Reserved2;//[16];      // byte [ 255: 240] Refer to the NVMe Management Interface Specification for definition

        //
        // byte 256 : 511, Admin Command Set Attributes
        //
        public short OACS;                         // byte [ 257: 256] M - Optional Admin Command Support (OACS)
        public char ACL;                    // byte [      258] M - Abort Command Limit (ACL)
        public char AERL;                   // byte [      259] M - Asynchronous Event Request Limit (AERL)
        public char FRMW;                         // byte [      260] M - Firmware Updates (FRMW)
        public char LPA;                          // byte [      261] M - Log Page Attributes (LPA)
        public char ELPE;                   // byte [      262] M - Error Log Page Entries (ELPE)
        public char NPSS;                   // byte [      263] M - Number of Power States Support (NPSS)
        public char AVSCC;                        // byte [      264] M - Admin Vendor Specific Command Configuration (AVSCC)
        public char APSTA;                        // byte [      265] O - Autonomous Power State Transition Attributes (APSTA)
        public short WCTEMP;             // byte [ 267: 266] M - Warning Composite Temperature Threshold (WCTEMP)
        public short CCTEMP;             // byte [ 269: 268] M - Critical Composite Temperature Threshold (CCTEMP)
        public short MTFA;               // byte [ 271: 270] O - Maximum Time for Firmware Activation (MTFA)
        public int HMPRE;              // byte [ 275: 272] O - Host Memory Buffer Preferred Size (HMPRE)
        public int HMMIN;              // byte [ 279: 276] O - Host Memory Buffer Minimum Size (HMMIN)
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
        public char[] TNVMCAP;//[16];        // byte [ 295: 280] O - Total NVM Capacity (TNVMCAP)
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
        public char[] UNVMCAP;//[16];        // byte [ 311: 296] O - Unallocated NVM Capacity (UNVMCAP)
        public int RPMBS;                        // byte [ 315: 312] O - Replay Protected Memory Block Support (RPMBS)
        public short EDSTT;              // byte [ 317: 316] O - Extended Device Self-test Time (EDSTT)  <rev1.3>
        public char DSTO;               // byte [      318] O - Device Self-test Options (DSTO)         <rev1.3>
        public char FWUG;               // byte [      319] M - Firmware Update Granularity (FWUG)      <rev1.3>
        public short KAS;                // byte [ 321: 320] M - Keep Alive Support (KAS)
        public short HCTMA;                        // byte [ 323: 322] O - Host Controlled Thermal Management Attributes (HCTMA)  <rev1.3>
        public short MNTMT;              // byte [ 325: 324] O - Minimum Thermal Management Temperature (MNTMT) <rev1.3>
        public short MXTMT;              // byte [ 327: 326] O - Maximum Thermal Management Temperature (MXTMT) <rev1.3>
        public int SANICAP;                      // byte [ 331: 328] O - Sanitize Capabilities (SANICAP)                             <rev1.3>
        public int HMMINDS;            // byte [ 335: 332] O - Host Memory Buffer Minimum Descriptor Entry Size (HMMINDS)  <rev1.4>
        public short HMMAXD;             // byte [ 337: 336] O - Host Memory Maximum Descriptors Entries (HMMAXD)            <rev1.4>
        public short NSETIDMAX;          // byte [ 339: 338] O - NVM Set Identifier Maximum (NSETIDMAX)                      <rev1.4>
        public short ENDGIDMAX;          // byte [ 341: 340] O - Endurance Group Identifier Maximum (ENDGIDMAX)              <rev1.4>
        public char ANATT;              // byte [      342] O - ANA Transition Time (ANATT)                                 <rev1.4>
        public char ANACAP;                       // byte [      343] O - Asymmetric Namespace Access Capabilities (ANACAP) <rev1.4>
        public int ANAGRPMAX;          // byte [ 347: 344] O - ANA Group Identifier Maximum (ANAGRPMAX)    <rev1.4>
        public int NANAGRPID;          // byte [ 351: 348] O - Number of ANA Group Identifiers (NANAGRPID) <rev1.4>
        public int PELS;               // byte [ 355: 352] O - Persistent Event Log Size (PELS)            <rev1.4>
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 156)]
        public char[] Reserved3;//[156];     // byte [ 511: 356]

        //
        // byte 512 : 703, NVM Command Set Attributes
        //
        public char SQES;                         // byte [      512] M - Submission Queue Entry Size (SQES)
        public char CQES;                         // byte [      513] M - Completion Queue Entry Size (CQES)
        public short MAXCMD;             // byte [ 515: 514] M - Maximum Outstanding Commands (MAXCMD)
        public int NN;                 // byte [ 519: 516] M - Number of Namespaces (NN)
        public short ONCS;                         // byte [ 521: 520] M - Optional NVM Command Support (ONCS)
        public short FUSES;                        // byte [ 523: 522] M - Fused Operation Support (FUSES)
        public char FNA;                          // byte [      524] M - Format NVM Attributes (FNA)
        public char VWC;                          // byte [      525] M - Volatile Write Cache (VWC)
        public short AWUN;               // byte [ 527: 526] M - Atomic Write Unit Normal (AWUN)
        public short AWUPF;              // byte [ 529: 528] M - Atomic Write Unit Power Fail (AWUPF)
        public char NVSCC;                        // byte [      530] M - NVM Vendor Specific Command Configuration (NVSCC)
        public char NWPC;                         // byte [      531] M - Namespace Write Protection Capabilities (NWPC) <rev1.4>
        public short ACWU;               // byte [ 533: 532] O - Atomic Compare & Write Unit (ACWU)
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 2)]
        public char[] Reserved5;//[2];           // byte [ 535: 534]
        public int SGLS;                         // byte [ 539: 536] O - SGL Support (SGLS)
        public int MNAN;               // byte [      540] O - Maximum Number of Allowed Namespaces (MNAN) <rev1.4>
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 224)]
        public char[] Reserved6;//[224];         // byte [ 767: 544]
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 256)]
        public char[] SUBNQN;//[256];            // byte [1023: 768] M - NVM Subsystem NVMe Qualified Name (SUBNQN)
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 768)]
        public char[] Reserved7;//[768];         // byte [1791:1024]
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 256)]
        public char[] Reserved8;//[256];         // byte [2047:1792] Refer to the NVMe over Fabrics specification
                                //NVME_POWER_STATE_DESC PDS[32];// byte [2079:2048] M - Power State 0 Descriptor (PSD0)
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 1024)]
        public char[] NVME_POWER_STATE_DESC; // 32*32
                                             // byte [3071:2080] O - Power State Descriptors from PS1 (PSD1) to PS31 (PSD31) 
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 1024)]
        public char[] VS; //[1024];              // byte [4095:3072] Vendor Specific
    }

    [StructLayout(LayoutKind.Sequential, Pack = 0)]
    public struct sSmartInfoData{
        public byte CriticalWarning;                  // byte [      0] Critical Warning
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 2)]
        public byte[] Temperature;              // byte [  2:  1] Composite Temperature
        public byte AvailableSpare;           // byte [      3] Available Spare
        public byte AvailableSpareThreshold;  // byte [      4] Available Spare Threshold
        public byte PercentageUsed;           // byte [      5] Percentage Used

        public byte EnduranceGroupSummary;            // byte [      6] Endurance Group Critical Warning Summary <rev1.4>
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 25)]
        public byte[] Reserved0;            // byte [ 31:  7]
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
        public byte[] DataUnitRead;         // byte [ 47: 32] Data Units Read
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
        public byte[] DataUnitWritten;      // byte [ 63: 48] Data Units Written
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
        public byte[] HostReadCommands;     // byte [ 79: 64] Host Read Commands
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
        public byte[] HostWrittenCommands;  // byte [ 95: 80] Host Write Commands
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
        public byte[] ControllerBusyTime;   // byte [111: 66] Controller Busy Time
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
        public byte[] PowerCycle;           // byte [127:112] Power Cycles
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
        public byte[] PowerOnHours;         // byte [143:128] Power On Hours
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
        public byte[] UnsafeShutdowns;      // byte [159:144] Unsafe Shutdowns
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
        public byte[] MediaErrors;          // byte [175:160] Media and Data Integrity Errors
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
        public byte[] ErrorInfoLogEntryNum; // byte [191:176] Number of Error Information Log Entries

        public int WCTempTime;               // byte [195:192] Warning Composite Temperature Time
        public int CCTempTime;               // byte [196:196] Critical Composite Temperature Time

        public short TemperatureSensor1;       // byte [201:200] Current temperature reported by temperature sensor 1.
        public short TemperatureSensor2;       // byte [203:202] Current temperature reported by temperature sensor 2.
        public short TemperatureSensor3;       // byte [205:204] Current temperature reported by temperature sensor 3.
        public short TemperatureSensor4;       // byte [207:206] Current temperature reported by temperature sensor 4.
        public short TemperatureSensor5;       // byte [209:208] Current temperature reported by temperature sensor 5.
        public short TemperatureSensor6;       // byte [211:210] Current temperature reported by temperature sensor 6.
        public short TemperatureSensor7;       // byte [213:212] Current temperature reported by temperature sensor 7.
        public short TemperatureSensor8;       // byte [215:214] Current temperature reported by temperature sensor 8.

        public int TMT1TransitionCount;      // byte [219:216] Thermal Management Temperature 1 Transition Count <rev1.3>
        public int TMT2TransitionCount;      // byte [223:220] Thermal Management Temperature 2 Transition Count <rev1.3>
        public int TMT1TotalTime;            // byte [227:224] Total Time For Thermal Management Temperature 1 <rev1.3>
        public int TMT2TotalTime;            // byte [231:228] Total Time For Thermal Management Temperature 2 <rev1.3>

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 280)]
        public byte[] Reserved1;           // byte [511:232]
    }
}
