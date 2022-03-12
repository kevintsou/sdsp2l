using System;
using System.Collections.Generic;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;

namespace Xpacer
{
    partial class Form1
    {
        [DllImport("nvmeio.dll")]
        static extern int iGetNVMeSmartInfo(int idx, IntPtr ptr);

        sSmartInfoData sSmartInfo;

        public void InitSmartListView() {
            sSmartInfo = new sSmartInfoData();

            // set view to details
            listViewSmart.View = System.Windows.Forms.View.Details;
            
            ///新增title
            ColumnHeader tittle = new ColumnHeader();
            tittle.Text = "Attributes";
            tittle.Width = 350;
            listViewSmart.Columns.Add(tittle);

            ColumnHeader value = new ColumnHeader();
            value.Text = "Value";
            value.Width = 200;
            listViewSmart.Columns.Add(value);

            //listViewSmart.Columns.Add("Value");//標題2
            //listViewSmart.Columns.Add("title 3");//標題3


            ///增加一筆資料
            //ListViewItem l = new ListViewItem("text");
            //listViewSmart.Items.Add(l);


            //listViewSmart.Items.Add("DATA1");

            //兩個子title  所以要add兩次
            //listViewSmart.Items[1].SubItems.Add("1");
            //listViewSmart.Items[1].SubItems.Add("2");

            //修改
            //listViewSmart.Items[1].SubItems[2].Text = "8";

        }

        private void CreateAndShowSmartInfo()
        {
            if ((tabControl.SelectedTab == tabSMART) && 
                    (iAvailDevCnt != 0))
            {
                ClearSmartListView();
                //MessageBox.Show("Create and show smart info");
                IntPtr ptr = Marshal.AllocHGlobal(8192);
                int result = iGetNVMeSmartInfo(iCurrentDevIdx, ptr);
                sSmartInfo = (sSmartInfoData)Marshal.PtrToStructure(ptr, typeof(sSmartInfoData));
                Marshal.FreeHGlobal(ptr);
                CreateSmartListView();
            }
        }

        public void CreateSmartListView()
        {
            listViewSmart.Items.Add("Critical Warning"); // 0
            
            listViewSmart.Items.Add("Composite Temperature"); // 2:1

            listViewSmart.Items.Add("Available Spare"); // 3
            listViewSmart.Items.Add("Available Spare Threshold"); // 4
            listViewSmart.Items.Add("Percentage Used"); // 5
            listViewSmart.Items.Add("Data Units Read"); // 47:32
            listViewSmart.Items.Add("Data Units Written"); // 63:48
            listViewSmart.Items.Add("Host Read Commands"); // 79:64
            listViewSmart.Items.Add("Host Write Commands"); // 95:80
            listViewSmart.Items.Add("Controller Busy Time"); // 111:96
            listViewSmart.Items.Add("Power Cycles"); // 127:112
            listViewSmart.Items.Add("Power On Hours"); // 143:128
            listViewSmart.Items.Add("Unsafe Shutdowns"); // 159:144

            listViewSmart.Items.Add("Media and Data Integrity Errors"); // 175:160
            listViewSmart.Items.Add("Number of Error Information Log Entries"); // 191:176
            listViewSmart.Items.Add("Warning Composite Temperature Time"); // 195:192
            listViewSmart.Items.Add("Critical Composite Temperature Time"); // 199:196

            listViewSmart.Items.Add("Temperature Sensor 1"); // 201:200
            listViewSmart.Items.Add("Temperature Sensor 2"); // 203:202
            listViewSmart.Items.Add("Temperature Sensor 3"); // 205:204
            listViewSmart.Items.Add("Temperature Sensor 4"); // 207:206
            listViewSmart.Items.Add("Temperature Sensor 5"); // 209:208
            listViewSmart.Items.Add("Temperature Sensor 6"); // 211:210
            listViewSmart.Items.Add("Temperature Sensor 7"); // 213:212
            listViewSmart.Items.Add("Temperature Sensor 8"); // 215:214

            listViewSmart.Items.Add("Thermal Management Temperature 1 Transition Count"); // 219:216
            listViewSmart.Items.Add("Thermal Management Temperature 2 Transition Count"); // 223:220
            listViewSmart.Items.Add("Total Time For Thermal Management Temperature 1");   // 227:224
            listViewSmart.Items.Add("Total Time For Thermal Management Temperature 2");   // 231:228


            listViewSmart.Items[0].SubItems.Add(sSmartInfo.CriticalWarning.ToString());
            listViewSmart.Items[1].SubItems.Add(((sSmartInfo.Temperature[1] << 8) + sSmartInfo.Temperature[0] -273).ToString() + "°C");
            listViewSmart.Items[2].SubItems.Add(sSmartInfo.AvailableSpare.ToString() + "%");
            listViewSmart.Items[3].SubItems.Add(sSmartInfo.AvailableSpareThreshold.ToString() + "%");
            listViewSmart.Items[4].SubItems.Add(sSmartInfo.PercentageUsed.ToString() + "%");


            listViewSmart.Items[5].SubItems.Add(((sSmartInfo.DataUnitRead[0]) +
                                                 (sSmartInfo.DataUnitRead[1] << 8) +
                                                 (sSmartInfo.DataUnitRead[2] << (8 * 1)) +
                                                 (sSmartInfo.DataUnitRead[3] << (8 * 2)) +
                                                 (sSmartInfo.DataUnitRead[4] << (8 * 3)) +
                                                 (sSmartInfo.DataUnitRead[5] << (8 * 4)) +
                                                 (sSmartInfo.DataUnitRead[6] << (8 * 5)) +
                                                 (sSmartInfo.DataUnitRead[7] << (8 * 6)) +
                                                 (sSmartInfo.DataUnitRead[8] << (8 * 7)) +
                                                 (sSmartInfo.DataUnitRead[9] << (8 * 8)) +
                                                 (sSmartInfo.DataUnitRead[10] << (8 * 9)) +
                                                 (sSmartInfo.DataUnitRead[11] << (8 * 10)) +
                                                 (sSmartInfo.DataUnitRead[12] << (8 * 11)) +
                                                 (sSmartInfo.DataUnitRead[13] << (8 * 12)) +
                                                 (sSmartInfo.DataUnitRead[14] << (8 * 13)) +
                                                 (sSmartInfo.DataUnitRead[15] << (8 * 14))
                                                 ).ToString() + " Sectors");

            listViewSmart.Items[6].SubItems.Add(((sSmartInfo.DataUnitWritten[0]) +
                                                 (sSmartInfo.DataUnitWritten[1] << 8) +
                                                 (sSmartInfo.DataUnitWritten[2] << 8 * 1) +
                                                 (sSmartInfo.DataUnitWritten[3] << 8 * 2) +
                                                 (sSmartInfo.DataUnitWritten[4] << 8 * 3) +
                                                 (sSmartInfo.DataUnitWritten[5] << 8 * 4) +
                                                 (sSmartInfo.DataUnitWritten[6] << 8 * 5) +
                                                 (sSmartInfo.DataUnitWritten[7] << 8 * 6) +
                                                 (sSmartInfo.DataUnitWritten[8] << 8 * 7) +
                                                 (sSmartInfo.DataUnitWritten[9] << 8 * 8) +
                                                 (sSmartInfo.DataUnitWritten[10] << 8 * 9) +
                                                 (sSmartInfo.DataUnitWritten[11] << 8 * 10) +
                                                 (sSmartInfo.DataUnitWritten[12] << 8 * 11) +
                                                 (sSmartInfo.DataUnitWritten[13] << 8 * 12) +
                                                 (sSmartInfo.DataUnitWritten[14] << 8 * 13) +
                                                 (sSmartInfo.DataUnitWritten[15] << 8 * 14)
                                                 ).ToString() + " Sectors");

            listViewSmart.Items[7].SubItems.Add(((sSmartInfo.HostReadCommands[0]) +
                                                 (sSmartInfo.HostReadCommands[1] << 8) +
                                                 (sSmartInfo.HostReadCommands[2] << 8 * 1) +
                                                 (sSmartInfo.HostReadCommands[3] << 8 * 2) +
                                                 (sSmartInfo.HostReadCommands[4] << 8 * 3) +
                                                 (sSmartInfo.HostReadCommands[5] << 8 * 4) +
                                                 (sSmartInfo.HostReadCommands[6] << 8 * 5) +
                                                 (sSmartInfo.HostReadCommands[7] << 8 * 6) +
                                                 (sSmartInfo.HostReadCommands[8] << 8 * 7) +
                                                 (sSmartInfo.HostReadCommands[9] << 8 * 8) +
                                                 (sSmartInfo.HostReadCommands[10] << 8 * 9) +
                                                 (sSmartInfo.HostReadCommands[11] << 8 * 10) +
                                                 (sSmartInfo.HostReadCommands[12] << 8 * 11) +
                                                 (sSmartInfo.HostReadCommands[13] << 8 * 12) +
                                                 (sSmartInfo.HostReadCommands[14] << 8 * 13) +
                                                 (sSmartInfo.HostReadCommands[15] << 8 * 14)
                                                 ).ToString() + " Commands");

            listViewSmart.Items[8].SubItems.Add(((sSmartInfo.HostWrittenCommands[0]) +
                                                 (sSmartInfo.HostWrittenCommands[1] << 8) +
                                                 (sSmartInfo.HostWrittenCommands[2] << 8 * 1) +
                                                 (sSmartInfo.HostWrittenCommands[3] << 8 * 2) +
                                                 (sSmartInfo.HostWrittenCommands[4] << 8 * 3) +
                                                 (sSmartInfo.HostWrittenCommands[5] << 8 * 4) +
                                                 (sSmartInfo.HostWrittenCommands[6] << 8 * 5) +
                                                 (sSmartInfo.HostWrittenCommands[7] << 8 * 6) +
                                                 (sSmartInfo.HostWrittenCommands[8] << 8 * 7) +
                                                 (sSmartInfo.HostWrittenCommands[9] << 8 * 8) +
                                                 (sSmartInfo.HostWrittenCommands[10] << 8 * 9) +
                                                 (sSmartInfo.HostWrittenCommands[11] << 8 * 10) +
                                                 (sSmartInfo.HostWrittenCommands[12] << 8 * 11) +
                                                 (sSmartInfo.HostWrittenCommands[13] << 8 * 12) +
                                                 (sSmartInfo.HostWrittenCommands[14] << 8 * 13) +
                                                 (sSmartInfo.HostWrittenCommands[15] << 8 * 14)
                                                 ).ToString() + " Commands");

            listViewSmart.Items[9].SubItems.Add(((sSmartInfo.ControllerBusyTime[0]) +
                                                 (sSmartInfo.ControllerBusyTime[1] << 8) +
                                                 (sSmartInfo.ControllerBusyTime[2] << 8 * 1) +
                                                 (sSmartInfo.ControllerBusyTime[3] << 8 * 2) +
                                                 (sSmartInfo.ControllerBusyTime[4] << 8 * 3) +
                                                 (sSmartInfo.ControllerBusyTime[5] << 8 * 4) +
                                                 (sSmartInfo.ControllerBusyTime[6] << 8 * 5) +
                                                 (sSmartInfo.ControllerBusyTime[7] << 8 * 6) +
                                                 (sSmartInfo.ControllerBusyTime[8] << 8 * 7) +
                                                 (sSmartInfo.ControllerBusyTime[9] << 8 * 8) +
                                                 (sSmartInfo.ControllerBusyTime[10] << 8 * 9) +
                                                 (sSmartInfo.ControllerBusyTime[11] << 8 * 10) +
                                                 (sSmartInfo.ControllerBusyTime[12] << 8 * 11) +
                                                 (sSmartInfo.ControllerBusyTime[13] << 8 * 12) +
                                                 (sSmartInfo.ControllerBusyTime[14] << 8 * 13) +
                                                 (sSmartInfo.ControllerBusyTime[15] << 8 * 14)
                                                 ).ToString() + " Minutes");

            listViewSmart.Items[10].SubItems.Add(((sSmartInfo.PowerCycle[0]) +
                                                 (sSmartInfo.PowerCycle[1] << 8) +
                                                 (sSmartInfo.PowerCycle[2] << 8 * 1) +
                                                 (sSmartInfo.PowerCycle[3] << 8 * 2) +
                                                 (sSmartInfo.PowerCycle[4] << 8 * 3) +
                                                 (sSmartInfo.PowerCycle[5] << 8 * 4) +
                                                 (sSmartInfo.PowerCycle[6] << 8 * 5) +
                                                 (sSmartInfo.PowerCycle[7] << 8 * 6) +
                                                 (sSmartInfo.PowerCycle[8] << 8 * 7) +
                                                 (sSmartInfo.PowerCycle[9] << 8 * 8) +
                                                 (sSmartInfo.PowerCycle[10] << 8 * 9) +
                                                 (sSmartInfo.PowerCycle[11] << 8 * 10) +
                                                 (sSmartInfo.PowerCycle[12] << 8 * 11) +
                                                 (sSmartInfo.PowerCycle[13] << 8 * 12) +
                                                 (sSmartInfo.PowerCycle[14] << 8 * 13) +
                                                 (sSmartInfo.PowerCycle[15] << 8 * 14)
                                                 ).ToString());

            listViewSmart.Items[11].SubItems.Add(((sSmartInfo.PowerOnHours[0]) +
                                                 (sSmartInfo.PowerOnHours[1] << 8) +
                                                 (sSmartInfo.PowerOnHours[2] << 8 * 1) +
                                                 (sSmartInfo.PowerOnHours[3] << 8 * 2) +
                                                 (sSmartInfo.PowerOnHours[4] << 8 * 3) +
                                                 (sSmartInfo.PowerOnHours[5] << 8 * 4) +
                                                 (sSmartInfo.PowerOnHours[6] << 8 * 5) +
                                                 (sSmartInfo.PowerOnHours[7] << 8 * 6) +
                                                 (sSmartInfo.PowerOnHours[8] << 8 * 7) +
                                                 (sSmartInfo.PowerOnHours[9] << 8 * 8) +
                                                 (sSmartInfo.PowerOnHours[10] << 8 * 9) +
                                                 (sSmartInfo.PowerOnHours[11] << 8 * 10) +
                                                 (sSmartInfo.PowerOnHours[12] << 8 * 11) +
                                                 (sSmartInfo.PowerOnHours[13] << 8 * 12) +
                                                 (sSmartInfo.PowerOnHours[14] << 8 * 13) +
                                                 (sSmartInfo.PowerOnHours[15] << 8 * 14)
                                                 ).ToString());

            listViewSmart.Items[12].SubItems.Add(((sSmartInfo.UnsafeShutdowns[0]) +
                                                 (sSmartInfo.UnsafeShutdowns[1] << 8) +
                                                 (sSmartInfo.UnsafeShutdowns[2] << 8 * 1) +
                                                 (sSmartInfo.UnsafeShutdowns[3] << 8 * 2) +
                                                 (sSmartInfo.UnsafeShutdowns[4] << 8 * 3) +
                                                 (sSmartInfo.UnsafeShutdowns[5] << 8 * 4) +
                                                 (sSmartInfo.UnsafeShutdowns[6] << 8 * 5) +
                                                 (sSmartInfo.UnsafeShutdowns[7] << 8 * 6) +
                                                 (sSmartInfo.UnsafeShutdowns[8] << 8 * 7) +
                                                 (sSmartInfo.UnsafeShutdowns[9] << 8 * 8) +
                                                 (sSmartInfo.UnsafeShutdowns[10] << 8 * 9) +
                                                 (sSmartInfo.UnsafeShutdowns[11] << 8 * 10) +
                                                 (sSmartInfo.UnsafeShutdowns[12] << 8 * 11) +
                                                 (sSmartInfo.UnsafeShutdowns[13] << 8 * 12) +
                                                 (sSmartInfo.UnsafeShutdowns[14] << 8 * 13) +
                                                 (sSmartInfo.UnsafeShutdowns[15] << 8 * 14)
                                                 ).ToString());

            listViewSmart.Items[13].SubItems.Add(((sSmartInfo.MediaErrors[0]) +
                                                 (sSmartInfo.MediaErrors[1] << 8) +
                                                 (sSmartInfo.MediaErrors[2] << 8 * 1) +
                                                 (sSmartInfo.MediaErrors[3] << 8 * 2) +
                                                 (sSmartInfo.MediaErrors[4] << 8 * 3) +
                                                 (sSmartInfo.MediaErrors[5] << 8 * 4) +
                                                 (sSmartInfo.MediaErrors[6] << 8 * 5) +
                                                 (sSmartInfo.MediaErrors[7] << 8 * 6) +
                                                 (sSmartInfo.MediaErrors[8] << 8 * 7) +
                                                 (sSmartInfo.MediaErrors[9] << 8 * 8) +
                                                 (sSmartInfo.MediaErrors[10] << 8 * 9) +
                                                 (sSmartInfo.MediaErrors[11] << 8 * 10) +
                                                 (sSmartInfo.MediaErrors[12] << 8 * 11) +
                                                 (sSmartInfo.MediaErrors[13] << 8 * 12) +
                                                 (sSmartInfo.MediaErrors[14] << 8 * 13) +
                                                 (sSmartInfo.MediaErrors[15] << 8 * 14)
                                                 ).ToString());

            listViewSmart.Items[14].SubItems.Add(((sSmartInfo.ErrorInfoLogEntryNum[0]) +
                                                 (sSmartInfo.ErrorInfoLogEntryNum[1] << 8) +
                                                 (sSmartInfo.ErrorInfoLogEntryNum[2] << 8 * 1) +
                                                 (sSmartInfo.ErrorInfoLogEntryNum[3] << 8 * 2) +
                                                 (sSmartInfo.ErrorInfoLogEntryNum[4] << 8 * 3) +
                                                 (sSmartInfo.ErrorInfoLogEntryNum[5] << 8 * 4) +
                                                 (sSmartInfo.ErrorInfoLogEntryNum[6] << 8 * 5) +
                                                 (sSmartInfo.ErrorInfoLogEntryNum[7] << 8 * 6) +
                                                 (sSmartInfo.ErrorInfoLogEntryNum[8] << 8 * 7) +
                                                 (sSmartInfo.ErrorInfoLogEntryNum[9] << 8 * 8) +
                                                 (sSmartInfo.ErrorInfoLogEntryNum[10] << 8 * 9) +
                                                 (sSmartInfo.ErrorInfoLogEntryNum[11] << 8 * 10) +
                                                 (sSmartInfo.ErrorInfoLogEntryNum[12] << 8 * 11) +
                                                 (sSmartInfo.ErrorInfoLogEntryNum[13] << 8 * 12) +
                                                 (sSmartInfo.ErrorInfoLogEntryNum[14] << 8 * 13) +
                                                 (sSmartInfo.ErrorInfoLogEntryNum[15] << 8 * 14)
                                                 ).ToString());

            listViewSmart.Items[15].SubItems.Add(sSmartInfo.WCTempTime.ToString());
            listViewSmart.Items[16].SubItems.Add(sSmartInfo.CCTempTime.ToString());

            listViewSmart.Items[17].SubItems.Add(sSmartInfo.TemperatureSensor1.ToString());
            listViewSmart.Items[18].SubItems.Add(sSmartInfo.TemperatureSensor2.ToString());
            listViewSmart.Items[19].SubItems.Add(sSmartInfo.TemperatureSensor3.ToString());
            listViewSmart.Items[20].SubItems.Add(sSmartInfo.TemperatureSensor4.ToString());
            listViewSmart.Items[21].SubItems.Add(sSmartInfo.TemperatureSensor5.ToString());
            listViewSmart.Items[22].SubItems.Add(sSmartInfo.TemperatureSensor6.ToString());
            listViewSmart.Items[23].SubItems.Add(sSmartInfo.TemperatureSensor7.ToString());
            listViewSmart.Items[24].SubItems.Add(sSmartInfo.TemperatureSensor8.ToString());

            listViewSmart.Items[25].SubItems.Add(sSmartInfo.TMT1TransitionCount.ToString());
            listViewSmart.Items[26].SubItems.Add(sSmartInfo.TMT2TransitionCount.ToString());
            listViewSmart.Items[27].SubItems.Add(sSmartInfo.TMT1TotalTime.ToString());
            listViewSmart.Items[28].SubItems.Add(sSmartInfo.TMT2TotalTime.ToString());

        }

        public void ClearSmartListView() {
            foreach (ListViewItem lvItem in listViewSmart.Items) {
                lvItem.Remove();
            }
        }

        private void DeleteItem(int idx) {
            listViewSmart.Items[idx].Remove();
        }

        private void btn_clearSmart_Click(object sender, EventArgs e)
        {
            ClearSmartListView();
        }
    }
}
