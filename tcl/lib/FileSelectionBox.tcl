proc FileSelectionBox {args} {
  global gli

  set types {
    {"All Files"	*		}
    {"Sight Files"	{.sight}	}
    {"Data Files"	{.dat}		}
    {"CGM Files"	{.cgm}		}
  }
  set option_list {title "File Selection" \
                  filters 0 \
                  action "open" \
                  ok Ok \
                  cancel Cancel \
                  x -1 \
                  y -1 \
                  checkfile "" \
                  transient -1 \
                  }

  if {[set invalid [GetOptions FileSelectionBox $option_list $args]] != ""} {
    tkerror "Invalid option: $invalid"
    return
  }

  set filetypes ""
  foreach filter $gli(FileSelectionBox,option,filters) {
    lappend filetypes [lindex $types $filter]
  }
  if {[set action [string tolower $gli(FileSelectionBox,option,action)]] \
                                                                 == "save"} {
    set command tk_getSaveFile
  } else {
    set command tk_getOpenFile
  }
  if {[info exists gli(filename)]} {
    set filename $gli(filename)
  } else {
    set filename ""
  }
  if {[info exists gli(dir)]} {
    set filename [$command -filetypes $filetypes -initialdir $gli(dir) \
                           -defaultextension $gli(filter) \
                           -title $gli(FileSelectionBox,option,title) \
                           -initialfile $filename]
  } else {
    set filename [$command -filetypes $filetypes \
                           -defaultextension $gli(filter) \
                           -title $gli(FileSelectionBox,option,title) \
                           -initialfile $filename]
  }

  if {$filename != ""} {
    set gli(dir) [file dirname $filename]
    set gli(filename) [file tail $filename]
    set gli(status) 1
  } else {
    set gli(status) -1
  }
  return 0
}
