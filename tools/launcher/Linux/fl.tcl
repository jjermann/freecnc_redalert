#!/usr/bin/wish -f

# FreeCNC Start-Up Programm
# Author : Bernd Ritter <comrad@drunkencat.net>
# License : GPL
# Date : OCT 25th, 2002
# Changes:
# Version 0.9.2		-	OCT 25th, 2002	-	Initial Release					
# Version 0.9.3		-	DEC 18th, 2002	-	Support for Version Recognition
#											Support for OS Recognition
# Version 0.9.4		-	DEC 19th, 2002  -	Support for Mission Selectboxes
# Version 0.9.4a        -       DEC 21th, 2002  -       Some Fixes
# Version 0.9.5         -       APR 07th, 2003  -       Filebox to select individual missions

set version 0.9.5

puts "This is the FreeCNC StartUp Program $version"



# Some Variables
# Operating System
set os unrecognised					

# Selected Mission
set fileSelected none				

# Mission types 
set variant "not recognized"		

# Variable for the mission list, varc 0 = demo, varc 1 = full, varc 2 = missionpack, varc 3 = unrecognised 
set varc 3							

# Global Variables
set res 640
set height 480
set fs 0
set mission none
set ns 0
set depth 16

set mp 0
set mpsrvfld localhost
set mpsrvprt 1995
set side GDI
set color red
set mpnickname NoName
set mpskirmish 1
set mpmap scm01ea


# Check for Operating System
# DOS or Windows/Mac-Version?
if {[file exists ../mix/updatec.mix] == 1} {
	if {[file exists ../mix/cclocal.mix] == 1} { 
		set os "C&C GOLD"
	} else {
		set os "MacC&C"
	}
} else {
	set os "DOS C&C"
}


# Check for Mission Pack
if {[file exists ../mix/sc-000.mix] == 1} {
	set variant "Full-Version with MissionPack"
	set varc 2	
}
# Check for Full Version
if {[file exists ../mix/conquer.mix] == 1} {
	set variant "Full-Version"
	set varc 1	
}
# Check for Demo-Version
if {[file exists ../mix/demo.mix] == 1} {
	set variant "Demo-Version"
	set varc 0
}





# Labels ##########################################################

label .titel 	-wraplength 5i \
		-justify center \
		-width 55 \
		-borderwidth 5 \
		-relief sunken \
		-text "FreeCNC Launcher $version - C&C $variant from $os -" 

label .titelresolution 	-wraplength 2i \
			-justify right \
			-width 25 \
			-text "Choose Resolution"

label .titelmission	-wraplength 2i \
			-justify right \
			-width 25 \
			-text "Choose Mission :"

label .mptitel	-wraplength 2i \
			-justify right \
			-width 25 \
			-text "Multiplayer-Setup"

label .mpserverfield	-wraplength 2i \
			-justify right \
			-width 25 \
			-text "Server Address"

label .gnloptionslbl	-wraplength 2i \
			-justify right \
			-width 25 \
			-text "General Options"

label .mpserverport	-wraplength 2i \
			-justify left \
			-width 4 \
			-text "Port"

label .singleplayerlbl	-wraplength 2i \
			-justify left \
			-width 20 \
			-text "Singleplayer Mode"


label .sidelabel	-wraplength 2i \
			-justify right \
			-width 25 \
			-text "Choose your Side"

label .colorlabel	-wraplength 2i \
			-justify right \
			-width 25 \
			-text "Choose your Color"

label .nicknamelabel	-wraplength 2i \
			-justify right \
			-width 25 \
			-text "Your Nick"

label .skirmishlabel	-wraplength 2i \
			-justify right \
			-width 25 \
			-text "How many Computer Opponents"		

label .mpmaplabel	-wraplength 2i \
			-justify right \
			-width 25 \
			-text "Multiplayer Map"

label .depthlabel	-wraplength 2i \
			-justify right \
			-width 25 \
			-text "Select Color Depth"

label .soundlabel	-wraplength 2i \
			-justify right \
			-width 25 \
			-text "Sound on/off"

label .openfile		-width 25 \
			-justify right \
			-text "File to open:"

entry .openfilex -textvariable open
set types {
	{{Individual Missions} {.ini} }
}
			
# Listboxes ##########################################################
scrollbar .s -command ".list yview"
listbox .list -selectmode single -width 20 -height 1 -yscroll ".s set"

scrollbar .smp -command ".listmp yview"
listbox .listmp -selectmode single -width 20 -height 1 -yscroll ".smp set"

# varc 0 = demo
# varc 1 = full
# varc 2 = missionpack


# Missions for the Demo-Version
if { $varc == 0 } {
	.list insert end scg01ea
	.list insert end scg03ea
	.list insert end scg05ea
	.list insert end scg06ea
	.list insert end scg07ea
	.list insert end scg10ea
}

# Missions for the Full-Version
if { $varc == 1 } {
	.list insert end "GDI Mission 1"
	.list insert end "GDI Mission 2"
	.list insert end "GDI Mission 3"
	.list insert end "GDI Mission 4a"
	.list insert end "GDI Mission 4b"
	.list insert end "GDI Mission 4c"
	.list insert end "GDI Mission 5a"
	.list insert end "GDI Mission 5b"
	.list insert end "GDI Mission 5c"
	.list insert end "GDI Mission 5d"
	.list insert end "GDI Mission 6"
	.list insert end "GDI Mission 7"
	.list insert end "GDI Mission 8a"
	.list insert end "GDI Mission 8b"
	.list insert end "GDI Mission 9"
	.list insert end "GDI Mission 10a"
	.list insert end "GDI Mission 10b"
	.list insert end "GDI Mission 11"
	.list insert end "GDI Mission 12a"
	.list insert end "GDI Mission 12b"
	.list insert end "GDI Mission 13a"
	.list insert end "GDI Mission 13b"
	.list insert end "GDI Mission 14a"
	.list insert end "GDI Mission 15a"
	.list insert end "GDI Mission 15b"
	.list insert end "GDI Mission 15c"
	
	.list insert end "NOD Mission 1a"
	.list insert end "NOD Mission 1b"	
	.list insert end "NOD Mission 2a"
	.list insert end "NOD Mission 2b"		
	.list insert end "NOD Mission 3a"
	.list insert end "NOD Mission 3b"		
	.list insert end "NOD Mission 4a"
	.list insert end "NOD Mission 4b"		
	.list insert end "NOD Mission 5a"	
	.list insert end "NOD Mission 6a"	
	.list insert end "NOD Mission 6b"	
	.list insert end "NOD Mission 6c"	
	.list insert end "NOD Mission 7a"	
	.list insert end "NOD Mission 7b"	
	.list insert end "NOD Mission 7c"			
	.list insert end "NOD Mission 8a"	
	.list insert end "NOD Mission 8b"
	.list insert end "NOD Mission 9a"	
	.list insert end "NOD Mission 9b"	
	.list insert end "NOD Mission 10a"	
	.list insert end "NOD Mission 10b"	
	.list insert end "NOD Mission 11a"	
	.list insert end "NOD Mission 11b"		
	.list insert end "NOD Mission 12a"	
	.list insert end "NOD Mission 12b"		
	.list insert end "NOD Mission 13a"	
	.list insert end "NOD Mission 13b"	
	.list insert end "NOD Mission 13c"	
}

# Missions for the Full-Version including Missionpack
if { $varc == 2 } {
	.list insert end "GDI Mission 1"
	.list insert end "GDI Mission 2"
	.list insert end "GDI Mission 3"
	.list insert end "GDI Mission 4a"
	.list insert end "GDI Mission 4b"
	.list insert end "GDI Mission 4c"
	.list insert end "GDI Mission 5a"
	.list insert end "GDI Mission 5b"
	.list insert end "GDI Mission 5c"
	.list insert end "GDI Mission 5d"
	.list insert end "GDI Mission 6"
	.list insert end "GDI Mission 7"
	.list insert end "GDI Mission 8a"
	.list insert end "GDI Mission 8b"
	.list insert end "GDI Mission 9"
	.list insert end "GDI Mission 10a"
	.list insert end "GDI Mission 10b"
	.list insert end "GDI Mission 11"
	.list insert end "GDI Mission 12a"
	.list insert end "GDI Mission 12b"
	.list insert end "GDI Mission 13a"
	.list insert end "GDI Mission 13b"
	.list insert end "GDI Mission 14a"
	.list insert end "GDI Mission 15a"
	.list insert end "GDI Mission 15b"
	.list insert end "GDI Mission 15c"
	
	.list insert end "NOD Mission 1a"
	.list insert end "NOD Mission 1b"	
	.list insert end "NOD Mission 2a"
	.list insert end "NOD Mission 2b"		
	.list insert end "NOD Mission 3a"
	.list insert end "NOD Mission 3b"		
	.list insert end "NOD Mission 4a"
	.list insert end "NOD Mission 4b"		
	.list insert end "NOD Mission 5a"	
	.list insert end "NOD Mission 6a"	
	.list insert end "NOD Mission 6b"	
	.list insert end "NOD Mission 6c"	
	.list insert end "NOD Mission 7a"	
	.list insert end "NOD Mission 7b"	
	.list insert end "NOD Mission 7c"			
	.list insert end "NOD Mission 8a"	
	.list insert end "NOD Mission 8b"
	.list insert end "NOD Mission 9a"	
	.list insert end "NOD Mission 9b"	
	.list insert end "NOD Mission 10a"	
	.list insert end "NOD Mission 10b"	
	.list insert end "NOD Mission 11a"	
	.list insert end "NOD Mission 11b"		
	.list insert end "NOD Mission 12a"	
	.list insert end "NOD Mission 12b"		
	.list insert end "NOD Mission 13a"	
	.list insert end "NOD Mission 13b"	
	.list insert end "NOD Mission 13c"	
	
	.list insert end "COVOP GDI Mission 1"
	.list insert end "COVOP GDI Mission 2"
	.list insert end "COVOP GDI Mission 3"
	.list insert end "COVOP GDI Mission 4"
	.list insert end "COVOP GDI Mission 5"
	.list insert end "COVOP GDI Mission 6"
	
	.list insert end "COVOP NOD Mission 1"
	.list insert end "COVOP NOD Mission 2"
	.list insert end "COVOP NOD Mission 3"
	.list insert end "COVOP NOD Mission 4"
	.list insert end "COVOP NOD Mission 5"
	.list insert end "COVOP NOD Mission 6"	
	.list insert end "COVOP NOD Mission 7"
	.list insert end "COVOP NOD Mission 8"	
	
}

# Missions for Multiplayer
	.listmp insert end scm01ea
	.listmp insert end scm02ea
	.listmp insert end scm03ea
	.listmp insert end scm04ea
	.listmp insert end scm05ea
	.listmp insert end scm06ea
	.listmp insert end scm07ea
	.listmp insert end scm08ea
	.listmp insert end scm09ea
	.listmp insert end scm70ea
	.listmp insert end scm71ea
	.listmp insert end scm72ea
	.listmp insert end scm73ea	
	.listmp insert end scm74ea	
	.listmp insert end scm77ea	
	.listmp insert end scm96ea	
	
	if {$varc == 2} {
		.listmp insert end scm50ea
		.listmp insert end scm51ea
		.listmp insert end scm52ea
		.listmp insert end scm60ea
		.listmp insert end scm61ea
		.listmp insert end scm62ea
		.listmp insert end scm75ea
		.listmp insert end scm90ea
		.listmp insert end scm97ea
		.listmp insert end scm98ea
	}



bind .list <ButtonRelease-1> { global fileSelected; set fileSelected [%W get [%W curselection]]; set mission $fileSelected; transMission; }
bind .listmp <ButtonRelease-1> { global fileSelected; set fileSelected [%W get [%W curselection]]; if { $mp == 1 } { set mission $fileSelected; transMission; }}




# Radiobuttons ####################################################
radiobutton .640 -text "640x480" -variable res -value 640 -justify right
radiobutton .800 -text "800x600" -variable res -value 800 -justify right
radiobutton .1024 -text "1024x768" -variable res -value 1024 -justify right

radiobutton .gdi -text "GDI" -variable side -value GDI -justify left
radiobutton .nod -text "NOD" -variable side -value NOD -justify left

radiobutton .red -text "red" -variable color -value red -justify center
radiobutton .orange -text "orange" -variable color -value orange -justify center
radiobutton .yellow -text "yellow" -variable color -value yellow -justify center
radiobutton .green -text "green" -variable color -value green -justify center
radiobutton .blue -text "blue" -variable color -value blue -justify center
radiobutton .turquoise -text "turquoise" -variable color -value turquoise -justify center

radiobutton .8bpp -text "8 bpp" -variable depth -value 8 -justify center
radiobutton .16bpp -text "16 bpp" -variable depth -value 16 -justify center
radiobutton .24bpp -text "24 bpp" -variable depth -value 24 -justify center
radiobutton .32bpp -text "32 bpp" -variable depth -value 32 -justify center
.16bpp select



# Checkbuttons #####################################################
checkbutton .mp -text "Multiplayer Mode" -relief flat -variable mp -onvalue 1 -offvalue 0
checkbutton .fs -text "Run in Fullscreen" -relief flat -variable fs -onvalue 1 -offvalue 0
checkbutton .ns -text "No sound" -relief flat -variable ns -onvalue 1 -offvalue 0

# Buttons ##########################################################
button .run -text "Run FreeCNC" -command checkForStart
button .openfilebutton -text "Open external Mission..." -command "set open \[tk_getOpenFile -filetypes \$types \]"

# Entry Fields ###################################################
entry .missionEntry -textvariable mission
entry .missionEntryind -textvariable open
entry .mpserverfieldEntry -textvariable mpsrvfld
entry .mpserverportEntry  -textvariable mpsrvprt -width 5
entry .mpnicknameEntry	  -textvariable mpnickname
entry .mpskirmishEntry	  -textvariable mpskirmish -width 2
entry .mpmapEntry	  -textvariable mpmap


# COLORS #################################################################################
.titel 			configure -bg white -fg black
.singleplayerlbl 	configure -bg white -fg black
.mptitel 		configure -bg white -fg black
.gnloptionslbl 		configure -bg white -fg black

# Subprogramm  ##########################################################
# translates the Text-Missionname into the cnc-mission-format
proc transMission args {
	global mission
	
	switch -glob $mission {
		"GDI Mission 1"		{ set mission "scg01ea" }
		"GDI Mission 2" 	{ set mission "scg02ea" }
		"GDI Mission 3" 	{ set mission "scg03ea" }
		"GDI Mission 4a" 	{ set mission "scg04ea" }
		"GDI Mission 4b" 	{ set mission "scg04wa" }
		"GDI Mission 4c" 	{ set mission "scg04wb" }		
		"GDI Mission 5a" 	{ set mission "scg05ea" }
		"GDI Mission 5b" 	{ set mission "scg05eb" }
		"GDI Mission 5c" 	{ set mission "scg05wa" }
		"GDI Mission 5d" 	{ set mission "scg05wb" }
		"GDI Mission 6" 	{ set mission "scg06ea" }
		"GDI Mission 7" 	{ set mission "scg07ea" }
		"GDI Mission 8a" 	{ set mission "scg08ea" }
		"GDI Mission 8b" 	{ set mission "scg08eb" }
		"GDI Mission 9" 	{ set mission "scg09ea" }
		"GDI Mission 10a" 	{ set mission "scg10ea" }
		"GDI Mission 10b" 	{ set mission "scg10eb" }		
		"GDI Mission 11" 	{ set mission "scg11ea" }
		"GDI Mission 12a" 	{ set mission "scg12ea" }
		"GDI Mission 12b" 	{ set mission "scg12eb" }
		"GDI Mission 13a" 	{ set mission "scg13ea" }
		"GDI Mission 13b" 	{ set mission "scg13eb" }
		"GDI Mission 14" 	{ set mission "scg14ea" }
		"GDI Mission 15a" 	{ set mission "scg15ea" }
		"GDI Mission 15b" 	{ set mission "scg15eb" }
		"GDI Mission 15c" 	{ set mission "scg15ec" }
		"NOD Mission 1a"	{ set mission "scb01ea" }
		"NOD Mission 1b"	{ set mission "scb01eb" }		
		"NOD Mission 2a"	{ set mission "scb02ea" }
		"NOD Mission 2b"	{ set mission "scb02eb" }				
		"NOD Mission 3a"	{ set mission "scb03ea" }
		"NOD Mission 3b"	{ set mission "scb03eb" }			
		"NOD Mission 4a"	{ set mission "scb04ea" }
		"NOD Mission 4b"	{ set mission "scb04eb" }			
		"NOD Mission 5a"	{ set mission "scb05ea" }		
		"NOD Mission 6a"	{ set mission "scb06ea" }		
		"NOD Mission 6b"	{ set mission "scb06eb" }		
		"NOD Mission 6c"	{ set mission "scb06ec" }				
		"NOD Mission 7a"	{ set mission "scb07ea" }		
		"NOD Mission 7b"	{ set mission "scb07eb" }		
		"NOD Mission 7c"	{ set mission "scb07ec" }			
		"NOD Mission 8a"	{ set mission "scb08ea" }		
		"NOD Mission 8b"	{ set mission "scb08eb" }			
		"NOD Mission 9a"	{ set mission "scb09ea" }		
		"NOD Mission 9b"	{ set mission "scb09eb" }					
		"NOD Mission 10a"	{ set mission "scb10ea" }		
		"NOD Mission 10b"	{ set mission "scb10eb" }			
		"NOD Mission 11a"	{ set mission "scb11ea" }		
		"NOD Mission 11b"	{ set mission "scb11eb" }			
		"NOD Mission 12a"	{ set mission "scb12ea" }		
		"NOD Mission 12b"	{ set mission "scb12eb" }		
		"NOD Mission 13a"	{ set mission "scb13ea" }		
		"NOD Mission 13b"	{ set mission "scb13eb" }			
		"NOD Mission 13c"	{ set mission "scb13ec" }					
		
		"COVOP GDI Mission 1" { set mission "scg22ea" }
		"COVOP GDI Mission 2" { set mission "scg23ea" }
		"COVOP GDI Mission 3" { set mission "scg36ea" }
		"COVOP GDI Mission 4" { set mission "scg38ea" }
		"COVOP GDI Mission 5" { set mission "scg40ea" }
		"COVOP GDI Mission 6" { set mission "scg41ea" }
		"COVOP GDI Mission 7" { set mission "scg50ea" }		
	
		"COVOP NOD Mission 1" { set mission "scg20ea" }
		"COVOP NOD Mission 2" { set mission "scg21ea" }
		"COVOP NOD Mission 3" { set mission "scg30ea" }
		"COVOP NOD Mission 4" { set mission "scg31ea" }
		"COVOP NOD Mission 5" { set mission "scg32ea" }
		"COVOP NOD Mission 6" { set mission "scg33ea" }
		"COVOP NOD Mission 7" { set mission "scg35ea" }		
		"COVOP NOD Mission 8" { set mission "scg37ea" }				

		
		default	{ puts "no mission selected" }
		
	}
}

proc messageBoxNoMIX args {
    set button \
        [tk_messageBox \
               -icon question \
               -type ok \
               -title Message \
               -parent . \
               -message "Please select a Mission for Singleplayer game or choose a map for Multiplayer game."]
}


proc checkForStart args {
    global mission
    global open

    if {$mission  == "none"} {
	if {$open == ""} {
	    messageBoxNoMIX
	} else {
	    runGame
	}
    } else {
	runGame
    }
}

proc runGame args {
	global res
	global height
	global fs
	global depth
	global mission
	global mp
	global mpsrvfld
	global mpsrvprt
	global side
	global color
	global mpnickname
	global mpskirmish
	global mpmap
	global ns
	global open

	set mycommand "./freecnc"
	set fsoption "-window"
	set whoption ""
	set loccommand ""
	
	if {$res == 640} {
		set height 480
	}
	if {$res == 800} {
		set height 600
	}
	if {$res == 1024} {
		set height 768
	}

	if {$fs == 1} {
	set fsoption "-fullscreen"
	}		

	if {$mpskirmish > 1} {
			incr mpskirmish
	}
	
        if {$mission == "none"} {
	    set mission $open
	    set mission [file tail $mission]
	    set mission [file rootname $mission]
	}

	puts "Running FreeCNC now..."
	if {$mp == 1} {
		puts "Connecting with Multiplayer Options"
		puts "Server : $mpsrvfld at $mpsrvprt"
		puts "Side : $side"
		puts "Nick : $mpnickname"
		puts "Computer Opponents : $mpskirmish"
		puts "Color : $color"

		if {$ns == 1} {
			exec ./freecnc -map $mpmap -w $res -h $height $fsoption \
				-skirmish $mpskirmish -nick $mpnickname \
				-colour $color -side $side -server $mpsrvfld -port $mpsrvprt \
				-nosound -bpp $depth
		} else {
			exec ./freecnc -map $mpmap -w $res -h $height $fsoption \
				-skirmish $mpskirmish -nick $mpnickname \
				-colour $color -side $side -server $mpsrvfld -port $mpsrvprt -bpp $depth
		}
			
	} else {
		puts "Starting Singleplayer Mode on Map $mission"
		if {$ns == 1} {
			exec ./freecnc -w $res -h $height $fsoption -map $mission -nosound -bpp $depth
		} else {
			exec ./freecnc -w $res -h $height $fsoption -map $mission -bpp $depth
		}
	}
	puts "Exiting"
	exit 0
}


###################################################################################
# Layout

# left part
grid .titel 		-row 0 -column 0 -columnspan 5 -sticky "we"
grid .singleplayerlbl	-row 1 -column 0 -sticky "w"
grid .openfile		-row 2 -column 0 -sticky "w"
grid .openfilebutton	-row 2 -column 0 -sticky "w"
grid .missionEntryind	-row 2 -column 1 -sticky "w"
grid .titelmission 	-row 3 -column 0 -sticky "w"
grid .list	    	-row 3 -column 1 -sticky "w"
grid .s			-row 3 -column 2 -sticky "w"
grid .gnloptionslbl	-row 4 -column 0 -sticky "w"
grid .titelresolution	-row 5 -column 0 -sticky "w"
grid .640		-row 5 -column 1 -sticky "w"
grid .800		-row 6 -column 1 -sticky "w"
grid .1024		-row 7 -column 1 -sticky "w"
grid .fs		-row 8 -column 1 -sticky "w"
grid .soundlabel	-row 9 -column 0 -sticky "w"
grid .ns		-row 9 -column 1 -sticky "w"
grid .depthlabel	-row 10 -column 0 -sticky "w"
grid .8bpp		-row 10 -column 1 -sticky "w"
grid .16bpp		-row 11 -column 1 -sticky "w"
grid .24bpp		-row 12 -column 1 -sticky "w"
grid .32bpp		-row 13 -column 1 -sticky "w"


# right part
grid .mptitel		        -row 1 -column 2
grid .mp		        -row 2 -column 3
grid .nicknamelabel	        -row 3 -column 2 -sticky "w"
grid .mpnicknameEntry		-row 3 -column 3 -sticky "w"
grid .mpserverfield		-row 4 -column 2 -sticky "w"
grid .mpserverfieldEntry	-row 4 -column 3 -sticky "w"
grid .mpserverport		-row 5 -column 2 -sticky "we"
grid .mpserverportEntry		-row 5 -column 3 -sticky "w"
grid .mpmaplabel		-row 6 -column 2 -sticky "w"
grid .listmp			-row 6 -column 3 -sticky "w"
grid .smp			-row 6 -column 4 -sticky "w"
grid .sidelabel			-row 7 -column 2 -sticky "w"
grid .gdi			-row 7 -column 3 -sticky "w"
grid .nod			-row 7 -column 4 -sticky "w"
grid .skirmishlabel		-row 8 -column 2 -sticky "w"
grid .mpskirmishEntry		-row 8 -column 3 -sticky "w"
grid .colorlabel		-row 9 -column 2 -sticky "w"
grid .red			-row 9 -column 3 -sticky "w"
grid .orange			-row 9 -column 4 -sticky "w"
grid .yellow			-row 10 -column 3 -sticky "w"
grid .green			-row 10 -column 4 -sticky "w"
grid .blue			-row 11 -column 3 -sticky "w"
grid .turquoise			-row 11 -column 4 -sticky "w"

grid .run			-row 13 -column 2 -columnspan 5 -sticky "w"


# EOF
