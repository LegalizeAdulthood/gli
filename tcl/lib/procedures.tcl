#
# DefineCursor - define cursors. If a cursor is set, it will be used when
# the pointer is in the window.
#
# Arguments:
#   window - specifies a list of toplevel windows or All
#   cursor - specifies the cursor that is to be displayed
#

proc DefineCursor {window {cursor {}}} {

  if {$window == "All"} {
    set window {.}
    foreach widget [winfo children .] {
      if {[winfo class $widget] == "Toplevel"} {
        lappend window $widget
      }
    }
  }

  foreach win $window {
    if {[winfo exists $win]} {$win config -cursor $cursor}
  }
  update idletasks

  return
}

#
# ErrorBox - create a dialog box including an error report and an Ok button
#
# Arguments:
#   errno - specifies either an index into a table with error messages or
#   the message string (if starting with ?). The message strings will be
#   obtained from gli(ErrList, errno)
#

proc ErrorBox {errno} {
  global gli

  set box 0
  while {[winfo exists .error$box] == 1} {
    incr box 1
  }

  if {[string index $errno 0] != "?"} {
    set message $gli(ErrList,$errno)
  } else {
    set message [string range $errno 1 end]
  }
  tk_dialog .error$box Error $message error 0 Ok
  return
}

#
# UpdateScaler - update a scale widget
#
# Arguments:
#   widget - scale widget
#   value - specifies the current value
#   min - specifies the minimum value
#   max - specifies the maximum value
#
# Results:
#   0 - normal
#   1 - non-existing widget
#

proc UpdateScaler {widget value min max} {

  if {[winfo exists $widget]} {
    $widget config -from $min -to $max
    $widget set $value
    $widget config -tickinterval [expr $max-$min]
    return 0
  } else {
    return 1
  }
}

#
# ParseFilename - parse a UNIX file specification
#
# Arguments:
#   path - specifies the name of the file
#
# Results:
#   final file specification
#

proc ParseFilename {path} {

  set list ""

  while {[string length $path]} {
    set word ""
    while {[string index $path 0] == "/"} {
      set path [string range $path 1 end]
    }
    set index 0
    while {[string index $path $index] != "/" &&
           $index < [string length $path]} {
      incr index 1
    }
    if {$index} {
      set word [string range $path 0 [expr $index-1]]
      if {$word != "."} {
        if {$word != ".."} {
          lappend list $word
        } else {
	  set end [expr [llength $list] - 1]
          if {$list != ""} {set list [lreplace $list $end $end]}
        }
      }
      set path [string range $path $index end]
    }
  }

  set path ""
  foreach element $list {
    append path /$element
  }
  if {$list == ""} {
    set path "/"
  }
 
  return $path
}

#
# SelectionBox - create a general dialog widget including a scrolling
# list of alternatives (one per line), a Cancel button and an Ok button.
#
# Arguments:
#   list - specifies a list of strings that may be selected
#   args - specifies optional arguments
#
# Results:
#   the selected list element
#   gli(status) 1 - Ok
#              -1 - Cancel
#

proc SelectionBox {list args} {

  global gli tk_version

  set option_list {title List \
                   ok Ok \
                   cancel Cancel \
                   width 24 \
                   height 15 \
                   x -1 \
                   y -1 \
                   font -1 \
                   transient -1 \
                  }

  if {[winfo exists .selectionbox]} {return}

  if {[set invalid [GetOptions SelectionBox $option_list $args]] != ""} {
    ErrorBox "?invalid option: $invalid"
    return
  }

  toplevel .selectionbox

  set x $gli(SelectionBox,option,x)
  set y $gli(SelectionBox,option,y)
  if {$x != -1 || $y != -1} {
    if {$x == -1} {set x 400}
    if {$y == -1} {set y 300}
    wm geometry .selectionbox +$x+$y
  }
  wm title .selectionbox $gli(SelectionBox,option,title)
  if {$gli(SelectionBox,option,transient) != -1} {
    wm transient .selectionbox $gli(SelectionBox,option,transient)
  }

  if {[info exists gli(tkversion)] == 0} {
    set gli(tkversion) [string index $tk_version 0]
  }

  frame .selectionbox.f1
  frame .selectionbox.f2

  listbox .selectionbox.f1.listbox \
    -yscrollcommand {.selectionbox.f1.scrollbar set} \
    -width $gli(SelectionBox,option,width) \
    -height $gli(SelectionBox,option,height)
  if {$gli(SelectionBox,option,font) != -1} {
    .selectionbox.f1.listbox config -font $gli(SelectionBox,option,font)
  }

  if {$gli(tkversion) >= 4} {
    .selectionbox.f1.listbox config -selectmode normal
  } else {
    tk_listboxSingleSelect .selectionbox.f1.listbox
  }

  scrollbar .selectionbox.f1.scrollbar -orient vertical \
    -command {.selectionbox.f1.listbox yview}
  button .selectionbox.f2.ok -text $gli(SelectionBox,option,ok) \
    -command {set gli(SelectionBox,status) 1}
  button .selectionbox.f2.cancel -text $gli(SelectionBox,option,cancel) \
    -command {set gli(SelectionBox,status) -1}
  
  pack .selectionbox.f1.listbox .selectionbox.f1.scrollbar -fill y -side left
  pack .selectionbox.f2.ok -side left -padx 1m -pady 1m
  pack .selectionbox.f2.cancel -side right -padx 1m -pady 1m

  pack .selectionbox.f1 .selectionbox.f2 -fill x

  foreach element $list {
    .selectionbox.f1.listbox insert end $element
  }

  bind .selectionbox.f1.listbox <Double-ButtonPress-1> \
    {set gli(SelectionBox,status) 1}
  bind .selectionbox <Any-Key> {break}

  set gli(SelectionBox,status) 0

  set savedFocus [focus]
  grab .selectionbox

  tkwait variable gli(SelectionBox,status)

  set index [.selectionbox.f1.listbox curselection]
  if {$index != ""} {
    set selection [.selectionbox.f1.listbox get $index]
  } else {
    set selection ""
  }

  set gli(status) $gli(SelectionBox,status)
  update idletasks
  destroy .selectionbox

  catch {focus $savedFocus}

  return $selection
}

#
# InputBox - create a toplevel window with entry-widget, a Cancel button
#            and an Ok button.
#
# Arguments:
#   args - specifies optional arguments
#
# Results:
#   input string
#   gli(status) 1 - Ok
#              -1 - Cancel
#

proc InputBox {args} {

  global gli tk_version

  set option_list {title Entry \
                   ok Ok \
                   cancel Cancel \
                   width 30 \
                   x -1 \
                   y -1 \
                   font -1 \
                   transient -1 \
                   default " " \
                  }

  if {[winfo exists .inputbox]} {return}

  if {[set invalid [GetOptions InputBox $option_list $args]] != ""} {
    ErrorBox "?invalid option: $invalid"
    return
  }
  if {$gli(InputBox,option,default) == " "} {
    set gli(InputBox,option,default) ""
  }

  toplevel .inputbox

  set x $gli(InputBox,option,x)
  set y $gli(InputBox,option,y)
  if {$x != -1 || $y != -1} {
    if {$x == -1} {set x 400}
    if {$y == -1} {set y 300}
    wm geometry .inputbox +$x+$y
  }
  wm title .inputbox $gli(InputBox,option,title)
  if {$gli(InputBox,option,transient) != -1} {
    wm transient .inputbox $gli(InputBox,option,transient)
  }

  if {[info exists gli(tkversion)] == 0} {
    set gli(tkversion) [string index $tk_version 0]
  }

  frame .inputbox.f1
  frame .inputbox.f2

  entry .inputbox.f1.entry -textvariable gli(InputBox,input) \
                           -width $gli(InputBox,option,width)
  if {$gli(InputBox,option,font) != -1} {
    .inputbox.f1.entry config -font $gli(InputBox,option,font)
  }

  button .inputbox.f2.ok -text $gli(InputBox,option,ok) \
    -command {set gli(InputBox,status) 1}
  button .inputbox.f2.cancel -text $gli(InputBox,option,cancel) \
    -command {set gli(InputBox,status) -1}
  
  pack .inputbox.f1.entry -fill y -padx 1m -pady 1m
  pack .inputbox.f2.ok -side left -padx 1m -pady 1m
  pack .inputbox.f2.cancel -side right -padx 1m -pady 1m

  pack .inputbox.f1 .inputbox.f2 -fill x

  bind .inputbox.f1.entry <Return> {.inputbox.f2.ok invoke}
  bind .inputbox <Any-Key> {break}

  set gli(InputBox,input) $gli(InputBox,option,default)

  set gli(InputBox,status) 0

  update
  .inputbox.f1.entry icursor end

  set savedFocus [focus]
  grab .inputbox
  focus .inputbox.f1.entry

  tkwait variable gli(InputBox,status)

  set gli(status) $gli(InputBox,status)
  update idletasks
  destroy .inputbox

  catch {focus $savedFocus}

  return $gli(InputBox,input)
}


#
# InformationBox - create a dialog box including a message and an Ok button
#
# Arguments:
#   list - specifies a list of strings that is to be displayed (one per line)
#   args - specifies optional arguments
#

proc InformationBox {list args} {

  global gli

  set option_list {title Information \
                   x -1 \
                   y -1 \
                   width -1 \
                   height -1 \
                   ok Ok \
                   font -1 \
                   transient -1 \
                  }

  if {[winfo exists .informationbox]} {return}

  if {[set invalid [GetOptions InformationBox $option_list $args]] != ""} {
    ErrorBox "?invalid option: $invalid"
    return
  }

  toplevel .informationbox

  wm title .informationbox $gli(InformationBox,option,title)
  set x $gli(InformationBox,option,x)
  set y $gli(InformationBox,option,y)
  if {$x != -1 || $y != -1} {
    if {$x == -1} {set x 400}
    if {$y == -1} {set y 300}
    wm geometry .informationbox +$x+$y
  }

  set index 0
  foreach element $list {
    label .informationbox.label$index -text $element
    if {$gli(InformationBox,option,font) != -1} {
      .informationbox.label$index config \
        -font $gli(InformationBox,option,font)
    }
    pack .informationbox.label$index -padx 2m
    incr index 1
  }

  button .informationbox.ok -text $gli(InformationBox,option,ok) \
    -command {set gli(InformationBox,status) 1}
  pack .informationbox.ok -pady 2m

  set width $gli(InformationBox,option,width)
  set height $gli(InformationBox,option,height)
  if {$width != -1 || $height != -1} {
    wm withdraw .informationbox
    update idletasks
    if {$width == -1} {
      set width [winfo reqwidth .informationbox]
    }
    if {$height == -1} {
      set height [winfo reqheight .informationbox]
    }
    wm geometry .informationbox $width\x$height
  }

  wm deiconify .informationbox
  if {$gli(InformationBox,option,transient) != -1} {
    wm transient .informationbox $gli(InformationBox,option,transient)
  }

  bind .informationbox <Any-Key> {break}
  bind .informationbox <Return> {.informationbox.ok invoke}

  set gli(InformationBox,status) 0

  set savedFocus [focus]
  focus .informationbox
  grab .informationbox

  tkwait variable gli(InformationBox,status)

  set gli(status) $gli(InformationBox,status)
  update idletasks
  destroy .informationbox

  catch {focus $savedFocus}

  return
}

#
# GetOptions - parse a list of options
#

proc GetOptions {class option_defaults {argv {}}} {
  global gli

  for {set i 0} {$i < [llength $option_defaults]} {incr i 2} {
    set option [lindex $option_defaults $i]
    set default [lindex $option_defaults [expr $i+1]]

    if {$default == ""} {
      set default false
      set gli($class,option,$option,flag) 1
    } else {
      set gli($class,option,$option,flag) 0
    }

    set gli($class,option,$option,set) 0
    set gli($class,option,$option) $default
    lappend options $option
    lappend defaults $default
  }

  set i 0
  while {$i < [llength $argv]} {
    set option [lindex $argv $i]
    if {[string index $option 0] != "-"} {
      return $option
    } else {
      set option [string range $option 1 end]
    }
    if {[set index [lsearch -glob $options $option*]] == -1} { 
      return -$option
    } else {
      set option [lindex $options $index]
      if {$gli($class,option,$option,flag)} {
        set gli($class,option,$option) true
      } else {
        incr i 1
        if {$i < [llength $argv]} {
          set gli($class,option,$option) [lindex $argv $i]
        } else {
          set gli($class,option,$option) ""
        }
        set gli($class,option,$option,set) 1
      }
    }

    incr i 1
  }

  return
}

#
# Text - create a text widget with two scrollbars
#
# Arguments:
#   filename - specifies the name of the file to be opened
#   args - specifies optional arguments
#

proc Text {filename args} {

  global gli

  set option_list "title $filename \
                   width 80 \
                   height 24 \
                   x -1 \
                   y -1 \
                   font -1 \
                   transient -1 \
                   see 0 \
                   begin 1 \
                   end -1 \
                   grab true \
                  "

  if {[set invalid [GetOptions Text $option_list $args]] != ""} {
    ErrorBox "?invalid option: $invalid"
    return
  }

  if {[winfo exists .text]} {
    return
  }

  if {![file exists $filename]} {
    update idletasks
    ErrorBox "?File not found: $filename"
    return
  }
  set gli(Text,filename) $filename

  toplevel .text 
  if {$gli(Text,option,x) != -1 || $gli(Text,option,y) != -1} {
    set x $gli(Text,option,x)
    set y $gli(Text,option,y)
    if {$x == -1} {set x 300}
    if {$y == -1} {set y 250}
    wm geometry .text +$x+$y
  }

  if {$gli(Text,option,transient) != -1} {
    wm transient .text $gli(Text,option,transient)
  }
  wm title .text $gli(Text,option,title)

  frame .text.y
  frame .text.x
  frame .text.buttons

  text .text.y.text -xscrollcommand {.text.x.scroll set} \
    -yscrollcommand {.text.y.scroll set} -wrap none \
    -width $gli(Text,option,width) -height $gli(Text,option,height)

  scrollbar .text.y.scroll -orient vertical \
    -command {.text.y.text yview}
  scrollbar .text.x.scroll -orient horizontal -command {.text.y.text xview}

  button .text.buttons.print -text Print -command {
    set range [expr $gli(Text,option,end) - $gli(Text,option,begin) + 1]
    exec head -$gli(Text,option,end) $gli(Text,filename) | tail -$range |\
          lpr
  }
  button .text.buttons.quit -text Quit -command {
    set gli(Text,status) 1
    if {$gli(Text,option,grab) != "true"} {ExitAppl .text Text}
  }

  pack .text.y .text.x .text.buttons -side top -fill x

  pack .text.y.text .text.y.scroll -side left -fill y
  pack .text.x.scroll -fill x
  pack .text.buttons.print -side left
  pack .text.buttons.quit -side right


  set stream [open $filename r]
  set begin $gli(Text,option,begin)
  set end $gli(Text,option,end)
  set zeile 1
  while {[gets $stream line] >= 0} {
    if {($zeile >= $begin) && ($end == -1 || $zeile <= $end)} {
      .text.y.text insert insert "$line\n"
    }
    incr zeile 1
  }
  close $stream
  if {$end == -1} {set gli(Text,option,end) [expr $zeile-1]}
  .text.y.text see $gli(Text,option,see).0


  bind .text <Any-Key> {break}

  bind .text <Up> {.text.y.text yview scroll -1 units; break}
  bind .text <Down> {.text.y.text yview scroll 1 units; break}
  bind .text <Shift-Up> {.text.y.text yview scroll -1 pages; break}
  bind .text <Shift-Down> {.text.y.text yview scroll 1 pages; break}
  bind .text <Prior> {.text.y.text yview scroll -1 pages; break}
  bind .text <Next> {.text.y.text yview scroll 1 pages; break}
  bind .text <Left> {.text.y.text xview scroll -1 units; break}
  bind .text <Right> {.text.y.text xview scroll 1 units; break}
  bind .text <Shift-Left> {.text.y.text xview scroll -1 pages; break}
  bind .text <Shift-Right> {.text.y.text xview scroll 1 pages; break}
  bind .text <Alt-Up> {.text.y.text see 1.0; break}
  bind .text <Alt-Down> {.text.y.text see end; break}
  bind .text <Help> {
    set x [winfo x .text]
    set y [winfo y .text]
    InformationBox "{Cursor Up - 1 line up}
                    {Cursor Down - 1 line down}
                    {Shift-Cursor Up / Prev - 1 page up}
                    {Shift-Cursor Down / Next - 1 page down}
                    {Cursor Left - 1 character left}
                    {Cursor Right - 1 character right}
                    {Shift-Cursor Left - 1 page left}
                    {Shift-Cursor Right - 1 page right}
                    {Alt-Cursor Up - go to top of text}
                    {Alt-Cursor Down - go to bottom of text}" \
                   -x [expr $x+150] -y [expr $y+100] -title "Text Help"
    break
  }

  if {$gli(Text,option,font) != -1} {
    if {[catch "label .text.test -font $gli(Text,option,font)"]} {
      update idletasks
      ErrorBox "?Font $gli(Text,option,font) doesnt exist"
    } else {
      .text.y.text config -font $gli(Text,option,font)
    }
    catch {destroy .text.test}
  }
  .text.y.text config -state disabled

  update idletasks
  set gli(Text,status) 0

  if {$gli(Text,option,grab) == "true"} {
    set savedFocus [focus]
    grab .text

    tkwait variable gli(Text,status)

    ExitAppl .text Text

    catch {focus $savedFocus}
  }

  return
}

proc ExitAppl {widget appl_name} {
  global gli

  set gli(status) $gli($appl_name,status)
  
  update idletasks
  destroy $widget
}
