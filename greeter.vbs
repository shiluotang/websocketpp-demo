While Not WScript.StdIn.AtEndOfStream
    line = WScript.StdIn.ReadLine()
    WScript.Stdout.WriteLine("Hello, " & line & "!")
Wend
