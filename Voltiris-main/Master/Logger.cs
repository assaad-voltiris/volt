using System.Text.Json.Serialization;

namespace Voltiris
{
    public enum LogLevelEnum
    { 
        Trace, // Contain the most detailed messages. These messages may contain sensitive app data. These messages are disabled by default and should not be enabled in production.
        Debug, // For debugging and development. Use with caution in production due to the high volume.
        Information, // Tracks the general flow of the app. May have long-term value.
        Warning, // For abnormal or unexpected events. Typically includes errors or conditions that don't cause the app to fail.
        Error, // For errors and exceptions that cannot be handled. These messages indicate a failure in the current operation or request, not an app-wide failure.
        Critical, // For failures that require immediate attention. Examples: data loss scenarios, out of disk space.
        None // Specifies that no messages should be written.
    }

    

    // Log to file
    // Singleton: see implementation 4 of https://csharpindepth.com/articles/singleton
    public class Logger
    {
        // --- start of singleton implementation ---

        private static readonly Logger instance = new Logger ();

        // Explicit static constructor to tell C# compiler
        // not to mark type as beforefieldinit
        static Logger () {}

        public static Logger Instance { get { return instance; }}

        // --- end of singleton implementation ---
        
        
        private const string fileExt = ".log";
        private readonly object fileLock = new object();
        private readonly string datetimeFormat;
        private readonly string logFilename;

        public LogLevelEnum LogLevel { get; set; } = LogLevelEnum.Information;

        public static int GetLogLevelAsInt (LogLevelEnum l)
        {
            switch (l)
            {
                case LogLevelEnum.Trace:       return 0;
                case LogLevelEnum.Debug:       return 1;
                case LogLevelEnum.Information: return 2;
                case LogLevelEnum.Warning:     return 3;
                case LogLevelEnum.Error:       return 4;
                case LogLevelEnum.Critical:    return 5;
                case LogLevelEnum.None:        return 6;
            }
            return 6;
        }

        // Constructor will only be called by Singletin
        private Logger ()
        {
            datetimeFormat = "yyyy-MM-ddTHH:mm:ss.fffZ"; // Store date in a format supported by JS;
            logFilename = System.Reflection.Assembly.GetExecutingAssembly().GetName().Name + fileExt;

            // Log file header line
            string logHeader = logFilename + " is created";
            if (!System.IO.File.Exists(logFilename))
                WriteFormattedLog (LogLevelEnum.Information, logFilename + " is created.");
            WriteFormattedLog (LogLevelEnum.Information, "Logger started");
        }

        public class LogItem
        {
            public string Date {get; set;}  = "";

            [JsonConverter(typeof(JsonStringEnumConverter))]
            public LogLevelEnum LogLevel {get; set;}
            
            public string Description {get; set;} = "";
        }

        public List<LogItem> ReadLogs ()
        {
            var result = new List<LogItem> ();
            
            lock (fileLock) 
            {
                using (var reader = new System.IO.StreamReader(logFilename, System.Text.Encoding.UTF8))
                {
                    while (true)
                    {
                        var line = reader.ReadLine();
                        if (line == null)
                            break;
                        try 
                        {
                            string[] items = line.Split ( '\t');
                            Enum.TryParse(items [1], out LogLevelEnum logLevelItem);
                            result.Add (new LogItem { Date = items [0], LogLevel = logLevelItem, Description = items [2]});
                        }
                        catch
                        {
                            continue; // Invalid line structure in log
                        }
                    }
                }
            }
            return result;              
        }

        private void WriteLine (string text)
        {
            try
            {
                if (string.IsNullOrEmpty (text))
                {
                    return;
                }
                lock (fileLock) 
                {
                    using (var writer = new System.IO.StreamWriter(logFilename, true, System.Text.Encoding.UTF8))
                    {
                        writer.WriteLine(text);
                    }
                }
            }
            catch
            {
                throw; // No error if log cannot be saved on disk !
            }
        }

        private void WriteFormattedLog (LogLevelEnum level, string text)
        {
            WriteLine (System.DateTime.Now.ToString(datetimeFormat) + "\t" + level.ToString () + "\t" + text);
        }
            
        public static void Trace(string text) 
        {
            if (GetLogLevelAsInt (Instance.LogLevel) <= GetLogLevelAsInt (LogLevelEnum.Trace))
                Instance.WriteFormattedLog (LogLevelEnum.Trace, text);
        }
        
        public static void Debug(string text)
        {
            if (GetLogLevelAsInt (Instance.LogLevel) <= GetLogLevelAsInt (LogLevelEnum.Debug))
                Instance.WriteFormattedLog (LogLevelEnum.Debug, text);
        }

        public static void Information(string text)
        {
            if (GetLogLevelAsInt (Instance.LogLevel) <= GetLogLevelAsInt (LogLevelEnum.Information))
                Instance.WriteFormattedLog (LogLevelEnum.Information, text);
        }
        
        public static void Warning(string text)
        {
            if (GetLogLevelAsInt (Instance.LogLevel) <= GetLogLevelAsInt (LogLevelEnum.Warning))
                Instance.WriteFormattedLog (LogLevelEnum.Warning, text);
        }

        public static void Error(string text)
        {
            if (GetLogLevelAsInt (Instance.LogLevel) <= GetLogLevelAsInt (LogLevelEnum.Error))
                Instance.WriteFormattedLog (LogLevelEnum.Error, text);
        }
        
        public static void Critical(string text)
        {
            if (GetLogLevelAsInt (Instance.LogLevel) <= GetLogLevelAsInt (LogLevelEnum.Critical))
                Instance.WriteFormattedLog (LogLevelEnum.Critical, text);
        }
    }
}