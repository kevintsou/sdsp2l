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
using System.IO;

namespace Xpacer
{
    partial class Form1
    {
        StreamWriter sw;
        string fileName;
        bool log_exist = false;

        public void vWriteLogFile(string logString)
        {
            vOpenExistLog();
            sw.WriteLine(DateTime.Now.ToString() + " : " + logString);
            vCloseLogFile();
        }

        public void vCloseLogFile() {
            sw.Close();
        }

        public void vCreateLogFile() {
            string folderName = string.Format("{0}\\log", Directory.GetCurrentDirectory()); ;
            string filePath = string.Format("{0}\\log\\{1}{2}.txt", Directory.GetCurrentDirectory(), "Logfile_", DateTime.Now.ToString("yyyy_dd_HH_mm_ss"));
            fileName = filePath;
            System.IO.Directory.CreateDirectory(folderName);
            //開始寫檔
            //sw = new StreamWriter(filePath, true);
            log_exist = true;
        }

        public void vOpenExistLog() {
            if (log_exist == true)
            {
                sw = new StreamWriter(fileName, true);
            }
            else {
                vCreateLogFile();
                sw = new StreamWriter(fileName, true);
            }
        }

    }
}
