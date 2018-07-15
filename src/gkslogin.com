$! Graphical Kernel System (GKS) system-wide login procedure
$
$  IF .NOT. F$GETDVI("SYS$COMMAND","TT_ANSICRT") THEN EXIT
$  DEV = F$EXTRACT(0,3,F$GETDVI("SYS$COMMAND","DEVNAM"))
$  
$VWS:
$  WSTYPE = "41"
$  IF DEV .EQS. "_WT" THEN GOTO DONE
$
$DECW:
$  WSTYPE = "211"
$  IF DEV .EQS. "_TW" .OR. DEV .EQS. "_FT" THEN GOTO DONE
$  IF DEV .EQS. "_RT" .AND. F$TRNLNM("SYS$REM_NODE") .NES. "" THEN GOTO DONE
$  
$OTHER:
$  ANSWERBACK == ""
$  LIST_TYPES = "5 7 8 16 17 38 41 51 53 61 62 72 82 201 204 207 210 211"

$  ON CONTROL_Y THEN GOTO INTERRUPT
$  SET TERMINAL /NOECHO
$  READ /PROMPT="" /TIME_OUT=3 /ERROR=ERROR  SYS$COMMAND  ANSWERBACK
$  GOSUB RESET_TERMINAL
$
$  IF ANSWERBACK .EQS. "" THEN GOTO ERROR
$  GOSUB CHECK
$  IF WSTYPE .NES. " " THEN GOTO DONE
$  WRITE SYS$OUTPUT ""
$
$INQUIRE:
$  READ /PROMPT= -
	"Enter GKS workstation type or CTRL/Z (Exit or F10) to continue: " -
        /END_OF_FILE=EXIT  SYS$COMMAND  ANSWERBACK
$  IF ANSWERBACK .EQS. "" THEN GOTO SHOW
$  GOSUB CHECK
$  IF WSTYPE .NES. " " THEN GOTO DONE
$  
$RETRY:
$  WRITE SYS$OUTPUT "Invalid GKS workstation type specification"
$
$SHOW:
$  TYPE SYS$INPUT

Please enter one of the following workstation types:

    5   Workstation independent    61    PostScript printer (black
        segment storage                  and white)
    7   CGM binary                 62    Color PostScript printer
    8   CGM clear text             72    TEK4014
   16   VT330 (black and white)    82    TEK4107, Monterey MX series
   17   VT340 with color           201   TAB 132/15-G
   38   LN03 PLUS                  204   Monterey MG200
   41   UIS window system          207   IBM Personal Computer
   51   HP7475 pen plotter         210   X Display (output only)
   53   HP7550 pen plotter         211   X Display
   
$
$  GOTO INQUIRE
$
$DONE:
$  ASSIGN /NOLOG  SYS$COMMAND  WK01
$  ASSIGN /NOLOG  "''WSTYPE'"  GKS$WSTYPE
$  ASSIGN /NOLOG  "''WSTYPE'"  GKS3D$WSTYPE
$  ASSIGN /NOLOG  "''WSTYPE'"  GLI_WSTYPE
$EXIT:
$  EXIT
$
$ERROR:
$  GOSUB RESET_TERMINAL
$  WRITE SYS$OUTPUT ""
$  WRITE SYS$OUTPUT "Failed to request GKS workstation type"
$  GOTO INQUIRE
$
$CHECK:
$  ANSWERBACK = F$EDIT(ANSWERBACK,"TRIM, UPCASE")
$  NUM = 0
$LOOP:
$  WSTYPE = F$ELEMENT(NUM," ",LIST_TYPES)
$  IF WSTYPE .EQS. " " THEN RETURN
$  IF WSTYPE .EQS. ANSWERBACK THEN RETURN
$  NUM = NUM+1
$  GOTO LOOP
$
$INTERRUPT:
$  GOSUB RESET_TERMINAL
$  EXIT
$
$RESET_TERMINAL:
$  READ /PROMPT="" /TIME_OUT=0 /ERROR=CONTINUE  SYS$COMMAND  DUMMY
$CONTINUE:
$  SET TERMINAL /ECHO
$  RETURN
