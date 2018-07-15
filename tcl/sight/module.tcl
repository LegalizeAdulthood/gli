#
#  SIGHT-Module to be called by GLI-Modules
#
#  Author:
#
#   Gunnar Grimm
#   Research Centre Juelich
#   Institute for Solid State Research
#


proc initprog_1 {} {
  global env tcl_platform td_env
  global seq_num

  if {[file writable .] == 0} {
    puts "gli: cannot write to current (working) directory"
    exit
  }

  if {[info exists env(GLI_GKSM)]} {
    set td_env(gli_gksm) $env(GLI_GKSM)
  } else {
    set td_env(gli_gksm) gli.gksm
  }
  if {[info exists env(GLI_POINTS)]} {
    set td_env(gli_points) $env(GLI_POINTS)
  } else {
    set td_env(gli_points) 65536
  }

  if {$tcl_platform(platform) == "windows"} {
    set td_env(gli_wstype) 41
  } else {
    set td_env(gli_wstype) 212
  }

  set seq_num 0
}

proc initprog_2 {} {
  global DEVELOPMENT
  global gli const
  global view settings
  global tcl_precision
  global protocol

  set view(set_orientation) 0
  set protocol(commands) ""

  set settings(xfrom) 0
  set settings(xto) 1
  set settings(yfrom) 0
  set settings(yto) 1
  
  set view(transformation) 1
  for {set i 0} {$i <= 8} {incr i 1} {
    set settings(trafo,$i) "0.125 0.9 0.125 0.675"
    set settings(window,$i) "0 1 0 1"
    set settings(flipx,$i) 0
    set settings(flipy,$i) 0
    set settings(xlog,$i) 0
    set settings(ylog,$i) 0
  }

  set tcl_precision 15

  wm title . "GLI Sight"

  set settings(selected_type) "none"
  fill_settings_box .settings

# GLI settings
  gli DEFINE SYMBOL SIGHT_INFO_CB sight_info
  gli GKS CLOSE_WS TERMINAL
  init_gli
  
#  view_orientation $view(orientation)
  set gli(busyflag) 0

# error messages
  set gli(ErrList,NoValidNumber) "Please give a valid integer- or real-value"
  set gli(ErrList,NoValidInteger) "Please give a valid integer-value"
  set gli(ErrList,MustBeGreater) "Second value must be greater than first"
  set gli(ErrList,CouldntExecute) "Couldn't execute command"
  set gli(ErrList,WrongNumber) "Wrong number of arguments"
  set gli(ErrList,UnknownKeyword) "Unknown Keyword"
  set gli(ErrList,OriginNotPositive) "Origin must be positive to apply logarithmic transformation"

# ButtonBindings
  bind .display.gli <ButtonPress-1> {
    if {$gli(busyflag) >= 1} {
      incr gli(busyflag) -1
    } elseif {$gli(busyflag) == 0} {
      switch $view(orientation) {
        PORTRAIT {
          set height [expr $const(display_width)*$view(factor)]
          set max [expr $height*$view(factor)]
        }
        LANDSCAPE {
          set height [expr $const(display_height)*$view(factor)]
          set max [expr $const(display_width)*$view(factor)]
        }
      }
      gli_prot SIGHT SELECT OBJECT [expr %x./$max] [expr ($height-%y.)/$max]
if {$DEVELOPMENT} {puts [gli SIGHT INQUIRE]}
      set trafo [lindex [split [gli SIGHT INQUIRE] ","] 0]
      set trafoident [lindex  $trafo 0]
      set trafonr [lindex $trafo 1]
      if {$trafoident == "Transformation:" && $trafonr != ""} {
        view_transformation $trafonr
      }
    }
  }
  bind .display.gli <ButtonPress-2> {
    if {$gli(busyflag) == -1} {
      set gli(busyflag) 0
    }
  }
  
}

proc exitprog {} {
  file_exit
}


proc resizewindow {} {
  place_layout
  update
  gli GKS CLOSE_WS WK1
  init_gli
}

##############
# procedures #
##############

proc PlaceToplevel {w} {
  global placetoplevel

  set width 8
  set height 8
  set framew 11
  set frameh 35
  set xincr 50

  set avg_winwidth 300
  set avg_winheight 300

  if {![info exists placetoplevel]} {
    set placetoplevel(counter) 0
    set placetoplevel(basex) \
         [expr ([winfo screenwidth $w]-$width*($framew+$xincr)-$avg_winwidth)/2]
    set placetoplevel(basey) \
         [expr ([winfo screenheight $w]-$height*$frameh-$avg_winheight)/2]
  }


  set x [expr $placetoplevel(counter)/$height]
  set y [expr $placetoplevel(counter)%$height]
  set xpos [expr $placetoplevel(basex)+$x*$xincr+$y*$framew]
  set ypos [expr $placetoplevel(basey)+$y*$frameh]

  wm geometry $w +$xpos+$ypos

  if {[incr placetoplevel(counter)] >= [expr $width*$height]} {
    set placetoplevel(counter) 0
  }
}

proc gli_prot_cli {command} {
  regsub -all {\\\"} $command "\"" prot_command
  actualize_cli $prot_command
  eval "return \[gli $command]"
}

proc gli_prot {args} {
  actualize_cli [string tolower $args]
  eval "return \[gli $args]"
}

proc gli {args} {
  global errorInfo
  global gli
  global DEVELOPMENT

if {$DEVELOPMENT} {puts "GLI: $args"}
  DefineCursor . watch
  set command ""
  foreach arg $args {
    set command "$command $arg"
  }
  if {[catch {set rc [: $command]}]} {
    set sight_error ""
    catch {set sight_error "\n[: PRINT \$SIGHT_ERROR]"}
    set err "$gli(ErrList,CouldntExecute)\n${command}${sight_error}"
    ErrorBox "?$err"
    set rc "ERROR"
  }
  DefineCursor .

  return $rc
}

proc sight_info {info} {
  global settings view
  global seq_num
  global DEVELOPMENT

if {$DEVELOPMENT} {puts "SIGHT INFO: $info"}

  foreach infoline [split $info "\n"] {
    set seq_num [expr $seq_num + 1]
    set key [lindex $infoline 0]
    set parms [lrange $infoline 1 end]

    switch $key {
      "select" {
        sight_info_select $parms
      }
      "deselect" {
        set settings(selected_type) "none"
        actualize_attrwindow
      }
      "viewport" {
        sight_info_viewport $parms
      }
      "window" {
        sight_info_window [lrange $parms 0 3]
        gli SIGHT DESELECT
#gli sight select last
      }
      "xform" {
        actualize_transformation $parms
      }
      "open" {
        if {[string toupper $parms] != $view(orientation)} {
          view_orientation $parms
        }
        get_trafo_data
      }
      "clear" -
      "import" -
      "create" {
        get_trafo_data
      }
      "orientation" {
        if {[string toupper $parms] != $view(orientation)} {
          view_orientation $parms
        }
      }
      "scale" {
        sight_info_scale [lindex $parms 0]
        sight_info_flip [lindex $parms 1]
      }
    }
  }
}

proc sight_info_select {parms} {
  global settings

  set key [lindex $parms 0]

  switch  $key {
    "fill_area" {
      set settings(selected_type) "array"
      set settings(fillcolor) [lindex $parms 3]
    }
    "polymarker" {
      set settings(selected_type) "marker"
      set settings(mtype) [lindex $parms 1]
      set settings(markersize) [lindex $parms 2]
      set settings(markercolor) [lindex $parms 3]
    }
    "polyline" {
      set settings(selected_type) "line"
      set settings(ltype) [lindex $parms 1]
      set settings(linewidth) [lindex $parms 2]
      set settings(linecolor) [lindex $parms 3]
    }
    "error_bars" {
      set settings(selected_type) "error_bars"
      set settings(mtype) [lindex $parms 1]
      set settings(markersize) [lindex $parms 2]
      set settings(markercolor) [lindex $parms 3]
      set settings(linewidth) [lindex $parms 4]
      set settings(linecolor) [lindex $parms 5]
    }
    "text" {
      set settings(selected_type) "text"
      if {[lindex $parms 1] != "symbol"} {
        set settings(textfamily) "[lindex $parms 1],[lindex $parms 2]"
      } else {
        set settings(textfamily) [lindex $parms 1]
      }
      set direction [lindex $parms 3]
      if {$direction == "normal"} {set direction right}
      set alignment [lindex $parms 4]
      if {$alignment == "normal"} {set alignment left}
      set settings(textdirection) $direction
      set settings(textalignment) $alignment
      set settings(textsize) [lindex $parms 6]
      set settings(textcolor) [lindex $parms 7]
    }
    "axes" {
      set settings(selected_type) "axes"
      set majorx [lindex $parms 5]
      if {$majorx < 0} {
        set majorx [expr -$majorx]
        set labelsx 0
      } else {
        set labelsx 1
      }
      set majory [lindex $parms 6]
      if {$majory < 0} {
        set majory [expr -$majory]
        set labelsy 0
      } else {
        set labelsy 1
      }
      set settings(axestickx) [lindex $parms 1]
      set settings(axesticky) [lindex $parms 2]
      set settings(axesoriginx) [lindex $parms 3]
      set settings(axesoriginy) [lindex $parms 4]
      set settings(axesmajorx) $majorx
      set settings(axesmajory) $majory
      set settings(axesticksize) [lindex $parms 7]
      set settings(axeslabelsx) $labelsx
      set settings(axeslabelsy) $labelsy
      set settings(linewidth) [lindex $parms 8]
      set settings(linecolor) [lindex $parms 9]
      if {[lindex $parms 10] != "symbol"} {
        set settings(textfamily) "[lindex $parms 10],[lindex $parms 11]"
      } else {
        set settings(textfamily) [lindex $parms 10]
      }
      set settings(textsize) [lindex $parms 12]
      set settings(textcolor) [lindex $parms 13]
    }
    "image" {
      set settings(selected_type) "image"
      set settings(image,xstart) [lindex $parms 2]
      set settings(image,ystart) [lindex $parms 3]
      set settings(image,xsize) [lindex $parms 4]
      set settings(image,ysize) [lindex $parms 5]
      set settings(image,xmin) [lindex $parms 6]
      set settings(image,xmax) [lindex $parms 7]
      set settings(image,ymin) [lindex $parms 8]
      set settings(image,ymax) [lindex $parms 9]
    }
    default {}
  }

  actualize_attrwindow
}

proc sight_info_viewport {parms {viewport {}} } {
  global settings view

  if {$viewport == ""} {set viewport $view(transformation)}
  set settings(trafo,$viewport) [lrange $parms 0 3]
  actualize_canvas
#  sight_info_window $settings(window,$viewport)
}
proc sight_info_window {window {viewport {}} } {
  global settings view

  if {$viewport == ""} {set viewport $view(transformation)}
  set settings(window,$viewport) $window
  if {$viewport == $view(transformation)} {
    set settings(xfrom) [lindex $window 0]
    set settings(xto) [lindex $window 1]
    set settings(yfrom) [lindex $window 2]
    set settings(yto) [lindex $window 3]
  }
}

proc sight_info_flip {flip {viewport {}} } {
  global settings view

  set flipx [set flipy 0]
  switch -- $flip {
    "none" {}
    "flip_x" {set flipx 1}
    "flip_y" {set flipy 1}
    "flip_xy" {set flipx 1; set flipy 1}
default {puts "!!!!!!! $flip"}
  }

  if {$viewport == ""} {set viewport $view(transformation)}
  set settings(flipx,$viewport) $flipx
  set settings(flipy,$viewport) $flipy
  if {$viewport == $view(transformation)} {
    set settings(flipx) $flipx
    set settings(flipy) $flipy
  }
}

proc sight_info_scale {scale {viewport {}} } {
  global settings view

  set xlog [set ylog 0]
  switch -- $scale {
    "linear" {}
    "x_log" {set xlog 1}
    "y_log" {set ylog 1}
    "xy_log" {set xlog 1; set ylog 1}
default {puts "scale !!!!!!! $scale"}
  }

  set err 0
  if {$xlog && $settings(xfrom) <= 0} {
    set xlog 0
    ErrorBox OriginNotPositive
    set err 1
  }
  if {$ylog && $settings(yfrom) <= 0} {
    set ylog 0
    if {!$err} {ErrorBox OriginNotPositive}
    set err 1
  }

  if {$viewport == ""} {set viewport $view(transformation)}
  set settings(xlog,$viewport) $xlog
  set settings(ylog,$viewport) $ylog
  if {$viewport == $view(transformation)} {
    set settings(xlog) $xlog
    set settings(ylog) $ylog
  }

  if {$err} {set_log}
}

proc init_gli {} {
  global td_env view const
  global tcl_platform
  
  if {$tcl_platform(platform) == "windows"} {
    gli DEFINE LOGICAL GKSconid \"[winfo id .display.gli]\"
  } else {
    gli DEFINE LOGICAL GKSconid \"[winfo screen .]![winfo id .display.gli]\"
  }
  gli GKS OPEN_WS WK1 $td_env(gli_wstype)
  gli SIGHT OPEN_SIGHT
  gli SIGHT SET ORIENTATION $view(orientation)

  set quotient [expr $const(display_height)./$const(display_width)]
  switch $view(orientation) {
    PORTRAIT {gli GKS SET WS_WINDOW 0 $quotient 0 1}
    LANDSCAPE {gli GKS SET WS_WINDOW 0 1 0 $quotient}
  }
  gli SIGHT REDRAW
}

proc actualize_canvas {} {
  global const
  global settings view

  set wtc $settings(canvaslabel)

  set width 180
  set height [expr $width/$const(ratio)]
  set factor [expr $width-7]

  if {$view(orientation) == "PORTRAIT"} {
    set i $width
    set width $height
    set height $i
  }

  $wtc delete all
  $wtc configure -width $width
  $wtc configure -height $height

# set canvas-borders
  set x 2
  set y [expr $height-3]

  set act_trafo $view(transformation)
  if {$act_trafo} {
    $wtc create rect [expr $x+[lindex $settings(trafo,$act_trafo) 0]*$factor] \
             [expr $y-[lindex $settings(trafo,$act_trafo) 2]*$factor] \
             [expr $x+[lindex $settings(trafo,$act_trafo) 1]*$factor] \
             [expr $y-[lindex $settings(trafo,$act_trafo) 3]*$factor] \
                   -fill [lindex $settings(trafo,colors) [expr $act_trafo-1]] \
                   -outline [lindex $settings(trafo,colors) [expr $act_trafo-1]]
  }
  for {set i 8} {$i >= 1} {incr i -1} {
    if {$i != $act_trafo} {
           $wtc create rect [expr $x+[lindex $settings(trafo,$i) 0]*$factor] \
                    [expr $y-[lindex $settings(trafo,$i) 2]*$factor] \
                    [expr $x+[lindex $settings(trafo,$i) 1]*$factor] \
                    [expr $y-[lindex $settings(trafo,$i) 3]*$factor] \
                         -outline [lindex $settings(trafo,colors) [expr $i-1]]
    }
  }

}


proc set_log {} {
  global settings view

  switch [expr $settings(xlog)*2+$settings(ylog)] {
    0 {set arg linear}
    1 {set arg y_log}
    2 {set arg x_log}
    3 {set arg xy_log}
  }

  gli_prot SIGHT SET SCALE $arg CURRENT
}

proc set_flip {} {
  global settings view

  set settings(flipx,$view(transformation)) $settings(flipx)
  set settings(flipy,$view(transformation)) $settings(flipy)

  switch [expr $settings(flipx)*2+$settings(flipy)] {
    0 {set arg none}
    1 {set arg flip_y}
    2 {set arg flip_x}
    3 {set arg flip_xy}
  }

  gli_prot SIGHT SET SCALE CURRENT $arg
}

proc fill_settings_box {w} {
  global settings
  global view
  global const

  set settings(trafo,colors) "black white red green blue cyan yellow magenta"

# transformation-canvas

  set wt $w.transform
  frame $wt -relief $const(frame_relief) -bd 1

  label $wt.headline -text "Transformations"
  set wtc [set settings(canvaslabel) $wt.canvas]
  canvas $wtc -background grey60
  actualize_canvas
  set settings(buttonslabel) $wt.buttons
  frame $wt.buttons
  frame $wt.colors

  pack $wt.headline
  pack $wt.canvas -pady 4
  pack $wt.buttons $wt.colors
  for {set i 0} {$i <= 8} {incr i 1} {
    button $wt.buttons.$i -text $i -command "view_transformation $i" \
                          -padx 2 -pady 0 -width 1
    pack $wt.buttons.$i -side left -fill x
    button $wt.colors.$i -state disabled -relief flat -padx 2 -pady 0 -width 1
    if {$i} {
      $wt.colors.$i config -bg [lindex $settings(trafo,colors) [expr $i-1]]
    }
    pack $wt.colors.$i -side left -fill x
  }
  $wt.buttons.$view(transformation) config -fg red

# window-settings

  set ww $w.window
  frame $ww -relief $const(frame_relief) -bd 1
  label $ww.head -text "Window Settings"
  frame $ww.x
  label $ww.x.fromtxt -text "X from" -underline 0
  entry $ww.x.from -textvariable settings(xfrom) -width 8
  label $ww.x.totxt -text "to"
  entry $ww.x.to -textvariable settings(xto) -width 8
  frame $ww.y
  label $ww.y.fromtxt -text "Y from" -underline 0
  entry $ww.y.from -textvariable settings(yfrom) -width 8
  label $ww.y.totxt -text "to"
  entry $ww.y.to -textvariable settings(yto) -width 8

  frame $ww.scale
  checkbutton $ww.scale.x -text "X-Log" -variable settings(xlog) \
                          -command {set_log}
  checkbutton $ww.scale.y -text "Y-Log" -variable settings(ylog) \
                          -command {set_log}
  frame $ww.flip
  checkbutton $ww.flip.x -text "Flip X" -variable settings(flipx) \
                         -command set_flip
  checkbutton $ww.flip.y -text "Flip Y" -variable settings(flipy) \
                         -command set_flip

  pack $ww.head $ww.x $ww.y $ww.scale $ww.flip -pady 2 -expand 1 -fill x
  pack $ww.x.fromtxt $ww.x.from $ww.x.totxt $ww.x.to -side left
  pack $ww.y.fromtxt $ww.y.from $ww.y.totxt $ww.y.to -side left
  pack $ww.scale.x $ww.scale.y -side left -expand 1 -fill x
  pack $ww.flip.x $ww.flip.y -side left -expand 1 -fill x

  bind $ww.x.from <Return> {
    if {$settings(xfrom) == ""} {set settings(xfrom) 0}
    if {[test_axesvalue xfrom]} {
      .settings.window.x.to select range 0 end
      focus .settings.window.x.to
    }
  }

  bind $ww.x.to <Return> {
    if {$settings(xto) == ""} {set settings(xto) [expr $settings(xfrom)+1]}
    if {[test_axesvalue xto] && [compare_axesvalues xfrom xto]} {
      .settings.window.y.from select range 0 end
      focus .settings.window.y.from
    }
  }
  bind $ww.y.from <Return> {
    if {$settings(yfrom) == ""} {set settings(yfrom) 0}
    if {[test_axesvalue yfrom]} {
      .settings.window.y.to select range 0 end
      focus .settings.window.y.to
    }
  }
  bind $ww.y.to <Return> {
    if {$settings(yto) == ""} {set settings(yto) [expr $settings(yfrom)+1]}
    if {[catch {set settings(yto) [expr $settings(yto)]}]} {
      ErrorBox NoValidNumber
      %W select range 0 end
    } elseif {$settings(yto) <= $settings(yfrom)} {
      ErrorBox MustBeGreater
      %W select range 0 end
    } else {
      focus .
      set_window
    }
  }


  bind . <Control-Key-x> {
    .settings.window.x.from select range 0 end
    focus .settings.window.x.from
  }
  bind . <Control-Key-y> {
    .settings.window.y.from select range 0 end
    focus .settings.window.y.from
  }

# button for attribute window

  set wm $w.attrwindow
  button $wm -text "Attribute Window" -command open_attrwindow

# button for cli

  set wc $w.cli
  button $wc -text "Command Window" -command open_cli


# pack frames
  pack $wt $ww -pady 7 -padx 2 -ipady 4 -fill x
  pack $wm $wc -pady 7 -padx 2

}

proc set_fillstyle {type index} {
  gli_prot SIGHT SET FILLSTYLE $type
  if {$index != -1} {
    gli_prot SIGHT SET FILLINDEX $index
  }
}


proc test_axesvalue {var} {
  global settings

  if {[catch {set settings($var) [expr $settings($var)]}]} {
    ErrorBox NoValidNumber
    set w .settings.window.$var
    $w select range 0 end
    focus $w
    return 0
  } else {
    return 1
  }
}

proc compare_axesvalues {varfrom varto} {
  global settings

  if {$settings($varfrom) >= $settings($varto)} {
    ErrorBox MustBeGreater
    set w .settings.window.[string index $varfrom 0].from
    $w select range 0 end
    focus $w
    return 0
  } else {
    return 1  
  }

}

# set window to user-defined values
proc test_window {} {
  if {[test_axesvalue xfrom] && [test_axesvalue xto] && \
      [test_axesvalue yfrom] && [test_axesvalue yto] && \
      [compare_axesvalues xfrom xto] && [compare_axesvalues yfrom yto]} {
    return 1
  } else {
    return 0
  }
}

proc set_window {} {
  global settings view

  if {[test_window]} {
    gli SIGHT SET WINDOW $settings(xfrom) $settings(xto) \
                         $settings(yfrom) $settings(yto)
    set settings(window,$view(transformation)) \
             "$settings(xfrom) $settings(xto) $settings(yfrom) $settings(yto)"
    return 1
  } else {
    return 0
  }
}


# init menu for attribute window
proc init_menu {attr w text {cmdformat {}} } {
  global menu
  global settings const

  if {$cmdformat == ""} {
    if {$attr == "ltype"} {\
      set sattr "linetype"
    } elseif {$attr == "mtype"} {
      set sattr "markertype"
    } else {
      set sattr $attr
    }
    set cmdformat "SIGHT SET $sattr %s"
  }

  set entries $menu(attr,$attr)
  frame $w
  label $w.text -text $text
  menubutton $w.menub -menu $w.menub.menu -relief raised \
               -image $menu(attr,$attr,$settings($attr),image)
  menu $w.menub.menu -tearoff 0
  foreach entry $entries {
    if {[info exists menu(attr,$attr,$entry)]} {
      set entries2 $menu(attr,$attr,$entry)
      set newmenu $w.menub.menu.$entry
      $w.menub.menu add cascade -menu $newmenu \
                                  -image $menu(attr,$attr,$entry,image)
      menu $newmenu -tearoff 0
      foreach entry2 $entries2 {
        $newmenu add command \
                         -image $menu(attr,$attr,$entry,$entry2,image) \
                         -command "\
            set settings(attr) $entry,$entry2;\
            gli_prot [format $cmdformat $entry] $entry2;\
            $w.menub config -image $menu(attr,$attr,$entry,$entry2,image)"
      }
    } else {
      $w.menub.menu add command \
                             -image $menu(attr,$attr,$entry,image) \
                             -command "\
                   set settings(attr) $entry; \
                   gli_prot [format $cmdformat $entry]; \
                   $w.menub config -image $menu(attr,$attr,$entry,image)"
    }
  }
  pack $w -pady 3
  pack $w.text $w.menub -side left
}

proc incr_origin {value flag dim} {
  global settings

  if {[test_window]} {
    set window [expr $settings(${dim}to)-$settings(${dim}from)]
    set log [expr int(log($window)/log(10))]
    set quotient [expr $window/pow(10,$log)]
    if {$quotient >= 5} {
      set increment [expr pow(10,$log)]
    } elseif {$quotient >= 3} {
      set increment [expr pow(10,$log)*0.5]
    } else {
      set increment [expr pow(10,$log-1)]
    }

    set rc [expr round((${value}${flag}${increment})/$increment) * $increment]
    if {$rc < $settings(${dim}from)} {
      set rc $settings(${dim}from)
    } elseif {$rc > $settings(${dim}to)} {
      set rc $settings(${dim}to)
    }
    return $rc
  } else {
    return $value
  }
}

proc incr_tick {value flag dim} {
  global settings

  if {[test_window]} {
    set window [expr $settings(${dim}to)-$settings(${dim}from)]
    set log [expr floor(log($window)/log(10))]
    set quotient [expr $window/pow(10,$log)]
    if {$quotient >= 5} {
      set increment [expr pow(10,$log-1)]
    } else {
      set increment [expr pow(10,$log-1)*0.5]
    }

    set rc [expr round((${value}${flag}${increment})/$increment) * $increment]
    if {$rc <= [expr pow(10,$log-2)]} {set rc [expr pow(10,$log-2)]}
    return $rc
  } else {
    return $value
  }
}
proc incr_major {value flag dim} {
  set rc [expr int(${value}${flag}1)]
  if {$rc < 0} {set rc 0}
  return $rc
}
proc incr_ticksize {value flag dim} {
  set increment 0.001
  set rc [expr round((${value}${flag}${increment})/$increment) * $increment]
  if {$rc == 0} {set rc [expr ${rc}${flag}${increment}]}
  return $rc
}

proc verify_origin {w index dim} {
  global settings

  set value $settings($index)
  if {[catch {set value [expr $value]}]} {
    ErrorBox NoValidNumber
    $w select range 0 end
    focus $w
    return 0
  }
  if {$value < $settings(${dim}from)} {
    set value $settings(${dim}from)
  } elseif {$value > $settings(${dim}to)} {
    set value $settings(${dim}to)
  }
  set settings($index) $value
  focus .
  return 1
}
proc verify_tick {w index dim} {
  global settings

  set value $settings($index)
  if {[catch {set value [expr $value]}]} {
    ErrorBox NoValidNumber
    $w select range 0 end
    focus $w
    return 0
  }
  set window [expr $settings(${dim}to)-$settings(${dim}from)]
  set log [expr int(log($window)/log(10))]
  if {$value <= [expr pow(10,$log-2)]} {set value [expr pow(10,$log-2)]}
  set settings($index) $value
  focus .
  return 1
}
proc verify_major {w index dim} {
  global settings

  set value $settings($index)
  if {[catch {set value [expr $value]}]} {
    ErrorBox NoValidNumber
    $w select range 0 end
    focus $w
    return 0
  }
  if {[expr $value-int($value)] != 0} {
    ErrorBox NoValidInteger
    $w select range 0 end
    focus $w
    return 0
  }
  if {$value < 0} {set value 0}
  set settings($index) $value
  focus .
  return 1
}
proc verify_ticksize {w index dim} {
  global settings

  set value $settings($index)
  if {[catch {set value [expr $value]}]} {
    ErrorBox NoValidNumber
    $w select range 0 end
    focus $w
    return 0
  }
  if {$value == 0} {set value 0.001}
  set settings($index) $value
  focus .
  return 1  
}


proc init_entry {w index incr_funct verify_funct dim} {
  global settings
  global file

  frame $w
  frame $w.left
  frame $w.right
  entry $w.left.entry -textvariable settings($index) -width 5
  button $w.right.up -bitmap @$file(bitmappath)/up.xbm -width 10 -height 10 \
                     -command "\
           set settings($index) \[$incr_funct \$settings($index) + $dim\]; \
           draw_axes"
  button $w.right.down -bitmap @$file(bitmappath)/down.xbm -width 10 -height 10\
                     -command "\
           set settings($index) \[$incr_funct \$settings($index) - $dim\]; \
           draw_axes"

  pack $w.left $w.right -side left
  pack $w.left.entry
  pack $w.right.up $w.right.down

  bind $w.left.entry <Return> \
            "if \{\[$verify_funct $w.left.entry $index $dim\]\} \{draw_axes\}"
}

proc init_checkbutton {w index} {
  global settings

  checkbutton $w -text on -variable settings($index) -command draw_axes
}

proc draw_axes {} {
  global settings

  edit_remove

  set majorx $settings(axesmajorx)
  set majory $settings(axesmajory)
  if {!$settings(axeslabelsx)} {set majorx -$majorx}
  if {!$settings(axeslabelsy)} {set majory -$majory}

  gli_prot SIGHT AXES $settings(axestickx) $settings(axesticky) \
                      $settings(axesoriginx) $settings(axesoriginy) \
                      $majorx $majory \
                      $settings(axesticksize)

  gli_prot SIGHT SELECT LAST
}

proc modify_line {w args} {
  if {$args == "all"} {
    label $w.headline -text "Last selected item: Line"
    pack $w.headline
  }
  pack $w -fill x

  if {$args == "all"} {
    init_menu ltype $w.type "Linetype   "
  }
  init_menu linecolor $w.color "Linecolor   "
  init_menu linewidth $w.width "Linewidth   "
}

proc modify_marker {w} {
 label $w.headline -text "Last selected item: Marker(s)"
 pack $w
 pack $w.headline

 init_menu mtype $w.type "Markertype   "
 init_menu markercolor $w.color "Markercolor   "
 init_menu markersize $w.size "Makersize   "
}

proc modify_array {w} {
 label $w.headline -text "Last selected item: Filled Array"
 pack $w
 pack $w.headline

 init_menu fillcolor $w.color "Fillcolor   "

 frame $w.style
 label $w.style.txt1 -text "Open"
 button $w.style.button -text "Fillstyle-Window" -command attr_fillstyle
 label $w.style.txt2 -text "to select Fillstyle"

 pack $w.style
 pack $w.style.txt1 $w.style.button $w.style.txt2 -side left
}

proc modify_text {w args} {
  if {$args == "all"} {
    label $w.headline -text "Last selected item: Text"
    pack $w.headline
  }
  pack $w -fill x

  init_menu textfamily $w.family "Textfamily   " "SIGHT SET TEXTFONT %s"
  init_menu textsize $w.size "Textsize   "
  if {$args == "all"} {
    init_menu textalignment $w.alignment "Textalignment   " \
                                                 "SIGHT SET TEXTHALIGN %s"
    init_menu textdirection $w.direction "Textdirection   "
  }
  init_menu textcolor $w.color "Textcolor   "
}

proc modify_axes {w} {
  label $w.headline -text "Last selected item: Axes"
  frame $w.txt
  label $w.txt.nix -text ""
  label $w.txt.origin -text "Origin"
  label $w.txt.tick -text "Tick-\nInterval"
  label $w.txt.major -text "Major\nTicks"
  label $w.txt.labels -text "Labels"
  frame $w.xentry
  label $w.xentry.txt -text " X    "
  init_entry $w.xentry.origin axesoriginx incr_origin verify_origin x
  init_entry $w.xentry.tick axestickx incr_tick verify_tick x
  init_entry $w.xentry.major axesmajorx incr_major verify_major x
  init_checkbutton $w.xentry.labels axeslabelsx
  frame $w.yentry
  label $w.yentry.txt -text " Y    "
  init_entry $w.yentry.origin axesoriginy incr_origin verify_origin y
  init_entry $w.yentry.tick axesticky incr_tick verify_tick y
  init_entry $w.yentry.major axesmajory incr_major verify_major y
  init_checkbutton $w.yentry.labels axeslabelsy
  frame $w.sizeentry
  label $w.sizeentry.txt -text "Tick-Size"
  init_entry $w.sizeentry.entry axesticksize incr_ticksize verify_ticksize xy

  pack $w
  pack $w.headline
  pack $w.txt -fill x
  pack $w.xentry $w.yentry $w.sizeentry -fill x -pady 5

  pack $w.txt.nix $w.txt.origin $w.txt.tick $w.txt.major \
       $w.txt.labels -side left -padx 15
  pack $w.xentry.txt $w.xentry.origin $w.xentry.tick $w.xentry.major \
       $w.xentry.labels -side left -padx 5
  pack $w.yentry.txt $w.yentry.origin $w.yentry.tick $w.yentry.major \
       $w.yentry.labels -side left -padx 5
  pack $w.sizeentry.txt $w.sizeentry.entry -side left -padx 5
}

proc create_position_canvas {w cmd} {

}

proc modify_image {w} {
return
  global settings

  label $w.headline -text "Last selected item: Image"
  label $w.posheadline -text "Image-position"
  frame $w.pos -bd 2 -relief ridge
  frame $w.pos.pos
  frame $w.pos.size
  create_position_canvas $w.pos.pos xstart ystart xsize ysize \
                                    set_image_size
  label $w.sizeheadline -text "Image-clipping"
  frame $w.size -bd 2 -relief ridge
  frame $w.size.pos
  frame $w.size.size
  create_position_canvas $w.pos.pos xmin ymin xmax ymax \
                                    set_image_position

  pack $w
  pack $w.headline
  pack $w.posheadline $w.pos $w.sizeheadline $w.size -fill x -pady 5
  pack $w.pos.pos $w.pos.size -fill x -pady 5
  pack $w.size.pos $w.size.size -fill x -pady 5
}

proc open_attrwindow {} {
  if {[winfo exists .attrwindow]} {return}

  toplevel .attrwindow
  button .attrwindow.close -text "close" -command {
    destroy .attrwindow
    update
    edit_redraw
  }
  pack .attrwindow.close -side bottom
  wm title .attrwindow "Attribute Window"
  wm transient .attrwindow .

  set geom [lrange [split [wm geometry .] "+"] 1 2]
  set xpos [expr [lindex $geom 0]+500]
  set ypos [expr [lindex $geom 1]+200]
  wm geometry .attrwindow +$xpos+$ypos

  actualize_attrwindow
}

proc actualize_attrwindow {} {
  global menu const
  global settings

  if {![winfo exists .attrwindow]} {return}

  set mp .attrwindow.parms
  catch {destroy $mp}
  frame $mp -relief ridge -bd 1

  switch -exact $settings(selected_type) {
    "line" {
      modify_line $mp all
    }
    "marker" {
      modify_marker $mp
    }
    "text" {
      modify_text $mp all
    }
    "array" {
      modify_array $mp
    }
    "error_bars" {
      frame $mp.marker -relief ridge -bd 1
      frame $mp.line -relief ridge -bd 1
      modify_marker $mp.text 
      modify_line $mp.line axes
      pack $mp
    }
    "axes" {
      frame $mp.axes -relief ridge -bd 1
      frame $mp.text -relief ridge -bd 1
      frame $mp.line -relief ridge -bd 1
      modify_axes $mp.axes
      modify_text $mp.text axes
      modify_line $mp.line axes
      pack $mp
    }
    "image" {
      modify_image $mp
    }
    "none" {
      label $mp.headline -text "No actual item selected"
      pack $mp
      pack $mp.headline
    }
    default {
      label $mp.headline -text "No item to modify selected"
      pack $mp
      pack $mp.headline
    }
  }
}

proc set_orientation {command} {

  if {[llength $command] != 4} {
    ErrorBox WrongNumber
    return
  }
  set arg [lindex $command 3]

  if {[string match "${arg}*" "landscape"]} {
    view_orientation landscape
  } elseif {[string match "${arg}*" "portrait"]} {
    view_orientation portrait
  } else {
    ErrorBox UnknownKeyword
  }

}

proc actualize_cli { {command {}} } {
  global protocol

  if {$command != ""} {lappend protocol(commands) $command}

  if {![winfo exists .cli]} {return}

  set w .cli.protocol.txt
  $w config -state normal

  for {set i [expr $protocol(count)+1]} {$i < [llength $protocol(commands)]} \
      {incr i 1} {
    $w insert end "[lindex $protocol(commands) $i]\n"
    set protocol(count) $i
  }
  $w config -state disabled
  $w see $i.0
}


proc takeSelection {w entry} {
  global protocol

  set index [expr [getSelection $w]-1]
  set protocol(act_command) [lindex $protocol(commands) $index]
  $entry icursor end
}

proc getSelection {w} {
  set range [$w tag ranges sel]
  if {$range == ""} {return -1}
  return [lindex [split [lindex $range 0] "."] 0]
}

proc moveSelection {w increment} {
  if {[set textpos [getSelection $w]] == -1} {
    set textpos [expr [lindex [split [$w index end] "."] 0]-1]
  }
  set min 1
  set max [expr [lindex [split [$w index end] "."] 0]-2]

  incr textpos $increment
  if {$textpos < $min} {
    set textpos $min
  } elseif {$textpos > $max} {
    set textpos $max
  }
  $w tag remove sel 0.0 end
  $w tag add sel $textpos.0 [expr $textpos+1].0
  $w see $textpos.0
}

proc clear_command {string w} {

  regsub -all {\\} $string "" string
  regsub -all "\"" $string "\\\"" string

  set count [regsub -all "\"" $string "x" dummy]
  if {[expr $count%2]} {
    tk_messageBox -title "error" -type ok -parent $w -icon error -message \
                  "The command contains an odd number of quotes"
    return ""
  } elseif {[catch {llength $string}]} {
    tk_messageBox -title "error" -type ok -parent $w -icon error \
                  -message "Syntax error"
    return ""
  }

  return $string
}

proc check_command {pattern command} {
   set i 0
   foreach keyword [lindex $pattern 0] abb [lindex $pattern 1] {
     set command_word [string tolower [lindex $command $i]]
     if {[string length $command_word] < $abb || \
         ![string match "${command_word}*" $keyword] || \
         ($abb == -1 && $command_word != "")} {
       return -1
     }
     incr i
   }
   set inv_command [lindex $pattern 2]
   if {$inv_command != ""} {eval "$inv_command"}
   return [lindex $pattern 3]
}

proc check_command_before {command} {

#     sight-command           abbr.    command                    0=don't go on
#                                                                 1=go on
  foreach pattern { \
     {"sight polyline"        "3 5 -1" "tools_line locate"        0} \
     {"sight polymarker"      "3 5 -1" "tools_marker locate"      0} \
     {"sight fill_area"       "3 2 -1" "tools_fill locate"        0} \
     {"sight bar_graph"       "3 1 -1" "tools_bar locate"         0} \
     {"sight plot"            "3 2 -1" "tools_plot locate"        0} \
                  } {
    if {[set rc [check_command $pattern $command]] != -1} {return $rc}
  }

  return 1
}

proc check_command_after {command} {

#     sight-command           abbr.    command                    0=dont go on
#                                                                 1=go on
  foreach pattern { \
     {"sight set orientation" "3 3 1"  "set_orientation $command" 0} \
                  } {
#     {"sight open_drawing"    "3 6"   "get_trafo_data"           1}
    if {[set rc [check_command $pattern $command]] != -1} {return}
  }
}

proc exec_command {command} {
  if {$command != ""} {
    set command [clear_command $command .cli]
    if {[check_command_before $command]} {
      set rc [gli_prot_cli "$command"]
      if {$rc != "ERROR"} {check_command_after $command}
    }
  }
}

proc open_cli {} {
  global protocol

  if {[winfo exists .cli]} {return}

  toplevel .cli

  set w .cli.protocol  
  frame $w
  text $w.txt -wrap none -width 80 -height 10 \
              -xscrollcommand "$w.hori set" -yscrollcommand "$w.vert set"
  scrollbar $w.hori -orient horizontal -command "$w.txt xview"
  scrollbar $w.vert -orient vertical -command "$w.txt yview"

  frame .cli.command
  label .cli.command.text -text "Enter Command:"
  entry .cli.command.entry -textvariable protocol(act_command)

  button .cli.exit -text "Close" -command {
    destroy .cli
    update
    edit_redraw
  }

  pack $w -padx 5 -pady 5 -expand 1 -fill both
  grid $w.txt -row 0 -column 0 -sticky nsew
  grid $w.hori -row 1 -column 0 -pady 3 -sticky we
  grid $w.vert -row 0 -column 1 -padx 3 -sticky ns
  pack .cli.command -padx 5 -pady 5 -fill x
  pack .cli.command.text -side left
  pack .cli.command.entry -side left -fill x -expand 1
  pack .cli.exit -padx 5 -pady 5 -anchor center

  grid columnconfigure $w 0 -weight 1
  grid rowconfigure $w 0 -weight 1

  PlaceToplevel .cli
  wm title .cli "Command Window"
  wm transient .cli .

  bind Text <B1-Motion> {}
  bind Text <Shift-B1-Motion> {}
  bind Text <Double-Button-1> {
    set index [expr [getSelection .cli.protocol.txt]-1]
    exec_command [lindex $protocol(commands) $index]
  }
  bind Text <Triple-Button-1> {}
  bind Text <Control-Button-1> {}
  bind Text <ButtonPress-1> {
    tkTextButton1 %W %x %y
    %W tag remove sel 0.0 end
    set tkPriv(selectMode) line
    tkTextSelectTo %W %x %y
    catch {%W mark set insert sel.first}
    takeSelection .cli.protocol.txt .cli.command.entry
  }

  bind .cli <Up> {
    moveSelection .cli.protocol.txt -1
    takeSelection .cli.protocol.txt .cli.command.entry
  }
  bind .cli <Down> {
    moveSelection .cli.protocol.txt 1
    takeSelection .cli.protocol.txt .cli.command.entry
  }
  bind .cli <Prior> {
    moveSelection .cli.protocol.txt -5
    takeSelection .cli.protocol.txt .cli.command.entry
  }
  bind .cli <Next> {
    moveSelection .cli.protocol.txt 5
    takeSelection .cli.protocol.txt .cli.command.entry
  }
  bind .cli <Shift-Left> {
    .cli.protocol.txt xview scroll -5 units
  }
  bind .cli <Shift-Right> {
    .cli.protocol.txt xview scroll 5 units
  }

  bind .cli.command.entry <Return> {
    exec_command $protocol(act_command)
    set protocol(act_command) ""
  }


  set protocol(count) -1
  actualize_cli
  focus .cli.command.entry
}

proc actualize_transformation {trafo} {
  global view settings

  set w $settings(buttonslabel)

  for {set i 0} {$i <= 8} {incr i 1} {
    $w.$i config -fg black
  }
  $w.$trafo config -fg red

  set view(transformation) $trafo

  actualize_canvas
  sight_info_window $settings(window,$trafo)
  set settings(flipx) $settings(flipx,$trafo)
  set settings(flipy) $settings(flipy,$trafo)
  set settings(xlog) $settings(xlog,$trafo)
  set settings(ylog) $settings(ylog,$trafo)
}

proc get_trafo_data {} {
  global DEVELOPMENT

  set trafodata [gli SIGHT INQUIRE XFORM]
  foreach trafoinfo [split $trafodata ";"] {
    if {[set trafoinfo [split $trafoinfo ","]] != ""} {
if {$DEVELOPMENT} {puts "TRAFO: $trafoinfo"}
      set i [string trim [lindex $trafoinfo 0]]
      set window [string trim [lindex $trafoinfo 1]]
      set viewport [string trim [lindex $trafoinfo 2]]
      set scaleflip [string trim [lindex $trafoinfo 3]]
      sight_info_window $window $i
      sight_info_viewport $viewport $i
      sight_info_scale [lindex $scaleflip 0] $i
      sight_info_flip [lindex $scaleflip 1] $i
    }
  }
}

proc filename {dir file} {
  if {$dir == "/"} {
    return "/$file"
  } else {
    return "$dir/$file"
  }

}


## menu-commands ##
## ============= ##

## file ##

proc file_create {} {
  global gli

  set gli(filter) ".sight"
  if {![set rc [FileSelectionBox -title "Create File" -filters "1 0"]]} {
    if {$gli(status) == 1} {
      gli_prot_cli \
             "sight create_drawing \\\"[filename $gli(dir) $gli(filename)]\\\""
    }
  }
  update
  gli_prot SIGHT REDRAW
}

proc file_open {} {
  global gli

  set gli(filter) ""
  if {![set rc [FileSelectionBox -title "Open File" -filters "1 0"]]} {
    if {$gli(status) == 1} {
      gli_prot_cli \
              "sight open_drawing \\\"[filename $gli(dir) $gli(filename)]\\\""
#      get_trafo_data
    }
  }
  update
  gli_prot SIGHT REDRAW
}

proc file_save {} {
  global gli

  set gli(filter) ".sight"
  if {![set rc \
        [FileSelectionBox -title "Save File" -filters "1 0" -action "save"]]} {
    if {$gli(status) == 1} {
      gli_prot_cli \
            "sight close_drawing \\\"[filename $gli(dir) $gli(filename)]\\\""
    }
  }
  update
  gli_prot SIGHT REDRAW
}

proc file_close {} {
  gli_prot SIGHT CLOSE_DRAWING
}

proc file_read {} {
  global gli

  set gli(filter) ""
  if {![set rc [FileSelectionBox -title "Read Data" -filters "2 0"]]} {
    if {$gli(status) == 1} {
      gli_prot_cli "read \\\"[filename $gli(dir) $gli(filename)]\\\" x y"
    }
  }
  update
  gli_prot SIGHT REDRAW
}
proc file_write {} {
  global gli

  set gli(filter) ".dat"
  if {![set rc \
        [FileSelectionBox -title "Write Data" -filters "2 0" -action "save"]]} {
    if {$gli(status) == 1} {
      gli_prot_cli "write \\\"[filename $gli(dir) $gli(filename)]\\\" x y"
    }
  }
  update
  gli_prot SIGHT REDRAW
}

proc file_image {args} {
  global gli

  set gli(filter) ""
  if {![set rc \
        [FileSelectionBox -title "Read portable image file" -filters "0"]]} {
    if {$gli(status) == 1} {
      gli_prot_cli "sight image \\\"[filename $gli(dir) $gli(filename)]\\\""
    }
  }
  update
  gli_prot SIGHT REDRAW
}

proc file_import {args} {
  global gli


  set gli(filter) ""
  switch -- $args {
    "image_file" {
      if {![set rc [FileSelectionBox -title "Import imagefile" -filters "0"]]} {
        if {$gli(status) == 1} {
          gli_prot_cli \
       "sight image \\\"|convert [filename $gli(dir) $gli(filename)] pnm:-\\\""
        }
      }
    }
    "binary" -
    "clear_text" {
      if {![set rc [FileSelectionBox -title "Import CGM $args" -filters "3 0"]]} {
        if {$gli(status) == 1} {
          gli_prot_cli \
             "view \\\"[filename $gli(dir) $gli(filename)]\\\" cgm_$args sight"
        }
      }
    }
  }
  update
  gli_prot SIGHT REDRAW
}

proc file_print {} {
  gli_prot SIGHT PRINT
}

proc file_capture {} {
  gli_prot SIGHT CAPTURE
}

proc file_exit {} {
  exit
}

## edit ##

proc edit_redraw {} {
  gli_prot SIGHT REDRAW
}
proc edit_select {args} {
  gli_prot SIGHT SELECT $args
}
proc edit_deselect {} {
  global settings

  gli_prot SIGHT DESELECT
#  set settings(selected_type) "none"
#  actualize_attrwindow
}
proc edit_cut {} {
  gli_prot SIGHT CUT
}
proc edit_paste {} {
  gli_prot SIGHT PASTE
}
proc edit_pop {} {
  gli_prot SIGHT POP
}
proc edit_push {} {
  gli_prot SIGHT PUSH
}
proc edit_move {} {
  gli_prot SIGHT MOVE
}
proc edit_remove {} {
  gli_prot SIGHT REMOVE
}
proc edit_pick {args} {
  gli_prot SIGHT PICK_$args
}
proc edit_clear {} {
  gli_prot SIGHT CLEAR
}


## tools ##

proc tools_line {args} {
  global gli

  switch -exact $args {
    "locate" {
      set gli(busyflag) -1
      gli_prot SIGHT POLYLINE
    }
    "data" {
      gli_prot SIGHT POLYLINE X Y
    }
  }
}

proc tools_spline {args} {
  switch -exact $args {
    "natural" {
      set code "0"
    }
    "smooth" {
      set code "-1"
    }
    "bspline" {
      set code "-2"
    }
  }

  gli_prot SIGHT SPLINE X Y $code
}

proc tools_error {args} {
  gli_prot SIGHT ERROR_BARS X Y E1 E2 $args
}

proc tools_marker {args} {
  global gli

  switch -exact $args {
    "locate" {
      set gli(busyflag) -1
      gli_prot SIGHT POLYMARKER
    }
    "data" {
      gli_prot SIGHT POLYMARKER X Y
    }
  }
}

proc tools_text {} {
  gli_prot SIGHT REQUEST_LOCATOR TXT_X TXT_Y
  set text [InputBox -title "Enter Text:" -width 50]
  update
  edit_redraw
  if {$text != ""} {eval gli_prot SIGHT TEXT TXT_X TXT_Y \"$text\"}
}

proc tools_fill {args} {
  global gli

  switch -exact $args {
    "locate" {
      set gli(busyflag) -1
      gli_prot SIGHT FILL_AREA
    }
    "data" {
      gli_prot SIGHT FILL_AREA X Y
    }
  }
}

proc tool_bar {args} {
  global gli

  switch -exact $args {
    "locate" {
      set gli(busyflag) -1
      gli_prot SIGHT BAR_GRAPH
    }
    "data" {
      gli_prot SIGHT BAR_GRAPH X Y
    }
  }
}

proc tools_plot {args} {
  global gli

  switch -exact $args {
    "locate" {
      set gli(busyflag) -1
      gli_prot SIGHT PLOT
    }
    "data" {
      gli_prot SIGHT PLOT X Y
    }
  }
}

proc tools_axes {} {
  gli_prot SIGHT AXES
}

proc tools_grid {} {
  gli_prot SIGHT GRID
}

## attr ##

proc attr_ltype {args} {
  gli_prot SIGHT SET LINETYPE $args
}
proc attr_linewidth {args} {
  gli_prot SIGHT SET LINEWIDTH $args
}
proc attr_linecolor {args} {
  gli_prot SIGHT SET LINECOLOR $args
}

proc attr_mtype {args} {
  gli_prot SIGHT SET MARKERTYPE $args
}
proc attr_markersize {args} {
  gli_prot SIGHT SET MARKERSIZE $args
}
proc attr_markercolor {args} {
  gli_prot SIGHT SET MARKERCOLOR $args
}

proc attr_textfamily {args} {
  set family [lindex $args 0]
  if {[set mode [lindex $args 1]] != ""} {
    gli_prot SIGHT SET TEXTFONT $family $mode
  } else {
    gli_prot SIGHT SET TEXTFONT $family
  }
}
proc attr_textsize {args} {
  gli_prot SIGHT SET TEXTSIZE $args
}
proc attr_textalignment {args} {
  gli_prot SIGHT SET TEXTHALIGN $args
}
proc attr_textdirection {args} {
  gli_prot SIGHT SET TEXTDIRECTION $args
}
proc attr_textcolor {args} {
  gli_prot SIGHT SET TEXTCOLOR $args
}

proc attr_fillstyle {} {
  global bitmap

  if {[winfo exists .fillstyle]} {
    wm iconify .fillstyle
    wm deiconify .fillstyle
    return
  }

  DefineCursor . watch

  toplevel .fillstyle
  wm title .fillstyle "Fillstyles"

  frame .fillstyle.holframe
  button .fillstyle.holframe.hollow -text Hollow -command {
    set_fillstyle hollow -1
  }
  pack .fillstyle.holframe -fill x
  pack .fillstyle.holframe.hollow -side left

  set x 0
  set y 0
  set nr 0
  set end 0
  set frames .fillstyle.frame
  set actframe ${frames}0
  frame $actframe
  while {!$end} {
    set bitmapfile $bitmap(path)/attr_pattern_$nr.$bitmap(format)
    if {[file exists $bitmapfile]} {
      set image [image create photo -format $bitmap(format) -file $bitmapfile]
      button $actframe.button$nr -image $image \
                                 -command "set_fillstyle pattern $nr"
      pack $actframe.button$nr -side left
    } else {
      puts stderr "$bitmapfile does not exist"
    }
    incr nr 1
    if {$nr >= 120} {set end 1}
    incr x 1
    if {$x >= 16} {
      pack $actframe
      incr y 1
      set actframe $frames$y
      frame $actframe
      set x 0
    }
  }
  pack $actframe

  button .fillstyle.exit -text "close" -command {
    destroy .fillstyle
    update
    edit_redraw
  }
  pack .fillstyle.exit -pady 10

  DefineCursor .
}

proc attr_fillcolor {args} {
  gli_prot SIGHT SET FILLCOLOR $args
}


## view ##

proc view_viewport {} {
  global gli

  set gli(busyflag) 2
  gli_prot SIGHT SET VIEWPORT
}
proc view_window {} {
  global gli

  set gli(busyflag) 2
  gli_prot SIGHT SET WINDOW
}

proc view_orientation {args} {
  global view const
  global correctsize

  set view(set_orientation) 1
  set correctsize 0

  set orientation [string toupper $args]

  if {$view(orientation) != $orientation} {
    set view(orientation) $orientation
    place_layout
    place_buttons $orientation
#    actualize_canvas
  }
  update

  set correctsize 1
  set view(set_orientation) 0

  gli GKS CLOSE_WS WK1
  init_gli
  get_trafo_data
}

proc view_scale {args} {
  if {$args != "linear"} {set args "${args}_log"}
  gli_prot SIGHT SET SCALE $args CURRENT
}
proc view_transformation {args} {
  gli_prot SIGHT SELECT XFORM $args
  actualize_transformation $args
}

## customize ##

proc customize_snapgrid {args} {
  if {$args == "off"} {set args 0}
  gli_prot SIGHT SET SNAP_GRID [expr $args./100]
}
