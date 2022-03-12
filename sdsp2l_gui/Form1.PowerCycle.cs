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
        // PL2303 relative import
        [DllImport("nvmeio.dll")]
        static extern int iScanUartController(IntPtr portName);
        [DllImport("nvmeio.dll")]
        static extern void vEnableRelayUsedGpio();
        [DllImport("nvmeio.dll")]
        static extern void vDisableRelayUsedGpio();
        [DllImport("nvmeio.dll")]
        static extern int iGetGpioValue(int gpioIdx, IntPtr val);
        [DllImport("nvmeio.dll")]
        static extern int iSetGpioValue(int gpioIdx, char val);

        private static SynchronizationContext mainThreadSynContext;
        private static sPowerCycleTestConfig sPowerCycleTestCfg;
        private static sPowerCycleTestStatus sPowCycleTestStatus;

        private void vInitPowerCycleStatus() {
            sPowCycleTestStatus.bSTOP = false;
            sPowCycleTestStatus.iLOOP = 0;
            sPowCycleTestStatus.iLASTLOOP = 0;
            sPowCycleTestStatus.iTESTTIME = 0;  // reset timer
            sPowCycleTestStatus.eSTATE = ePowerCycleState.E_IDLE;
            sPowCycleTestStatus.eLASTSTATE = ePowerCycleState.E_IDLE;
        }

        private void watcher_EventArrived(object sender, EventArrivedEventArgs e)
        {
            mainThreadSynContext.Post(new SendOrPostCallback(OnConnected), e);//通知主線程
        }

        private void OnConnected(object state)//由於是主線程的同步對象Post調用，這個是在主線程中執行的
        {
            if (i == 1) return;
            else i = 1;
            //這裏就回到了主線程裏面了
            string pl2302_name = GetFullComputerDevices();
            bool result = pl2302_name.Equals("No COM Port device recognized");
            if (!result)
            {
                textBoxStatus.AppendText("  " + DateTime.Now + " : OnConnected : " + pl2302_name + Environment.NewLine);
            }
        }

        public string GetFullComputerDevices()
        {
            ManagementClass processClass = new ManagementClass("Win32_PnPEntity");
            ManagementObjectCollection Ports = processClass.GetInstances();
            string ComPort_number = "No COM Port device recognized";

            foreach (ManagementObject property in Ports)
            {
                if ((property.GetPropertyValue("Name") != null) &&
                    (property.GetPropertyValue("Name").ToString().Contains("Prolific USB-to-Serial Comm Port")))  //以顯示名稱為Prolific USB-to-Serial Comm Port的UART轉USB晶片PL2303為基底的裝置為例
                {
                    ComPort_number = property.GetPropertyValue("Name").ToString();
                    //ComPort_number = ComPort_number.Substring(ComPort_number.IndexOf("COM")).TrimEnd(')'); //取得COMx的字串並將其放置到ComPort_number這個string變數
                }
            }

            return ComPort_number; //回傳字串，如果都沒有就會回傳預設的"No COM Port device recognized"字串
        }

        public string GetComportName(string fullName)
        {
            return fullName.Substring(fullName.IndexOf("COM")).TrimEnd(')'); //取得COMx的字串並將其放置到ComPort_number這個string變數
        }

        private void vInitComController()
        {
            sPowerCycleTestCfg.bCOM_EXIST = false;

            ManagementEventWatcher watcher = new ManagementEventWatcher();
            WqlEventQuery query = new WqlEventQuery("SELECT * FROM Win32_DeviceChangeEvent WHERE EventType = 2");
            //EventType = 1是對insert和remove都有反應; 
            //EventType = 2是只有對insert有反應; 
            //EventType = 3是只有對remove有反應
            //以上可以視情況宣告為全域instances 方便一些應用b

            mainThreadSynContext = SynchronizationContext.Current; //在這裏記錄主線程的上下文

            watcher.EventArrived += new EventArrivedEventHandler(watcher_EventArrived);
            watcher.Query = query;
            watcher.Start();
            //如果要停掉watcher就呼叫watcher.Stop(); 不用時可以dispose()掉
            //注意上述這些初始動作要在serial port init之前

            string comportName = GetFullComputerDevices();
            bool result = comportName.Equals("No COM Port device recognized");
            if (!result)
            {
                // get the device name and create file
                comportName = GetComportName(comportName); // get the trimed name, only COMX
                IntPtr comName = (IntPtr)Marshal.StringToHGlobalAnsi(comportName);
                int iResult = iScanUartController(comName);
                Marshal.FreeHGlobal(comName);

                // enable all the gpio of pl2303
                vEnableRelayUsedGpio();

                sPowerCycleTestCfg.bCOM_EXIST = true; // set the global flag to true
                textBoxStatus.AppendText("  " + DateTime.Now + " : PL2302 Controller Connected : " + comportName + ", CreateFile Result : " + iResult + Environment.NewLine);
            }
            else
            {
                sPowerCycleTestCfg.bCOM_EXIST = false;
                textBoxStatus.AppendText("  " + DateTime.Now + " : No COM Port device recognized" + Environment.NewLine);
            }
        }

        private string sWriteSizeIdxToStr(eWriteSize idx) {
            string writeSize;

            switch (idx) {
                case eWriteSize.E_512BYTES:
                    writeSize = "512Bytes";
                    break;
                case eWriteSize.E_4KB:
                    writeSize = "4KB";
                    break;
                case eWriteSize.E_16KB:
                    writeSize = "16KB";
                    break;
                case eWriteSize.E_32KB:
                    writeSize = "32KB";
                    break;
                case eWriteSize.E_64KB:
                    writeSize = "64KB";
                    break;
                case eWriteSize.E_128KB:
                    writeSize = "128KB";
                    break;
                case eWriteSize.E_MIX_WRITESIZE:
                    writeSize = "MIX";
                    break;
                default:
                    writeSize = "512Bytes";
                    break;
            }

            return writeSize;
        }

        private void vInitPowerCycleTestVal()
        {
            // Init power cycle test configuration structure
            sPowerCycleTestCfg = new sPowerCycleTestConfig();

            // Write Size
            trackBar_writeSize.Value = (int)eWriteSize.E_MIX_WRITESIZE; // Default set to mix write size;
            textBox_writeSize.Text = sWriteSizeIdxToStr(eWriteSize.E_MIX_WRITESIZE);
            sPowerCycleTestCfg.iWRITE_SIZE = (int)eWriteSize.E_MIX_WRITESIZE;

            // Test Type initialize
            testTypeBox1.Items.Add("UGSD01: Basic UGSD test");
            //testTypeBox1.Items.Add("GSD/POR");
            testTypeBox1.SelectedIndex = 0;
            sPowerCycleTestCfg.iTEST_TYPE = 0;

            // Workload type
            workload_combobox.Items.Add("Write Only");
            workload_combobox.Items.Add("Read Write Mixed");
            workload_combobox.SelectedIndex = 0;
            sPowerCycleTestCfg.iWORK_LOAD = 0;

            // prefill
            checkbox_prefill.Checked = false;
            sPowerCycleTestCfg.bPREFILL = false;

            // output log file
            checkBox_output_log.Checked = true;
            sPowerCycleTestCfg.bOUTPUT_LOG = true;

            // rebuild timeout
            textBox_rebuildTimeout.Text = "10";
            sPowerCycleTestCfg.iREBUILD_TO = 10;

            // power off intervall
            textBox_powerOffTime.Text = "5";
            sPowerCycleTestCfg.iPOWOFF_IDLE_TIME = 5;

            // power on intervall
            textBox_powerOnTime.Text = "10";
            sPowerCycleTestCfg.iREMOVE_PWR_TIME = 10;

            // Queue depth
            textbox_qdepth.Text = "64";
            sPowerCycleTestCfg.iQUEUE_DEPTH = 64;

            // Test Loop
            textbox_testloop.Text = "1024";
            sPowerCycleTestCfg.iTEST_LOOP = 1024;

            // init powercycle test form 
            label_testTime.Text = " ";

            startTestBtn.Text = "START";
            startTestBtn.Image = Properties.Resources.power_64;

        }

        byte[] outBuffer = new byte[1024 * 512];
        byte[] inBuffer = new byte[1024 * 512];
        /*
         * this script is sequencial write spor test
         */
        private int iScript_UGSD01() {
            
            int curLba = 0, preLba = 0; // lba information for check
            int transWrap = 10;  // write data size
            int sectorCnt = 512;
            bool bExist = true;
            int iResult = 0;

            sPowCycleTestStatus.eSTATE = ePowerCycleState.E_INIT;

            // create and open the log file
            if (sPowerCycleTestCfg.bOUTPUT_LOG) {
                vCreateLogFile();
            }

            if (sPowerCycleTestCfg.bOUTPUT_LOG)
            {
                vWriteLogFile("UGSD01 Sript, sequential read write spor test");
            }

            sPowCycleTestStatus.eSTATE = ePowerCycleState.E_SCAN_DEVICE;

            iResult = iRescanDevice();
            bExist = false;
            if (iResult == 0)
                bExist = bRescanNVMeDevice(iCurrentDevIdx);

            if (bExist == false) { // device not exist after power cycle  
                if (sPowerCycleTestCfg.bOUTPUT_LOG)
                    vWriteLogFile("UGSD01 Sript, Device Do not exist");
                sPowCycleTestStatus.eSTATE = ePowerCycleState.E_TEST_FAIL;
                sPowCycleTestStatus.eFAILCODE = eFailCode.E_DEVICE_LOST;
                sPowCycleTestStatus.bSTOP = true;
            }

            while (true) {
                // terminate the test
                if (sPowCycleTestStatus.bSTOP == true) {
                    break;
                }
                sPowCycleTestStatus.iLOOP++;
                vWriteLogFile("UGSD01 Sript, Loop count: " + sPowCycleTestStatus.iLOOP.ToString());
                // stage 1 - write data to nand
                sPowCycleTestStatus.eSTATE = ePowerCycleState.E_WRITE;

                // wrap arount write lba once the transWrap is 0
                if ((--transWrap) == 0) {
                    curLba = 0;
                }

                preLba = curLba;

                for (int lba = curLba; lba < (curLba + sectorCnt); lba++) {
                    int offset = (lba - curLba) * 512;
                    outBuffer[offset] = (byte)(lba&0xFF);
                    outBuffer[offset + 1] = (byte)((lba>>8) & 0xFF);
                    outBuffer[offset + 2] = (byte)((lba>>16) & 0xFF);
                    outBuffer[offset + 3] = (byte)((lba>>24) & 0xFF);
                }

                IntPtr dataPtrWrite = Marshal.AllocHGlobal(512 * sectorCnt); 
                Marshal.Copy(outBuffer, 0, dataPtrWrite, 512 * sectorCnt);
                iResult = iNVMeWrite(iCurrentDevIdx, curLba, sectorCnt, dataPtrWrite);
                vWriteLogFile("UGSD01 Sript, write complete, result: " + iResult);
                // write fail
                if (iResult != 0) { 
                    // TBD
                }

                Marshal.FreeHGlobal(dataPtrWrite);
                Thread.Sleep(sPowerCycleTestCfg.iREMOVE_PWR_TIME * 1000);

                // stage 2 - remove the device power
                sPowCycleTestStatus.eSTATE = ePowerCycleState.E_POWER_OFF;
                iSetGpioValue(2, (char)0);
                vWriteLogFile("UGSD01 Sript, remove power");
                Thread.Sleep(sPowerCycleTestCfg.iPOWOFF_IDLE_TIME * 1000);

                sPowCycleTestStatus.eSTATE = ePowerCycleState.E_SCAN_DEVICE;
                iResult = iRescanDevice();
                
                if (iResult == 0)
                    bExist = bRescanNVMeDevice(iCurrentDevIdx);
                vWriteLogFile("UGSD01 Sript, scan device, result: " + iResult.ToString());

                if ((bExist) || (iResult == 1)) 
                {
                    //textBoxStatus.AppendText(DateTime.Now + " : Rescan Device Exist: " + bExist + Environment.NewLine);
                    if (sPowerCycleTestCfg.bOUTPUT_LOG)
                        vWriteLogFile("UGSD01 Sript, Rescan Device exist, expeted not exist");
                    sPowCycleTestStatus.eFAILCODE = eFailCode.E_DEVICE_EXIST;
                    sPowCycleTestStatus.bSTOP = true;
                    sPowCycleTestStatus.eSTATE = ePowerCycleState.E_TEST_FAIL;
                    break;
                }

                // stage 3 - power on the device 
                sPowCycleTestStatus.eSTATE = ePowerCycleState.E_POWER_ON;
                iSetGpioValue(2, (char)1);
                vWriteLogFile("UGSD01 Sript, power on device");
                if (sPowCycleTestStatus.bSTOP == true)
                {
                    break;
                }

                Thread.Sleep(sPowerCycleTestCfg.iREBUILD_TO * 1000);

                sPowCycleTestStatus.eSTATE = ePowerCycleState.E_SCAN_DEVICE;
                iResult = iRescanDevice();
                bExist = false;
                if (iResult == 0)
                    bExist = bRescanNVMeDevice(iCurrentDevIdx);
                vWriteLogFile("UGSD01 Sript, scan device, result:" + bExist.ToString());

                // device not exist after power cycle
                if (bExist == false) {   
                    //textBoxStatus.AppendText(DateTime.Now + " : Rescan Device Do not exist: " + bExist + Environment.NewLine);
                    if (sPowerCycleTestCfg.bOUTPUT_LOG)
                        vWriteLogFile("UGSD01 Sript, Rescan Device Do not exist");
                    sPowCycleTestStatus.bSTOP = true;
                    sPowCycleTestStatus.eFAILCODE = eFailCode.E_DEVICE_LOST;
                    sPowCycleTestStatus.eSTATE = ePowerCycleState.E_TEST_FAIL;
                    break;
                }

                if (sPowCycleTestStatus.bSTOP == true)
                {
                    break;
                }

                // stage 4 - check the data integrity 
                sPowCycleTestStatus.eSTATE = ePowerCycleState.E_VERIFY;
                IntPtr dataPtrRead = Marshal.AllocHGlobal(512 * sectorCnt);
                iResult = iNVMeRead(iCurrentDevIdx, preLba, sectorCnt, dataPtrRead);
                vWriteLogFile("UGSD01 Sript, read verify data, result:" + iResult.ToString());
                if (iResult != 0) { 
                    // TBD
                }

                Marshal.Copy(dataPtrRead, inBuffer, 0, 512 * sectorCnt);
                Marshal.FreeHGlobal(dataPtrRead);

                for (int lba = preLba; lba < (preLba + sectorCnt); lba++)
                {
                    int offset = (lba - preLba) * 512;

                    if ((inBuffer[offset] == (byte)(lba & 0xFF)) &&
                       (inBuffer[offset + 1] == (byte)((lba >> 8) & 0xFF)) &&
                       (inBuffer[offset + 2] == (byte)((lba >> 16) & 0xFF)) &&
                       (inBuffer[offset + 3] == (byte)((lba >> 24) & 0xFF)))
                    {


                    }
                    else {
                        if (sPowerCycleTestCfg.bOUTPUT_LOG)
                        {
                            vWriteLogFile("UGSD01 Sript, Compare LBA fail, expected: " + lba.ToString() + ", read: " +
                            inBuffer[offset + 3].ToString() + inBuffer[offset + 2].ToString() + inBuffer[offset + 1].ToString() + inBuffer[offset].ToString());
                        }
                        sPowCycleTestStatus.bSTOP = true;
                        sPowCycleTestStatus.eSTATE = ePowerCycleState.E_TEST_FAIL;
                        sPowCycleTestStatus.eFAILCODE = eFailCode.E_MIS_COMPARE;
                        break;
                    }

                }

                if (sPowCycleTestStatus.iLOOP >= sPowerCycleTestCfg.iTEST_LOOP) {
                    // test completed
                    vWriteLogFile("Power Cycle Test Success!!");
                    sPowCycleTestStatus.bSTOP = true;
                    sPowCycleTestStatus.eSTATE = ePowerCycleState.E_TEST_PASS;
                    break;
                }
            } // while(true)
            
            if (sPowerCycleTestCfg.bOUTPUT_LOG) {
                vWriteLogFile("Power Cycle Test Completed");
                vCloseLogFile();
            }
            return 0;
        }

        private int iTestScript_GSD01() {

            return 0;
        }
    }
}
