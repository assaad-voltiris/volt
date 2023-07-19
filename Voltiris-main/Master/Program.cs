// See https://genhttp.org/documentation/
using GenHTTP.Engine;
using GenHTTP.Modules.IO;
using GenHTTP.Modules.Practices;
using GenHTTP.Modules.Webservices;
using GenHTTP.Modules.Security;
using GenHTTP.Modules.Layouting;
using GenHTTP.Modules.Placeholders;
using GenHTTP.Modules.StaticWebsites;

namespace Voltiris
{
    class Program
    { 
        static void Main(string[] args)
        { 
            try {

                // Create static website
                var tree = ResourceTree.FromDirectory("./web");
                var app = StaticWebsite.From(tree);
             
                var services = Layout.Create()
                        .Index (Page.From("Home", "Hello World!")) // http://localhost:8080
                        .Add("api", app) // http://localhost:8080/api/
                        .AddService<SettingsWebServer>("settings") // http://localhost:8080/settings/...
                        .AddService<CommandsWebServer>("cmd") // http://localhost:8080/cmd/...
                        .Add(CorsPolicy.Permissive());

                Host.Create()
                    .Console()
                    .Defaults()
                    .Handler(services)
                    .Run();
         
            }
            catch (Exception e)
            {
                Console.WriteLine(e.ToString ());
            }
            
            Console.WriteLine("End!");
            System.Environment.Exit(1);
        }
    }
}