$name = $args[0]

g++ assembler.c

if ($LastExitCode -eq 0) {
    .\a.exe "$name.txt" "$name.bin"

    g++ processor.c

    if ($LastExitCode -eq 0) {
        .\a.exe "$name.bin"
    }
}
