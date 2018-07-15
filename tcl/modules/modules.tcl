#!/usr/bin/env wish -f
#
#  GLI-Modules
#
#   builds environment for graphical tools like GLI-Sight or GLI-Image.
#   argument is an init-file which contains the program-specifications.
#
#  System-call:
#
#    gli -f modules.tcl initfile
#
#  Author:
#
#   Gunnar Grimm
#   Research Centre Juelich
#   Institute for Solid State Research
#


set DEVELOPMENT 0

if {$DEVELOPMENT} {
  set const(homepath) $env(HOME)
  set const(tclpath) $env(HOME)/gli/tcl/lib
} else {
  set const(homepath) $env(GLI_HOME)
  set const(tclpath) $env(GLI_HOME)/tcl/lib
}

###
   set initfilename $const(homepath)/tcl/sight/init
###

# read_line reads next line, ignoring lines beginning with "\n"(empty line)
# or "#"(commentary)
# return empty string if EOF

proc read_line {fp} {

  while {([set bytes_read [gets $fp line]] == 0) || \
         ($bytes_read != -1 && [string index $line 0] == "#")} {}

  if {$bytes_read != 0} {
    return $line
  } else {
    return ""
  }
}


# read init_file

set externfiles "menudata textdata module shorthelp index bitmappath"
set constants "display_height display_width mb_padx mb_pady min_factor ratio"
set jumplabels "init1 init2 exit resize"

if {[file exists $initfilename] && [file readable $initfilename]} {
  set fp [open $initfilename r]
  while {[set line [read_line $fp]] != ""} {
    foreach keyword $externfiles {
      if {$keyword == [string tolower [lindex $line 0]]} {
        set file($keyword) "$const(homepath)/[lindex $line 1]"
      }
    }
    foreach keyword $constants {
      if {$keyword == [string tolower [lindex $line 0]]} {
        set const($keyword) [lindex $line 1]
      }
    }
    foreach keyword $jumplabels {
      if {$keyword == [string tolower [lindex $line 0]]} {
        set jump($keyword) [lindex $line 1]
      }
    }
  }
  close $fp
} else {
  puts stderr "Can't read $initfilename"
  exit
}

foreach externfile $externfiles {
  if {![info exists file($externfile)]} {
    puts stderr "'$externfile'-specification is missing."
    exit
  }
}
foreach constant $constants {
  if {![info exists const($constant)]} {
    puts stderr "definition for '$constant' is missing."
    exit
  }
}

# include module

if {[catch {source $file(module)}]} {
  puts stderr "Can't include $file(module)"
  exit
}

lappend auto_path $const(tclpath)

# call module_init 1
if {[info exists jump(init1)]} {
  eval $jump(init1)
}

######### PROCEDURES

# read menu_definitions from file

proc read_menu {menufile} {

  global menu
  global bitmap

  set fp [open $menufile r]

  set path ""
  while {[set line [read_line $fp]] != ""} {
    set tabs 0
    while {[string index $line $tabs] == "	"} {incr tabs 1}
    if {$tabs > [llength $path]} {
      lappend path $last_entry
      set menu([join $path ","]) ""
    } elseif {$tabs < [llength $path]} {
      set path [lrange $path 0 [expr $tabs-1]]
    }
    if {$tabs == 0} {
      set menu([lindex $line 0],buttonframe) [lindex $line 1]
      set bitmapname ""
    } else {
      set bitmapname [lindex $line 1]
    }
    set last_entry [lindex $line 0]     
    set command [lindex $line 2]

    set index [join $path ","]
    lappend menu($index) $last_entry
    if {$last_entry != "*"} {
# build text index
      set text_index "menu_[join [concat $path $last_entry] "_"]"
# output for text-file
      set menu($index,$last_entry,text) $text_index
# create image
      if {$bitmapname != "*" && $bitmapname != ""} {
        if {$bitmapname == "+"} {
          set bitmapname [join [concat $path $last_entry] "_"]
        }
        set bitmapfile $bitmap(path)/$bitmapname.$bitmap(format)
        if {[file exists $bitmapfile]} {
          set image [image create photo -format $bitmap(format) \
                                        -file $bitmapfile]
          set menu($index,$last_entry,image) $image
        } else {
          puts stderr "$bitmapfile does not exist"
        }
      }
# set command
      if {$command != "*" && $command != ""} {
        set menu($index,$last_entry,command) $command
      }
    }
  }

  close $fp
}

# read text-file

proc read_text {textfile} {
  global text
  global tk_version

  set fp [open $textfile r]

  while {[set line [read_line $fp]] != ""} {
    set label [lrange $line 1 end]
    if {[string index [lindex $label 0] 0] == "$"} {
      set label $text([string range [lindex $label 0] 1 end])
    }
    set text([lindex $line 0]) $label
  }

  close $fp
}

# define menu- and button-structure

proc define_menu {widget buttonwidget commands path} {
  global menu text
  global bitmap
  global const

  foreach undercommand $commands {
    if {$undercommand != "*"} {
      set label $text($menu($path,$undercommand,text))
# set bitmapfile
      if {[info exists menu($path,$undercommand,image)]} {
        set image "$menu($path,$undercommand,image)"
      } else {
        set image ""
      }
# set command
      if {[info exists menu($path,$undercommand,command)]} {
        set command $menu($path,$undercommand,command)
      } else {
        set command ""
      }
# build structure
      if {[info exists menu($path,$undercommand)]} {
        set newbuttonmenu ""
        if {$image != ""} {
          if {[llength [split $buttonwidget "."]] == 4} {
            if {$command != ""} {
              button $buttonwidget.$undercommand -image $image \
                 -command "$command"
            } else {
              set newbuttonmenu $buttonwidget.$undercommand.menu
              menubutton $buttonwidget.$undercommand -image $image \
                                -menu $newbuttonmenu -relief raised \
                                -width $const(mb_width) \
                                -height $const(mb_height)
            }
          } else {
            if {$command != ""} {
              $buttonwidget add command -image $image -command "$command" \
            } else {
              set newbuttonmenu $buttonwidget.$undercommand
              $buttonwidget add cascade -image $image \
                                    -menu $newbuttonmenu 
            }
          }
          if {$newbuttonmenu != ""} {menu $newbuttonmenu -tearoff 0}
        }
        set newmenu $widget.$undercommand
        $widget add cascade -label $label -menu $newmenu
        menu $newmenu -tearoff 0
        define_menu $newmenu $newbuttonmenu $menu($path,$undercommand) \
                    "$path,$undercommand"
      } else {
# build command
        set args "$path,$undercommand"
        regsub "," $args "_" args
        set args [split $args ","]
        $widget add command -label $label -command "$args"
        if {$image != ""} {
          if {$command != ""} {set args $command}
          if {[llength [split $buttonwidget "."]] == 4} {
            button $buttonwidget.$undercommand -image $image \
                   -command "$args"
          } else {
            $buttonwidget add command -image $image -command "$args"
          }
        }
      }
    } else {
      $widget add separator
    }
  }

}


# place layout in landscape or portrait

proc place_layout {} {
  global const
  global view

  switch $view(orientation) {
    PORTRAIT {
               set display_width \
                            [expr int($const(display_height)*$view(factor))+4]
               set display_height \
                            [expr int($const(display_width)*$view(factor))+4]
               set menu_width [expr $display_width+2*$const(buttons_height)]
               set buttons_width $const(buttons_height)
               set buttons_height $display_height
               set settings_height $buttons_height
               set display_x $buttons_width
               set buttons_y $const(menu_height)
               set help_y [expr $const(menu_height)+$display_height]
               set width [expr $display_width+2*$const(buttons_height)]
               set height [expr $display_height+2*$const(menu_height)]
    }
    LANDSCAPE {
               set display_width \
                            [expr int($const(display_width)*$view(factor))+4]
               set display_height \
                            [expr int($const(display_height)*$view(factor))+4]
               set menu_width [expr $display_width+$const(buttons_height)]
               set buttons_width $display_width	
               set buttons_height $const(buttons_height)
               set settings_height [expr $display_height+$const(buttons_height)]
               set display_x 0
               set buttons_y [expr $const(menu_height)+$display_height]
               set help_y [expr $const(menu_height)+$display_height+ \
                                $const(buttons_height)]
               set width [expr $display_width+$const(buttons_height)]
               set height [expr $const(menu_height)*2+$display_height+ \
                                $const(buttons_height)]
    }
  }

  place forget .menu
  place forget .display
  place forget .settings
  place forget .buttons
  place forget .help

  place .menu -x 0 -y 0 -width $menu_width -height $const(menu_height)
  place .display -x $display_x -y $const(menu_height) -width $display_width \
                 -height $display_height
  place .settings -x [expr $display_x+$display_width] -y $const(menu_height) \
                 -width $const(settings_width) -height $settings_height
  place .buttons -x 0 -y $buttons_y -width $buttons_width \
                 -height $buttons_height 
  place .help -x 0 -y $help_y -width $menu_width -height $const(help_height)

  wm geometry . $width\x$height+$const(geomx)+$const(geomy)

  set const(width) $width
  set const(height) $height
}

proc place_buttons {orientation} {
  global const

  switch $orientation {
    LANDSCAPE {
      set frameside top
      set buttonside left
      set anchor w
      set framepad pady
      set buttonpad padx
    }
    PORTRAIT {
      set frameside left
      set buttonside top
      set anchor n
      set framepad padx
      set buttonpad pady
    }
  }

  foreach frame [winfo children .buttons] {
    pack $frame -side $frameside -anchor $anchor -$framepad 2
    foreach mainmenu [winfo children $frame] {
      pack $mainmenu -side $buttonside -$buttonpad 5 -$framepad 0
      foreach button [winfo children $mainmenu] {
        if {[winfo class $button] == "Menubutton"} {
          pack $button -side $buttonside \
                       -$buttonpad [expr $const(mb_padx)+2] \
                       -$framepad [expr $const(mb_pady)+2]
        } else {
          pack $button -side $buttonside \
                       -$buttonpad $const(mb_padx) -$framepad $const(mb_pady)
        }
      }
    }
  }

}


######### MAIN

set stdfont -adobe-helvetica-bold-r-normal--12-120-75-75-p-70-iso8859-1
option add *font $stdfont
option add *Frame*bd 1

set view(orientation) LANDSCAPE
set view(factor) 1
set language english

set bitmap(path) $file(bitmappath)
set bitmap(format) "gif"

# screen coords

set const(menu_height) 20

set const(settings_width) [expr $const(display_width)-$const(display_height)]
set const(buttons_height) $const(settings_width)

set const(help_height) 20

#set const(width) [expr $const(display_width)+$const(settings_width)]
#set const(height) [expr $const(menu_height)+$const(display_height)+ \
#                  $const(buttons_height)+$const(help_height)]

set const(mb_width) 34
set const(mb_height) $const(mb_width)
if {![info exists const(mb_padx)]} {set const(mb_padx) 0}
if {![info exists const(mb_pady)]} {set const(mb_pady) 0}

set const(geomx) 40
set const(geomy) 50

set const(button_fg) black
set const(frame_relief) ridge

set correctsize 0

# read menu-definitions
if {[file readable $file(menudata)]} {
  read_menu $file(menudata)
} else {
  puts stderr "Can't read menufile '$file(menudata)'."
  exit
}

# read text
if {[file readable $file(textdata)]} {
  read_text $file(textdata)
} else {
  puts stderr "Can't read textfile '$file(textdata)'."
  exit
}
######### DEFINE LAYOUT

frame .menu -relief $const(frame_relief) -bd 1
frame .display -relief $const(frame_relief) -bd 1
frame .display.gli -bg white
frame .settings -relief $const(frame_relief) -bd 1
frame .buttons -relief $const(frame_relief) -bd 1
label .help -relief $const(frame_relief) -textvariable shorthelp_text

# define menus and buttons
foreach mainmenu $menu() {
  menubutton .menu.$mainmenu -text $text(menu_$mainmenu) \
                             -menu .menu.$mainmenu.menu
  menu .menu.$mainmenu.menu -tearoff 0
  set buttonframe $menu($mainmenu,buttonframe)
  if {![winfo exists .buttons.f$buttonframe]} {
    frame .buttons.f$buttonframe
  }
  frame .buttons.f$buttonframe.$mainmenu -relief ridge -bd 1
  define_menu .menu.$mainmenu.menu .buttons.f$buttonframe.$mainmenu \
              $menu($mainmenu) $mainmenu
  pack .menu.$mainmenu -side left
}

button .settings.exit -text "Exit" -command {
# call module_exit
  if {[info exists jump(exit)]} {
    eval $jump(exit)
  }
  exit
}

######### PLACE LAYOUT

place_layout
place_buttons $view(orientation)

pack .display.gli -fill both -expand true
pack .settings.exit -side bottom


# key bindings
bind Entry <Shift-Delete> {
  %W delete 0 end
}

bind Entry <Delete> {
  if {[%W select present]} {
    %W delete sel.first sel.last
  } else {
    set index [%W index insert]
    if {$index != 0} {
      %W delete [expr $index - 1]
    }
  }
}

bind . <Configure> {
  set geom [lrange [split [winfo geometry .] +] 1 2]
  set const(geomx) [lindex $geom 0]
  set const(geomy) [lindex $geom 1]

  if {$correctsize == 1} {
   
    set geom [split [lindex [split [winfo geometry .] +] 0] x]
    set width [lindex $geom 0]
    set height [lindex $geom 1]

    if {[expr abs($width-$const(width))] >= 5 || \
        [expr abs($height-$const(height))] >= 5} {
  
      incr width -3
      incr height -3

      switch $view(orientation) {
        PORTRAIT {
          set incr_width [expr $const(buttons_height)*2]
          set incr_height [expr $const(menu_height)*2]
          set width [expr $width-$incr_width]
          set height [expr $height-$incr_height]
          if {$const(ratio) != -1} {set width [expr $width*$const(ratio)]}
        }
        LANDSCAPE {
          set incr_width [expr $const(buttons_height)]
          set incr_height [expr $const(menu_height)*2+$const(buttons_height)]
          set width [expr $width-$incr_width]
          set height [expr $height-$incr_height]
          if {$const(ratio) != -1} {set height [expr $height*$const(ratio)]}
        }
      }
      if {$const(ratio) != -1} {
        if {$height < $width} {
          set width $height
        } else {
          set height $width
        }
        set view(factor) [expr $width/$const(display_width).]
        if {$view(factor) < $const(min_factor)} {
          set view(factor) $const(min_factor)
        }
      } else {
        set const(display_width) $width
        set const(display_height) $height
        set view(factor) 1
      }

      set correctsize 0
      if {[info exists jump(resize)]} {
        eval $jump(resize)
      }
    } else {
      set correctsize 0
      wm geometry . $const(width)x$const(height)+$const(geomx)+$const(geomy)
    }
    update
    set correctsize 1
  }
}

update

# call module_init 2
if {[info exists jump(init2)]} {
  eval $jump(init2)
}

# init shorthelp
DefineCursor . watch
InitShorthelp $file(shorthelp) $file(index) shorthelp_text
DefineCursor .

update
set correctsize 1
