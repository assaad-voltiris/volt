using System.Diagnostics;
using GenHTTP.Modules.Webservices;
using System.Text.RegularExpressions;
using System.Text.Json.Serialization;

namespace Voltiris
{
    public class CommandData
    {
        // Data of the command
        internal List<byte> data = new List<byte> ();

        public void add (byte value)
        {
            data.Add (value);
        }

        public void add (ushort value)
        {
            data.Add ((byte) ((value >> 8) & 0xff));
            data.Add ((byte) (value & 0xff));
        }

        public void addCRC8 ()
        {
            byte crc8 = 0;
            for (var i = 0; i < data.Count; i++)
                crc8 += data[i];
            add (crc8);
        }

        public string convertToString ()
        {
            return BitConverter.ToString(data.ToArray ()).Replace("-","");
        }
    }

    public class Commands
    {
        public enum ResultType
        {
            Succeed,    // Command template representing a success
            Error,      // Command template representing a failure
            Unknown,    // Only used by CommandsEngine: valid command structure but unknow
            ComTimeout, // Only used by CommandsEngine: serial com. timeout
            ComError,   // Only used by CommandsEngine: internal error
            ArgError    // Only used by webserver
        }

        public class ExpectedResponse
        {
            public ResultType Result { get; set; }

            public enum FieldType
            {
                SlaveAddress,
                Function,
                Address,
                Data,
                Count,
                CRC8
            }

            public uint Count {get; private set; } = 0;

            protected class ExpectedValue
            {
                public uint index;
                public enum Type {UINT8, UINT16};
                public Type type;
                public ushort value;
            }
            protected List<ExpectedValue> expectedValues = new List<ExpectedValue> ();

            // Tuple is <index, count, value>            
            public List<Tuple<uint, uint, FieldType>> fields = new List<Tuple<uint, uint, FieldType>> ();

            public void add8 (FieldType field, uint count)
            {
                fields.Add (new Tuple<uint, uint, FieldType> (Count, count, field));
                Count += count;
            }
            public void add8 (FieldType field, uint count, byte expectedValue)
            {
                var ev = new ExpectedValue
                {
                    index = Count,
                    type  = ExpectedValue.Type.UINT8,
                    value = (ushort) expectedValue
                };
                expectedValues.Add (ev);
                add8 (field, count);
            }

            public void add16 (FieldType field, uint count)
            {
                fields.Add (new Tuple<uint, uint, FieldType> (Count, 2 * count, field));
                Count += 2 * count;
            }
            public void add16 (FieldType field, uint count, ushort expectedValue)
            {
                var ev = new ExpectedValue
                {
                    index = Count,
                    type  = ExpectedValue.Type.UINT16,
                    value = expectedValue
                };
                expectedValues.Add (ev);
                add16 (field, count);
            }

            public void addCRC8 ()
            {
                add8 (FieldType.CRC8, 1);
            }

            public bool match (CommandData cmd)
            {
                if (cmd.data.Count != Count)
                    return false;
                foreach (var ev in expectedValues)
                {
                    switch (ev.type)
                    {
                        case ExpectedValue.Type.UINT8:
                            if (cmd.data [(int) ev.index] != (byte) ev.value)
                                return false;
                            break;

                        case ExpectedValue.Type.UINT16:
                            {
                                int hi  = (int) cmd.data [(int) ev.index];
                                int low = (int) cmd.data [(int) ev.index + 1];
                                if ((hi << 8 | low) != (int) ev.value)
                                    return false;
                            }
                            break;

                        default:
                            Debug.Assert (false);
                            break;
                    }
                }
                    
                var hasCRC8 = fields.Find (x => x.Item3 == FieldType.CRC8);
                if (hasCRC8 == null)
                    return true;
                
                byte crc8 = 0;
                for (var i = 0; i < (int) hasCRC8.Item1; i++)
                    crc8 += cmd.data[i];
                return crc8 == cmd.data [index: (int) hasCRC8.Item1];
            }

            public byte[]? get (FieldType field, CommandData cmd)
            {
                var hasField = fields.Find (x => x.Item3 == field);
                if (hasField == null)
                    return null;
                byte[] result = new byte[hasField.Item2];
                for (var i = 0; i < hasField.Item2; i++)
                    result [i] = cmd.data [(int) hasField.Item1 + i];
                return result;
            }

            public byte? get8 (FieldType field, CommandData cmd)
            {
                byte[]? values = get (field, cmd);
                if (values == null)
                    return null;
                Debug.Assert (values.Length == 1);
                return values[0];
            }

            public ushort? get16 (FieldType field, CommandData cmd)
            {
                byte[]? values = get (field, cmd);
                if (values == null)
                    return null;
                Debug.Assert (values.Length == 2);
                return (ushort) (((int) values[0]) << 8 | (int) values[1]);
            }
        }

        protected const byte ReadInputResults = 0x04;
       
        internal static CommandData readUint16ResultsQuery (byte slaveAddress, ushort address, ushort numberOfResults,
                                                              out List<ExpectedResponse> expectedResponses)
        {
            if (numberOfResults > 127)
                throw new ArgumentOutOfRangeException ("Maximun 127 Results allowed");

            var cmd = new CommandData ();
            cmd.add (slaveAddress);
            cmd.add (ReadInputResults);
            cmd.add (address);
            cmd.add (numberOfResults);
            cmd.addCRC8 ();

            var success = new ExpectedResponse { Result = Commands.ResultType.Succeed };
            success.add8    (ExpectedResponse.FieldType.SlaveAddress, 1, slaveAddress);
            success.add8    (ExpectedResponse.FieldType.Function, 1, ReadInputResults);
            success.add8    (ExpectedResponse.FieldType.Count, 1, (byte) (numberOfResults * 2)); 
            success.add16   (ExpectedResponse.FieldType.Data, (byte) numberOfResults);
            success.addCRC8 ();

            var failure = new ExpectedResponse { Result = Commands.ResultType.Error };
            failure.add8    (ExpectedResponse.FieldType.SlaveAddress, 1, slaveAddress);
            failure.add8    (ExpectedResponse.FieldType.Function, 1, ReadInputResults);
            failure.add8    (ExpectedResponse.FieldType.Count, 1, 0);
            failure.addCRC8 ();

            expectedResponses = new List<ExpectedResponse> ();
            expectedResponses.Add (success);
            expectedResponses.Add (failure);

            return cmd;
        }

        protected const byte PresetMultipleRegisters = 0x10;

        internal static CommandData writeUint16ResultsQuery (byte slaveAddress, ushort address, byte [] data,
                                                               out List<ExpectedResponse> expectedResponses)
        {
            if (data.Length > 0xff)
                throw new ArgumentOutOfRangeException ("Maximun data size of 0xff allowed");

            var cmd = new CommandData ();
            cmd.add (slaveAddress);
            cmd.add (PresetMultipleRegisters);
            cmd.add (address);

            int nbRegisters = data.Length / 2;
            if (nbRegisters * 2 < data.Length)
                nbRegisters++;

            cmd.add ((ushort)nbRegisters);
            cmd.add ((byte)data.Length);
            for (var i = 0; i < data.Length; i++)
                cmd.add (data[i]);
            cmd.addCRC8 ();

            var success = new ExpectedResponse { Result = Commands.ResultType.Succeed };
            success.add8    (ExpectedResponse.FieldType.SlaveAddress, 1, slaveAddress);
            success.add8    (ExpectedResponse.FieldType.Function, 1, PresetMultipleRegisters);
            success.add16   (ExpectedResponse.FieldType.Address, 1, address);
            success.add16   (ExpectedResponse.FieldType.Count, 1, (ushort) nbRegisters);
            success.addCRC8 ();

            var failure = new ExpectedResponse { Result = Commands.ResultType.Error };
            failure.add8    (ExpectedResponse.FieldType.SlaveAddress, 1, slaveAddress);
            failure.add8    (ExpectedResponse.FieldType.Function, 1, PresetMultipleRegisters);
            failure.add16   (ExpectedResponse.FieldType.Address, 1, address);
            failure.add16   (ExpectedResponse.FieldType.Count, 1, 0);
            failure.addCRC8 ();

            expectedResponses = new List<ExpectedResponse> ();
            expectedResponses.Add (success);
            expectedResponses.Add (failure);

            return cmd;
        }
    }

    public class CommandsWebServer
    {
        // --- start of singleton implementation ---

        private static CommandsWebServer? instance = null;

        public static CommandsWebServer? Instance { get { return instance; }}

        // --- end of singleton implementation ---

        // Constructor ONLY called by web engine
        public CommandsWebServer ()
        {
            Debug.Assert (instance == null);
            instance = this;
        }

        private Commands.ResultType execute (CommandData cmd, List<Commands.ExpectedResponse> expectedResponses,
                                             out Commands.ExpectedResponse? response, out CommandData result,
                                             bool timeoutWarning = true)
        {
            result = new CommandData ();
            response = null;
            try {
                SerialCom.Instance.WriteAscii (cmd, timeoutWarning);
                
                if (expectedResponses.Count == 0)
                    return Commands.ResultType.Succeed;

                SerialCom.Instance.ReadAscii (out List<byte> buffer);
                result.data = buffer.ToList ();
                
                foreach (var er in expectedResponses)
                {
                    if (er.match (result))
                    {
                        response = er;
                        return er.Result;
                    }
                }
                Logger.Trace ("Unknown response: " + result.convertToString ());
                return Commands.ResultType.Unknown;
            }
            catch (TimeoutException)
            {
                if (timeoutWarning)
                    Logger.Trace ("Timeout when executing command: " + cmd.convertToString ());
                return Commands.ResultType.ComTimeout;
            }
            catch (Exception)
            {
                Logger.Trace ("Error when executing command: " + cmd.convertToString ());
                return Commands.ResultType.ComError;
            }
        }      

        public class Result
        {
            [JsonConverter(typeof(JsonStringEnumConverter))]
            public Commands.ResultType Status {get; set;} = Commands.ResultType.Error;

            public List<int> Values {get; set;} = new List<int> ();
        }

        [ResourceMethod(path: "test")] 
        public string test ()
        {
            return new string ("This is a test");
        }

        [ResourceMethod("getRegister")]
        public Result getRegister (int id, int address) // http://localhost:8080/cmd/getRegister?id=1&address=256  --> {"status":"Succeed","values":[0]}
        {
            var result = new Result ();

            if (id < 0 || id > 247 || address < 0 || address > 0xffff)
            {
                result.Status =  Commands.ResultType.ArgError;
                return result;
            }

            lock (this)
            {
                var query = Commands.readUint16ResultsQuery ((byte) id, (ushort) address, 
                                            1, out List<Commands.ExpectedResponse> expectedResponses);

                result.Status = execute (query, expectedResponses, 
                                         out Commands.ExpectedResponse? responseTemplate,
                                         out CommandData responseData);

                if (result.Status == Commands.ResultType.Succeed)
                {   
                    Debug.Assert (responseTemplate != null);
                    ushort? value = responseTemplate.get16 (Commands.ExpectedResponse.FieldType.Data, responseData);
                    Debug.Assert (value != null);
                    result.Values.Add ((int) value);
                }
            }

            return result;
        }

        [ResourceMethod("setRegister")]
        public Result setRegister (int id, int address, int value) // http://localhost:8080/cmd/setRegister?id=1&address=800&value=-1  --> {"status":"Succeed","values":[]}
        {
            var result = new Result ();

            if (id < 0 || id > 247 || address < 0 || address > 0xffff || value < Int16.MinValue || value > 0xffff)
            {
                result.Status =  Commands.ResultType.ArgError;
                return result;
            }

            if (value < 0)
                value = unchecked((ushort) value);

            lock (this)
            {
                var data = new byte [2];
                data[0] = (byte) (value >> 8);
                data[1] = (byte) (value & 0xff);
                
                var query = Commands.writeUint16ResultsQuery ((byte) id, (ushort) address,
                                            data, out List<Commands.ExpectedResponse> expectedResponses);

                result.Status = execute (query, expectedResponses, 
                                    out Commands.ExpectedResponse? responseTemplate,
                                    out CommandData responseData);
            }
            return result;
        }

        [ResourceMethod("hardReset")]
        public string hardReset (int id) // http://localhost:8080/cmd/hardReset?id=0 --> Succeed
        {
            if (id < 0 || id > 247)
                return Commands.ResultType.ArgError.ToString ();

            lock (this)
            {
                var query = Commands.writeUint16ResultsQuery ((byte) id, 0, new byte[0], 
                                            out List<Commands.ExpectedResponse> expectedResponses);
                
                expectedResponses.Clear (); // Do not expect any responses as slaves are reseting!

                return execute (query, expectedResponses, 
                                out Commands.ExpectedResponse? responseTemplate,
                                out CommandData responseData).ToString ();
            }
        }

        byte toBin (string val)
        {
            int result = 0;
            int mask = 1;
            Debug.Assert (val.Length == 8);
            for (int i = 7; i >= 0; i--)
            {
                if (val[i] != '0')
                    result |= mask;
                mask <<= 1;
            }
            return (byte) result;
        }

        [ResourceMethod("resetSlaves")]
        public string resetSlaves (string ids) // http://localhost:8080/cmd/resetSlaves?ids=00000000000000000000000000000001 --> Succeed
        {
            if (ids.Length != 32)
                return Commands.ResultType.ArgError.ToString ();

            var data8 = new byte [4];
            data8[0]=toBin (ids.Substring (0, 8));
            data8[1]=toBin (ids.Substring (8, 8));
            data8[2]=toBin (ids.Substring (16, 8));
            data8[3]=toBin (ids.Substring (24, 8));
            
            lock (this)
            {
                var query = Commands.writeUint16ResultsQuery (0, 1, data8, 
                                            out List<Commands.ExpectedResponse> expectedResponses);
                
                expectedResponses.Clear (); // Do not expect any responses as this function is interpreted by all slaves!

                return execute (query, expectedResponses, 
                                out Commands.ExpectedResponse? responseTemplate,
                                out CommandData responseData).ToString ();
            }
        }


        protected Result serialNumber (int id, bool timeoutWarning) 
        {
            var result = new Result ();

            if (id < 0 || id > 247)
            {
                result.Status =  Commands.ResultType.ArgError;
                return result;
            }

            lock (this)
            {
                var query = Commands.readUint16ResultsQuery ((byte) id, 2, 
                                            4 /* 4 * uint16 */, out List<Commands.ExpectedResponse> expectedResponses);

                result.Status = execute (query, expectedResponses, 
                                         out Commands.ExpectedResponse? responseTemplate,
                                         out CommandData responseData, timeoutWarning);

                if (result.Status == Commands.ResultType.Succeed)
                {   
                    Debug.Assert (responseTemplate != null);
                    var serial = responseTemplate.get (Commands.ExpectedResponse.FieldType.Data, responseData);
                    Debug.Assert (serial != null);
                    for (var i = 0; i < serial.Length; i++)
                        result.Values.Add (serial[i]);
                }
            }
            return result;
        }

        const ushort memoryStartAddress  = 0x200;
        const ushort memoryEndAddress    = 0x2fe;
        const ushort memorySize = memoryEndAddress - memoryStartAddress;

        [ResourceMethod("readMemory")]
        public Result readMemory (int id, int address, int size) // http://localhost:8080/cmd/readMemory?id=1&address=0&size=16 --> {"status":"Succeed","values":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]}
        {
            var result = new Result ();

            int sizeInUint16 = size / 2;
            if (size % 2 == 1)
                sizeInUint16++;

            if (id < 0 || id > 247 || address + sizeInUint16 * 2 > memorySize)
            {
                result.Status =  Commands.ResultType.ArgError;
                return result;
            }

            lock (this)
            {
                var queryData = Commands.readUint16ResultsQuery ((byte) id, (ushort) (memoryStartAddress + address), (ushort) sizeInUint16,
                                            out List<Commands.ExpectedResponse> expectedResponsesData);

                result.Status = execute (queryData, expectedResponsesData, 
                                         out Commands.ExpectedResponse? responseTemplateData,
                                         out CommandData responseData);

                if (result.Status != Commands.ResultType.Succeed)
                    return result;

                Debug.Assert (responseTemplateData != null);
                var data = responseTemplateData.get (Commands.ExpectedResponse.FieldType.Data, responseData);
                Debug.Assert (data != null);
                Debug.Assert (data.Length >= size);
                for (var i = 0; i < size; i++)
                    result.Values.Add (data[i]);
            }
            return result;
        }

        [ResourceMethod("writeMemory")]
        public Result writeMemory (int id, int address, string data) // http://localhost:8080/cmd/writeMemory?id=1&address=0&data=0001020304ff --> {"status":"Succeed","values":[]}
        {
            var result = new Result ();
            var dataByte = new List<byte> (); 

            try {
                var match = Regex.Match(data, @"^([0-9a-fA-F][0-9a-fA-F])+$");

                if (match.Success && match.Groups.Count >= 2 && match.Groups[1].Success)
                {
                    foreach (Capture c in match.Groups[1].Captures)
                        dataByte.Add (SerialCom.toByte(c.Value));
                }
                else
                    throw (new Exception ("Invalid data string: '" + data + "'"));
            }
            catch (Exception)
            {
                result.Status = Commands.ResultType.ArgError;
                return result;
            }

            if (id < 0 || id > 247 || address < 0 || address + dataByte.Count > 0xff)
            {
                result.Status = Commands.ResultType.ArgError;
                return result;
            }

            lock (this)
            {
                var writeData = Commands.writeUint16ResultsQuery ((byte) id, (ushort) (memoryStartAddress + address), dataByte.ToArray (),
                                    out List<Commands.ExpectedResponse> expectedResponsesData);

                result.Status = execute (writeData, expectedResponsesData, 
                                         out Commands.ExpectedResponse? responseTemplateData,
                                         out CommandData responseData);
            }
            return result;
        }

        [ResourceMethod("getOptionInfo")]
        public Result getOptionInfo (int id, int optIndex) // http://localhost:8080/cmd/getOptionInfo?id=1&optIndex=0 --> {"status":"Succeed","values":[222,173,190,239,192,254,186,190]}
        {
            var result = new Result ();

            if (id < 0 || id > 247 || optIndex >= 0x50 - 0x20)
            {
                result.Status =  Commands.ResultType.ArgError;
                return result;
            }
           
            int size = 0;

            lock (this)
            {
                var querySize = Commands.readUint16ResultsQuery ((byte) id, (ushort) (0x20 + optIndex), 
                                            1, out List<Commands.ExpectedResponse> expectedResponsesSize);

                result.Status = execute (querySize, expectedResponsesSize, 
                                         out Commands.ExpectedResponse? responseTemplateSize,
                                         out CommandData responseSize);

                if (result.Status != Commands.ResultType.Succeed)
                    return result;
                
                Debug.Assert (responseTemplateSize != null);
                int? sizeInBytes = responseTemplateSize.get16 (Commands.ExpectedResponse.FieldType.Data, responseSize);
                Debug.Assert (sizeInBytes != null);
                size = (int) sizeInBytes;
            }
            result = readMemory (id, 0, (int) size);
                
            return result;
        }

        [ResourceMethod("serialNumber")]
        public Result serialNumber (int id) // http://localhost:8080/cmd/serialNumber?id=1 --> {"status":"Succeed","values":[222,173,190,239,192,254,186,190]}
        {
            return serialNumber (id, true);
        }

        [ResourceMethod("detectSlaves")]
        public Result detectSlaves () // http://localhost:8080/cmd/detectSlaves --> {"status":"Succeed","values":[1]}
        {
            var result = new Result ();
            const int retries = 3; // !!!

            var detected = new List<int> ();

            for (var i = 0; i < retries; i++)
            {
                var problem = new List<int> ();

                string mask = "";
                for (int sn = 1; sn < 33; sn++)
                {   
                    if (detected.Contains (sn))
                        continue;
                    var detect = serialNumber (sn, false);
                    
                    //if (i == 0 && sn == 1)
                    //    detect.Status = Commands.ResultType.Error;

                    switch (detect.Status)
                    {
                        case Commands.ResultType.Succeed:
                            detected.Add (sn);
                            mask = "1" + mask;
                            continue;
                        case Commands.ResultType.ComTimeout:
                            // No slave at this address
                            mask = "0" + mask;
                            continue;
                        default:
                            mask = "0" + mask;
                            problem.Add (sn);
                            continue;
                    }
                }
                Logger.Trace ("Slave detection, iteration " + i + ", detected: " + detected.Count + ", problematic: " + problem.Count);
                if (problem.Count == 0)
                    break;
                resetSlaves (mask);
            }

            result.Values = detected;       
            result.Status = Commands.ResultType.Succeed;

            return result;
        }
    }
}