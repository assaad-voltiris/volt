using System.Text;
using System.IO.Ports;
using System.Text.RegularExpressions;
using System.Runtime.InteropServices;

namespace Voltiris
{
    // Serial communication
    // See: https://learn.microsoft.com/en-us/dotnet/api/system.io.ports.serialport?view=dotnet-plat-ext-7.0
    // Singleton: see implementation 4 of https://csharpindepth.com/articles/singleton

    // For RS485, check Controlling the RTS and DTR pins of Serial Port using C#
    // https://www.xanthium.in/Serial-Programming-using-Csharp-on-Windows

    public class SerialCom
    {
        // --- start of singleton implementation ---

        private static readonly SerialCom instance = new SerialCom ();

        // Explicit static constructor to tell C# compiler
        // not to mark type as beforefieldinit
        static SerialCom () {}

        public static SerialCom Instance { get { return instance; }}

        // --- end of singleton implementation ---


        private SerialPort serialPort = new SerialPort ();

        private readonly object serialLock = new object();

        private SerialCom ()
        {
            foreach (string s in SerialPort.GetPortNames())
                Logger.Trace ("Serial port '" + s +"' is available");
        }

        public void open (string serialPortName)
        {
            lock (serialLock)
            {
                try {
                    serialPort.PortName  = serialPortName;
                    serialPort.BaudRate  = 115200;
                    serialPort.Parity    = Parity.None;
                    serialPort.DataBits  = 8;
                    serialPort.StopBits  = StopBits.One;
                    //serialPort.Handshake = Handshake.None;
                    //serialPort.RtsEnable = true; // RS485
                    //serialPort.DtrEnable = true; // RS485

                    // Set the read/write timeouts
                    serialPort.ReadTimeout  = 100; //500; // !!!
                    serialPort.WriteTimeout = 500; // !!!

                    serialPort.Open();

                    serialPort.DiscardOutBuffer ();
                    serialPort.DiscardInBuffer ();

                    if (RuntimeInformation.IsOSPlatform(OSPlatform.OSX))
                    {
                        // Very slow port initialization on Mac.
                        // Have not found a clean way to check it!
                        Thread.Sleep (2000);
                    }
                    else
                        Logger.Warning ("SerialPort initialization not tested on this platform");

                    Logger.Information ("Serial port '" + serialPort.PortName + "' opened");
                }
                catch (UnauthorizedAccessException e)
                {
                    serialPort.Close ();
                    Logger.Critical ("Serial port access problem: " + e.ToString ());
                }
                catch (Exception e)
                {
                    serialPort.Close ();
                    Logger.Critical ("Cannot open serial port because: " + e.ToString ());
                }
            }
        }

        public void WriteAscii (CommandData cmd, bool timeoutWarning = true)
        {
            lock (serialLock)
            {
                if (!serialPort.IsOpen)
                    throw new Exception ("Serial port is not open");

                try {
                    string? cmdString = ":" + cmd.convertToString () + "\r\n";
                    Console.WriteLine ("WRITE: "+ cmdString);
                    var cmdBytes = Encoding.ASCII.GetBytes(cmdString);
                    serialPort.Write (cmdBytes, 0, cmdBytes.Length);
                }
                catch (InvalidOperationException e) // The specified port is not open.
                {
                    serialPort.Close ();
                    Logger.Critical ("Serial port disconnection: " + e.ToString ());
                    throw;
                }
                catch (TimeoutException) // The operation did not complete before the time-out period ended.
                {
                    if (timeoutWarning)
                        Logger.Warning ("Timeout during write operation");
                    throw;
                }
                catch (Exception) // ArgumentNullException
                {
                    throw;
                }
            }
        }

        public static byte toByte (char c)
        {
            if (c >= '0' && c <= '9')
                return (byte) (c - '0');
            if (c >= 'a' && c <= 'f')
                return (byte) (c - 'a' + 10);
            if (c >= 'A' && c <= 'F')
                return (byte) (c - 'A' + 10);
            throw new ArgumentOutOfRangeException ();    
        }

        public static byte toByte (string str)
        {
            if (str.Length != 2)
                throw new ArgumentOutOfRangeException ();

            byte hi  = toByte (str[0]);
            byte low = toByte (str[1]);

            return (byte) (hi << 4 | low);
        }

        public void ReadAscii (out List<byte> buffer)
        {
            buffer = new List<byte> ();

            lock (serialLock)
            {
                if (!serialPort.IsOpen)
                    throw new Exception ("Serial port is not open");

                try {
                    
                    var data = serialPort.ReadLine ();
                    Console.WriteLine ("READ: "+ data);

                    var match = Regex.Match(data, @":([0-9a-fA-F][0-9a-fA-F])+\r");

                    if (match.Success && match.Groups.Count >= 2 && match.Groups[1].Success)
                    {
                        foreach (Capture c in match.Groups[1].Captures)
                            buffer.Add (toByte(c.Value));
                    }
                }
                catch (InvalidOperationException e) // The specified port is not open.
                {
                    serialPort.Close ();
                    Logger.Critical ("Serial port disconnection: " + e.ToString ());
                    throw;
                }
                catch (TimeoutException) // The operation did not complete before the time-out period ended.
                {
                    Logger.Warning ("Timeout during write operation");
                    throw;
                }
                catch (Exception)
                {
                    throw;
                }
            }   
        }
    }
}