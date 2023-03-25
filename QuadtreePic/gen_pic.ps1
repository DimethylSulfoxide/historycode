rm .\times.log;
rm .\pics -Recurse
mkdir pics

for ($i = 100; $i -lt 450; $i = $i + 10) {
    $t = "./tree -i a.ppm -o ./pics/b-" + $i.ToString() + ".ppm -t " + $i.ToString();
    $t2 = ($t + " -m")
    $time = Measure-Command -Expression {Invoke-Expression $t};
    $time2 = Measure-Command -Expression {Invoke-Expression $t2};
    ($i.ToString() + "`t" + $time.TotalSeconds.ToString() + "`t" + $time2.TotalSeconds.ToString()) | Out-File -FilePath .\times.log -Append
}