#!/usr/bin/perl
# 
# Rules.ini to freecnc .ini file converter
# by Jonas Jermann <jjermann@gmx.net>
#
# TODO: DOG, MEDI, V2RL, CA, DD, etc...

use Getopt::Long;
my %set,$opt;

# initial values
$set{struct}=1;
$set{unit}=1;
$set{weapon}=1;
$set{prereqs}=1;

GetOptions(
  "structure|s" => sub { $set{unit}=0;$set{weapon}=0; },
  "unit|u"      => sub { $set{struct}=0;$set{weapon}=0; },
  "weapon|w"    => sub { $set{struct}=0;$set{unit}=0; },
) || print STDERR "Usage: rules2freecnc.pl -[suw] rules.ini\n";

if (!$ARGV[0]) { $set{file}="rules.ini"; }


open(RULES,"cat $set{file}|");
  print "; Converted rules.ini ($set{unit},$set{weapon},$set{struct})\n";
  while(<RULES>)
  {
    $opt=$_; chop $opt;

# What to print...
    if ($opt=~m/^\[V2RL\]/) { $set{atunit}=1; }
    if ($opt=~m/^\[IRON\]/) { $set{atstruct}=1; $set{atunit}=0; }
    if ($opt=~m/^\[Colt45\]/) { $set{atweapon}=1; $set{atstruct}=0; }
    if ($opt=~m/^\[Clear\]/) { $set{atweapon}=0; }
    next if (!(($set{atunit} && $set{unit})||($set{atstruct} && $set{struct})||($set{atweapon} && $set{weapon})));
    if ($opt=~m/^\[.*/) {
      $set{end}=1;
#PRE PRINTS
      print "prerequisites=none\n" if ($set{prereq} eq 0 && ($set{atunit} eq 1 || $set{atstruct} eq 1));
      print "\n"; 
    }

# unittype (the print has to be after DOG, HARV, JEEP debug, see at end)
    if ($opt=~m/^\[V2RL\]/) { $set{utype}=1; }
    if ($opt=~m/^\[SS\]/) { $set{utype}=2; }
    if ($opt=~m/^\[DOG\]/) { $set{utype}=0; }
    if ($opt=~m/^\[BADR\]/) { $set{utype}=3; }

#NORMAL PRINTS
# Structures/Units
    if ($opt=~m/Owner/)
    {
      $opt=~s/Owner/owners/;
    }
    if ($set{atunit} || $set{atstruct}) { $opt=~s/Image=(.*)/image1=$1.SHP/; }
    if ($opt=~m/Prerequisite/) { $opt=~s/Prerequisite/prerequisites/; $set{prereq}=1; }
    $opt=~s/Strength/health/;
    $opt=~s/Armor/armour/;
    $opt=~s/TechLevel/techlevel/;
    next if ($opt=~m/Primary=none/);
    $opt=~s/Primary/primary_weapon/;
    $opt=~s/Secondary/secondary_weapon/;
    $opt=~s/Cost/cost/;
    $opt=~s/Speed/speed/;
    $opt=~s/Cloakable/cloakable/;
    $opt=~s/Invisible/invisible/;
    $opt=~s/Sight/sight/;
    $opt=~s/Cloakable/cloakable/;
    if ($opt=~m/Power=-.*$/) { $opt=~s/Power=-(.*)$/drain=$1/; }
    elsif ($opt=~m/Power=.*$/) { $opt=~s/Power=(.*)$/power=$1/; }
    $opt=~s/Powered/powered/;
    $opt=~s/Passengers=(.*)/maxpassengers=$1\npassengersallow=0/;

# Weapons
    $opt=~s/Projectile/projectile/;
    $opt=~s/Warhead/warhead/;
    $opt=~s/Range/range/;
    $opt=~s/Damage/damage/;
    $opt=~s/Burst/burst/;
    $opt=~s/ROF/reloadtime/; #?
    $opt=~s/Report=(.*)/firesound=$1.AUD/;
    if ($set{atweapon}) { $opt=~s/Image=(.*)$/fireimage=$1.SHP/; }

# Warhead
    $opt=~s/Verses=([0-9]*)%,([0-9]*)%,([0-9]*)%,([0-9]*)%,([0-9]*)%/versus=$1,$2,$3,$4,$5/;
    $opt=~s/Ore/tiberium/;
    $opt=~s/Wall/wall/;
    $opt=~s/Ore/tiberium/;
    $opt=~s/InfDeath/ideath/;
    if ($opt=~m/Explosion=1/) { $opt=~s/Explosion=1/explosionimage=piff.shp/; }
    elsif ($opt=~m/Explosion=2/) { $opt=~s/Explosion=2/explosionimage=piffpiff.shp/; }
    elsif ($opt=~m/Explosion=3/) { $opt=~s/Explosion=3/explosionimage=fire1.shp/; } # which fire?
    elsif ($opt=~m/Explosion=4/) { $opt=~s/Explosion=4/explosionimage=frag1.shp/; }
    elsif ($opt=~m/Explosion=5/) { $opt=~s/Explosion=5/explosionimage=veh-hit3.shp/; }
    elsif ($opt=~m/Explosion=6/) { $opt=~s/Explosion=6/explosionimage=atomsfx.shp/; }

# Projectile
    $opt=~s/Arcing/arcing/;
    $opt=~s/High/high/;
    if ($set{atweapon}) { $opt=~s/Image/image/; }
    $opt=~s/Animates/animates/;
    $opt=~s/Rotates/rotates/;

# yes -> 1, no -> 0
    $opt=~s/yes$/1/;
    $opt=~s/no$/0/;

# Remove the rest (upper case start)
    $opt=~s/^[[:upper:]].*//;
    $opt=~s/primary_weapon=None.*//;
    $opt=~s/^\;.*//;    
    next if ($opt eq "");

#PRINT OPTION!
    print "$opt\n";

#POST PRINTS
# Specific (types=?, MLRS?)
   if (($set{atunit} eq 1) && ($opt=~m/^\[/)) { print "unittype=$set{utype}\n"; } #utype
   if (($set{atunit} eq 1 || $set{atstruct} eq 1) && ($opt=~m/^\[/)) { print "buildlevel=1\n"; }
   if ($opt=~m/\[MCV\]/) { print "deploysto=FACT\ndeployface=14\n"; } #what's deployface?
   if ($opt=~m/\[1TNK\]/) { print "layers=2\nloopend=31\nloopend2=31\nsectype=6\n"; }
   if ($opt=~m/\[2TNK\]/) { print "layers=2\nloopend=31\nloopend2=31\nsectype=6\n"; }
   if ($opt=~m/\[3TNK\]/) { print "layers=2\nloopend=31\nloopend2=31\nsectype=6\n"; }
   if ($opt=~m/\[4TNK\]/) { print "layers=2\nloopend=31\nloopend2=31\nsectype=6\n"; }
   if ($opt=~m/\[JEEP\]/) { print "layers=2\nloopend=31\nloopend2=31\nsectype=6\n"; }
#   if ($opt=~m/\[V2RL\]/) { print "layers=4\nloopend=31\nloopend2=31\nloopend3=7\nloopend4=7\nsectype=5\n"; }
   if ($opt=~m/\[V2RL\]/) { print "loopend=31\n"; }
   if ($opt=~m/\[MRJ\]/) { print "layers=2\nloopend=31\nloopend2=31\nsectype=6\n"; }
   if ($opt=~m/\[CA\]/) { print "layers=2\nimage2=TURR.SHP\nloopend=15\nloopend2=31\nsectype=6\n"; }
   if ($opt=~m/\[DD\]/) { print "layers=2\nimage2=SSAM.SHP\nloopend=15\nloopend2=31\nsectype=6\n"; }

   if ($opt=~m/\[E7\]/) { print "talkback=tanya\n"; }
   if ($opt=~m/\[SPY\]/) { print "talkback=spy\n"; }
   if ($opt=~m/\[THF\]/) { print "talkback=thief\n"; }
   if ($opt=~m/\[DOG\]/) { print "talkback=dog\n"; }
   if ($opt=~m/\[EINSTEIN\]/) { print "talkback=einstein\n"; }
   if ($opt=~m/\[E6\]/) { print "talkback=engineer\n"; }
   if ($opt=~m/\[MEDI\]/) { print "talkback=medic\n"; }

   if ($opt=~m/\[WEAP\]/) { print "layers=2\nimage2=WEAP2.SHP\nloopend2=3\nanimtype=3\nprimary=1\n"; }
   if ($opt=~m/\[PROC\]/) { print "passengersallow=HARV\n"; }
   if ($opt=~m/\[WEAF\]/) { print "layers=2\nimage2=WEAP2.SHP\nloopend2=3\nanimtype=3\n"; }
   if ($opt=~m/\[AFLD\]/) { print "delay=10\nloopend=7\nanimtype=1\nprimary=1\n"; }
   if ($opt=~m/\[HPAD\]/) { print "loopend=6\nanimtype=3\nprimary=1\n"; }
   if ($opt=~m/\[BARR\]/) { print "loopend=9\nanimtype=1\nprimary=1\n"; }
   if ($opt=~m/\[TENT\]/) { print "loopend=9\nanimtype=1\nprimary=1\n"; }
   if ($opt=~m/\[FACT\]/) { print "loopend=25\nanimtype=3\nprimary=1\n"; }
   if ($opt=~m/\[SYRD\]/) { print "primary=1\n"; }
   if ($opt=~m/\[SPEN\]/) { print "primary=1\n"; }
   if ($opt=~m/\[AGUN\]/) { print "loopend=63\nanimtype=6\nturret=1\n"; } #64?
   if ($opt=~m/\[GUN\]/) { print "loopend=63\nanimtype=6\nturret=1\n"; } #64?
   if ($opt=~m/\[SAM\]/) { print "loopend=33\nanimtype=6\nturret=1\n"; } #33?
   if ($opt=~m/\[FIX\]/) { print "loopend=5\nanimtype=3\n"; }
   if ($opt=~m/\[GAP\]/) { print "loopend=31\nanimtype=1\n"; }
   if ($opt=~m/\[HOSP\]/) { print "loopend=3\nanimtype=1\n"; }
   if ($opt=~m/\[IRON\]/) { print "loopend=10\nanimtype=3\n"; }
   if ($opt=~m/\[PDOX\]/) { print "loopend=28\nanimtype=3\n"; }
   if ($opt=~m/\[SILO\]/) { print "loopend=3\nanimtype=0\n"; }
   if ($opt=~m/\[TSLA\]/) { print "loopend=9\nanimtype=3\n"; }
   if ($opt=~m/\[V19\]/) { print "delay=5\nloopend=13\nanimtype=1\n"; } #(afair 19,19,1)

#change some flags...
    if ($set{end}) {
      $set{end}=0;
      $set{prereq}=0;
    }

  }
close(RULES);
