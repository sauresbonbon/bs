# Test von cd (Wechsel in /tmp) und pwd (aktuelles Verzeichnis anzeigen)
cd /tmp
pwd

# Test: cd in nicht existierendes Verzeichnis (Fehlerfall)
cd xx

# Test: cd ohne Argument (zurück ins Home-Verzeichnis)
cd
pwd

# Test von logischen Operatoren && und || (Erfolg/Misserfolg von Befehlen)
true && echo yay
false && echo nope
true || echo nope
false || echo yay

# Test: Verhalten bei Eingabe über stdin (cat wartet auf Input)
cat && echo nope
cat || echo yay

# Test von Output-Redirection (Überschreiben mit >)
echo hallo > f
cat f

# Test von Append-Redirection (Anhängen mit >>)
echo hallo >> f
cat f

# Test von Input-Redirection (<)
cat < f

# Kombination von Input- und Output-Redirection
cat < f >> f1
cat >> f1 < f

# Anzeige des Ergebnisses
cat f1

# Test: Fehler bei nicht existierender Eingabedatei
cat < xyz

# Test: Datei anlegen und Rechte ändern (keine Schreibrechte)
touch outfile
chmod 000 outfile

# Test: Fehler bei Output-Redirection ohne Berechtigung
ls >> outfile

# Test einfacher Pipes
cat | cat | cat

# Pipe zwischen zwei Programmen
ls -l | wc

# Test: Pipe mit Programm ohne stdin-Verarbeitung
xterm | cat

# Binärdaten durch Pipe verarbeiten
cat /bin/bash | od -x | head -1

# Längere Pipe-Kette
cat | cat | cat | cat | cat | cat | rev

# Test: Status des letzten Befehls
status

# Test: Hintergrundprozesse (&)
ls -l xzy &
xterm &
xterm &
ps &

# Status von Hintergrundprozessen
status

# Prozess beenden (Signal senden)
kill -9 xtermpid

# Status nach Prozessbeendigung
status

# Lange Pipe zur Belastungsprobe
cat | cat | cat | cat | cat | cat | cat | cat | cat | rev

# Danach müssen ein paar Eingaben gemacht werden!
# bshell [/home/ar/bshell/src]> cat | cat | cat | cat | cat | cat | cat | cat | cat | rev
# hallo
# ollah
#
#

# Anschließend: In einem neuen Terminal -> Killen eines Prozesses in der Mitte der Pipe. (Die Pid muss man sich mittel ps heraussuchen)

kill -9 <pid_eines_prozesses>

# Danach muss ein paar mal die Enter Taste gedrückt werden.

# Status erneut prüfen
status

# Die Ausgabe muss so aussehen (andere PIDS):
#     PID     PGID     STATUS                 NAME
# 3514981  3514972    exit(0)                  rev
# 3514980  3514972    exit(0)                  cat
# 3514979  3514972    exit(0)                  cat
# 3514978  3514972    exit(0)                  cat
# 3514977  3514972    exit(0)                  cat
# 3514976  3514972  signal(9)                  cat
# 3514975  3514972 signal(13)                  cat
# 3514974  3514972 signal(13)                  cat
# 3514973  3514972 signal(13)                  cat
# 3514972  3514972 signal(13)                  cat


# Wichtig ist hierbei, dass der Prozess, der das SIGKILL Signal bekommen hat, in der Liste mir signal(9) auftaucht,
# alle Prozesse mit niedrigerer Pid müssen das signal(13) (SIGPIPE) aufzeigen und alle mit höherer mit den status exit(0)
# Nur dann ist die Hausübung shell-advanced bestanden

