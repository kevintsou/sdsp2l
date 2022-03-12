
namespace Xpacer
{
    partial class Form1
    {
        /// <summary>
        ///  Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        ///  Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        ///  Required method for Designer support - do not modify
        ///  the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(Form1));
            this.tabControl = new System.Windows.Forms.TabControl();
            this.powerCycleTab = new System.Windows.Forms.TabPage();
            this.button12 = new System.Windows.Forms.Button();
            this.button11 = new System.Windows.Forms.Button();
            this.label_testTime = new System.Windows.Forms.Label();
            this.textBox_rebuildTimeout = new System.Windows.Forms.TextBox();
            this.textbox_qdepth = new System.Windows.Forms.TextBox();
            this.textBox_writeSize = new System.Windows.Forms.TextBox();
            this.textBox_powerOnTime = new System.Windows.Forms.TextBox();
            this.textBox_powerOffTime = new System.Windows.Forms.TextBox();
            this.textbox_testloop = new System.Windows.Forms.TextBox();
            this.label14 = new System.Windows.Forms.Label();
            this.queue_depth = new System.Windows.Forms.Label();
            this.checkBox_output_log = new System.Windows.Forms.CheckBox();
            this.workload_combobox = new System.Windows.Forms.ComboBox();
            this.testTypeBox1 = new System.Windows.Forms.ComboBox();
            this.checkbox_prefill = new System.Windows.Forms.CheckBox();
            this.startTestBtn = new System.Windows.Forms.Button();
            this.label9 = new System.Windows.Forms.Label();
            this.label8 = new System.Windows.Forms.Label();
            this.label7 = new System.Windows.Forms.Label();
            this.label5 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.trackBar_writeSize = new System.Windows.Forms.TrackBar();
            this.tabSMART = new System.Windows.Forms.TabPage();
            this.btn_clearSmart = new System.Windows.Forms.Button();
            this.listViewSmart = new System.Windows.Forms.ListView();
            this.tabFeature = new System.Windows.Forms.TabPage();
            this.listViewFeature = new System.Windows.Forms.ListView();
            this.label21 = new System.Windows.Forms.Label();
            this.featureIdList = new System.Windows.Forms.ComboBox();
            this.label22 = new System.Windows.Forms.Label();
            this.textBoxDW11 = new System.Windows.Forms.TextBox();
            this.textBoxSEL = new System.Windows.Forms.TextBox();
            this.btnGetFeature = new System.Windows.Forms.Button();
            this.label20 = new System.Windows.Forms.Label();
            this.btnSetFeature = new System.Windows.Forms.Button();
            this.performanceTab = new System.Windows.Forms.TabPage();
            this.rescan_device = new System.Windows.Forms.Button();
            this.label19 = new System.Windows.Forms.Label();
            this.button10 = new System.Windows.Forms.Button();
            this.button1 = new System.Windows.Forms.Button();
            this.label18 = new System.Windows.Forms.Label();
            this.label17 = new System.Windows.Forms.Label();
            this.label16 = new System.Windows.Forms.Label();
            this.label15 = new System.Windows.Forms.Label();
            this.button9 = new System.Windows.Forms.Button();
            this.button8 = new System.Windows.Forms.Button();
            this.button7 = new System.Windows.Forms.Button();
            this.button6 = new System.Windows.Forms.Button();
            this.button5 = new System.Windows.Forms.Button();
            this.button4 = new System.Windows.Forms.Button();
            this.button3 = new System.Windows.Forms.Button();
            this.button2 = new System.Windows.Forms.Button();
            this.deviceListBox = new System.Windows.Forms.ComboBox();
            this.textBoxStatus = new System.Windows.Forms.TextBox();
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.timer_1sec = new System.Windows.Forms.Timer(this.components);
            this.timer_300MS = new System.Windows.Forms.Timer(this.components);
            this.btnClearOutput = new System.Windows.Forms.Button();
            this.pictureBox_pass = new System.Windows.Forms.PictureBox();
            this.pictureBox_fail = new System.Windows.Forms.PictureBox();
            this.tabControl.SuspendLayout();
            this.powerCycleTab.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar_writeSize)).BeginInit();
            this.tabSMART.SuspendLayout();
            this.tabFeature.SuspendLayout();
            this.performanceTab.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox_pass)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox_fail)).BeginInit();
            this.SuspendLayout();
            // 
            // tabControl
            // 
            this.tabControl.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.tabControl.Controls.Add(this.powerCycleTab);
            this.tabControl.Controls.Add(this.tabSMART);
            this.tabControl.Controls.Add(this.tabFeature);
            this.tabControl.Controls.Add(this.performanceTab);
            this.tabControl.Font = new System.Drawing.Font("Microsoft JhengHei UI", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point);
            this.tabControl.ItemSize = new System.Drawing.Size(80, 30);
            this.tabControl.Location = new System.Drawing.Point(12, 53);
            this.tabControl.Name = "tabControl";
            this.tabControl.SelectedIndex = 0;
            this.tabControl.Size = new System.Drawing.Size(860, 379);
            this.tabControl.TabIndex = 5;
            this.tabControl.SelectedIndexChanged += new System.EventHandler(this.tabControl1_SelectedIndexChanged);
            // 
            // powerCycleTab
            // 
            this.powerCycleTab.Controls.Add(this.pictureBox_fail);
            this.powerCycleTab.Controls.Add(this.pictureBox_pass);
            this.powerCycleTab.Controls.Add(this.button12);
            this.powerCycleTab.Controls.Add(this.button11);
            this.powerCycleTab.Controls.Add(this.label_testTime);
            this.powerCycleTab.Controls.Add(this.textBox_rebuildTimeout);
            this.powerCycleTab.Controls.Add(this.textbox_qdepth);
            this.powerCycleTab.Controls.Add(this.textBox_writeSize);
            this.powerCycleTab.Controls.Add(this.textBox_powerOnTime);
            this.powerCycleTab.Controls.Add(this.textBox_powerOffTime);
            this.powerCycleTab.Controls.Add(this.textbox_testloop);
            this.powerCycleTab.Controls.Add(this.label14);
            this.powerCycleTab.Controls.Add(this.queue_depth);
            this.powerCycleTab.Controls.Add(this.checkBox_output_log);
            this.powerCycleTab.Controls.Add(this.workload_combobox);
            this.powerCycleTab.Controls.Add(this.testTypeBox1);
            this.powerCycleTab.Controls.Add(this.checkbox_prefill);
            this.powerCycleTab.Controls.Add(this.startTestBtn);
            this.powerCycleTab.Controls.Add(this.label9);
            this.powerCycleTab.Controls.Add(this.label8);
            this.powerCycleTab.Controls.Add(this.label7);
            this.powerCycleTab.Controls.Add(this.label5);
            this.powerCycleTab.Controls.Add(this.label4);
            this.powerCycleTab.Controls.Add(this.label3);
            this.powerCycleTab.Controls.Add(this.trackBar_writeSize);
            this.powerCycleTab.Location = new System.Drawing.Point(4, 34);
            this.powerCycleTab.Name = "powerCycleTab";
            this.powerCycleTab.Padding = new System.Windows.Forms.Padding(3);
            this.powerCycleTab.Size = new System.Drawing.Size(852, 341);
            this.powerCycleTab.TabIndex = 1;
            this.powerCycleTab.Text = "Power Cycle";
            this.powerCycleTab.UseVisualStyleBackColor = true;
            this.powerCycleTab.Click += new System.EventHandler(this.powerCycleTab_Click);
            // 
            // button12
            // 
            this.button12.Location = new System.Drawing.Point(735, 44);
            this.button12.Name = "button12";
            this.button12.Size = new System.Drawing.Size(75, 23);
            this.button12.TabIndex = 28;
            this.button12.Text = "button12";
            this.button12.UseVisualStyleBackColor = true;
            this.button12.Visible = false;
            this.button12.Click += new System.EventHandler(this.button12_Click);
            // 
            // button11
            // 
            this.button11.Enabled = false;
            this.button11.Location = new System.Drawing.Point(735, 15);
            this.button11.Name = "button11";
            this.button11.Size = new System.Drawing.Size(75, 23);
            this.button11.TabIndex = 27;
            this.button11.Text = "button11";
            this.button11.UseVisualStyleBackColor = true;
            this.button11.Visible = false;
            this.button11.Click += new System.EventHandler(this.button11_Click);
            // 
            // label_testTime
            // 
            this.label_testTime.AutoSize = true;
            this.label_testTime.Font = new System.Drawing.Font("Microsoft JhengHei UI", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point);
            this.label_testTime.Location = new System.Drawing.Point(704, 189);
            this.label_testTime.Name = "label_testTime";
            this.label_testTime.Size = new System.Drawing.Size(14, 15);
            this.label_testTime.TabIndex = 26;
            this.label_testTime.Text = "T";
            // 
            // textBox_rebuildTimeout
            // 
            this.textBox_rebuildTimeout.Location = new System.Drawing.Point(157, 166);
            this.textBox_rebuildTimeout.Name = "textBox_rebuildTimeout";
            this.textBox_rebuildTimeout.Size = new System.Drawing.Size(77, 23);
            this.textBox_rebuildTimeout.TabIndex = 23;
            this.textBox_rebuildTimeout.Text = "30";
            this.textBox_rebuildTimeout.TextChanged += new System.EventHandler(this.textBox_rebuildTimeout_TextChanged);
            this.textBox_rebuildTimeout.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.textBox_rebuildTimeout_KeyPress);
            // 
            // textbox_qdepth
            // 
            this.textbox_qdepth.Enabled = false;
            this.textbox_qdepth.Location = new System.Drawing.Point(101, 204);
            this.textbox_qdepth.MaxLength = 20;
            this.textbox_qdepth.Name = "textbox_qdepth";
            this.textbox_qdepth.Size = new System.Drawing.Size(74, 23);
            this.textbox_qdepth.TabIndex = 21;
            this.textbox_qdepth.Text = "64";
            // 
            // textBox_writeSize
            // 
            this.textBox_writeSize.BackColor = System.Drawing.SystemColors.Control;
            this.textBox_writeSize.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.textBox_writeSize.Enabled = false;
            this.textBox_writeSize.Location = new System.Drawing.Point(169, 309);
            this.textBox_writeSize.MaxLength = 20;
            this.textBox_writeSize.Name = "textBox_writeSize";
            this.textBox_writeSize.Size = new System.Drawing.Size(65, 16);
            this.textBox_writeSize.TabIndex = 16;
            this.textBox_writeSize.Text = "512Byte";
            this.textBox_writeSize.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            // 
            // textBox_powerOnTime
            // 
            this.textBox_powerOnTime.Location = new System.Drawing.Point(169, 126);
            this.textBox_powerOnTime.Name = "textBox_powerOnTime";
            this.textBox_powerOnTime.Size = new System.Drawing.Size(77, 23);
            this.textBox_powerOnTime.TabIndex = 13;
            this.textBox_powerOnTime.Text = "10";
            this.textBox_powerOnTime.TextChanged += new System.EventHandler(this.textBox_powerOnTime_TextChanged);
            this.textBox_powerOnTime.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.powerOff_keypress);
            // 
            // textBox_powerOffTime
            // 
            this.textBox_powerOffTime.Location = new System.Drawing.Point(191, 88);
            this.textBox_powerOffTime.Name = "textBox_powerOffTime";
            this.textBox_powerOffTime.Size = new System.Drawing.Size(77, 23);
            this.textBox_powerOffTime.TabIndex = 12;
            this.textBox_powerOffTime.Text = "10";
            this.textBox_powerOffTime.TextChanged += new System.EventHandler(this.textBox_powerOffTime_TextChanged);
            this.textBox_powerOffTime.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.powerOfint_keypress);
            // 
            // textbox_testloop
            // 
            this.textbox_testloop.Location = new System.Drawing.Point(66, 53);
            this.textbox_testloop.Margin = new System.Windows.Forms.Padding(50);
            this.textbox_testloop.MaxLength = 20;
            this.textbox_testloop.Name = "textbox_testloop";
            this.textbox_testloop.Size = new System.Drawing.Size(74, 23);
            this.textbox_testloop.TabIndex = 11;
            this.textbox_testloop.Text = "1024";
            this.textbox_testloop.TextChanged += new System.EventHandler(this.textbox_testloop_TextChanged);
            this.textbox_testloop.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.testLoop_keypress);
            // 
            // label14
            // 
            this.label14.AutoSize = true;
            this.label14.Location = new System.Drawing.Point(18, 169);
            this.label14.Name = "label14";
            this.label14.Size = new System.Drawing.Size(133, 15);
            this.label14.TabIndex = 22;
            this.label14.Text = "Rebuild Timeout (sec):";
            // 
            // queue_depth
            // 
            this.queue_depth.AutoSize = true;
            this.queue_depth.Enabled = false;
            this.queue_depth.Location = new System.Drawing.Point(19, 207);
            this.queue_depth.Name = "queue_depth";
            this.queue_depth.Size = new System.Drawing.Size(55, 15);
            this.queue_depth.TabIndex = 20;
            this.queue_depth.Text = "QDepth:";
            // 
            // checkBox_output_log
            // 
            this.checkBox_output_log.AutoSize = true;
            this.checkBox_output_log.Location = new System.Drawing.Point(339, 56);
            this.checkBox_output_log.Name = "checkBox_output_log";
            this.checkBox_output_log.Size = new System.Drawing.Size(105, 19);
            this.checkBox_output_log.TabIndex = 19;
            this.checkBox_output_log.Text = "Create log file";
            this.checkBox_output_log.UseVisualStyleBackColor = true;
            this.checkBox_output_log.CheckedChanged += new System.EventHandler(this.checkBox_output_log_CheckedChanged);
            // 
            // workload_combobox
            // 
            this.workload_combobox.Enabled = false;
            this.workload_combobox.FormattingEnabled = true;
            this.workload_combobox.Location = new System.Drawing.Point(101, 241);
            this.workload_combobox.Name = "workload_combobox";
            this.workload_combobox.Size = new System.Drawing.Size(129, 23);
            this.workload_combobox.TabIndex = 17;
            this.workload_combobox.SelectedIndexChanged += new System.EventHandler(this.workload_combobox_SelectedIndexChanged);
            // 
            // testTypeBox1
            // 
            this.testTypeBox1.FormattingEnabled = true;
            this.testTypeBox1.Location = new System.Drawing.Point(66, 15);
            this.testTypeBox1.Name = "testTypeBox1";
            this.testTypeBox1.Size = new System.Drawing.Size(638, 23);
            this.testTypeBox1.TabIndex = 6;
            this.testTypeBox1.SelectedIndexChanged += new System.EventHandler(this.testTypeBox1_SelectedIndexChanged);
            // 
            // checkbox_prefill
            // 
            this.checkbox_prefill.AutoSize = true;
            this.checkbox_prefill.Enabled = false;
            this.checkbox_prefill.Location = new System.Drawing.Point(339, 87);
            this.checkbox_prefill.Name = "checkbox_prefill";
            this.checkbox_prefill.Size = new System.Drawing.Size(86, 19);
            this.checkbox_prefill.TabIndex = 10;
            this.checkbox_prefill.Text = "Prefill data";
            this.checkbox_prefill.UseVisualStyleBackColor = true;
            this.checkbox_prefill.CheckedChanged += new System.EventHandler(this.prefill_checkbox);
            // 
            // startTestBtn
            // 
            this.startTestBtn.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.startTestBtn.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.startTestBtn.FlatAppearance.BorderSize = 0;
            this.startTestBtn.FlatAppearance.MouseOverBackColor = System.Drawing.Color.FromArgb(((int)(((byte)(224)))), ((int)(((byte)(224)))), ((int)(((byte)(224)))));
            this.startTestBtn.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.startTestBtn.Font = new System.Drawing.Font("Microsoft JhengHei UI", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point);
            this.startTestBtn.Location = new System.Drawing.Point(684, 238);
            this.startTestBtn.Name = "startTestBtn";
            this.startTestBtn.Padding = new System.Windows.Forms.Padding(5);
            this.startTestBtn.Size = new System.Drawing.Size(162, 87);
            this.startTestBtn.TabIndex = 9;
            this.startTestBtn.Text = "START";
            this.startTestBtn.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.startTestBtn.TextImageRelation = System.Windows.Forms.TextImageRelation.TextBeforeImage;
            this.startTestBtn.UseVisualStyleBackColor = true;
            this.startTestBtn.Click += new System.EventHandler(this.startTestBtn_Click);
            // 
            // label9
            // 
            this.label9.AutoSize = true;
            this.label9.Location = new System.Drawing.Point(18, 129);
            this.label9.Name = "label9";
            this.label9.Size = new System.Drawing.Size(145, 15);
            this.label9.TabIndex = 8;
            this.label9.Text = "Power off idle time (sec):";
            // 
            // label8
            // 
            this.label8.AutoSize = true;
            this.label8.Location = new System.Drawing.Point(18, 91);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(167, 15);
            this.label8.TabIndex = 7;
            this.label8.Text = "Time to remove power (sec):";
            // 
            // label7
            // 
            this.label7.AutoSize = true;
            this.label7.Location = new System.Drawing.Point(18, 19);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(42, 15);
            this.label7.TabIndex = 6;
            this.label7.Text = "Script:";
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Enabled = false;
            this.label5.Location = new System.Drawing.Point(19, 244);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(66, 15);
            this.label5.TabIndex = 3;
            this.label5.Text = "Workload:";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(18, 56);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(45, 15);
            this.label4.TabIndex = 2;
            this.label4.Text = "Loops:";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Enabled = false;
            this.label3.Location = new System.Drawing.Point(19, 284);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(66, 15);
            this.label3.TabIndex = 1;
            this.label3.Text = "Write Size:";
            // 
            // trackBar_writeSize
            // 
            this.trackBar_writeSize.Enabled = false;
            this.trackBar_writeSize.Location = new System.Drawing.Point(101, 280);
            this.trackBar_writeSize.Maximum = 6;
            this.trackBar_writeSize.Name = "trackBar_writeSize";
            this.trackBar_writeSize.Size = new System.Drawing.Size(209, 45);
            this.trackBar_writeSize.TabIndex = 0;
            this.trackBar_writeSize.Value = 6;
            this.trackBar_writeSize.Scroll += new System.EventHandler(this.trackBar_writeSize_Scroll);
            // 
            // tabSMART
            // 
            this.tabSMART.Controls.Add(this.btn_clearSmart);
            this.tabSMART.Controls.Add(this.listViewSmart);
            this.tabSMART.Location = new System.Drawing.Point(4, 34);
            this.tabSMART.Name = "tabSMART";
            this.tabSMART.Padding = new System.Windows.Forms.Padding(3);
            this.tabSMART.Size = new System.Drawing.Size(852, 341);
            this.tabSMART.TabIndex = 2;
            this.tabSMART.Text = "Smart Info";
            this.tabSMART.UseVisualStyleBackColor = true;
            this.tabSMART.Click += new System.EventHandler(this.tabSMART_Click);
            this.tabSMART.Enter += new System.EventHandler(this.tabSMART_Enter);
            // 
            // btn_clearSmart
            // 
            this.btn_clearSmart.Location = new System.Drawing.Point(689, 298);
            this.btn_clearSmart.Name = "btn_clearSmart";
            this.btn_clearSmart.Size = new System.Drawing.Size(113, 25);
            this.btn_clearSmart.TabIndex = 1;
            this.btn_clearSmart.Text = "Clear Smart";
            this.btn_clearSmart.UseVisualStyleBackColor = true;
            this.btn_clearSmart.Visible = false;
            this.btn_clearSmart.Click += new System.EventHandler(this.btn_clearSmart_Click);
            // 
            // listViewSmart
            // 
            this.listViewSmart.HideSelection = false;
            this.listViewSmart.Location = new System.Drawing.Point(13, 17);
            this.listViewSmart.Name = "listViewSmart";
            this.listViewSmart.Size = new System.Drawing.Size(804, 318);
            this.listViewSmart.TabIndex = 0;
            this.listViewSmart.UseCompatibleStateImageBehavior = false;
            // 
            // tabFeature
            // 
            this.tabFeature.Controls.Add(this.listViewFeature);
            this.tabFeature.Controls.Add(this.label21);
            this.tabFeature.Controls.Add(this.featureIdList);
            this.tabFeature.Controls.Add(this.label22);
            this.tabFeature.Controls.Add(this.textBoxDW11);
            this.tabFeature.Controls.Add(this.textBoxSEL);
            this.tabFeature.Controls.Add(this.btnGetFeature);
            this.tabFeature.Controls.Add(this.label20);
            this.tabFeature.Controls.Add(this.btnSetFeature);
            this.tabFeature.Location = new System.Drawing.Point(4, 34);
            this.tabFeature.Name = "tabFeature";
            this.tabFeature.Padding = new System.Windows.Forms.Padding(3);
            this.tabFeature.Size = new System.Drawing.Size(852, 341);
            this.tabFeature.TabIndex = 3;
            this.tabFeature.Text = "Feature Cmd";
            this.tabFeature.UseVisualStyleBackColor = true;
            this.tabFeature.Enter += new System.EventHandler(this.tabFeature_Enter);
            // 
            // listViewFeature
            // 
            this.listViewFeature.HideSelection = false;
            this.listViewFeature.Location = new System.Drawing.Point(17, 59);
            this.listViewFeature.Name = "listViewFeature";
            this.listViewFeature.Size = new System.Drawing.Size(775, 275);
            this.listViewFeature.TabIndex = 9;
            this.listViewFeature.UseCompatibleStateImageBehavior = false;
            // 
            // label21
            // 
            this.label21.AutoSize = true;
            this.label21.Location = new System.Drawing.Point(17, 12);
            this.label21.Name = "label21";
            this.label21.Size = new System.Drawing.Size(112, 15);
            this.label21.TabIndex = 8;
            this.label21.Text = "Feature Command";
            // 
            // featureIdList
            // 
            this.featureIdList.FormattingEnabled = true;
            this.featureIdList.Location = new System.Drawing.Point(17, 30);
            this.featureIdList.Name = "featureIdList";
            this.featureIdList.Size = new System.Drawing.Size(334, 23);
            this.featureIdList.TabIndex = 6;
            this.featureIdList.SelectedIndexChanged += new System.EventHandler(this.featureIdList_SelectedIndexChanged);
            // 
            // label22
            // 
            this.label22.AutoSize = true;
            this.label22.Location = new System.Drawing.Point(449, 12);
            this.label22.Name = "label22";
            this.label22.Size = new System.Drawing.Size(88, 15);
            this.label22.TabIndex = 7;
            this.label22.Text = "DW11 (for set)";
            // 
            // textBoxDW11
            // 
            this.textBoxDW11.Location = new System.Drawing.Point(449, 30);
            this.textBoxDW11.Name = "textBoxDW11";
            this.textBoxDW11.Size = new System.Drawing.Size(100, 23);
            this.textBoxDW11.TabIndex = 6;
            this.textBoxDW11.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.textBoxDW11_KeyPress);
            // 
            // textBoxSEL
            // 
            this.textBoxSEL.Location = new System.Drawing.Point(368, 30);
            this.textBoxSEL.Name = "textBoxSEL";
            this.textBoxSEL.Size = new System.Drawing.Size(75, 23);
            this.textBoxSEL.TabIndex = 1;
            this.textBoxSEL.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.textBoxSEL_KeyPress);
            // 
            // btnGetFeature
            // 
            this.btnGetFeature.Location = new System.Drawing.Point(555, 22);
            this.btnGetFeature.Name = "btnGetFeature";
            this.btnGetFeature.Size = new System.Drawing.Size(116, 31);
            this.btnGetFeature.TabIndex = 5;
            this.btnGetFeature.Text = "Get Feature";
            this.btnGetFeature.UseVisualStyleBackColor = true;
            this.btnGetFeature.Click += new System.EventHandler(this.btnGetFeature_Click);
            // 
            // label20
            // 
            this.label20.AutoSize = true;
            this.label20.Location = new System.Drawing.Point(368, 12);
            this.label20.Name = "label20";
            this.label20.Size = new System.Drawing.Size(73, 15);
            this.label20.TabIndex = 2;
            this.label20.Text = "Select/Save";
            // 
            // btnSetFeature
            // 
            this.btnSetFeature.Location = new System.Drawing.Point(677, 22);
            this.btnSetFeature.Name = "btnSetFeature";
            this.btnSetFeature.Size = new System.Drawing.Size(115, 31);
            this.btnSetFeature.TabIndex = 0;
            this.btnSetFeature.Text = "Set Feature";
            this.btnSetFeature.UseVisualStyleBackColor = true;
            this.btnSetFeature.Click += new System.EventHandler(this.btnSetFeature_Click);
            // 
            // performanceTab
            // 
            this.performanceTab.Controls.Add(this.rescan_device);
            this.performanceTab.Controls.Add(this.label19);
            this.performanceTab.Controls.Add(this.button10);
            this.performanceTab.Controls.Add(this.button1);
            this.performanceTab.Controls.Add(this.label18);
            this.performanceTab.Controls.Add(this.label17);
            this.performanceTab.Controls.Add(this.label16);
            this.performanceTab.Controls.Add(this.label15);
            this.performanceTab.Controls.Add(this.button9);
            this.performanceTab.Controls.Add(this.button8);
            this.performanceTab.Controls.Add(this.button7);
            this.performanceTab.Controls.Add(this.button6);
            this.performanceTab.Controls.Add(this.button5);
            this.performanceTab.Controls.Add(this.button4);
            this.performanceTab.Controls.Add(this.button3);
            this.performanceTab.Controls.Add(this.button2);
            this.performanceTab.Location = new System.Drawing.Point(4, 34);
            this.performanceTab.Name = "performanceTab";
            this.performanceTab.Padding = new System.Windows.Forms.Padding(3);
            this.performanceTab.Size = new System.Drawing.Size(852, 341);
            this.performanceTab.TabIndex = 0;
            this.performanceTab.Text = "Test";
            this.performanceTab.UseVisualStyleBackColor = true;
            // 
            // rescan_device
            // 
            this.rescan_device.Location = new System.Drawing.Point(92, 222);
            this.rescan_device.Name = "rescan_device";
            this.rescan_device.Size = new System.Drawing.Size(169, 23);
            this.rescan_device.TabIndex = 16;
            this.rescan_device.Text = "Rescan Device";
            this.rescan_device.UseVisualStyleBackColor = true;
            this.rescan_device.Click += new System.EventHandler(this.rescan_device_Click);
            // 
            // label19
            // 
            this.label19.AutoSize = true;
            this.label19.Location = new System.Drawing.Point(26, 193);
            this.label19.Name = "label19";
            this.label19.Size = new System.Drawing.Size(27, 15);
            this.label19.TabIndex = 15;
            this.label19.Text = "RW";
            // 
            // button10
            // 
            this.button10.Location = new System.Drawing.Point(186, 193);
            this.button10.Name = "button10";
            this.button10.Size = new System.Drawing.Size(75, 23);
            this.button10.TabIndex = 14;
            this.button10.Text = "Read";
            this.button10.UseVisualStyleBackColor = true;
            this.button10.Click += new System.EventHandler(this.button10_Click);
            // 
            // button1
            // 
            this.button1.Location = new System.Drawing.Point(92, 193);
            this.button1.Name = "button1";
            this.button1.Size = new System.Drawing.Size(75, 23);
            this.button1.TabIndex = 13;
            this.button1.Text = "Write";
            this.button1.UseVisualStyleBackColor = true;
            this.button1.Click += new System.EventHandler(this.button1_Click_1);
            // 
            // label18
            // 
            this.label18.AutoSize = true;
            this.label18.Location = new System.Drawing.Point(26, 131);
            this.label18.Name = "label18";
            this.label18.Size = new System.Drawing.Size(43, 15);
            this.label18.TabIndex = 12;
            this.label18.Text = "GPIO3";
            // 
            // label17
            // 
            this.label17.AutoSize = true;
            this.label17.Location = new System.Drawing.Point(26, 102);
            this.label17.Name = "label17";
            this.label17.Size = new System.Drawing.Size(43, 15);
            this.label17.TabIndex = 11;
            this.label17.Text = "GPIO2";
            // 
            // label16
            // 
            this.label16.AutoSize = true;
            this.label16.Location = new System.Drawing.Point(26, 73);
            this.label16.Name = "label16";
            this.label16.Size = new System.Drawing.Size(43, 15);
            this.label16.TabIndex = 10;
            this.label16.Text = "GPIO1";
            // 
            // label15
            // 
            this.label15.AutoSize = true;
            this.label15.Location = new System.Drawing.Point(26, 44);
            this.label15.Name = "label15";
            this.label15.Size = new System.Drawing.Size(43, 15);
            this.label15.TabIndex = 9;
            this.label15.Text = "GPIO0";
            // 
            // button9
            // 
            this.button9.Location = new System.Drawing.Point(186, 127);
            this.button9.Name = "button9";
            this.button9.Size = new System.Drawing.Size(75, 23);
            this.button9.TabIndex = 8;
            this.button9.Text = "LOW";
            this.button9.UseVisualStyleBackColor = true;
            this.button9.Click += new System.EventHandler(this.button9_Click);
            // 
            // button8
            // 
            this.button8.Location = new System.Drawing.Point(92, 127);
            this.button8.Name = "button8";
            this.button8.Size = new System.Drawing.Size(75, 23);
            this.button8.TabIndex = 7;
            this.button8.Text = "HIGH";
            this.button8.UseVisualStyleBackColor = true;
            this.button8.Click += new System.EventHandler(this.button8_Click);
            // 
            // button7
            // 
            this.button7.Location = new System.Drawing.Point(186, 98);
            this.button7.Name = "button7";
            this.button7.Size = new System.Drawing.Size(75, 23);
            this.button7.TabIndex = 6;
            this.button7.Text = "LOW";
            this.button7.UseVisualStyleBackColor = true;
            this.button7.Click += new System.EventHandler(this.button7_Click);
            // 
            // button6
            // 
            this.button6.Location = new System.Drawing.Point(92, 98);
            this.button6.Name = "button6";
            this.button6.Size = new System.Drawing.Size(75, 23);
            this.button6.TabIndex = 5;
            this.button6.Text = "HIGH";
            this.button6.UseVisualStyleBackColor = true;
            this.button6.Click += new System.EventHandler(this.button6_Click);
            // 
            // button5
            // 
            this.button5.Location = new System.Drawing.Point(92, 69);
            this.button5.Name = "button5";
            this.button5.Size = new System.Drawing.Size(75, 23);
            this.button5.TabIndex = 4;
            this.button5.Text = "HIGH";
            this.button5.UseVisualStyleBackColor = true;
            this.button5.Click += new System.EventHandler(this.button5_Click);
            // 
            // button4
            // 
            this.button4.Location = new System.Drawing.Point(186, 40);
            this.button4.Name = "button4";
            this.button4.Size = new System.Drawing.Size(75, 23);
            this.button4.TabIndex = 3;
            this.button4.Text = "LOW";
            this.button4.UseVisualStyleBackColor = true;
            this.button4.Click += new System.EventHandler(this.button4_Click);
            // 
            // button3
            // 
            this.button3.Location = new System.Drawing.Point(186, 69);
            this.button3.Name = "button3";
            this.button3.Size = new System.Drawing.Size(75, 23);
            this.button3.TabIndex = 2;
            this.button3.Text = "LOW";
            this.button3.UseVisualStyleBackColor = true;
            this.button3.Click += new System.EventHandler(this.button3_Click);
            // 
            // button2
            // 
            this.button2.Location = new System.Drawing.Point(92, 40);
            this.button2.Name = "button2";
            this.button2.Size = new System.Drawing.Size(75, 23);
            this.button2.TabIndex = 1;
            this.button2.Text = "HIGH";
            this.button2.UseVisualStyleBackColor = true;
            this.button2.Click += new System.EventHandler(this.button2_Click);
            // 
            // deviceListBox
            // 
            this.deviceListBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.deviceListBox.FormattingEnabled = true;
            this.deviceListBox.Location = new System.Drawing.Point(12, 27);
            this.deviceListBox.Name = "deviceListBox";
            this.deviceListBox.Size = new System.Drawing.Size(860, 23);
            this.deviceListBox.TabIndex = 1;
            this.deviceListBox.SelectedIndexChanged += new System.EventHandler(this.deviceListBox_SelectedIndexChanged);
            // 
            // textBoxStatus
            // 
            this.textBoxStatus.AllowDrop = true;
            this.textBoxStatus.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.textBoxStatus.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.textBoxStatus.Location = new System.Drawing.Point(12, 458);
            this.textBoxStatus.Margin = new System.Windows.Forms.Padding(6);
            this.textBoxStatus.Multiline = true;
            this.textBoxStatus.Name = "textBoxStatus";
            this.textBoxStatus.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.textBoxStatus.Size = new System.Drawing.Size(860, 241);
            this.textBoxStatus.TabIndex = 2;
            this.textBoxStatus.TextChanged += new System.EventHandler(this.textBoxStatus_TextChanged);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Font = new System.Drawing.Font("Microsoft JhengHei UI", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point);
            this.label1.ForeColor = System.Drawing.SystemColors.ControlDarkDark;
            this.label1.Location = new System.Drawing.Point(12, 9);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(50, 17);
            this.label1.TabIndex = 3;
            this.label1.Text = "Device";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Font = new System.Drawing.Font("Microsoft JhengHei UI", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point);
            this.label2.ForeColor = System.Drawing.SystemColors.ControlDarkDark;
            this.label2.Location = new System.Drawing.Point(12, 435);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(53, 17);
            this.label2.TabIndex = 4;
            this.label2.Text = "Output";
            // 
            // timer_1sec
            // 
            this.timer_1sec.Enabled = true;
            this.timer_1sec.Interval = 1000;
            this.timer_1sec.Tick += new System.EventHandler(this.timer1_Tick);
            // 
            // timer_300MS
            // 
            this.timer_300MS.Enabled = true;
            this.timer_300MS.Interval = 300;
            this.timer_300MS.Tick += new System.EventHandler(this.timer_300mis_Tick);
            // 
            // btnClearOutput
            // 
            this.btnClearOutput.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnClearOutput.BackColor = System.Drawing.SystemColors.Window;
            this.btnClearOutput.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btnClearOutput.Location = new System.Drawing.Point(751, 661);
            this.btnClearOutput.Name = "btnClearOutput";
            this.btnClearOutput.Size = new System.Drawing.Size(95, 23);
            this.btnClearOutput.TabIndex = 6;
            this.btnClearOutput.Text = "Clear Output";
            this.btnClearOutput.UseVisualStyleBackColor = false;
            this.btnClearOutput.Click += new System.EventHandler(this.btnClearOutput_Click);
            // 
            // pictureBox_pass
            // 
            this.pictureBox_pass.Image = ((System.Drawing.Image)(resources.GetObject("pictureBox_pass.Image")));
            this.pictureBox_pass.Location = new System.Drawing.Point(339, 126);
            this.pictureBox_pass.Name = "pictureBox_pass";
            this.pictureBox_pass.Size = new System.Drawing.Size(249, 105);
            this.pictureBox_pass.TabIndex = 29;
            this.pictureBox_pass.TabStop = false;
            this.pictureBox_pass.Visible = false;
            // 
            // pictureBox_fail
            // 
            this.pictureBox_fail.Image = ((System.Drawing.Image)(resources.GetObject("pictureBox_fail.Image")));
            this.pictureBox_fail.Location = new System.Drawing.Point(357, 137);
            this.pictureBox_fail.Name = "pictureBox_fail";
            this.pictureBox_fail.Size = new System.Drawing.Size(249, 127);
            this.pictureBox_fail.TabIndex = 30;
            this.pictureBox_fail.TabStop = false;
            this.pictureBox_fail.Visible = false;
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 15F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.SystemColors.Control;
            this.ClientSize = new System.Drawing.Size(884, 711);
            this.Controls.Add(this.btnClearOutput);
            this.Controls.Add(this.tabControl);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.textBoxStatus);
            this.Controls.Add(this.deviceListBox);
            this.Font = new System.Drawing.Font("Microsoft JhengHei UI", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimumSize = new System.Drawing.Size(900, 700);
            this.Name = "Form1";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Xpacer 2021 v0.1.0";
            this.Load += new System.EventHandler(this.Form1_Load);
            this.tabControl.ResumeLayout(false);
            this.powerCycleTab.ResumeLayout(false);
            this.powerCycleTab.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar_writeSize)).EndInit();
            this.tabSMART.ResumeLayout(false);
            this.tabFeature.ResumeLayout(false);
            this.tabFeature.PerformLayout();
            this.performanceTab.ResumeLayout(false);
            this.performanceTab.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox_pass)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox_fail)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion
        private System.Windows.Forms.ComboBox deviceListBox;
        private System.Windows.Forms.TextBox textBoxStatus;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Timer timer_1sec;
        private System.Windows.Forms.Timer timer_300MS;
        private System.Windows.Forms.Button btnClearOutput;
        private System.Windows.Forms.TabPage performanceTab;
        private System.Windows.Forms.Button rescan_device;
        private System.Windows.Forms.Label label19;
        private System.Windows.Forms.Button button10;
        private System.Windows.Forms.Button button1;
        private System.Windows.Forms.Label label18;
        private System.Windows.Forms.Label label17;
        private System.Windows.Forms.Label label16;
        private System.Windows.Forms.Label label15;
        private System.Windows.Forms.Button button9;
        private System.Windows.Forms.Button button8;
        private System.Windows.Forms.Button button7;
        private System.Windows.Forms.Button button6;
        private System.Windows.Forms.Button button5;
        private System.Windows.Forms.Button button4;
        private System.Windows.Forms.Button button3;
        private System.Windows.Forms.Button button2;
        private System.Windows.Forms.TabPage tabFeature;
        private System.Windows.Forms.ListView listViewFeature;
        private System.Windows.Forms.Label label21;
        private System.Windows.Forms.ComboBox featureIdList;
        private System.Windows.Forms.Label label22;
        private System.Windows.Forms.TextBox textBoxDW11;
        private System.Windows.Forms.TextBox textBoxSEL;
        private System.Windows.Forms.Button btnGetFeature;
        private System.Windows.Forms.Label label20;
        private System.Windows.Forms.Button btnSetFeature;
        private System.Windows.Forms.TabPage tabSMART;
        private System.Windows.Forms.Button btn_clearSmart;
        private System.Windows.Forms.ListView listViewSmart;
        private System.Windows.Forms.TabPage powerCycleTab;
        private System.Windows.Forms.Button button12;
        private System.Windows.Forms.Button button11;
        private System.Windows.Forms.Label label_testTime;
        private System.Windows.Forms.TextBox textBox_rebuildTimeout;
        private System.Windows.Forms.TextBox textbox_qdepth;
        private System.Windows.Forms.TextBox textBox_writeSize;
        private System.Windows.Forms.TextBox textBox_powerOnTime;
        private System.Windows.Forms.TextBox textBox_powerOffTime;
        private System.Windows.Forms.TextBox textbox_testloop;
        private System.Windows.Forms.Label label14;
        private System.Windows.Forms.Label queue_depth;
        private System.Windows.Forms.CheckBox checkBox_output_log;
        private System.Windows.Forms.ComboBox workload_combobox;
        private System.Windows.Forms.ComboBox testTypeBox1;
        private System.Windows.Forms.CheckBox checkbox_prefill;
        private System.Windows.Forms.Button startTestBtn;
        private System.Windows.Forms.Label label9;
        private System.Windows.Forms.Label label8;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.TrackBar trackBar_writeSize;
        private System.Windows.Forms.TabControl tabControl;
        private System.Windows.Forms.PictureBox pictureBox_pass;
        private System.Windows.Forms.PictureBox pictureBox_fail;
    }
}

