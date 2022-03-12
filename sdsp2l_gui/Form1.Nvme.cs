using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using System.Threading;
using System.Management;

namespace Xpacer
{
    partial class Form1
    {
        // NVMe relative import
        [DllImport("nvmeio.dll")]
        static extern int testdll(int x);
        [DllImport("nvmeio.dll")]
        static extern int iScanAndGetAvailDeviceList(bool alloc);
        [DllImport("nvmeio.dll")]
        static extern int iGetNVMeDevName(int idx, IntPtr pDevName);
        [DllImport("nvmeio.dll")]
        static extern int iGetNVMeDevSn(int idx, IntPtr pSn);
        [DllImport("nvmeio.dll")]
        static extern int iGetNVMeDevFwVer(int idx, IntPtr pFwVer);
        [DllImport("nvmeio.dll")]
        static extern int iGetNVMeDevRevision(int idx, IntPtr pRevision);
        [DllImport("nvmeio.dll")]
        static extern int iGetNVMeDevIdentifyData(int idx, IntPtr ptr);
        [DllImport("nvmeio.dll")]
        static extern int iNVMeWrite(int idx, int lba, int sectorCount, IntPtr databuffer);
        [DllImport("nvmeio.dll")]
        static extern int iNVMeRead(int idx, int lba, int sectorCount, IntPtr databuffer);
        [DllImport("nvmeio.dll")]
        static extern int iRescanDevice();
        [DllImport("nvmeio.dll")]
        static extern bool bCheckDevExist(int idx);
        [DllImport("nvmeio.dll")]
        static extern void vReleaseAllDevHandle();


        static private int iAvailDevCnt;    // total device count
        static private int iCurrentDevIdx;  // nvme device index
        static private string[] sDevName;   // nvme device name

        sIdentifyControllerData idContData;

        public void GetDeviceIdentifyData(int idx)
        {
            IntPtr ptr = Marshal.AllocHGlobal(4096);
            int result = iGetNVMeDevIdentifyData(idx, ptr);
            idContData = (sIdentifyControllerData)Marshal.PtrToStructure(ptr, typeof(sIdentifyControllerData));
        }

        public void InitIdentifyData()
        {
            idContData = new sIdentifyControllerData();

            idContData.SN = new char[20];
            idContData.MN = new char[40];
            idContData.FR = new char[8];
            idContData.IEEE = new char[3];
            idContData.Reserved0 = new char[9];

            idContData.FGUID = new char[16];
            idContData.Reserved1 = new char[106];
            idContData.Reserved2 = new char[16];
            idContData.TNVMCAP = new char[16];
            idContData.UNVMCAP = new char[16];

            idContData.Reserved3 = new char[156];
            idContData.Reserved5 = new char[2];
            idContData.Reserved6 = new char[224];
            idContData.SUBNQN = new char[256];
            idContData.Reserved7 = new char[768];

            idContData.Reserved8 = new char[256];
            idContData.NVME_POWER_STATE_DESC = new char[32 * 32];
            idContData.VS = new char[1024];
        }

        private void InitializeNVMeDev()
        {
            // rescan device avoid the device lost
            iRescanDevice();

            iAvailDevCnt = 0;
            iAvailDevCnt = iScanAndGetAvailDeviceList(true);

            // There are ssd device available
            if (iAvailDevCnt == -3)
            {
                textBoxStatus.AppendText("  " + DateTime.Now + " : No available device founded or UAC setting error" + Environment.NewLine);
            }
            else if (iAvailDevCnt != 0)
            {
                IntPtr pDevName = Marshal.AllocHGlobal(41);

                // allocate dev name string buffer    
                sDevName = new string[iAvailDevCnt];
                for (int idx = 0; idx < iAvailDevCnt; idx++)
                {
                    iGetNVMeDevName(idx, pDevName);
                    sDevName[idx] = Marshal.PtrToStringAnsi(pDevName);
                    deviceListBox.Items.Add(sDevName[idx]);
                }
                deviceListBox.SelectedIndex = 0;
                iCurrentDevIdx = 0;

                // Free HGlobal memory
                Marshal.FreeHGlobal(pDevName);

            }
            else
            {
                textBoxStatus.AppendText("  " + DateTime.Now + " : No available nvme device founded" + Environment.NewLine);
                deviceListBox.Items.Add("No Available Device");
                deviceListBox.SelectedIndex = 0;
                iCurrentDevIdx = 0;
            }

        }

        private bool bRescanNVMeDevice(int scanIdx) {
            if (iAvailDevCnt == 0) {
                return false;
            }

            vReleaseAllDevHandle();
            int iAvailNow = iScanAndGetAvailDeviceList(false);

            if (iAvailNow <= scanIdx)
            {
                return false;
            }

            IntPtr pDevName = Marshal.AllocHGlobal(41);
            string strDevName = new string("");
            
            iGetNVMeDevName(scanIdx, pDevName);
            strDevName = Marshal.PtrToStringAnsi(pDevName);
            bool bResult = strDevName.Equals(sDevName[scanIdx]);
            Marshal.FreeHGlobal(pDevName);
            return bResult;
        }


    }
}
