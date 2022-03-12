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
    public partial class Form1 : Form
    {
        private int i;


        public Form1()
        {
            i = 0;

            InitializeComponent();
            InitializeNVMeDev();
            InitIdentifyData();

            vInitPowerCycleTestVal();
            vInitComController();

            // SMART 
            InitSmartListView();
        }

        private void textBoxStatus_TextChanged(object sender, EventArgs e)
        {

        }

        private void deviceListBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            iCurrentDevIdx = deviceListBox.SelectedIndex;
            Object selectedItem = deviceListBox.SelectedItem;

            if (iAvailDevCnt == 0)
            {
                //textBoxStatus.AppendText(DateTime.Now + selectedItem.ToString() + Environment.NewLine);
            }
            else
            {
                textBoxStatus.AppendText("  " + "(" + iCurrentDevIdx.ToString() + ") " + selectedItem.ToString() + Environment.NewLine);

                IntPtr pSn = Marshal.AllocHGlobal(21);
                IntPtr pFwVer = Marshal.AllocHGlobal(9);
                IntPtr pRevision = Marshal.AllocHGlobal(16);

                iGetNVMeDevSn(iCurrentDevIdx, pSn);
                iGetNVMeDevFwVer(iCurrentDevIdx, pFwVer);
                //iGetNVMeDevRevision(iCurrentDevIdx, pRevision);
                GetDeviceIdentifyData(iCurrentDevIdx);

                //textBoxStatus.AppendText("SerialNumber (SN): " + Marshal.PtrToStringAnsi(pSn) + Environment.NewLine);
                //textBoxStatus.AppendText("Firmware Revision (FR): " + Marshal.PtrToStringAnsi(pFwVer) + Environment.NewLine);
                //textBoxStatus.AppendText("NVMe Version (VER): " + Marshal.PtrToStringAnsi(pRevision) + Environment.NewLine);

                textBoxStatus.AppendText("  " + "SerialNumber (SN): " + (new string(idContData.SN)) + Environment.NewLine);
                textBoxStatus.AppendText("  " + "Firmware Revision (FR): " + (new string(idContData.FR)) + Environment.NewLine);
                //textBoxStatus.AppendText("NVMe Version (VER): " + Marshal.PtrToStringAnsi(pRevision) + Environment.NewLine);

                Marshal.FreeHGlobal(pSn);
                Marshal.FreeHGlobal(pFwVer);
                Marshal.FreeHGlobal(pRevision);

                CreateAndShowSmartInfo();
            }
        }

        private void tabControl1_SelectedIndexChanged(object sender, EventArgs e)
        {
            //textBoxStatus.AppendText(DateTime.Now + " : tabControl1_SelectedIndexChanged" + Environment.NewLine);
        }

        private void button1_Click(object sender, EventArgs e)
        {

        }

        private void Form1_Load(object sender, EventArgs e)
        {

        }

        public void StartForm() {
            //Application.Run(new FormSplashScreen());
        }

        // Timer 1 sec callback
        private void timer1_Tick(object sender, EventArgs e)
        {
            // counting the time elapesed
            if ((sPowCycleTestStatus.eSTATE != ePowerCycleState.E_IDLE) &&
                (sPowCycleTestStatus.eSTATE != ePowerCycleState.E_TEST_FAIL) &&
                (sPowCycleTestStatus.eSTATE != ePowerCycleState.E_TEST_PASS))
            {
                sPowCycleTestStatus.iTESTTIME++;
                int hour = sPowCycleTestStatus.iTESTTIME / 3600;
                int min = (sPowCycleTestStatus.iTESTTIME % 3600) / 60;
                int sec = (sPowCycleTestStatus.iTESTTIME % 60);
                string string1 =  "Loop : " + sPowCycleTestStatus.iLOOP + Environment.NewLine + "Time : " + hour + "H : " + min + "M :" + sec + "S";
                label_testTime.Text = string1;
            }
        }

        // 限制只能輸入數字
        private void testLoop_keypress(object sender, KeyPressEventArgs e)
        {
            if (((int)e.KeyChar < 48 | (int)e.KeyChar > 57) & (int)e.KeyChar != 8)
            {
                e.Handled = true;
            }
        }

        private void powerOfint_keypress(object sender, KeyPressEventArgs e)
        {
            if (((int)e.KeyChar < 48 | (int)e.KeyChar > 57) & (int)e.KeyChar != 8)
            {
                e.Handled = true;
            }
        }

        private void powerOff_keypress(object sender, KeyPressEventArgs e)
        {
            if (((int)e.KeyChar < 48 | (int)e.KeyChar > 57) & (int)e.KeyChar != 8)
            {
                e.Handled = true;
            }
        }

        private void prefill_checkbox(object sender, EventArgs e)
        {
            textBoxStatus.AppendText("  " + DateTime.Now + " : prefill_checkbox: " + checkbox_prefill.Checked + Environment.NewLine);
            
        }

        private void trackBar_writeSize_Scroll(object sender, EventArgs e)
        {
            textBox_writeSize.Text = sWriteSizeIdxToStr((eWriteSize)trackBar_writeSize.Value);
        }

        private void testTypeBox1_SelectedIndexChanged(object sender, EventArgs e)
        {

        }

        private void workload_combobox_SelectedIndexChanged(object sender, EventArgs e)
        {

        }

        private void runPowerCycleTest() {
            int iResult = iScript_UGSD01();
        }
        Thread testThread;
        private void startTestBtn_Click(object sender, EventArgs e)
        {
            //if (startTestBtn.Text.Equals("START"))
            // idle state , start the power cycle test
            if (sPowCycleTestStatus.eSTATE == ePowerCycleState.E_IDLE)
            {
                if (sPowerCycleTestCfg.bCOM_EXIST)
                {
                    // setup powercycle test status
                    vInitPowerCycleStatus();

                    startTestBtn.Text = "STOP";
                    startTestBtn.Image = Properties.Resources.power_off_64;
                    testThread = new Thread(runPowerCycleTest);
                    testThread.Start();

                    textBox_powerOffTime.Enabled = false;
                    textbox_testloop.Enabled = false;
                    textBox_powerOnTime.Enabled = false;
                    textBox_rebuildTimeout.Enabled = false;
                    testTypeBox1.Enabled = false;
                    checkBox_output_log.Enabled = false;

                    pictureBox_pass.Visible = false;
                    pictureBox_fail.Visible = false;

                    textBoxStatus.AppendText("  " + DateTime.Now + " : Start power cycle test" + Environment.NewLine);
                }
                else
                { // COM device doesn't exist
                    textBoxStatus.AppendText("  " + DateTime.Now + " : No COM port device recognized" + Environment.NewLine);
                }
            }
            else
            {
                // TBD, use thread to stop the test
                sPowCycleTestStatus.bSTOP = true;   // request stop test
                sPowCycleTestStatus.iTESTTIME = 0;  // reset timer
                textBoxStatus.AppendText("  " + DateTime.Now + " : Wait for the test terminated" + Environment.NewLine);
                testThread.Join();
                startTestBtn.Text = "START";
                startTestBtn.Image = Properties.Resources.power_64;

                sPowCycleTestStatus.eSTATE = ePowerCycleState.E_IDLE;
                sPowCycleTestStatus.eLASTSTATE = ePowerCycleState.E_IDLE;

                textBox_powerOffTime.Enabled = true;
                textbox_testloop.Enabled = true;
                textBox_powerOnTime.Enabled = true;
                textBox_rebuildTimeout.Enabled = true;
                testTypeBox1.Enabled = true;
                checkBox_output_log.Enabled = true;

                textBoxStatus.AppendText("  " + DateTime.Now + " : Stop power cycle test" + Environment.NewLine);
            }
        }

        private void button2_Click(object sender, EventArgs e)
        {
            iSetGpioValue(0, (char)1);
            textBoxStatus.AppendText("  " + DateTime.Now + " : Set GPIO 0 high" + Environment.NewLine);
        }

        private void button4_Click(object sender, EventArgs e)
        {
            iSetGpioValue(0, (char)0);
            textBoxStatus.AppendText("  " + DateTime.Now + " : Set GPIO 0 low" + Environment.NewLine);
        }

        private void button5_Click(object sender, EventArgs e)
        {
            iSetGpioValue(1, (char)1);
            textBoxStatus.AppendText("  " + DateTime.Now + " : Set GPIO 1 high" + Environment.NewLine);
        }

        private void button3_Click(object sender, EventArgs e)
        {
            iSetGpioValue(1, (char)0);
            textBoxStatus.AppendText("  " + DateTime.Now + " : Set GPIO 1 low" + Environment.NewLine);
        }

        private void button6_Click(object sender, EventArgs e)
        {
            iSetGpioValue(2, (char)1);
            textBoxStatus.AppendText("  " + DateTime.Now + " : Set GPIO 2 high" + Environment.NewLine);
        }

        private void button7_Click(object sender, EventArgs e)
        {
            iSetGpioValue(2, (char)0);
            textBoxStatus.AppendText("  " + DateTime.Now + " : Set GPIO 2 low : " + Environment.NewLine);
        }

        private void button8_Click(object sender, EventArgs e)
        {
            iSetGpioValue(3, (char)1);
            textBoxStatus.AppendText("  " + DateTime.Now + " : Set GPIO 3 high : " + Environment.NewLine);
        }

        private void button9_Click(object sender, EventArgs e)
        {
            iSetGpioValue(3, (char)0);
            textBoxStatus.AppendText("  " + DateTime.Now + " : Set GPIO 3 low : " + Environment.NewLine);
        }

        private void button1_Click_1(object sender, EventArgs e)
        {
            int sectorCnt = 512;
            int lba = 512;
            char[] databuffer = new char[1024 * 512];
            databuffer[0] = 'K';
            databuffer[1] = 'E';
            databuffer[2] = 'V';
            
            string str = new string(databuffer);
            IntPtr dataPtr = Marshal.StringToHGlobalAnsi(str);
            
            int iResult = iNVMeWrite(iCurrentDevIdx, lba, sectorCnt, dataPtr);
            
            Marshal.FreeHGlobal(dataPtr);
            textBoxStatus.AppendText("  " + DateTime.Now + " : Write API TEST : " + iResult + Environment.NewLine);
        }

        private void button10_Click(object sender, EventArgs e)
        {
            int sectorCnt = 512;
            int lba = 512;
            IntPtr dataPtr = Marshal.AllocHGlobal(1024 * 512);
            int iResult = iNVMeRead(iCurrentDevIdx, lba, sectorCnt, dataPtr);
            string str = Marshal.PtrToStringAnsi(dataPtr);
            Marshal.FreeHGlobal(dataPtr);
            textBoxStatus.AppendText("  " + DateTime.Now + " : Read API TEST : " + iResult + Environment.NewLine);
            textBoxStatus.AppendText("  " + DateTime.Now + " : " + str + Environment.NewLine);
        }

        private void rescan_device_Click(object sender, EventArgs e)
        {
            int iResult = 0;
            iResult = iRescanDevice();
            if(iResult == 0)
            {
                bool bExist = bRescanNVMeDevice(iCurrentDevIdx);
                if (bExist)
                {
                    textBoxStatus.AppendText("  " + DateTime.Now + " : Rescan Device Exist: " + bExist + Environment.NewLine);

                }
                else { // device not exist after power cycle  
                    textBoxStatus.AppendText("  " + DateTime.Now + " : Rescan Device Do not exist: " + bExist + Environment.NewLine);
                }
            }
            //textBoxStatus.AppendText(DateTime.Now + " : Rescan Device : " + iResult + Environment.NewLine);
        }

        private void timer_300mis_Tick(object sender, EventArgs e)
        {
            // show information once the test state changed
            if (sPowCycleTestStatus.eSTATE != sPowCycleTestStatus.eLASTSTATE)
            {
                // show loop count
                if (sPowCycleTestStatus.iLASTLOOP != sPowCycleTestStatus.iLOOP)
                {
                    textBoxStatus.AppendText("  " + DateTime.Now + " : Loop: " + sPowCycleTestStatus.iLOOP + Environment.NewLine);
                    sPowCycleTestStatus.iLASTLOOP = sPowCycleTestStatus.iLOOP;
                }

                switch (sPowCycleTestStatus.eSTATE) {
                    case ePowerCycleState.E_IDLE:
                        //textBoxStatus.AppendText(DateTime.Now + ", Test idle" + Environment.NewLine);
                        break;

                    case ePowerCycleState.E_TEST_FAIL:
                        sPowCycleTestStatus.iTESTTIME = 0;  // reset timer
                        startTestBtn.Text = "START";
                        startTestBtn.Image = Properties.Resources.power_64;
                        string fail_msg = "";
                        switch (sPowCycleTestStatus.eFAILCODE) {
                            case eFailCode.E_DEVICE_EXIST:
                                fail_msg = "Device exist in power off state, remove power fail";
                                break;
                            case eFailCode.E_DEVICE_LOST:
                                fail_msg = "Device lost";
                                break;
                            case eFailCode.E_MIS_COMPARE:
                                fail_msg = "Data miscompare";
                                break;
                            case eFailCode.E_READ_FAIL:
                                fail_msg = "Data read fail";
                                break;
                            case eFailCode.E_WRITE_FAIL:
                                fail_msg = "Data write fail";
                                break;
                            default:
                                fail_msg = "Fail type is unknown";
                                break;
                        }
                        testThread.Join();
                        //MessageBox.Show("Power cycle test fail!! " + Environment.NewLine + fail_msg);

                        textBoxStatus.AppendText("  " + DateTime.Now + " : Power cycle test fail!! " + fail_msg + ", loop count: " + sPowCycleTestStatus.iLOOP + Environment.NewLine);
                        
                        sPowCycleTestStatus.eLASTSTATE = sPowCycleTestStatus.eSTATE;
                        sPowCycleTestStatus.eSTATE = ePowerCycleState.E_IDLE;
                        pictureBox_fail.Visible = true;

                        textBox_powerOffTime.Enabled = true;
                        textbox_testloop.Enabled = true;
                        textBox_powerOnTime.Enabled = true;
                        textBox_rebuildTimeout.Enabled = true;
                        testTypeBox1.Enabled = true;
                        checkBox_output_log.Enabled = true;

                        return;

                    case ePowerCycleState.E_TEST_PASS:
                        sPowCycleTestStatus.iTESTTIME = 0;  // reset timer
                        startTestBtn.Text = "START";
                        startTestBtn.Image = Properties.Resources.power_64;
   
                        textBoxStatus.AppendText("  " + DateTime.Now + " : Test pass, total loop count: " + sPowCycleTestStatus.iLOOP + Environment.NewLine);
                        testThread.Join();
                        //MessageBox.Show("Power cycle test pass!!");
                        sPowCycleTestStatus.eLASTSTATE = sPowCycleTestStatus.eSTATE;
                        sPowCycleTestStatus.eSTATE = ePowerCycleState.E_IDLE;
                        pictureBox_pass.Visible = true;

                        textBox_powerOffTime.Enabled = true;
                        textbox_testloop.Enabled = true;
                        textBox_powerOnTime.Enabled = true;
                        textBox_rebuildTimeout.Enabled = true;
                        testTypeBox1.Enabled = true;
                        checkBox_output_log.Enabled = true;
                        return;

                    case ePowerCycleState.E_INIT:
                        textBoxStatus.AppendText("  " + DateTime.Now + " : Test configuration initialization" + Environment.NewLine);
                        break;

                    case ePowerCycleState.E_POWER_OFF:
                        textBoxStatus.AppendText("  " + DateTime.Now + " : Remove device power" + Environment.NewLine);
                        break;

                    case ePowerCycleState.E_POWER_ON:
                        textBoxStatus.AppendText("  " + DateTime.Now + " : Resume the device power" + Environment.NewLine);
                        break;

                    case ePowerCycleState.E_SCAN_DEVICE:
                        textBoxStatus.AppendText("  " + DateTime.Now + " : Scan device" + Environment.NewLine);
                        break;

                    case ePowerCycleState.E_VERIFY:
                        textBoxStatus.AppendText("  " + DateTime.Now + " : Verify data" + Environment.NewLine);
                        break;

                    case ePowerCycleState.E_WRITE:
                        textBoxStatus.AppendText("  " + DateTime.Now + " : Write data" + Environment.NewLine);
                        break;

                    default:
                        break;
                }
                //textBoxStatus.AppendText(DateTime.Now + ", Last Sts:" + sPowCycleTestStatus.eLASTSTATE + ", Sts: "+ sPowCycleTestStatus.eSTATE + Environment.NewLine);
                sPowCycleTestStatus.eLASTSTATE = sPowCycleTestStatus.eSTATE;
            }
        }

        private void textBox_powerOffTime_TextChanged(object sender, EventArgs e)
        {
            if (textBox_powerOffTime.Text != "")
            {
                sPowerCycleTestCfg.iREMOVE_PWR_TIME = Convert.ToInt32(textBox_powerOffTime.Text);
            }
        }

        private void tabSMART_Enter(object sender, EventArgs e)
        {
            //MessageBox.Show("SMART Tab Select");
            CreateAndShowSmartInfo();   // create and show the smart info of current selected drive
        }

        private void tabSMART_Click(object sender, EventArgs e)
        {

        }

        private void textbox_testloop_TextChanged(object sender, EventArgs e)
        {
            if (textbox_testloop.Text != "")
            {
                sPowerCycleTestCfg.iTEST_LOOP = Convert.ToInt32(textbox_testloop.Text);
            }
        }

        private void textBoxFID_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (((int)e.KeyChar < 48 | (int)e.KeyChar > 57) & ((int)e.KeyChar != 8))
            {
                e.Handled = true;
            }
        }

        private void textBoxSEL_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (((int)e.KeyChar < 48 | (int)e.KeyChar > 57) & ((int)e.KeyChar != 8))
            {
                e.Handled = true;
            }
        }

        private void textBoxDW11_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (((int)e.KeyChar < 48 | (int)e.KeyChar > 57) & 
                ((int)e.KeyChar < 65 | (int)e.KeyChar > 70) & 
                ((int)e.KeyChar < 97 | (int)e.KeyChar > 102) & 
                ((int)e.KeyChar != 8) & ((int)e.KeyChar != 120))
            {
                e.Handled = true;
            }
        }

        private void btnSetFeature_Click(object sender, EventArgs e)
        {
            if (iAvailDevCnt == 0) {
                return;
            }

            if (textBoxSEL.Text != "")
                sel_save = Convert.ToInt32(textBoxSEL.Text);
            if (textBoxDW11.Text != "")
            {
                //dw11 = Convert.ToInt32(textBoxDW11.Text);
                String input = textBoxDW11.Text.ToString();

                // check the fromat of input data
                if (input.Length > 1) {
                    if ((input.Substring(0, 2) == "0x") || (input.Substring(0, 2) == "0X"))
                    {
                        // hex
                        input = textBoxDW11.Text.ToString().Substring(2);
                        dw11 = int.Parse(input, System.Globalization.NumberStyles.AllowHexSpecifier);
                    }
                    else {
                        // dec
                        dw11 = Convert.ToInt32(textBoxDW11.Text);
                    }
                }
                else {
                    // dec
                    dw11 = Convert.ToInt32(textBoxDW11.Text);
                }
                
            }
            vIssueSetFeatureCmd(featureIdList.SelectedIndex, sel_save, dw11);
        }

        private void btnGetFeature_Click(object sender, EventArgs e)
        {
            if (iAvailDevCnt == 0)
            {
                return;
            }

            if (textBoxSEL.Text != "")
                sel_save = Convert.ToInt32(textBoxSEL.Text);
            vIssueGetFeatureCmd(featureIdList.SelectedIndex, sel_save);
        }

        private void btnClearOutput_Click(object sender, EventArgs e)
        {
            textBoxStatus.Clear();
        }

        private void textBox_powerOnTime_TextChanged(object sender, EventArgs e)
        {
            if (textBox_powerOnTime.Text != "")
            {
                sPowerCycleTestCfg.iPOWOFF_IDLE_TIME = Convert.ToInt32(textBox_powerOnTime.Text);
            }
        }

        private void textBox_rebuildTimeout_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (((int)e.KeyChar < 48 | (int)e.KeyChar > 57) & (int)e.KeyChar != 8)
            {
                e.Handled = true;
            }
        }

        private void textBox_rebuildTimeout_TextChanged(object sender, EventArgs e)
        {
            if (textBox_rebuildTimeout.Text != "")
            {
                sPowerCycleTestCfg.iREBUILD_TO = Convert.ToInt32(textBox_rebuildTimeout.Text);
            }
        }

        private void button11_Click(object sender, EventArgs e)
        {
            startTestBtn.Text = "START";
            startTestBtn.Image = Properties.Resources.power_64;
        }

        private void button12_Click(object sender, EventArgs e)
        {
            startTestBtn.Text = "STOP";
            startTestBtn.Image = Properties.Resources.power_off_64;
        }

        private void powerCycleTab_Click(object sender, EventArgs e)
        {

        }

        private void checkBox_output_log_CheckedChanged(object sender, EventArgs e)
        {
            if (checkBox_output_log.Checked) {
                textBoxStatus.AppendText("  " + DateTime.Now + ": Create log file" + Environment.NewLine);
                sPowerCycleTestCfg.bOUTPUT_LOG = true;
            }
            else {
                textBoxStatus.AppendText("  " + DateTime.Now + ": Do not create log file" + Environment.NewLine);
                sPowerCycleTestCfg.bOUTPUT_LOG = false;
            }
            
        }
    }
}
