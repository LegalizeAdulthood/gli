#
# OnlineHelp and InitShorthelp - produce Online-Help to specified widgets 
#
# OnlineHelp:
# Opens toplevel window to display help-text.
#
# Arguments:
#   widget - specifies widget to help on
#   helpfile - name of the file including help-text
#   indexfile - file containing widgetnames and their associated keywords
#   args - specifies optional arguments
#
#
# InitShorthelp:
# Generates bindings on widgets, which copy short help-information into
# a specified textvariable, if the mousepointer enters.
#
# Arguments:
#   shorthelpfile - file consisting of shorthelp-text (one line) in association
#                   with the keyword
#   indexfile - see above
#   textvariable - name of textvariable to get help-information
#
# Author:
#    G.Grimm
#


proc GetMatchingWidget {widget key} {
  set matching ""
  set widgets [winfo children $widget]
  foreach widget $widgets {
    if {[string match $key $widget]} {
      set matching [concat $matching $widget]
    } elseif {[winfo children $widget] != ""} {
      set matching [concat $matching [GetMatchingWidget $widget $key]]
    }
  }

  return $matching
}

proc InitShorthelp {shorthelpfile indexfile textvariable} {

  set textfile [open $shorthelpfile r]
  set help_text ""
  while {[gets $textfile line] != -1} {
    lappend help_text [lindex $line 0]
    lappend help_text [lrange $line 1 end]
  }
  close $textfile

  set file [open $indexfile r]
  while {[gets $file line] != -1} {
    set keyword [lindex $line 0]
    set pattern_list [lrange $line 1 end]
    set index [lsearch -exact $help_text $keyword]
    if {$index != -1} {
      set textline [lindex $help_text [expr $index+1]]
      foreach pattern $pattern_list {
        set widget_list [GetMatchingWidget . $pattern]
        foreach widget $widget_list {
          bind $widget <Enter> [concat +set $textvariable [list $textline]]
          bind $widget <Leave> [concat +set $textvariable \"\"]
        }
      }
    }
  }
  close $file
}


proc OnlineHelp {widget helpfile indexfile {{args} {}} } {

  global gli

  set option_list {title Hilfe \
               	   x -1 \
     		   y -1 \
            	   width 75 \
		   max_height 24 \
		   font -1 \
		  }

  if {[set invalid [GetOptions OnlineHelp $option_list $args]] != ""} {
    tkerror "invalid option: $invalid"
    return
  }

  set file [open $indexfile r]
  set found 0
  while {[gets $file line] != -1 && !$found} {
    set keyword [lindex $line 0]
    set pattern_list [lrange $line 1 end]
    foreach pattern $pattern_list {
      if {[string match $pattern $widget]} {
        set found 1
      }
    }
  }
  close $file

  if {$found} {
    DefineCursor {All} watch
    set file [open $helpfile r]

    set zeile 1
    set text_begin -1
    set text_end -1
    while {[gets $file line] != -1} {
      if {[string length $line] && [string index $line 0] != " "} {
        if {[lsearch -exact $line $keyword] != -1} {
          set text_begin [expr $zeile+1]
        } else {
          if {$text_begin != -1 && $text_end == -1} {
            set text_end [expr $zeile-1]
          }
        }
      }
      incr zeile 1
    }
    close $file

    if {$text_end == -1} {set text_end [expr $zeile-1]}

    set height [expr $text_end-$text_begin+1]

    set max_height $gli(OnlineHelp,option,max_height)
    if {$height > $max_height} {set height $max_height}

    if {$text_begin != -1} {
      Text $helpfile -begin $text_begin -end $text_end -grab false \
        -width $gli(OnlineHelp,option,width) -height $height \
        -title $gli(OnlineHelp,option,title) \
        -x $gli(OnlineHelp,option,x) -y $gli(OnlineHelp,option,y) \
        -font $gli(OnlineHelp,option,font)
    }
    DefineCursor {All}
  }

}
