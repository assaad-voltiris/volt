using System.Net;
using System.Text;
using System.Text.Json;
using System.Diagnostics;
using GenHTTP.Modules.Webservices;
using System.Text.Json.Serialization;

namespace Voltiris
{
    public class Settings
    {
        [JsonConverter(typeof(JsonStringEnumConverter))]
        public LogLevelEnum LogLevel {
            get { return Logger.Instance.LogLevel; }
            set { Logger.Instance.LogLevel = value; }
        }

        public string SerialPortName {get; set;} = "";
    }

    // An instance of this class is created by web engine
    public class SettingsWebServer
    {
        // --- start of singleton implementation ---

        private static SettingsWebServer? instance = null;

        public static SettingsWebServer? Instance { get { return instance; }}

        // Constructor ONLY called by web engine
        public SettingsWebServer ()
        {
            Debug.Assert (instance == null);
            instance = this;
            load ();
        }

        // --- end of singleton implementation ---

        protected Settings settings = new Settings ();

        public const string fileName = "settings.json";

        protected void load ()
        {
            try {
                using (var fs = File.Open(fileName, FileMode.Open, FileAccess.Read))
                {
                    var loadedSettings = JsonSerializer.Deserialize<Settings>(fs);
                    if (loadedSettings != null)
                        settings = loadedSettings;

                    SerialCom.Instance.open (settings.SerialPortName);
                } 
            }
            catch (Exception e)
            {
                Logger.Critical ("Cannot open settings file:" + e.ToString ());
            }
        }

        protected void save ()
        {
            try {
                using (var fs = File.Open(fileName, FileMode.Create, FileAccess.Write))
                {
                    var settingsString = JsonSerializer.Serialize<Settings>(settings);
                    {
                        if (settingsString != null)
                        {
                            Byte[] data = new UTF8Encoding(true).GetBytes(settingsString);
                            fs.Write(data, 0, data.Length);
                        }
                    }
                }
            }
            catch (Exception e)
            {
                Logger.Critical ("Cannot save settings on disk" + e.ToString ());
            }
        }


        [ResourceMethod("logLevels")]
        public List<string> GetLogLevels() // http://localhost:8080/settings/logLevels --> ["Debug","Info","Warnings","Errors"]
        {
            // https://stackoverflow.com/questions/50634416/c-sharp-convert-list-of-enum-values-to-list-of-strings
            var list = (from action in (LogLevelEnum[]) Enum.GetValues(typeof(LogLevelEnum)) select action.ToString()).ToList();
            return list;
        }

        [ResourceMethod("getLogLevel")]
        public string GetLogLevel() // http://localhost:8080/settings/getLogLevel --> Errors
        {
            return settings.LogLevel.ToString();
        }

        [ResourceMethod("setLogLevel")]
        public string? SetLogLevel(string level) // http://localhost:8080/settings/setLogLevel?level=Debug --> Debug 
        {
            try {
                Enum.TryParse(level, out LogLevelEnum newlogLevel);
                settings.LogLevel = newlogLevel;
                save ();
                return newlogLevel.ToString();
            }
            catch (ArgumentException e)
            {
                Logger.Error ("Invalid logger value '" + level + "': " + e.ToString ());       
            }
            catch (Exception e)
            {
                Logger.Error ("Cannot set log level because: " + e.ToString ());       
            }
            return settings.LogLevel.ToString ();
        }

        [ResourceMethod("getLogs")]
        public List<Logger.LogItem> GetLogs() // http://localhost:8080/settings/getLogs --> [{"date":"2023-06-28 16:58:17.655","logLevel":2,"description":"Voltiris.log is created."} ...
        {
            return Logger.Instance.ReadLogs ();
        }

        [ResourceMethod("getSerialPortName")]
        public string GetSerialPortName() // http://localhost:8080/settings/getSerialPortName --> xyz
        {
            return settings.SerialPortName;
        }

        [ResourceMethod("setSerialPortName")]
        public string SetSerialPortName(string name) // http://localhost:8080/settings/setSerialPortName?name=xyz  --> xyz
        {
            settings.SerialPortName = WebUtility.UrlDecode (name);
            save ();
            return settings.SerialPortName;
        }
    }
}

    /*
    public class BookResource
    {
        public List<Book> books = new  List<Book>
        {
            new Book { Name = "Moby Dick" }, 
            new Book { Name = "Bible" }
        };

        [ResourceMethod("get/:id")] // http://localhost:8080/books/get/0 --> {"name":"Moby Dick"}
        public Book? GetBook(int id)
        {
            return books [id]; // (id < 0 || id >= books.Count) ? null: books [id];
        }

        [ResourceMethod("add")] // http://localhost:8080/books/add?book=%7B"Name":"My%20Book"%7D
        public List<Book> AddBook(string book)
        {
            Book? b = JsonSerializer.Deserialize<Book>(book);

            if (b == null)
                throw new Exception ("Invalid book structure");

            books.Add (b);
            return books;
        }

        [ResourceMethod("list")]
        public List<Book> GetBooks() // http://localhost:8080/books/list --> [{"name":"Moby Dick"},{"name":"Bible"}]
        {
            return books;
        }

        [ResourceMethod("add_test")]
        public int Add(int a, int b) { // http://localhost:8080/books/add_test?a=1&b=2 --> 3
            return a + b;
        }

        [ResourceMethod("hello_world")]
        public String SayHello() { // http://localhost:8080/hello_world --> Hello World
            return "Hello World";
        }    
    }
    */