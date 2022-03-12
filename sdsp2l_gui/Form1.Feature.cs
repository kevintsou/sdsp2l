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
        int sel_save = 0, dw11 = 0;

        [DllImport("nvmeio.dll")]
        static extern int iGetFeatureCmd(int devIdx, int _dwFId, int _iType, int _dwCDW11, IntPtr _pulData);
        [DllImport("nvmeio.dll")]
        static extern int iSetFeatureCmd(int devIdx, int _dwFId, int _save, int _dwCDW11);

        private void vInitFeatureCommandList() {

            featureIdList.Items.Clear();

            // Insert all feature command to list box
            featureIdList.Items.Add("Arbitration");
            featureIdList.Items.Add("Power Management");
            featureIdList.Items.Add("LBA Range Type");
            featureIdList.Items.Add("Temperature Threshold");
            featureIdList.Items.Add("Error Recovery");
            featureIdList.Items.Add("Volatile Write Cache");
            featureIdList.Items.Add("Number of Queues");
            featureIdList.Items.Add("Interrupt Coalescing");
            featureIdList.Items.Add("Interrupt Vector Configuration");
            featureIdList.Items.Add("Write Atomicity Normal");
            featureIdList.Items.Add("Asynchronous Event Configuration");
            featureIdList.Items.Add("Autonomous Power State Transition");
            featureIdList.Items.Add("Host Memory Buffer");
            featureIdList.Items.Add("Timestamp");
            featureIdList.Items.Add("Keep Alive Timer");
            featureIdList.Items.Add("Host Controlled Thermal Management");
            featureIdList.Items.Add("Non-Operational Power State Config");

            featureIdList.SelectedIndex = (int)eFEATURE_ID_ORDER.E_ARBITRATION;

            ClearFeatureListView();

            textBoxSEL.Text = "0";
            textBoxDW11.Text = "0";
        }

        public void ClearFeatureListView()
        {
            listViewFeature.Clear();
            listViewFeature.View = System.Windows.Forms.View.Details;
            
            ///新增title
            ColumnHeader tittle = new ColumnHeader();
            tittle.Text = "Attributes";
            tittle.Width = 350;
            listViewFeature.Columns.Add(tittle);

            ColumnHeader value = new ColumnHeader();
            value.Text = "Value";
            value.Width = 200;
            listViewFeature.Columns.Add(value);
        }

        private void vIssueSetFeatureCmd(int idx, int save, int dw11) {
            int opcode = idx + 1;

            int result = iSetFeatureCmd(iCurrentDevIdx, opcode, save, dw11);

            // send get feature command fail
            if (result != 0)
            {
                ClearFeatureListView();
                textBoxStatus.AppendText("  " + DateTime.Now + "Set Feature Command Fail, OP: " + (eFEATURE_ID_ORDER)idx + Environment.NewLine);
                return;
            }
            else
            {
                //textBoxStatus.AppendText("  " + DateTime.Now + "Get Feature Command Success, OP: " + (eFEATURE_ID_ORDER)idx + Environment.NewLine);
            }
        }

        private void vIssueGetFeatureCmd(int idx, int sel) {
            
            int opcode = idx + 1;
            int dw0 = 0;

            //sel = 0; // temp
            dw11 = 0;
            IntPtr ptr = Marshal.AllocHGlobal(4);
            int result = iGetFeatureCmd(iCurrentDevIdx, opcode, sel, dw11, ptr);
            
            dw0 = Marshal.ReadInt32(ptr);  // get value from interger ptr
            Marshal.FreeHGlobal(ptr);
            //MessageBox.Show(dw0.ToString());

            // send get feature command fail
            if (result != 0)
            {
                ClearFeatureListView();
                textBoxStatus.AppendText("  " + DateTime.Now + "Get Feature Command Fail, OP: " + (eFEATURE_ID_ORDER)idx + Environment.NewLine);
                return;
            }
            else {
                //textBoxStatus.AppendText("  " + DateTime.Now + "Get Feature Command Success, OP: " + (eFEATURE_ID_ORDER)idx + Environment.NewLine);
            }

            ClearFeatureListView();

            // parsing the detail information 
            switch ((eFEATURE_ID_ORDER)idx)
            {
                case eFEATURE_ID_ORDER.E_ARBITRATION:
                    listViewFeature.Items.Add("High Priority Weight (HPW)");
                    listViewFeature.Items.Add("Medium Priority Weight (MPW)");
                    listViewFeature.Items.Add("Low Priority Weight (LPW)");
                    listViewFeature.Items.Add("Arbitration Burst (AB)");

                    listViewFeature.Items[0].SubItems.Add(((dw0 >> 24) & 0xF).ToString());
                    listViewFeature.Items[1].SubItems.Add(((dw0 >> 16) & 0xF).ToString());
                    listViewFeature.Items[2].SubItems.Add(((dw0 >> 8) & 0xF).ToString());
                    listViewFeature.Items[3].SubItems.Add(((dw0 >> 0) & 0x7).ToString());
                    break;
                case eFEATURE_ID_ORDER.E_POWER_MANAGMENT:
                    listViewFeature.Items.Add("Workload Hint (WH)");
                    listViewFeature.Items.Add("Power State (PS)");

                    listViewFeature.Items[0].SubItems.Add(((dw0 >> 5) & 0x7).ToString());
                    listViewFeature.Items[1].SubItems.Add(((dw0 >> 0) & 0x1F).ToString());
                    break;
                case eFEATURE_ID_ORDER.E_LBA_RANGE_TYPE:
                    listViewFeature.Items.Add("Number of LBA Ranges (NUM)");
                    listViewFeature.Items[0].SubItems.Add(((dw0 >> 0) & 0x3F).ToString());
                    break;
                case eFEATURE_ID_ORDER.E_TEMP_THRESHOLD:
                    listViewFeature.Items.Add("Threshold Type Select (THSEL)");
                    listViewFeature.Items.Add("Threshold Temperature Select (TMPSEL)");
                    listViewFeature.Items.Add("Temperature Threshold (TMPTH)");

                    listViewFeature.Items[0].SubItems.Add(((dw0 >> 20) & 0x3).ToString());
                    listViewFeature.Items[1].SubItems.Add(((dw0 >> 16) & 0xF).ToString());
                    listViewFeature.Items[2].SubItems.Add((((dw0 >> 0) & 0xFFFF) - 273).ToString() + "°C");
                    break;
                case eFEATURE_ID_ORDER.E_ERROR_RECOVERY:
                    listViewFeature.Items.Add("Deallocated or Unwritten Logical Block Error Enable (DULBE)");
                    listViewFeature.Items.Add("Time Limited Error Recovery (TLER)");

                    listViewFeature.Items[0].SubItems.Add(((dw0 >> 16) & 0x1).ToString());
                    listViewFeature.Items[1].SubItems.Add(((dw0 >> 0) & 0xFFFF).ToString());
                    break;
                case eFEATURE_ID_ORDER.E_VOLITILE_WRITE_CACHE:
                    listViewFeature.Items.Add("Volatile Write Cache Enable (WCE)");
                    listViewFeature.Items[0].SubItems.Add(((dw0 >> 0) & 0x1).ToString());
                    break;
                case eFEATURE_ID_ORDER.E_NUMBER_OF_QUEUE:
                    listViewFeature.Items.Add("Number of I/O Completion Queues Requested (NCQR)");
                    listViewFeature.Items.Add("Number of I/O Submission Queues Requested (NSQR)");

                    listViewFeature.Items[0].SubItems.Add(((dw0 >> 16) & 0xFFFF).ToString());
                    listViewFeature.Items[1].SubItems.Add(((dw0 >> 0) & 0xFFFF).ToString());
                    break;
                case eFEATURE_ID_ORDER.E_INTERRUPT_COALESCING:
                    listViewFeature.Items.Add("Aggregation Time (TIME)");
                    listViewFeature.Items.Add("Aggregation Threshold (THR)");

                    listViewFeature.Items[0].SubItems.Add(((dw0 >> 8) & 0xFF).ToString());
                    listViewFeature.Items[1].SubItems.Add(((dw0 >> 0) & 0xFF).ToString());
                    break;
                case eFEATURE_ID_ORDER.E_INTERRUPT_VEC_CONFIG:
                    listViewFeature.Items.Add("Coalescing Disable (CD)");
                    listViewFeature.Items.Add("Interrupt Vector (IV)");

                    listViewFeature.Items[0].SubItems.Add(((dw0 >> 16) & 0x1).ToString());
                    listViewFeature.Items[1].SubItems.Add(((dw0 >> 0) & 0xFFFF).ToString());
                    break;
                case eFEATURE_ID_ORDER.E_WRITE_AUTOMICITY_NORMAL:
                    listViewFeature.Items.Add("Disable Normal (DN)");

                    listViewFeature.Items[0].SubItems.Add(((dw0 >> 0) & 0x1).ToString());
                    break;
                case eFEATURE_ID_ORDER.E_ASYNC_EVENT_CONFIG:
                    listViewFeature.Items.Add("Telemetry Log Notices");
                    listViewFeature.Items.Add("Firmware Activation Notices");
                    listViewFeature.Items.Add("Namespace Attribute Notices");
                    listViewFeature.Items.Add("SMART / Health Critical Warnings");

                    listViewFeature.Items[0].SubItems.Add(((dw0 >> 10) & 0x1).ToString());
                    listViewFeature.Items[1].SubItems.Add(((dw0 >> 9) & 0x1).ToString());
                    listViewFeature.Items[2].SubItems.Add(((dw0 >> 8) & 0x1).ToString());
                    listViewFeature.Items[3].SubItems.Add(((dw0 >> 0) & 0xFF).ToString());
                    break;
                case eFEATURE_ID_ORDER.E_APST:
                    listViewFeature.Items.Add("Autonomous Power State Transition Enable (APSTE)");

                    listViewFeature.Items[0].SubItems.Add(((dw0 >> 0) & 0x1).ToString());
                    break;
                case eFEATURE_ID_ORDER.E_HOST_MEM_BUFFER:
                    break;
                case eFEATURE_ID_ORDER.E_TIMESTAMP:
                    break;
                case eFEATURE_ID_ORDER.E_KEEP_ALIVE_TIMER:
                    listViewFeature.Items.Add("Keep Alive Timeout (KATO)");

                    listViewFeature.Items[0].SubItems.Add(((dw0 >> 0)).ToString());
                    break;
                case eFEATURE_ID_ORDER.E_HOST_CONTROL_THERMAL_MANG:
                    listViewFeature.Items.Add("Thermal Management Temperature 1 (TMT1)");
                    listViewFeature.Items.Add("Thermal Management Temperature 2 (TMT2)");

                    listViewFeature.Items[0].SubItems.Add((((dw0 >> 16) & 0xFFFF) - 273).ToString() + "°C");
                    listViewFeature.Items[1].SubItems.Add((((dw0 >> 0) & 0xFFFF) - 273).ToString() + "°C");
                    break;
                case eFEATURE_ID_ORDER.E_NON_OPERATION_PWR_STATE_CFG:
                    listViewFeature.Items.Add("Non-Operational Power State Permissive Mode Enable (NOPPME)");

                    listViewFeature.Items[0].SubItems.Add(((dw0 >> 0) & 0x1).ToString());
                    break;
            }

        }

        private void tabFeature_Enter(object sender, EventArgs e)
        {
            vInitFeatureCommandList();
        }

        private void featureIdList_SelectedIndexChanged(object sender, EventArgs e)
        {
            
        }
    }
}
