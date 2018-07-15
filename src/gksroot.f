C*
C* Copyright @ 1984 - 1994   Josef Heinen
C*
C* Permission to use, copy, and distribute this software and its
C* documentation for any purpose with or without fee is hereby granted,
C* provided that the above copyright notice appear in all copies and
C* that both that copyright notice and this permission notice appear
C* in supporting documentation.
C*
C* Permission to modify the software is granted, but not the right to
C* distribute the modified code.  Modifications are to be distributed
C* as patches to released version.
C*
C* This software is provided "as is" without express or implied warranty.
C*
C* Send your comments or suggestions to
C*  J.Heinen@kfa-juelich.de.
C*
C*

        SUBROUTINE GSNT (TNR,WN,VP)
C               set up normalization transformation

        INTEGER TNR
        REAL WN(4),VP(4),TRAN(2,3)
        REAL X, Y

C               include GKS symbol definitions
        INCLUDE 'gksdefs.i'
C               include GKS description table
        INCLUDE 'gksdescr.i'

        REAL A(0:MXNTNR),B(0:MXNTNR),C(0:MXNTNR),D(0:MXNTNR)
        INTEGER I, J
        REAL MAT(2,3), XRES, YRES

        SAVE A,B,C,D,MAT

        A(TNR) = (VP(2)-VP(1))/(WN(2)-WN(1))
        B(TNR) = VP(1)-WN(1)*A(TNR)
        C(TNR) = (VP(4)-VP(3))/(WN(4)-WN(3))
        D(TNR) = VP(3)-WN(3)*C(TNR)

        RETURN


        ENTRY GNT (X,Y,TNR)
C               normalization transformation

        IF (TNR .GT. 0) THEN
            X = A(TNR)*X+B(TNR)
            Y = C(TNR)*Y+D(TNR)
        END IF

        RETURN


        ENTRY GDNT (X,Y,TNR)
C               denormalization transformation

        X = (X-B(TNR))/A(TNR)
        Y = (Y-D(TNR))/C(TNR)

        RETURN


        ENTRY GCNT (X,Y,TNR)
C               character normalization transformation

        IF (TNR .GT. 0) THEN
            X = A(TNR)*X
            Y = C(TNR)*Y
        END IF

        RETURN


        ENTRY GSST (TRAN)
C               set up segment transformation

        DO 1, I=1,2
            DO 2, J=1,3
                MAT(I,J) = TRAN(I,J)
   2        CONTINUE
   1    CONTINUE

        RETURN


        ENTRY GST (X,Y)
C               segment transformation

        XRES = X*MAT(1,1) + Y*MAT(2,1) + MAT(1,3)
        YRES = X*MAT(1,2) + Y*MAT(2,2) + MAT(2,3)
        X = XRES
        Y = YRES

        RETURN


        ENTRY GCST (X,Y)
C               character segment transformation

        XRES = X*MAT(1,1) + Y*MAT(2,1)
        YRES = X*MAT(1,2) + Y*MAT(2,2)
        X = XRES
        Y = YRES

        RETURN
        END


        SUBROUTINE GSDT (WN,VP)
C               set device transformation

        REAL WN(4), VP(4)
        REAL CURWN(4), CURVP(4)

        INTEGER I
        REAL X, X1, X2, XMAX, XMIN
        REAL Y, Y1, Y2, YMAX, YMIN

        INTEGER EMPTY,LEFT,RIGHT,BOTTOM,TOP
        PARAMETER (EMPTY=0,LEFT=1,RIGHT=2,BOTTOM=4,TOP=8)

        INTEGER ERRIND, CLSW, GR_BC
        REAL CLRT(4)

        REAL CXL, CXR, CYB, CYT
C       REAL H
        REAL WINDOW(4), VIEWPT(4)

        LOGICAL VISIBL
        INTEGER S,S1,S2,GCODE

        COMMON /GKSOPT/ GR_BC
CDEC$ PSECT /GKSOPT/ NOSHR

        SAVE WINDOW,VIEWPT,CXL,CXR,CYB,CYT

C               include GKS symbol definitions
        INCLUDE 'gksdefs.i'

        DO 1 I = 1,4
          WINDOW(I) = WN(I)
          VIEWPT(I) = VP(I)
   1    CONTINUE

C               inquire clipping indicator
        CALL GQCLIP (ERRIND,CLSW,CLRT)

        IF (CLSW .EQ. GCLIP) THEN
          CXL = MAX(CLRT(1),WINDOW(1))
          CXR = MIN(CLRT(2),WINDOW(2))
          CYB = MAX(CLRT(3),WINDOW(3))
          CYT = MIN(CLRT(4),WINDOW(4))
        ELSE
          CXL = WINDOW(1)
          CXR = WINDOW(2)
          CYB = WINDOW(3)
          CYT = WINDOW(4)
        END IF

C               apply segment transformation
        IF (GR_BC .EQ. 0) THEN
          CALL GST (CXL,CYB)
          CALL GST (CXR,CYT)
        END IF
        IF (CXL .GT. CXR) THEN
          H = CXL
          CXL = CXR
          CXR = H
        END IF
        IF (CYB .GT. CYT) THEN
          H = CYB
          CYB = CYT
          CYT = H
        END IF

        RETURN


        ENTRY GQDT (CURWN, CURVP)
C               inquire device transformation

        DO 2 I = 1,4
          CURWN(I) = WINDOW(I)
          CURVP(I) = VIEWPT(I)
   2    CONTINUE

        RETURN


        ENTRY GQCLRG (XMIN,XMAX,YMIN,YMAX)
C               inquire clipping region

        XMIN = CXL
        XMAX = CXR
        YMIN = CYB
        YMAX = CYT

        RETURN


        ENTRY GLCLIP (X1,Y1,X2,Y2,VISIBL)
C               line clipping algorithm

        S1 = GCODE(X1,Y1,CXL,CXR,CYB,CYT)
        S2 = GCODE(X2,Y2,CXL,CXR,CYB,CYT)
        VISIBL = .TRUE.

        DO 3 I = 0,3

          IF (S1 .EQ. EMPTY .AND. S2 .EQ. EMPTY) RETURN

          VISIBL = IAND(S1,S2) .EQ. EMPTY
          IF (.NOT. VISIBL) RETURN

          S = S1
          IF (S .EQ. EMPTY) S = S2

          IF (IAND(LEFT,S) .NE. EMPTY) THEN
            Y = Y1+(Y2-Y1)*(CXL-X1)/(X2-X1)
            X = CXL
          ELSE
            IF (IAND(RIGHT,S) .NE. EMPTY) THEN
              Y = Y1+(Y2-Y1)*(CXR-X1)/(X2-X1)
              X = CXR
            ELSE
              IF (IAND(BOTTOM,S) .NE. EMPTY) THEN
                X = X1+(X2-X1)*(CYB-Y1)/(Y2-Y1)
                Y = CYB
              ELSE
                IF (IAND(TOP,S) .NE. EMPTY) THEN
                  X = X1+(X2-X1)*(CYT-Y1)/(Y2-Y1)
                  Y = CYT
                END IF
              END IF
            END IF
          END IF

          IF (S .EQ. S1) THEN
            X1 = X
            Y1 = Y
            S1 = GCODE(X,Y,CXL,CXR,CYB,CYT)
          ELSE
            X2 = X
            Y2 = Y
            S2 = GCODE(X,Y,CXL,CXR,CYB,CYT)
          END IF

   3    CONTINUE

        RETURN
        END


        INTEGER FUNCTION GCODE(X,Y,CXL,CXR,CYB,CYT)

        REAL CXL, CXR, CYB, CYT, X, Y
        INTEGER LEFT,RIGHT,BOTTOM,TOP
        PARAMETER (LEFT=1,RIGHT=2,BOTTOM=4,TOP=8)

        GCODE = 0
        IF (X .LT. CXL) THEN
          GCODE = LEFT
        ELSE
          IF (X .GT. CXR) GCODE = RIGHT
        END IF
        IF (Y .LT. CYB) THEN
          GCODE = GCODE+BOTTOM
        ELSE
          IF (Y .GT. CYT) GCODE = GCODE+TOP
        END IF

        RETURN
        END


        SUBROUTINE GPOLIN (N,PX,PY,LTYPE,TNR,MOVE,DRAW)
C               polyline utility

        REAL FEPS
        PARAMETER (FEPS = 1.0E-06)

        INTEGER I, LTYPE, N, TNR
        REAL VX, VY, X, XOLD, Y, YOLD 
        REAL PX(N),PY(N)
        EXTERNAL MOVE,DRAW

        LOGICAL CLIP,VISIBL
        REAL CXL,CXR,CYB,CYT

C               include GKS symbol definitions
        INCLUDE 'gksdefs.i'

        CALL GSDASH (LTYPE)

        IF (LTYPE .NE. 0) THEN

            XOLD = PX(1)
            YOLD = PY(1)
            CALL GNT (XOLD,YOLD,TNR)
            CALL GST (XOLD,YOLD)
            CLIP = .TRUE.

C               loop to output line
            DO 1 I = 2,N
              X = PX(I)
              Y = PY(I)
              CALL GNT (X,Y,TNR)
              CALL GST (X,Y)
C                   save virtual position
              VX = X
              VY = Y
              CALL GLCLIP (XOLD,YOLD,X,Y,VISIBL)

C                   be sure that line is visible
              IF (VISIBL) THEN
                IF (CLIP) THEN
C                   re-initialize polyline sequence
                  CALL MOVE (XOLD,YOLD)
                  CLIP = .FALSE.

                END IF
                CALL DRAW (X,Y)
              END IF

              CLIP = .NOT.VISIBL .OR.
     *            (ABS(X-VX) .GT. FEPS) .OR. (ABS(Y-VY) .GT. FEPS) 
              XOLD = VX
              YOLD = VY

   1        CONTINUE

        ELSE

C               inquire clipping rectangle
            CALL GQCLRG (CXL,CXR,CYB,CYT)

            XOLD = PX(N)
            YOLD = PY(N)
            CALL GNT (XOLD,YOLD,TNR)
            CALL GST (XOLD,YOLD)
            CLIP = .TRUE.

C               loop to output polygon
            DO 2 I = 1,N
              X = PX(I)
              Y = PY(I)
              CALL GNT (X,Y,TNR)
              CALL GST (X,Y)
C                   save virtual position
              VX = X
              VY = Y
              CALL GLCLIP (XOLD,YOLD,X,Y,VISIBL)

              IF (CLIP) THEN
                XOLD = MIN(MAX(XOLD,CXL),CXR)
                YOLD = MIN(MAX(YOLD,CYB),CYT)

                IF (I .EQ. 1) THEN
C                 re-initialize polygon sequence
                  CALL MOVE (XOLD,YOLD)
                ELSE
                  CALL DRAW (XOLD,YOLD)
                END IF
                CLIP = .FALSE.

              END IF
              X = MIN(MAX(X,CXL),CXR)
              Y = MIN(MAX(Y,CYB),CYT)
              CALL DRAW (X,Y)

              CLIP = .NOT.VISIBL .OR.
     *            (ABS(X-VX) .GT. FEPS) .OR. (ABS(Y-VY) .GT. FEPS) 
              XOLD = VX
              YOLD = VY

   2        CONTINUE

        END IF

        RETURN
        END


        SUBROUTINE GMOVE (X,Y,MOVE)
C               dashed line generator

        REAL X, Y

        EXTERNAL MOVE

        CALL GMOVTO (X,Y)
        CALL MOVE (X,Y)

        RETURN
        END


        SUBROUTINE GDASH (X,Y,MOVE,DRAW)
C               draw a dashed line

        REAL FEPS
        PARAMETER (FEPS = 1.0E-06)

        REAL X, Y
        EXTERNAL MOVE, DRAW
        
        REAL DIAG, DIST, DX, DY, RX, RY, SEGLEN
        REAL DASH, XD, YD
        INTEGER IDASH, ITEMPA, LEN, LTYPE
        LOGICAL NEWSEG, DARK

C               dash table
        INTEGER DTABLE(0:9,-30:4)

C               include GKS symbol definitions
        INCLUDE 'gksdefs.i'

        COMMON /GKSLIN/ LTYPE, RX, RY, SEGLEN, NEWSEG
CDEC$ PSECT /GKSLIN/ NOSHR
        
        SAVE
C
C       -8 = triple_dots    -4 = long-short-dash    1 = solid
C       -7 = double_dots    -3 = long dash          2 = dashed
C       -6 = spaced_dot     -2 = dash-3-dots        3 = dotted
C       -5 = spaced-dash    -1 = dash-2-dots        4 = dash-dotted
C
        DATA DTABLE /
     *  8,  4, 2, 4, 2, 4, 2, 4, 6, 0,  6,  4, 2, 4, 2, 4, 6, 0, 0, 0,
     *  4,  4, 2, 4, 6, 0, 0, 0, 0, 0,  8,  3, 2, 3, 2, 3, 2, 3, 6, 0,
     *  6,  3, 2, 3, 2, 3, 6, 0, 0, 0,  4,  3, 2, 3, 6, 0, 0, 0, 0, 0,
     *  8,  3, 2, 3, 2, 3, 2, 3, 4, 0,  6,  3, 2, 3, 2, 3, 4, 0, 0, 0,
     *  4,  3, 2, 3, 4, 0, 0, 0, 0, 0,  2,  1, 1, 0, 0, 0, 0, 0, 0, 0,
     *  2,  1, 2, 0, 0, 0, 0, 0, 0, 0,  2,  1, 6, 0, 0, 0, 0, 0, 0, 0,
     *  2,  1, 8, 0, 0, 0, 0, 0, 0, 0,  6,  1, 3, 1, 3, 1, 6, 0, 0, 0,
     *  4,  1, 3, 1, 6, 0, 0, 0, 0, 0,  8,  6, 2, 1, 2, 1, 2, 1, 2, 0,
     *  6,  6, 2, 1, 2, 1, 2, 0, 0, 0,  4,  6, 2, 1, 2, 0, 0, 0, 0, 0,
     *  4,  9, 3, 5, 3, 0, 0, 0, 0, 0,  2,  9, 3, 0, 0, 0, 0, 0, 0, 0,
     *  2,  5, 5, 0, 0, 0, 0, 0, 0, 0,  2,  5, 3, 0, 0, 0, 0, 0, 0, 0,
     *  6,  1, 4, 1, 4, 1, 8, 0, 0, 0,  4,  1, 4, 1, 8, 0, 0, 0, 0, 0,
     *  2,  1, 1, 0, 0, 0, 0, 0, 0, 0,  2,  8, 1, 0, 0, 0, 0, 0, 0, 0,
     *  4, 16, 5, 8, 5, 0, 0, 0, 0, 0,  2, 16, 5, 0, 0, 0, 0, 0, 0, 0,
     *  8,  8, 4, 1, 4, 1, 4, 1, 4, 0,  6,  8, 4, 1, 4, 1, 4, 0, 0, 0,
     *  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,
     *  2,  8, 5, 0, 0, 0, 0, 0, 0, 0,  2,  1, 2, 0, 0, 0, 0, 0, 0, 0,
     *  4,  8, 4, 1, 4, 0, 0, 0, 0, 0 /

        IF (LTYPE .EQ. GLSOLI .OR. LTYPE .EQ. 0) THEN
C               draw a solid line
          CALL DRAW (X,Y)

        ELSE

C               draw a dashed line
          LEN = DTABLE(0,LTYPE)

C               initialize variables
          XD = RX
          YD = RY
          DX = X-XD
          DY = Y-YD
          DIST = SQRT(DX*DX+DY*DY)

          IF (DIST .GT. 0.) THEN

            DIAG = DIST
            IF (.NOT. NEWSEG) GO TO 2
            IDASH = LEN

C               loop to output line
   1        IDASH = MOD(IDASH,LEN)+1
   2        ITEMPA = IDASH
            DASH = 0.0025*DTABLE(ITEMPA,LTYPE)
            IF (ABS(SEGLEN) .LE. FEPS) SEGLEN = DASH
            XD = XD+DX*SEGLEN/DIAG
            YD = YD+DY*SEGLEN/DIAG
            DARK = MOD(ITEMPA,2) .EQ. 1
            NEWSEG = SEGLEN .LT. DIST

C                       be sure that line segment will not overshoot point
            IF (NEWSEG) THEN
              RX = XD
              RY = YD
C                       see if line is a dark vector
              IF (DARK) THEN
                CALL DRAW (XD,YD)
              ELSE
                CALL MOVE (XD,YD)
              END IF

C               calculate remaining distance to point
              DIST = DIST-SEGLEN
              SEGLEN = 0.
              GO TO 1

            ELSE

C               output remainder of line
              RX = X
              RY = Y
C               calculate segment fragment unused
              SEGLEN = SEGLEN-DIST
              IF (DARK .OR. (ABS(SEGLEN) .LE. FEPS)) CALL DRAW (X,Y)

            END IF
          END IF
        END IF

        RETURN
        END

        
        SUBROUTINE GSDASH (TYPE)
C               select a dash pattern

        INTEGER TYPE
        REAL X, Y

        REAL RX, RY, SEGLEN
        LOGICAL NEWSEG
        
        COMMON /GKSLIN/ LTYPE, RX, RY, SEGLEN, NEWSEG
CDEC$ PSECT /GKSLIN/ NOSHR
        
        SAVE

        LTYPE = TYPE

        RETURN


        ENTRY GMOVTO (X,Y)
C               re-initialize dashed line sequence

        RX = X
        RY = Y
        SEGLEN = 0.
        NEWSEG = .TRUE.

        RETURN
        END


        SUBROUTINE GPOLMK (N,PX,PY,POLIN)
C               polymarker

        INTEGER I, N
        REAL PX(N),PY(N)
        EXTERNAL POLIN

C               include GKS symbol definitions
        INCLUDE 'gksdefs.i'

        INTEGER ERRIND,TNR,MTYPE,FONT
        REAL X(2),Y(2)

        CHARACTER*1 SYMBOL(-20:5)

        DATA FONT /-22/
        DATA SYMBOL /'*','@','7','5','B','8','.','$','-','"',
     *               '#','%','&','3','+','6',')','4',',','2',
     *               ' ','I','/','1','*','0'/

C               inquire current normalization transformation number
        CALL GQCNTN (ERRIND,TNR)
C               inquire marker type
        CALL GQMK (ERRIND,MTYPE)

        DO 1 I = 1,N
C               loop to output marker symbols
          X(1) = PX(I)
          Y(1) = PY(I)

          CALL GNT (X(1),Y(1),TNR)

          IF (MTYPE .EQ. GPOINT) THEN
C               draw point
            X(2) = X(1)
            Y(2) = Y(1)
            CALL POLIN (2,X,Y,GLSOLI,0)
          ELSE
C               draw marker symbol - switch to the software character
C               generator
            CALL GCHGEN (X(1),Y(1),SYMBOL(MTYPE),FONT,POLIN,POLIN,
     *                   .TRUE.)
          END IF

   1    CONTINUE

        RETURN
        END


        SUBROUTINE GSIMPM (N,PX,PY,MARKER)
C               polymarker simulation routine

        INTEGER I, N
        REAL PX(N),PY(N)
        EXTERNAL MARKER

        INTEGER ERRIND,TNR,MTYPE
        REAL XN,YN,CXL,CXR,CYB,CYT

C               include GKS symbol definitions
        INCLUDE 'gksdefs.i'

C               inquire current normalization transformation number
C               and current clipping region
        CALL GQCNTN (ERRIND,TNR)
        CALL GQCLRG (CXL,CXR,CYB,CYT)

C               inquire marker type
        CALL GQMK (ERRIND,MTYPE)

        DO 1 I = 1,N
C               loop to output marker symbols
          XN = PX(I)
          YN = PY(I)
          CALL GNT (XN,YN,TNR)
          CALL GST (XN,YN)

          IF (CXL.LE.XN .AND. XN .LE.CXR .AND.
     *        CYB.LE.YN .AND. YN .LE.CYT)
     *      CALL MARKER (XN,YN,MTYPE)

   1    CONTINUE

        RETURN
        END


        SUBROUTINE GSIMTX (PX,PY,NCHARS,CHARS,POLIN,FILLA)
C               text simulation routine

        REAL PX, PY
        INTEGER NCHARS
        CHARACTER CHARS*(*)
        EXTERNAL POLIN,FILLA

C               include GKS symbol definitions
        INCLUDE 'gksdefs.i'

        INTEGER IPOS
        INTEGER ERRIND,TNR,FONT,PREC,ALH,ALV,PATH
        INTEGER TXX,TXX1,SIZE,BOTTOM,BASE,CAP,TOP,WIDTH,SPACE

        REAL AX, AY, CHSP
        REAL X0, XN, XNEW, XSPACE
        REAL Y0, YN, YNEW, YSPACE
        REAL AFAC(GAHNOR:GARITE)
        INTEGER XFAC(GRIGHT:GDOWN),YFAC(GRIGHT:GDOWN)

        DATA AFAC /0.,0.,-0.5,-1./
        DATA XFAC /1,-1,0,0/ , YFAC /0,0,1,-1/

C               inquire current normalization transformation number
        CALL GQCNTN (ERRIND,TNR)

C               calculate text origin
        X0 = PX
        Y0 = PY
        CALL GNT (X0,Y0,TNR)

C               inquire text font and precision
        CALL GQTXFP (ERRIND,FONT,PREC)
        IF (PREC .NE. GSTRKP) CALL GMAPF (FONT)

C               draw software character - set up character
C               transformation
        CALL GSCT
C             compute text extent
        CALL GTXE (CHARS(1:NCHARS),FONT,PREC,TXX,SIZE,
     *             BOTTOM,BASE,CAP,TOP)

C             apply character spacing
        CALL GQCHSP (ERRIND,CHSP)
        SPACE = NINT(CHSP*SIZE)
        TXX = TXX+NCHARS*SPACE

C             set up x and y increments to align the text
        CALL GQTXAL (ERRIND,ALH,ALV)
        AX = 0
        IF (ALH.NE.GAHNOR) AX = AFAC(ALH)*TXX
        AY = 0
        IF (ALV.EQ.GABOTT) AY = BASE-BOTTOM
        IF (ALV.EQ.GAHALF) AY = -(CAP-BASE)*0.5
        IF (ALV.EQ.GACAP) AY = BASE-CAP
        IF (ALV.EQ.GATOP) AY = BASE-TOP

C             fit x increment to the text path
        CALL GQTXP (ERRIND,PATH)
        IF (PATH .EQ. GLEFT) THEN
          CALL GTXE (CHARS(1:1),FONT,PREC,TXX1,SIZE,
     *               BOTTOM,BASE,CAP,TOP)
          AX = -AX-TXX1
        ELSE IF (PATH .EQ. GUP .OR. PATH .EQ. GDOWN) THEN
          AX = AFAC(ALH)*SIZE
        END IF
        CALL GCT (AX,AY,SIZE)

        XN = X0+AX
        YN = Y0+AY

        DO 1 IPOS = 1,NCHARS
C             loop to output characters

C             compute character width
          CALL GTXE (CHARS(IPOS:IPOS),FONT,PREC,WIDTH,SIZE,
     *               BOTTOM,BASE,CAP,TOP)

C             calculate new character cell origin
          XSPACE = (WIDTH+SPACE)*XFAC(PATH)
          YSPACE = (TOP-BOTTOM+SPACE)*YFAC(PATH)
          CALL GCT (XSPACE,YSPACE,SIZE)

          XNEW = XN+XSPACE
          YNEW = YN+YSPACE

C             stroke precision - generate software character
          CALL GCHGEN (XN,YN,CHARS(IPOS:IPOS),FONT,POLIN,FILLA,
     *      .FALSE.)

          XN = XNEW
          YN = YNEW

   1    CONTINUE

        RETURN
        END


        SUBROUTINE GKTEXT (PX,PY,CHARS,POLIN,FILLA,LABEL)
C               text

        CHARACTER CHARS*(*)
        EXTERNAL POLIN,FILLA,LABEL

C               include GKS symbol definitions
        INCLUDE 'gksdefs.i'

        INTEGER IPOS
        INTEGER ERRIND,TNR,FONT,PREC,ALH,ALV,PATH
        INTEGER TXX,TXX1,SIZE,BOTTOM,BASE,CAP,TOP,WIDTH,SPACE

        REAL AX, AY, PX, PY, CHSP
        REAL X0, XN, XNEW, XSPACE
        REAL Y0, YN, YNEW, YSPACE
        REAL AFAC(GAHNOR:GARITE)
        INTEGER XFAC(GRIGHT:GDOWN),YFAC(GRIGHT:GDOWN)

        REAL CXL,CXR,CYB,CYT

        DATA AFAC /0.,0.,-0.5,-1./
        DATA XFAC /1,-1,0,0/ , YFAC /0,0,1,-1/

C               inquire current normalization transformation number
        CALL GQCNTN (ERRIND,TNR)

C               inquire current clipping region
        CALL GQCLRG (CXL,CXR,CYB,CYT)

C               calculate text origin
        X0 = PX
        Y0 = PY
        CALL GNT (X0,Y0,TNR)

C               inquire text font and precision
        CALL GQTXFP (ERRIND,FONT,PREC)
        IF (PREC .NE. GSTRKP) CALL GMAPF (FONT)

        IF (PREC .EQ. GSTRP) THEN

          CALL GST (X0,Y0)
C               string precision - output hardware characters
          IF (CXL.LE.X0 .AND. X0.LE.CXR .AND.
     *        CYB.LE.Y0 .AND. Y0.LE.CYT) CALL LABEL (X0,Y0,CHARS)

        ELSE
C               draw software character - set up character
C               transformation
          CALL GSCT

C               compute text extent
          CALL GTXE (CHARS,FONT,PREC,TXX,SIZE,BOTTOM,BASE,CAP,TOP)

C               apply character spacing
          CALL GQCHSP (ERRIND,CHSP)
          SPACE = NINT(CHSP*SIZE)
          TXX = TXX+LEN(CHARS)*SPACE

C               set up x and y increments to align the text
          CALL GQTXAL (ERRIND,ALH,ALV)
          AX = 0
          IF (ALH.NE.GAHNOR) AX = AFAC(ALH)*TXX
          AY = 0
          IF (ALV.EQ.GABOTT) AY = BASE-BOTTOM
          IF (ALV.EQ.GAHALF) AY = -(CAP-BASE)*0.5
          IF (ALV.EQ.GACAP) AY = BASE-CAP
          IF (ALV.EQ.GATOP) AY = BASE-TOP

C               fit x increment to the text path
          CALL GQTXP (ERRIND,PATH)
          IF (PATH .EQ. GLEFT) THEN
            CALL GTXE (CHARS(1:1),FONT,PREC,TXX1,SIZE,
     *                 BOTTOM,BASE,CAP,TOP)
            AX = -AX-TXX1
          ELSE IF (PATH .EQ. GUP .OR. PATH .EQ. GDOWN) THEN
            AX = AFAC(ALH)*SIZE
          END IF
          CALL GCT (AX,AY,SIZE)

          XN = X0+AX
          YN = Y0+AY

          DO 1 IPOS = 1,LEN(CHARS)
C               loop to output characters

C               compute character width
            CALL GTXE (CHARS(IPOS:IPOS),FONT,PREC,WIDTH,SIZE,
     *                 BOTTOM,BASE,CAP,TOP)                               

C               calculate new character cell origin
            XSPACE = (WIDTH+SPACE)*XFAC(PATH)
            YSPACE = (TOP-BOTTOM+SPACE)*YFAC(PATH)
            CALL GCT (XSPACE,YSPACE,SIZE)

            XNEW = XN+XSPACE
            YNEW = YN+YSPACE

            IF (PREC .EQ. GCHARP) THEN

              CALL GST (XN,YN)
C               character precision - output hardware character
              IF (CXL.LE.XN .AND. XN.LE.CXR .AND.
     *            CYB.LE.YN .AND. YN.LE.CYT)
     *          CALL LABEL (XN,YN,CHARS(IPOS:IPOS))

            ELSE
C               stroke precision - generate software character
              CALL GCHGEN (XN,YN,CHARS(IPOS:IPOS),FONT,POLIN,FILLA,
     *          .FALSE.)

            END IF

            XN = XNEW
            YN = YNEW

   1      CONTINUE

        END IF

        RETURN
        END


        SUBROUTINE GQTEXT (PX,PY,CHARS,QX,QY,RX,RY)
C               inquire text extent

        CHARACTER CHARS*(*)
        REAL RX(4),RY(4)

C               include GKS symbol definitions
        INCLUDE 'gksdefs.i'

        INTEGER I, IPOS
        INTEGER ERRIND,TNR,FONT,PREC,ALH,ALV,PATH
        INTEGER TXX,TXX1,WIDTH,SIZE,BOTTOM,BASE,CAP,TOP,SPACE

        REAL AX, AY, CHSP, PX, PY, QX, QY, ASPACE
        REAL X0, XN, XSPACE
        REAL Y0, YN, YSPACE
        REAL AFAC(GAHNOR:GARITE)
        INTEGER XFAC(GRIGHT:GDOWN),YFAC(GRIGHT:GDOWN)

        DATA AFAC /0.,0.,-0.5,-1./
        DATA XFAC /1,-1,0,0/ , YFAC /0,0,1,-1/

C               inquire current normalization transformation number
        CALL GQCNTN (ERRIND,TNR)

C               calculate text origin
        X0 = PX
        Y0 = PY
        CALL GNT (X0,Y0,TNR)

C               inquire text font and precision
        CALL GQTXFP (ERRIND,FONT,PREC)

C             draw software character - set up character
C             transformation
        CALL GSCT

C             compute text extent
        CALL GTXE (CHARS,FONT,PREC,TXX,SIZE,BOTTOM,BASE,CAP,TOP)

C             apply character spacing
        CALL GQCHSP (ERRIND,CHSP)
        SPACE = NINT(CHSP*SIZE)
        TXX = TXX+LEN(CHARS)*SPACE

C             set up x and y increments to align the text
        CALL GQTXAL (ERRIND,ALH,ALV)
        AX = 0
        IF (ALH.NE.GAHNOR) AX = AFAC(ALH)*TXX
        AY = 0
        IF (ALV.EQ.GABOTT) AY = BASE-BOTTOM
        IF (ALV.EQ.GAHALF) AY = -(CAP-BASE)*0.5
        IF (ALV.EQ.GACAP) AY = BASE-CAP
        IF (ALV.EQ.GATOP) AY = BASE-TOP
        ASPACE = AY

C             fit x increment to the text path
        CALL GQTXP (ERRIND,PATH)
        IF (PATH .EQ. GLEFT) THEN
          CALL GTXE (CHARS(1:1),FONT,PREC,TXX1,SIZE,
     *               BOTTOM,BASE,CAP,TOP)
          AX = -AX-TXX1
        ELSE IF (PATH .EQ. GUP .OR. PATH .EQ. GDOWN) THEN
          AX = AFAC(ALH)*SIZE
        END IF
        CALL GCT (AX,AY,SIZE)

        XN = X0+AX
        YN = Y0+AY
        RX(1) = XN
        RY(1) = YN

        DO 1 IPOS = 1,LEN(CHARS)

C             compute character width
          CALL GTXE (CHARS(IPOS:IPOS),FONT,PREC,WIDTH,SIZE,
     *               BOTTOM,BASE,CAP,TOP)

C             calculate new character cell origin
          XSPACE = (WIDTH+SPACE)*XFAC(PATH)
          YSPACE = (TOP-BOTTOM+SPACE)*YFAC(PATH)
          CALL GCT (XSPACE,YSPACE,SIZE)

          XN = XN+XSPACE
          YN = YN+YSPACE

   1    CONTINUE

        IF ((PATH .EQ. GLEFT .OR. PATH .EQ. GRIGHT) .AND.
     *    ALH .EQ. GACENT) THEN
          QX = X0
        ELSE IF (ALH .EQ. GARITE) THEN
          QX = RX(1)
        ELSE
          QX = XN
        END IF
        IF ((PATH .EQ. GUP .OR. PATH .EQ. GDOWN) .AND.
     *    ALV .EQ. GAHALF) THEN
          QY = Y0
        ELSE IF (ALH .EQ. GARITE) THEN
          QY = RY(1)
        ELSE
          QY = YN
        END IF

        XSPACE = 0.0
        YSPACE = -ASPACE
        CALL GCT (XSPACE,YSPACE,SIZE)
        QX = QX+XSPACE
        QY = QY+YSPACE

        CALL GDNT (QX,QY,TNR)

        XSPACE = 0.0
        YSPACE = BOTTOM-BASE
        CALL GCT (XSPACE,YSPACE,SIZE)
        RX(1) = RX(1)+XSPACE
        RY(1) = RY(1)+YSPACE
        RX(2) = XN+XSPACE
        RY(2) = YN+YSPACE

        XSPACE = 0.0
        YSPACE = TOP-BOTTOM
        CALL GCT (XSPACE,YSPACE,SIZE)
        RX(3) = RX(2)+XSPACE
        RY(3) = RY(2)+YSPACE
        RX(4) = RX(1)+XSPACE
        RY(4) = RY(1)+YSPACE

        DO 2 I=1,4
           CALL GDNT (RX(I),RY(I),TNR)
   2    CONTINUE

        RETURN
        END


        SUBROUTINE GMAPF (FONT)
C               map hardware text font to stroke font

        INTEGER FONT
        INTEGER FAMILY, TYPE, ROMAN(4), GREEK(4)

        DATA ROMAN /3,12,16,11/
        DATA GREEK /4, 7,10, 7/

        FONT = ABS(FONT)

        FAMILY = MOD(FONT-1,8)+1
        TYPE = MIN((FONT-1)/8+1,4)

        IF (FAMILY .NE. 7) THEN
            FONT = ROMAN(TYPE)
        ELSE
            FONT = GREEK(TYPE)
        END IF

        RETURN
        END


        SUBROUTINE GCHGEN (XORG,YORG,CHAR,FONT,POLIN,FILLA,FLAG)
C               character generator

        CHARACTER CHAR
        INTEGER I, IC, IX, IY, N
        INTEGER FONT
        EXTERNAL POLIN, FILLA
        LOGICAL FLAG

        INTEGER LSIZE
        REAL XORG, YORG, CHXP
        REAL MSZSC
        LOGICAL ITALIC

        INTEGER BUFF(256)
        INTEGER LEFT,RIGHT,SIZE,BOTTOM,BASE,CAP,TOP,LENGTH,C(2,124)
        EQUIVALENCE (BUFF(1),LEFT),(BUFF(2),RIGHT),(BUFF(3),SIZE)
        EQUIVALENCE (BUFF(4),BOTTOM),(BUFF(5),BASE),(BUFF(6),CAP)
        EQUIVALENCE (BUFF(7),TOP),(BUFF(8),LENGTH),(BUFF(9),C(1,1))

        INTEGER XMIN,XMAX,YMIN,YMAX
        REAL XSCALE,YSCALE,CENTER,HALF

        REAL XN,YN,PX(64),PY(64)
        INTEGER IFONT
        LOGICAL MONO

        INTEGER ERRIND
        REAL WN(4),VP(4)

C               include GKS symbol definitions
        INCLUDE 'gksdefs.i'

C               check for mono spaced font
        IFONT = ABS(FONT)/100
        MONO = IFONT .EQ. 2 .OR. IFONT .EQ. 3
        IFONT = MOD(FONT,100)

        IC = ICHAR(CHAR)
C               inquire character stroke
        CALL LOOKUP (FONT,IC,BUFF(1))

        IF (FLAG .OR. MONO) THEN

          XMIN = 127
          XMAX = 0
          YMIN = 127
          YMAX = 0

          DO 2, I = 1,LENGTH
            IX = C(1,I)
            IF (IX .GT. 127) IX = IX-256
            IX = ABS(IX)
            IY = C(2,I)
            XMIN = MIN(XMIN,IX)
            XMAX = MAX(XMAX,IX)
            YMIN = MIN(YMIN,IY)
            YMAX = MAX(YMAX,IY)
   2      CONTINUE

          IF (XMAX .LE. XMIN) THEN
            XMIN = LEFT
            XMAX = RIGHT
          END IF
          IF (YMAX .LE. YMIN) THEN
            YMIN = BASE
            YMAX = CAP
          END IF

        END IF

        IF (FLAG) THEN
C               inquire marker size scale factor
          CALL GQMKSC (ERRIND,MSZSC)

          XSCALE = 0.001*MSZSC/(XMAX-XMIN)
          YSCALE = 0.001*MSZSC/(YMAX-YMIN)

          CALL GQDT (WN,VP)
C               apply device transformation
          XSCALE = XSCALE*(WN(2)-WN(1))/(VP(2)-VP(1))
          YSCALE = YSCALE*(WN(4)-WN(3))/(VP(4)-VP(3))

          CENTER = 0.5*(XMIN+XMAX)
          HALF = 0.5*(YMIN+YMAX)

        ELSE
C               inquire character expansion factor
          CALL GQCHXP (ERRIND,CHXP)

C               check for italic character
          I = ABS(FONT)/100
          ITALIC = I .EQ. 1 .OR. I .EQ. 3

          LSIZE = SIZE
          IF (MONO) CENTER = 0.5*(XMIN+XMAX)

        END IF

        N = 0
        DO 1 I = 1,LENGTH

          IX = C(1,I)
          IF (IX .GT. 127) IX = IX-256
          IY = C(2,I)

          IF (IX .LT. 0) THEN
            IF (N .GT. 1) THEN
              IF (IFONT .EQ. -51) THEN
               IF (N .GT. 2) CALL FILLA (N,PX,PY,0)
              END IF
              CALL POLIN (N,PX,PY,GLSOLI,0)
              N = 0
            END IF
            IX = -IX
          END IF

          IF (FLAG) THEN
C               marker transformation
            XN = XSCALE*(IX-CENTER)
            YN = YSCALE*(IY-HALF)
          ELSE
            IF (LEFT .EQ. RIGHT) IX = IX+SIZE/2
            IF (MONO) THEN
              XN = IX-CENTER+SIZE*0.5
            ELSE
              XN = IX-LEFT
            END IF
            YN = IY-BASE
C               character transformation
            IF (ITALIC) THEN
              CALL GICT (XN,YN,LSIZE)
            ELSE
              CALL GCT (XN,YN,LSIZE)
            END IF
          END IF

          N = N+1
          PX(N) = XORG+XN
          PY(N) = YORG+YN

   1    CONTINUE
        IF (N .GT. 1) THEN
          IF (IFONT .EQ. -51) THEN
            IF (N .GT. 2) CALL FILLA (N,PX,PY,0)
          END IF
          CALL POLIN (N,PX,PY,GLSOLI,0)
        END IF

        RETURN
        END


        SUBROUTINE GTXE (CHARS,FONT,PREC,TXX,SIZE,BOTTOM,BASE,CAP,TOP)
C               text extend

        CHARACTER CHARS*(*)
        INTEGER IC, IPOS
        INTEGER FONT,PREC,TXX,SIZE,BOTTOM,BASE,CAP,TOP

C               include GKS symbol definitions
        INCLUDE 'gksdefs.i'

        INTEGER IFONT
        LOGICAL MONO

        INTEGER BUFF(256),LEFT,RIGHT
        EQUIVALENCE (BUFF(1),LEFT),(BUFF(2),RIGHT)

        TXX = 0

        IF (PREC .NE. GSTRP) THEN

C               check for mono spaced font
          IFONT = ABS(FONT)/100
          MONO = IFONT .EQ. 2 .OR. IFONT .EQ. 3

          DO 1 IPOS = 1,LEN(CHARS)

            IC = ICHAR(CHARS(IPOS:IPOS))
C               inquire character stroke
            CALL LOOKUP (FONT,IC,BUFF(1))

            IF (MONO) THEN
              TXX = TXX+21
            ELSE IF (IC .NE. 32) THEN
              TXX = TXX+RIGHT-LEFT
            ELSE
              TXX = TXX+BUFF(3)/2
            END IF

   1      CONTINUE

          IF (.NOT. MONO) THEN
            SIZE = BUFF(3)
          ELSE
            SIZE = 21
          END IF

        ELSE

          DO 2 IPOS = 1,LEN(CHARS)

            IC = ICHAR(CHARS(IPOS:IPOS))
C               inquire Adobe Font Metrics information
            CALL GKAFM (FONT,IC,BUFF)

            TXX = TXX+RIGHT-LEFT

   2      CONTINUE

          SIZE = BUFF(3)

        END IF

        BOTTOM = BUFF(4)
        BASE = BUFF(5)
        CAP = BUFF(6)
        TOP = BUFF(7)

        RETURN
        END


        SUBROUTINE GSCT
C               set up character transformation

        REAL X, Y, WIDTH, HEIGHT
        INTEGER SIZE

        REAL SINS, CHH, CHXP
        INTEGER ERRIND, TNR, FONT, PREC
        REAL XREL, YREL, CHUX, CHUY
        REAL UX, UY, BX, BY

C               include GKS symbol definitions
        INCLUDE 'gksdefs.i'

        SAVE

C               inquire current normalization transformation number
        CALL GQCNTN (ERRIND,TNR)
C               inquire character up vector
        CALL GQCHUP (ERRIND,CHUX,CHUY)
C               inquire character height
        CALL GQCHH (ERRIND,CHH)
C               inquire character expansion factor
        CALL GQCHXP (ERRIND,CHXP)
C               inquire text font and precision
        CALL GQTXFP (ERRIND,FONT,PREC)

C               scale to normalize the up vector
        SCALE = SQRT(CHUX*CHUX+CHUY*CHUY)
        CHUX = CHUX/SCALE
        CHUY = CHUY/SCALE

C               compute character up vector
        UX = CHUX*CHH
        UY = CHUY*CHH
C               normalize character up vector
        CALL GCNT (UX,UY,TNR)

C               compute character base vector (right angle to up vector)
        BX = CHUY*CHH
        BY = -CHUX*CHH
C               normalize character base vector
        CALL GCNT (BX,BY,TNR)
        BX = BX*CHXP
        BY = BY*CHXP

        SINS = 0.207912

        RETURN


        ENTRY GCT (X,Y,SIZE)
C               character transformation

        XREL = X/SIZE
        YREL = Y/SIZE

C               rotation transformation
        X = BX*XREL+UX*YREL
        Y = BY*XREL+UY*YREL

        RETURN


        ENTRY GICT (X,Y,SIZE)
C               italic character transformation

        XREL = X/SIZE
        YREL = Y/SIZE
        XREL = XREL+SINS*YREL

C               rotation transformation
        X = BX*XREL+UX*YREL
        Y = BY*XREL+UY*YREL

        RETURN


        ENTRY GCHH (HEIGHT)
C               inquire character height

        WIDTH = 0.0
        HEIGHT = SQRT(UX*UX+UY*UY)
        CALL GCST (WIDTH, HEIGHT)
        HEIGHT = SQRT(WIDTH*WIDTH+HEIGHT*HEIGHT)

        RETURN
        END


        SUBROUTINE GFILLA (N,PX,PY,TNR,POLIN,YRES)
C               fill area

        INTEGER N, TNR
        REAL PX(N), PY(N)
        EXTERNAL POLIN
        REAL YRES

        REAL D, DX, DY, INC
        REAL X0, XINC, XMAX, XMIN 
        REAL Y0, YINC, YMAX, YMIN

C               include GKS symbol definitions
        INCLUDE 'gksdefs.i'

        INTEGER ERRIND,STYLE,INDEX,HATCH

        INTEGER BORDER
        DATA BORDER /0/

C               inquire fill area interior style
        CALL GQFAIS (ERRIND,STYLE)
C               inquire fill area style index
        CALL GQFASI (ERRIND,INDEX)

C               compute minimum and maximum values
        CALL GCMIMA(N,PX,XMIN,XMAX)
        CALL GCMIMA(N,PY,YMIN,YMAX)
        CALL GNT(XMIN,YMIN,TNR)
        CALL GNT(XMAX,YMAX,TNR)

        IF (STYLE .EQ. GHOLLO) THEN

C               draw the border
          CALL POLIN (N,PX,PY,BORDER,TNR)

        ELSE IF (STYLE .EQ. GSOLID) THEN

C               solid fill area
          X0 = XMIN
          XINC = 0.
          DX = XMAX-XMIN
          Y0 = YMIN
          YINC = YRES
          DY = 0.
          CALL GFINE (N,PX,PY,TNR,X0,XINC,DX,XMAX,Y0,YINC,DY,YMAX,
     *                POLIN)

        ELSE IF (STYLE .EQ. GHATCH) THEN
C               hatched fill area

          HATCH = MOD(INDEX-1,6) + 1
          IF (INDEX .GT. 6) THEN
            INC = 0.02
          ELSE
            INC = 0.01
          END IF

          IF (HATCH .EQ. 1 .OR. HATCH .EQ. 5) THEN
C               put out vertical lines
            X0 = XMIN
            XINC = INC
            DX = 0.
            Y0 = YMIN
            YINC = 0.
            DY = YMAX-YMIN
            CALL GFINE (N,PX,PY,TNR,X0,XINC,DX,XMAX,Y0,YINC,DY,YMAX,
     *                  POLIN)
          END IF

          IF (HATCH .EQ. 2 .OR. HATCH .EQ. 5) THEN
C               put out horizontal lines
            X0 = XMIN
            XINC = 0.
            DX = XMAX-XMIN
            Y0 = YMIN
            YINC = INC
            DY = 0.
            CALL GFINE (N,PX,PY,TNR,X0,XINC,DX,XMAX,Y0,YINC,DY,YMAX,
     *                  POLIN)
          END IF

          IF (HATCH .EQ. 3 .OR. HATCH .EQ. 6) THEN
C               put out 45 degree lines
            D = MAX(XMAX-XMIN,YMAX-YMIN)
            X0 = XMIN
            XINC = 0.
            DX = D
            Y0 = YMIN-D
            YINC = INC*SQRT(2.0)
            DY = D
            CALL GFINE (N,PX,PY,TNR,X0,XINC,DX,XMAX,Y0,YINC,DY,YMAX,
     *                  POLIN)
          END IF

          IF (HATCH .EQ. 4 .OR. HATCH .EQ. 6) THEN
C               put out -45 degree lines
            D = MAX(XMAX-XMIN,YMAX-YMIN)
            X0 = XMAX
            XINC = 0.
            DX = -D
            Y0 = YMIN-D
            YINC = INC*SQRT(2.0)
            DY = D
            CALL GFINE (N,PX,PY,TNR,X0,XINC,DX,XMAX,Y0,YINC,DY,YMAX,
     *                  POLIN)
          END IF

        ELSE
C               pattern emulation not yet implemented - draw the border
          CALL POLIN (N,PX,PY,BORDER,TNR)

        END IF

        RETURN
        END


        SUBROUTINE GKSFA (N,PX,PY,TNR,POLIN,YRES)
C               solid fill area

        INTEGER N
        REAL PX(N),PY(N)
        INTEGER TNR
        EXTERNAL POLIN
        REAL YRES

        REAL DX, DY
        REAL X0, XINC, XMAX, XMIN 
        REAL Y0, YINC, YMAX, YMIN

C               compute minimum and maximum values
        CALL GCMIMA(N,PX,XMIN,XMAX)
        CALL GCMIMA(N,PY,YMIN,YMAX)

C               solid fill area
        X0 = XMIN
        XINC = 0.
        DX = XMAX-XMIN
        Y0 = YMIN
        YINC = YRES
        DY = 0.
        CALL GFINE (N,PX,PY,TNR,X0,XINC,DX,XMAX,Y0,YINC,DY,YMAX,POLIN)

        RETURN
        END


        SUBROUTINE GFINE (N,PX,PY,TNR,X0,XINC,DX,XEND,Y0,YINC,DY,YEND,
     *                    POLIN)
C               fill routine

        INTEGER N
        REAL PX(N),PY(N)
        INTEGER TNR
        REAL X0, XINC, DX, XEND, Y0, YINC, DY, YEND
        EXTERNAL POLIN

        REAL FEPS
        PARAMETER (FEPS = 1.0E-06)

        INTEGER I, IM1, INC, L, NI
        REAL X1, X2, X3, X4, XI, EPSX
        REAL Y1, Y2, Y3, Y4, YI, EPSY

        LOGICAL FLAG
        REAL SX(128),SY(128)

C               include GKS symbol definitions
        INCLUDE 'gksdefs.i'

        EPSX = ABS((XEND-X0)*1E-5)
        EPSY = ABS((YEND-Y0)*1E-5)

        L = 0
C               next line
   1    L = L+1
        X1 = X0+L*XINC
        Y1 = Y0+L*YINC
        IF (X1 .GT. XEND .OR. Y1 .GT. YEND) RETURN
        X2 = X1+DX
        Y2 = Y1+DY

C               calculate all intersections of the line given by the points
C               (x1,y1),(x2,y2) with the polygon's vertices
        NI = 0

        DO 2 I = 1,N

          IF (I .EQ. 1) THEN
            IM1 = N
          ELSE
            IM1 = I-1
          END IF
          X3 = PX(IM1)
          Y3 = PY(IM1)
          CALL GNT (X3,Y3,TNR)
          X4 = PX(I)
          Y4 = PY(I)
          CALL GNT (X4,Y4,TNR)
          CALL GINTS (X1,Y1,X2,Y2,X3,Y3,X4,Y4,XI,YI,FLAG)

C               check if there is an intersection
          IF (FLAG) THEN
C               be sure that point not outside the polygon
            IF(XI.GE.MIN(X3,X4)-EPSX .AND.
     *         XI.LE.MAX(X3,X4)+EPSX .AND.
     *         YI.GE.MIN(Y3,Y4)-EPSY .AND.
     *         YI.LE.MAX(Y3,Y4)+EPSY) THEN
              NI = NI+1
              SX(NI) = XI
              SY(NI) = YI
            END IF
          END IF

   2    CONTINUE

        IF (NI .GT. 0) THEN
C               set sort mode
          INC = MOD(L,2)
C               sort on X coordinates
          IF (ABS(XINC) .LE. FEPS) THEN
            CALL GSORT (NI,SX,SY,INC)
          ELSE
C               sort on Y coordinates
            CALL GSORT (NI,SY,SX,INC)
          END IF

          DO 3 I = 1,NI-1,2
            CALL POLIN (2,SX(I),SY(I),GLSOLI,0)
   3      CONTINUE

        END IF

        GO TO 1

        END


        SUBROUTINE GCMIMA (N,P,PMIN,PMAX)
C               compute minimum and maximum

        INTEGER N
        REAL P(N)
        REAL PMAX, PMIN

        INTEGER I

        PMIN = P(1)
        PMAX = P(1)
        DO 1 I = 2,N
          PMIN = MIN(PMIN,P(I))
          PMAX = MAX(PMAX,P(I))
   1    CONTINUE

        RETURN
        END


        SUBROUTINE GINTS (X1,Y1,X2,Y2,X3,Y3,X4,Y4,XI,YI,FLAG)
C               compute intersection

        REAL FEPS
        PARAMETER (FEPS = 1.0E-06)

        LOGICAL FLAG,INF1,INF2
        REAL A1, A2
        REAL X1, X2, X3, X4, XI
        REAL Y1, Y2, Y3, Y4, YI

        INF1 = ABS(X1-X2) .LE. FEPS
        IF (.NOT. INF1) A1 = (Y2-Y1)/(X2-X1)
        INF2 = ABS(X3-X4) .LE. FEPS 
        IF (.NOT. INF2) A2 = (Y4-Y3)/(X4-X3)
        IF (INF1) THEN
          IF (INF2) THEN
            FLAG = .FALSE.
          ELSE
            XI = X1
            YI = Y3+A2*(XI-X3)
            FLAG = .TRUE.
          END IF
        ELSE
          IF (INF2) THEN
            XI = X3
            YI = Y1+A1*(XI-X1)
            FLAG = .TRUE.
          ELSE
            FLAG = ABS(A1-A2) .GT. FEPS 
            IF (FLAG) THEN
              XI = (Y1-Y3-A1*X1+A2*X3)/(A2-A1)
              YI = Y1+A1*(XI-X1)
            END IF
          END IF
        END IF

        RETURN
        END


        SUBROUTINE GSORT (N,X,Y,INC)
C               sort

        INTEGER N, INC
        REAL X(N),Y(N)

        REAL H
        INTEGER I, J
        LOGICAL SWITCH

        DO 1 I = 1,N-1
          DO 2 J = I,N
            IF (INC .EQ. 0) THEN
C               sort in decreasing order
              SWITCH = X(J) .LT. X(I)
            ELSE
C               sort in increasing order
              SWITCH = X(J) .GT. X(I)
            END IF
            IF (SWITCH) THEN
C               switch elements
              H = X(I)
              X(I) = X(J)
              X(J) = H
              H = Y(I)
              Y(I) = Y(J)
              Y(J) = H
            END IF
   2      CONTINUE
   1    CONTINUE

        RETURN
        END


        SUBROUTINE GCELLA (XMIN, XMAX, YMIN, YMAX, NCOLS, NROWS,
     *      DIMX, CI, GSCI, GFA)
C               cell array simulation routine

        REAL XMIN, XMAX, YMIN, YMAX
        INTEGER NCOLS, NROWS, DIMX, CI(DIMX, 1)

        EXTERNAL GSCI, GFA

        INTEGER ERRIND, TNR
        REAL DX, DY, X(5), Y(5)
        INTEGER I, J

C               inquire current transformation number
        CALL GQCNTN (ERRIND, TNR)

        DX = (XMAX - XMIN) / NCOLS
        DY = (YMAX - YMIN) / NROWS

        DO 1,  J = 1, NROWS
            DO 2,  I = 1, NCOLS
                CALL GSCI (CI(I, J))
                X(1) = XMIN + (I-1) * DX
                Y(1) = YMIN + (J-1) * DY
                X(2) = X(1)
                Y(2) = YMIN + J * DY
                X(3) = XMIN + I * DX
                Y(3) = Y(2)
                X(4) = X(3)
                Y(4) = Y(1)
                X(5) = X(1)
                Y(5) = Y(1)
                CALL GFA (5, X, Y, TNR)
   2        CONTINUE
   1    CONTINUE

        RETURN
        END


        SUBROUTINE GDEC (IVAL, NCHARS, CHARS)
C               convert an integer number to byte data

        CHARACTER*(*) CHARS
        INTEGER IVAL, NCHARS

        CHARACTER DIG(20), DIGIT(0:9)
        INTEGER I, VALUE, NDIG

        DATA DIGIT /'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'/

        NCHARS = 0
        IF (IVAL .LT. 0) THEN
          NCHARS = 1
          CHARS(1:1) = '-'
        END IF

        VALUE = ABS(IVAL)
        NDIG = 0
   1    CONTINUE
        NDIG = NDIG + 1
        DIG(NDIG) = DIGIT(MOD(VALUE, 10))
        VALUE = VALUE / 10
        IF (VALUE .NE. 0) GOTO 1

        DO 2, I = NDIG, 1, -1
          NCHARS = NCHARS + 1
          CHARS(NCHARS:NCHARS) = DIG(I)
   2    CONTINUE

        RETURN
        END


        SUBROUTINE GOCT (IVAL,CHARS)
C               convert a byte to octal value

        CHARACTER*(*) CHARS
        INTEGER IVAL, I, J, K, IDEC

        I = IVAL
        J = 64
        DO 1 K = 1,3
          IDEC = I/J
          CHARS(K:K) = CHAR(ICHAR('0')+IDEC)
          I = I-IDEC*J
          J = J/8
   1    CONTINUE

        RETURN
        END


        SUBROUTINE GSTR (NCHARS,CHARS,IVAL)
C               convert a string to an integer number

        INTEGER IVAL, L, NCHARS
        CHARACTER*(*) CHARS

        IVAL = 0
        DO 1 L = 1,NCHARS
          IF (CHARS(L:L) .GE. '0' .AND. CHARS(L:L) .LE. '9') THEN
            IVAL = IVAL*10 + ICHAR(CHARS(L:L))-ICHAR('0')
          ELSE
            RETURN
          END IF
   1    CONTINUE

        RETURN
        END


        SUBROUTINE GFLT (RVAL,NCHARS,CHARS)
C               convert a real number to a string

        REAL FEPS
        PARAMETER (FEPS = 1.0E-06)

        INTEGER I, J, N, IX, IEXP, ISIGN, ITSIZE
        INTEGER NCHAR, NCHARS, NFACTR, IWIDTH
        REAL RVAL, VALUE
        CHARACTER*(*) CHARS

        INTEGER CHR(16)
        PARAMETER (IWIDTH=5)

        VALUE = RVAL
        NCHARS = 0
   1    ISIGN = 0
        IEXP = 0
        IF (VALUE .LT. 0) ISIGN = 1
        IF (ABS(VALUE) .LE. FEPS) GOTO 12
        NCHAR = IWIDTH
        VALUE = ABS(VALUE)
C               convert number to canonical form, consisting of
C               the most significant digits (up to IWIDTH of them),
C               stored (in IX) as an integer less than 10^IWIDTH.
C               The exponent is stored as an integer (in NFACTR)
C               Note that 10^-IWIDTH is a fuzz factor, added for rounding
        NFACTR = INT(ALOG10(VALUE)+10.0**(-IWIDTH))-(IWIDTH-1)
C               note that 0.5 is added for rounding
        IX = INT(0.5+VALUE/10.0**NFACTR)
C               strip off trailing zeros
   3    IF (MOD(IX,10).NE.0) GOTO 4
        IX = IX/10
        NFACTR = NFACTR+1
        NCHAR = NCHAR -1
        GOTO 3
C               see if scientific notation necessary
    4   IF (NCHAR+IABS(NFACTR).LE.IWIDTH) GOTO 5
        IEXP = NFACTR+NCHAR-1
        N = NFACTR
        NFACTR = 1-NCHAR
        IF (IABS(IEXP).GT.IWIDTH-2) GOTO 5
        IEXP = 0
        NFACTR = N
C               convert canonical representation into ADE format
   5    IF (ISIGN.EQ.0) GOTO 6
        NCHARS = NCHARS+1
        CHR(NCHARS) = 45
   6    ITSIZE = NFACTR+NCHAR
        J = NINT(10.0**(NCHAR-1))
        IF (ITSIZE.LE.0) GOTO 9
C               store digits
   7    IF (J.EQ.0) GOTO 11
        I = IX/J
        NCHARS = NCHARS+1
        CHR(NCHARS) = 48+I
        IX = IX-J*I
        J = J/10
        NCHAR = NCHAR-1
        ITSIZE = ITSIZE-1
C               see if done
        IF (NCHAR.LE.0.AND.ITSIZE.LE.0) GOTO 11
        IF (ITSIZE.EQ.0) GOTO 9
C               see if zero fill unnecessary
        IF (NCHAR.NE.0) GOTO 7
C               insert trailing zero fill
   8    NCHARS = NCHARS+1
        CHR(NCHARS) = 48
        ITSIZE = ITSIZE-1
        IF (ITSIZE.NE.0) GOTO 8
        GOTO 11
C               insert a decimal point
   9    NCHARS = NCHARS+1
        CHR(NCHARS) = 46
        IF (ITSIZE.EQ.0) GOTO 7
C               insert leading zero fill
  10    NCHARS = NCHARS+1
        CHR(NCHARS) = 48
        ITSIZE = ITSIZE+1
        IF (ITSIZE.NE.0) GOTO 10
        GOTO 7
C               check for exponent
  11    IF (IEXP.EQ.0) GOTO 13
C               exponent required - will be converted to ADE by
C               above code (pseudo-recursively)
        NCHARS = NCHARS+1
        CHR(NCHARS) = 69
        VALUE = IEXP
        GOTO 1
C               zero handled as a special case
  12    NCHARS = NCHARS+1
        CHR(NCHARS) = 48

  13    DO 14 I = 1,NCHARS
          CHARS(I:I) = CHAR(CHR(I))
  14    CONTINUE

        RETURN
        END


        SUBROUTINE GKRMAP (COLI, R, G, B, NUMCOL, PALETT, CMAP)
C               remap palette

        INTEGER COLI, NUMCOL
        REAL R, G, B
        INTEGER PALETT(NUMCOL), CMAP(16)
        REAL RED, GREEN, BLUE, D, MDIST

        MDIST = 3.0
        DO 1, J = 1, NUMCOL
          CALL GQRGB(J, RED, GREEN, BLUE)
          D = ABS(RED-R) + ABS(GREEN-G) + ABS(BLUE-B)
          IF (D .LT. MDIST) THEN
            MDIST = D
            I = J
          END IF
   1    CONTINUE

        IF (MDIST .LT. 3.0) CMAP(COLI + 1) = PALETT(I)

        RETURN
        END
