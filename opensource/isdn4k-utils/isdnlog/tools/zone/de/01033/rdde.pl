#!/usr/bin/perl
require "getopts.pl";
&Getopts ("v:crdq:");
if (!$opt_q && !$opt_c && !$opt_r) {
&usage();
}
if ($opt_q) {
if ($opt_c || $opt_r) {
&usage();
}
}
if ($opt_c || $opt_r) {
if ($opt_q) {
&usage();
}
}
sub usage {
printf "%s -v <eigene vorwahl> [ -q <query vorwahl> | { -c | -r} ] [ -d ]\n", $0;
printf "where\t-d\tdebug\n";
printf "\t-c\tCity\n";
printf "\t-r\tR50\n";
exit (-1);
}
# ohne 0
$meinevorwahl = $opt_v if $opt_v;
$q_vorw = sprintf ("%s", $opt_q) if $opt_q;
$vorwahl = "/usr/lib/isdn/vorwahl.dat";
open (VORWAHL, "<$vorwahl") || die "cannot open $vorwahl: $!\n";
# alle Vorwahlen einlesen
read (VORWAHL, $anzahl, 2, 0);
$anz = unpack ("S", $anzahl);
printf ("Anzahl = %ld, Anz = %s\n", $anzahl, $anz) if $opt_d;
# Wert 4*$anz+3 weicht von der Vorgabe ab!
# Soll: 4*(AnzVorwahl + 1) -> 1 weniger
seek (VORWAHL, 4*$anz+3, 0);
$index=-1;
for ($i = 0; $i < $anz ; $i++) {
read (VORWAHL, $num, 2);
$nummer[$i] = unpack ("s", $num) + 32768;
printf ("nummer[%d]=%d\n", $i, $nummer[$i]) if $opt_d;
if ($meinevorwahl == $nummer[$i]) {
$index = $i;
printf ("Index = %d\n", $index) if $opt_d;
}
}
if($index==-1) {
print "UNKNOWN $meinevorwahl\n";
exit 2;
}
# Pointerliste einlesen
seek (VORWAHL, 3, 0);
for ($i = 0; $i < $anz; $i++) {
read (VORWAHL, $ptr, 4);
$pointer[$i] = unpack ("L", $ptr);
printf ("Pointer[%d]=%lu\n", $i, $pointer[$i]) if $opt_d;
}
# jetzt auf den Pointer zu meiner Vorwahl stellen
# Pointer - 1???
$p = $pointer[$index]-1;
seek (VORWAHL, $p, 0);
printf ("-> %lu\n", $p) if $opt_d;
# Nahbereich einlesen
read (VORWAHL, $anz_nah, 2);
$anzahl_nah = unpack ("S", $anz_nah);
printf ("Anzahl Nah-Nummern fÅr $meinevorwahl: %d\n", $anzahl_nah) if $opt_d;
for ($i = 0; $i < $anzahl_nah; $i++) {
read (VORWAHL, $num, 2);
$vorw = unpack ("s", $num) + 32768;
printf ("%d\n", $vorw) if $opt_c;
$a_vorw = sprintf ("%s", $vorw);
if ($opt_q && $q_vorw =~ /^$a_vorw/) {
printf "City\n";
close VORWAHL;
# exitcode 1 fÅr City
exit (1);
}
}
# R50 einlesen
read (VORWAHL, $anz_reg, 2);
$anzahl_reg = unpack ("s2", $anz_reg);
printf ("Anzahl R50-Nummern fÅr $meinevorwahl: %d\n", $anzahl_reg) if $opt_d;
for ($i = 0; $i < $anzahl_reg; $i++) {
read (VORWAHL, $num, 2);
$vorw = unpack ("s2", $num) + 32768;
printf ("%d\n", $vorw) if $opt_r;
$a_vorw = sprintf ("%s", $vorw);
if ($opt_q && $q_vorw =~ /^$a_vorw/) {
printf "R50\n";
close VORWAHL;
# exitcode 2 fÅr R50
exit (2);
}
}
close VORWAHL;
exit (0);

