#!/usr/bin/perl
# 
# Rules.ini to freecnc .ini file converter
# by Jonas Jermann <jjermann@gmx.net>

my $opt,$file;

if (!$ARGV[0]) { $file="rules.ini"; }

open(RULES,"cat $file|");
  print "; Converted rules.ini\n";
  while(<RULES>)
  {
    $opt=$_; chop $opt;

#NORMAL PRINTS
# Structures/Units
    if ($opt=~m/Owner/)
    {
      $opt=~s/Owner/owners/;
    }
    if ($opt=~m/Prerequisite/) { $opt=~s/Prerequisite/prerequisites/; }
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
  }
close(RULES);
