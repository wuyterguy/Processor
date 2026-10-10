g++ assembler.c

if ($LastExitCode -eq 0) {
    .\a.exe

    g++ processor.c

    if ($LastExitCode -eq 0) {
        .\a.exe @args
    }
}
