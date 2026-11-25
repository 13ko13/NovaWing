using System.Reflection;
using Effekseer;

// EffekseerCore.dll(1.80.7)を使って、エフェクトの動的パラメーターを書き換えたり、.efkproj を .efkefc/.efk に変換したりする小さな道具。
// ビルド: dotnet build -c Release -o out   (efkeq.csproj の HintPath に EffekseerCore.dll の場所)
// <bin> = EffekseerCore.dll があるフォルダ(例: C:\Users\SakamotoKou\Downloads\Effekseer1.80.7Win\Effekseer1.80.7Win\Tool\bin)
// 使い方:
//   efkeq <bin> <effect.efkefc>                                   式と入力を表示
//   efkeq <bin> <effect.efkefc> xml                               中身をXMLで表示
//   efkeq <bin> <in.efkproj> convert <out.efkefc>                 .efkefc で保存し、.efk も倍率50で書き出す
//   efkeq <bin> <effect.efkefc> apply <式の名前>=<コード> input<N>=<値> ...   式・入力を書き換えて保存+.efk書き出し
var bin = args[0];
var path = Path.GetFullPath(args[1]);
AppDomain.CurrentDomain.AssemblyResolve += (s, e) =>
{
    var p = Path.Combine(bin, new AssemblyName(e.Name).Name + ".dll");
    return File.Exists(p) ? Assembly.LoadFrom(p) : null;
};
Directory.SetCurrentDirectory(bin);
Run();

void Run()
{
    Core.Initialize("en");
    if (!Core.LoadFrom(path)) { Console.WriteLine("load failed"); Environment.Exit(1); }

    var dyn = Core.Dynamic;
    void Dump()
    {
        for (int i = 0; i < dyn.Inputs.Values.Count; i++)
            Console.WriteLine($"input{i} = {dyn.Inputs.Values[i].Input.Value}");
        foreach (var eq in dyn.Equations.Values)
            Console.WriteLine($"eq [{eq.Name.Value}]\n{eq.Code.Value}\n---");
    }
    Dump();
    if (args.Length >= 3 && args[2] == "xml")
    {
        var doc = Core.SaveAsXmlDocument(Core.Root);
        var sw = new StringWriter();
        doc.Save(sw);
        Console.WriteLine(sw.ToString());
        return;
    }
    if (args.Length >= 4 && args[2] == "convert")
    {
        // efkproj -> efkefc(保存) + efk(倍率50で書き出し)
        var outPath = Path.GetFullPath(args[3]);
        Core.SaveTo(outPath);
        var b = new Effekseer.Binary.Exporter().Export(Core.Root, 50f);
        File.WriteAllBytes(Path.ChangeExtension(outPath, ".efk"), b);
        Console.WriteLine($"converted -> {outPath} and .efk ({b.Length} bytes)");
        return;
    }
    if (args.Length < 3 || args[2] != "apply") return;

    foreach (var a in args.Skip(3))
    {
        int k = a.IndexOf('=');
        string key = a[..k], val = a[(k + 1)..].Replace("\\n", "\n");
        if (key.StartsWith("input"))
        {
            dyn.Inputs.Values[int.Parse(key[5..])].Input.SetValue(float.Parse(val));
            continue;
        }
        var eq = dyn.Equations.Values.FirstOrDefault(x => x.Name.Value == key)
                 ?? throw new Exception("no equation " + key);
        eq.Code.SetValue(val);
    }
    Console.WriteLine("=== after");
    Dump();

    Core.SaveTo(path);
    var bytes = new Effekseer.Binary.Exporter().Export(Core.Root, 50f);
    File.WriteAllBytes(Path.ChangeExtension(path, ".efk"), bytes);
    Console.WriteLine($"saved {path} and .efk ({bytes.Length} bytes)");
}
