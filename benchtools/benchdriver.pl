#!/usr/bin/perl -w
#
# Benchmark-Steuerscript
#
# Sollte via crontab @reboot mit Parameter "--continue" aufgerufen werden
# (evtl. via Hilfsscript um etwas zu warten bis das System "ruhig" ist)
#
# Zur vollständigen Funktion ist es notwendig, dass der ausführende Benutzer
# mittels sudo ohne Passwort den Befehl reboot ausführen darf.
#
#
# Aufbau der Benchmark-Scriptdatei:
# - Alle Leerzeichen am Zeilenanfang/-ende werden ignoriert
# - Alle Zeilen, die mit einem # anfangen werden ignoriert
# - Alle Leerzeilen werden ignoriert
# - Kommentare innerhalb einer Zeile sind zZt nicht moeglich
# - Ein Kommando pro Zeile
#   - reboot
#     Führt einen reboot aus. Wenn danach benchdriver.pl --continue aufgerufen
#     wird, wird die Ausführung der Scriptdatei hinter dem reboot fortgesetzt.
#   - run
#     Nimmt alles was nach dem run in der Zeile steht, ignoriert davon führende
#     Leerzeichen und ruft es mit system() auf
#   - echo
#     Nimmt alles was nach dem echo in der Zeile steht, ignoriert davon
#     führende Leerzeichen und gibt es aus.
#   - iterate <zahl>
#     Führt die Zeilen zwischen iterate und dem nächsten enditer
#     so oft aus wie <zahl> angibt. Es ist nur eine Iteration gleichzeitig
#     möglich, die aktuelle Iterationsnummer (0-basiert) steht in <ITERATION>
#   - enditer
#     Beendet einen iterate-Block
#   - FOO=BAR
#     Setzt die Variable FOO auf den Wert BAR. Variablennamen sind beliebige
#     Strings, die kein Whitespace, kein = und kein > enthalten dürften. Es
#     darf kein Whitespace zwischen dem Variablennamen und dem = stehen.
#
# - Variablensubstitution mit <VARIABLENNAME>
#   Definierte Variablen können mit <VARIABLENNAME> angesprochen werden.
#   Wenn keine Variable des angegebenen Namens existiert wird nach einer
#   Environment-Variablen dieses Namens gesucht. Wenn auch diese nicht
#   existiert wird die Ersetzung nicht ausgeführt. Die einzige vordefinierte
#   Variable ist <ITERATION>, innerhalb eines iterate..enditer-Blocks
#

use Cwd 'abs_path';
use Data::Dumper;
use strict;

my $HOME= $ENV{HOME};
my $no_act = 0;

$ENV{SUDO_ASKPASS} = "/bin/true";

# saved variables
my $config_file = "";
my $current_line = 0;
my $iter_startline = -1;
my $iter_count = 0;
my $iter_limit = -1;
my %VARS;

sub config_getline($) {
  my $linenum = shift;
  my $curline = 0;
  my $line;

  open CONF,"<",$config_file or die "Can't open $config_file: $!";
  while (defined($line=<CONF>)) {
    chomp $line;
    last if ($curline == $linenum);
    $curline++;
  }
  close CONF;

  return $line;
}

sub save_state() {
  unlink "$HOME/.benchstate.old";
  if (-e "$HOME/.benchstate") {
    rename "$HOME/.benchstate","$HOME/.benchstate.old" or die "Can't rename benchstate to old: $!";
  }
  open OUT,">","$HOME/.benchstate" or die "Can't open $HOME/.benchstate: $!";
  print OUT Data::Dumper->Dump([$config_file, $current_line, $iter_startline, $iter_count, $iter_limit, \%VARS],
                               [qw/config_file current_line iter_startline iter_count iter_limit VARS/]);
  close OUT;
}

sub read_state() {
  my $data;

  open IN,"<","$HOME/.benchstate" or die "Can't open $HOME/.benchstate: $!";
  while (<IN>) {
    $data .= $_;
  }
  close IN;
  # Hack, Data::Dumper can't dump actual hashes, just hashrefs
  my $VARS;
  eval $data;
  %VARS = %{$VARS};
}

# Option parsing
if (scalar(@ARGV) < 1 || scalar(@ARGV) > 2 || (scalar(@ARGV) == 2 && $ARGV[0] ne "-n")) {
  print "Usage: $0 [-n] configfile\n";
  exit 1;
} else {
  if (scalar(@ARGV) == 2 && $ARGV[0] eq "-n") {
    # no-act option
    $no_act = 1;
    shift @ARGV;
  }

  if ($ARGV[0] eq "--continue") {
    if (-e "$HOME/.benchstate") {
      read_state();
    } else {
      print "ERROR: $HOME/.benchstate is missing!\n";
      exit 1;
    }
  } else {
    $config_file = abs_path($ARGV[0]);
    if (! -e $config_file) {
      print "ERROR: Config file $config_file does not exist!\n";
      exit 1;
    }
  }
}

while (defined($_ = config_getline($current_line))) {
  $current_line++;
  next if /^\s*#/; # ignore comments
  next if /^\s*$/; # ignore empty lines

  # Remove whitespace from beginning and end
  s/^\s+//;
  s/\s+$//;

  # Replace variables
  while (/<([^ \t>]+)>/g) {
    my $var = $1;
    if (exists($VARS{$var})) {
      s/<$var>/$VARS{$var}/g;
    } elsif (exists($ENV{$var})) {
      s/<$var>/$ENV{$var}/g;
    } else {
      print "ERROR: Variable <$var> does not exist (neither locally, nor in environment).\n";
      exit 1;
    }
  }

  # Parse commands (in just about the worst way possible)
  if (/^reboot/) {
    if ($no_act) {
      print "(reboot)\n";
    } else {
      save_state();
      system "sudo -A shutdown -fr now 'requested by benchdriver.pl'";
      exit 0;
    }

  } elsif (/^run\s+(.*)$/) {
    # FIXME: Any need for quoting?
    # FIXME: Add run_background?
    if ($no_act) {
      print "run $1\n";
      my $program = (split / /,$1)[0];
      if (! -e $program) {
          print "ERROR: $program does not exist\n";
          exit 1;
      }
    } else {
      system $1;
    }

  } elsif (/^echo\s+(.*)$/) {
    print "$1\n";

  } elsif (/^iterate\s+(\d+)/) {
    if ($iter_startline >= 0) {
      print "ERROR: Nested iteration detected in line $current_line\n";
      exit 1;
    }
    $iter_startline = $current_line;
    $iter_count = 0;
    $iter_limit = $1;
    $VARS{ITERATION} = $iter_count;

  } elsif (/^enditer/) {
    if ($iter_startline < 0) {
      print "ERROR: enditer without iterate in line $current_line\n";
      exit 1;
    }
    $iter_count++;
    $VARS{ITERATION} = $iter_count;
    if (!$no_act && $iter_count < $iter_limit) {
      $current_line = $iter_startline;
    } else {
      $iter_startline = -1;
    }

  } else {
    if (/^([^ \t=]+)=(.*)/) {
      # set variable
      $VARS{$1} = $2;
    } else {

      print "ERROR: Can't parse line $current_line: >$_<\n";
      exit 1;
    }
  }
}

print "--- Done! ---\n";

save_state();
