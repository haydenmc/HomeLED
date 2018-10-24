using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading;
using System.Threading.Tasks;
using Windows.Devices.Enumeration;
using Windows.Devices.SerialCommunication;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Storage.Streams;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace HeatMapApp
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        public const UInt16 Vid = 0x2341;
        public const UInt16 Pid = 0x0043;
        private const int horizontalCells = 4;
        private const int verticalCells = 4;
        private const int maxTemp = 50;
        private const int minTemp = 20;
        private ObservableCollection<ThermalCell> thermalCells = new ObservableCollection<ThermalCell>();

        public MainPage()
        {
            this.InitializeComponent();
            for (var i = 0; i < horizontalCells * verticalCells; ++i)
            {
                thermalCells.Add(new ThermalCell());
            }
            StartSerial();
        }

        private async void StartSerial()
        {
            var deviceSelector = SerialDevice.GetDeviceSelectorFromUsbVidPid(Vid, Pid);
            var deviceInfo = await DeviceInformation.FindAllAsync(deviceSelector);
            if (deviceInfo != null && deviceInfo.Count > 0)
            {
                var serialDevice = await SerialDevice.FromIdAsync(deviceInfo[0].Id);
                serialDevice.BaudRate = 9600;
                serialDevice.DataBits = 8;
                serialDevice.Parity = SerialParity.None;
                serialDevice.StopBits = SerialStopBitCount.One;
                serialDevice.Handshake = SerialHandshake.None;

                await Task.Run(async () =>
                {
                    var reader = new DataReader(serialDevice.InputStream);
                    reader.InputStreamOptions = InputStreamOptions.Partial;
                    char[] buf = new char[128];
                    uint bufIndex = 0;
                    while (true)
                    {
                        uint bytesRead = await reader.LoadAsync(16);
                        for (var i = 0; i < bytesRead; ++i)
                        {
                            byte inByte = reader.ReadByte();
                            if ((char)inByte == '\n')
                            {
                                var inputLine = new String(buf.Take((int)bufIndex).ToArray());
                                updateCellValues(inputLine);
                                bufIndex = 0;
                            }
                            else
                            {
                                buf[bufIndex] = (char)inByte;
                                ++bufIndex;
                            }
                        }
                    }
                });
            }
        }

        private async void updateCellValues(string inputLine)
        {
            if (inputLine.EndsWith(','))
            {
                inputLine = inputLine.Substring(0, inputLine.Length - 1);
            }
            var cellValues = inputLine.Split(',');
            await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, new Windows.UI.Core.DispatchedHandler(() =>
            {
                for (var i = 0; i < cellValues.Length; ++i)
                {
                    if (i < thermalCells.Count)
                    {
                        float val = 0;
                        if (float.TryParse(cellValues[i], out val))
                        {
                            thermalCells[i].CellValue = val / 10f;
                        }
                    }
                }
            }));
        }

        public static SolidColorBrush ToColor(float num)
        {
            var percent = Math.Clamp((num - minTemp) / (float)(maxTemp - minTemp), 0, 1);
            var color = new Windows.UI.Color();
            color.A = 255;
            color.R = 255;
            color.G = (byte)(255 - percent * 255);
            color.B = (byte)(255 - percent * 255);

            return new SolidColorBrush(color);
        }
    }
}
