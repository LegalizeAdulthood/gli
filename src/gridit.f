      SUBROUTINE GRIDIT (ND,XD,YD,ZD,NXI,NYI,XI,YI,ZI,IWK,WK)
C
C DECLARATION STATEMENTS
      DIMENSION XD(ND),YD(ND),ZD(ND),XI(NXI),YI(NYI),ZI(NXI,NYI)
      DIMENSION IWK(*),WK(*)
      REAL XMIN,XMAX,YMIN,YMAX
      INTEGER I,IXI,IYI,MD,NCP,NDP
C
C PRELIMINARY PROCESSING
      XMIN = XD(1)
      XMAX = XMIN
      YMIN = YD(1)
      YMAX = YMIN
C
C CALCULATION OF MIN/MAX VALUES
      DO 10, I = 2,ND
         XMIN = MIN(XMIN,XD(I))
         XMAX = MAX(XMAX,XD(I))
         YMIN = MIN(YMIN,YD(I))
         YMAX = MAX(YMAX,YD(I))
   10 CONTINUE
C
C DETERMINE GRID POINTS INSIDE THE DATA AREA
      DO 20, IXI = 1,NXI
         XI(IXI) = XMIN+(IXI-1)/FLOAT(NXI-1)*(XMAX-XMIN)
   20 CONTINUE
      DO 30, IYI = 1,NYI
         YI(IYI) = YMIN+(IYI-1)/FLOAT(NYI-1)*(YMAX-YMIN)
   30 CONTINUE
C
C CALL THE SMOOTH SURFACE FIT ROUTINE
      MD = 1
      NCP = 4
      NDP = ND
      CALL IDSFFT(MD,NCP,NDP,XD,YD,ZD,NXI,NYI,XI,YI,ZI,IWK,WK)

      END


      SUBROUTINE IDCLDP(NDP,XD,YD,NCP,IPC)
C THIS SUBROUTINE SELECTS SEVERAL DATA POINTS THAT ARE CLOSEST          
C TO EACH OF THE DATA POINT.                                            
C THE INPUT PARAMETERS ARE                                              
C     NDP = NUMBER OF DATA POINTS,                                      
C     XD,YD = ARRAYS OF DIMENSION NDP CONTAINING THE X AND Y            
C           COORDINATES OF THE DATA POINTS,                             
C     NCP = NUMBER OF DATA POINTS CLOSEST TO EACH DATA                  
C           POINTS.                                                     
C THE OUTPUT PARAMETER IS                                               
C     IPC = INTEGER ARRAY OF DIMENSION NCP*NDP, WHERE THE               
C           POINT NUMBERS OF NCP DATA POINTS CLOSEST TO                 
C           EACH OF THE NDP DATA POINTS ARE TO BE STORED.               
C THIS SUBROUTINE ARBITRARILY SETS A RESTRICTION THAT NCP MUST          
C NOT EXCEED 25.                                                        
      REAL FEPS
      PARAMETER (FEPS = 1.0E-06)
C DECLARATION STATEMENTS                                                
      DIMENSION XD(NDP),YD(NDP),IPC(*)
      DIMENSION DSQ0(25),IPC0(25)
      DATA NCPMX/25/
C PRELIMINARY PROCESSING                                                
      NDP0 = NDP
      NCP0 = NCP
      DSQMN = 0.0
      IF (NDP0.GE.2) THEN
         IF (NCP0.GE.1 .AND. NCP0.LE.NCPMX .AND. NCP0.LT.NDP0) THEN
C CALCULATION                                                           
            DO 10 IP1 = 1,NDP0
C - SELECTS NCP POINTS.                                                 
               X1 = XD(IP1)
               Y1 = YD(IP1)
               J1 = 0
               DSQMX = 0.0
               DO 20 IP2 = 1,NDP0
                  IF (IP2.NE.IP1) THEN
                     DSQI = (XD(IP2)-X1)**2 + (YD(IP2)-Y1)**2
                     J1 = J1 + 1
                     DSQ0(J1) = DSQI
                     IPC0(J1) = IP2
                     IF (DSQI.GT.DSQMX) THEN
                        DSQMX = DSQI
                        JMX = J1
                     END IF

                     IF (J1.GE.NCP0) GO TO 30
                  END IF

   20          CONTINUE
   30          IP2MN = IP2 + 1
               IF (IP2MN.LE.NDP0) THEN
                  DO 40 IP2 = IP2MN,NDP0
                     IF (IP2.NE.IP1) THEN
                        DSQI = (XD(IP2)-X1)**2 + (YD(IP2)-Y1)**2
                        IF (DSQI.LT.DSQMX) THEN
                           DSQ0(JMX) = DSQI
                           IPC0(JMX) = IP2
                           DSQMX = 0.0
                           DO 50 J1 = 1,NCP0
                              IF (DSQ0(J1).GT.DSQMX) THEN
                                 DSQMX = DSQ0(J1)
                                 JMX = J1
                              END IF

   50                      CONTINUE
                        END IF

                     END IF

   40             CONTINUE
               END IF
C - CHECKS IF ALL THE NCP+1 POINTS ARE COLLINEAR.                       
               IP2 = IPC0(1)
               DX12 = XD(IP2) - X1
               DY12 = YD(IP2) - Y1
               DO 60 J3 = 2,NCP0
                  IP3 = IPC0(J3)
                  DX13 = XD(IP3) - X1
                  DY13 = YD(IP3) - Y1
                  IF (ABS(DY13*DX12-DX13*DY12).GT.FEPS) GO TO 70
   60          CONTINUE
C - SEARCHES FOR THE CLOSEST NONCOLLINEAR POINT.                        
               NCLPT = 0
               DO 80 IP3 = 1,NDP0
                  IF (IP3.NE.IP1) THEN
                     DO 90 J4 = 1,NCP0
                        IF (IP3.EQ.IPC0(J4)) GO TO 80
   90                CONTINUE
                     DX13 = XD(IP3) - X1
                     DY13 = YD(IP3) - Y1
                     IF (ABS(DY13*DX12-DX13*DY12).GT.FEPS) THEN
                        DSQI = (XD(IP3)-X1)**2 + (YD(IP3)-Y1)**2
                        IF (NCLPT.NE.0) THEN
                           IF (DSQI.GE.DSQMN) GO TO 80
                        END IF

                        NCLPT = 1
                        DSQMN = DSQI
                        IP3MN = IP3
                     END IF

                  END IF

   80          CONTINUE
               IF (NCLPT.EQ.0) THEN
                  GO TO 100

               ELSE
                  DSQMX = DSQMN
                  IPC0(JMX) = IP3MN
               END IF
C - REPLACES THE LOCAL ARRAY FOR THE OUTPUT ARRAY.                      
   70          J1 = (IP1-1)*NCP0
               DO 110 J2 = 1,NCP0
                  J1 = J1 + 1
                  IPC(J1) = IPC0(J2)
  110          CONTINUE
   10       CONTINUE
            RETURN

  100       WRITE (*,FMT=9010)
            GO TO 120

         END IF

      END IF
C ERROR EXIT                                                            
      WRITE (*,FMT=9000)
  120 WRITE (*,FMT=9020) NDP0,NCP0
      IPC(1) = 0
C FORMAT STATEMENTS FOR ERROR MESSAGES                                  
 9000 FORMAT (1X,/,' ***   IMPROPER INPUT PARAMETER VALUE(S).')
 9010 FORMAT (1X,/,' ***   ALL COLLINEAR DATA POINTS.')
 9020 FORMAT ('   NDP =',I5,5X,'NCP =',I5,/,
     +        ' ERROR DETECTED IN ROUTINE   IDCLDP',/)

      END


      SUBROUTINE IDGRID(XD,YD,NT,IPT,NL,IPL,NXI,NYI,XI,YI,NGP,IGP)
C THIS SUBROUTINE ORGANIZES GRID POINTS FOR SURFACE FITTING BY          
C SORTING THEM IN ASCENDING ORDER OF TRIANGLE NUMBERS AND OF THE        
C BORDER LINE SEGMENT NUMBER.                                           
C THE INPUT PARAMETERS ARE                                              
C     XD,YD = ARRAYS OF DIMENSION NDP CONTAINING THE X AND Y            
C           COORDINATES OF THE DATA POINTS, WHERE NDP IS THE            
C           NUMBER OF THE DATA POINTS,                                  
C     NT  = NUMBER OF TRIANGLES,                                        
C     IPT = INTEGER ARRAY OF DIMENSION 3*NT CONTAINING THE              
C           POINT NUMBERS OF THE VERTEXES OF THE TRIANGLES,             
C     NL  = NUMBER OF BORDER LINE SEGMENTS,                             
C     IPL = INTEGER ARRAY OF DIMENSION 3*NL CONTAINING THE              
C           POINT NUMBERS OF THE END POINTS OF THE BORDER               
C           LINE SEGMENTS AND THEIR RESPECTIVE TRIANGLE                 
C           NUMBERS,                                                    
C     NXI = NUMBER OF GRID POINTS IN THE X COORDINATE,                  
C     NYI = NUMBER OF GRID POINTS IN THE Y COORDINATE,                  
C     XI,YI = ARRAYS OF DIMENSION NXI AND NYI CONTAINING                
C           THE X AND Y COORDINATES OF THE GRID POINTS,                 
C           RESPECTIVELY.                                               
C THE OUTPUT PARAMETERS ARE                                             
C     NGP = INTEGER ARRAY OF DIMENSION 2*(NT+2*NL) WHERE THE            
C           NUMBER OF GRID POINTS THAT BELONG TO EACH OF THE            
C           TRIANGLES OR OF THE BORDER LINE SEGMENTS ARE TO             
C           BE STORED,                                                  
C     IGP = INTEGER ARRAY OF DIMENSION NXI*NYI WHERE THE                
C           GRID POINT NUMBERS ARE TO BE STORED IN ASCENDING            
C           ORDER OF THE TRIANGLE NUMBER AND THE BORDER LINE            
C           SEGMENT NUMBER.                                             
C DECLARATION STATEMENTS                                                
      DIMENSION XD(*),YD(*),IPT(*),IPL(*),XI(NXI),YI(NYI),
     +          NGP(*),IGP(*)
C PRELIMINARY PROCESSING                                                
      NT0 = NT
      NL0 = NL
      NXI0 = NXI
      NYI0 = NYI
      NXINYI = NXI0*NYI0
      XIMN = AMIN1(XI(1),XI(NXI0))
      XIMX = AMAX1(XI(1),XI(NXI0))
      YIMN = AMIN1(YI(1),YI(NYI0))
      YIMX = AMAX1(YI(1),YI(NYI0))
C DETERMINES GRID POINTS INSIDE THE DATA AREA.                          
      JNGP0 = 0
      JNGP1 = 2* (NT0+2*NL0) + 1
      JIGP0 = 0
      JIGP1 = NXINYI + 1
      DO 10 IT0 = 1,NT0
         NGP0 = 0
         NGP1 = 0
         IT0T3 = IT0*3
         IP1 = IPT(IT0T3-2)
         IP2 = IPT(IT0T3-1)
         IP3 = IPT(IT0T3)
         X1 = XD(IP1)
         Y1 = YD(IP1)
         X2 = XD(IP2)
         Y2 = YD(IP2)
         X3 = XD(IP3)
         Y3 = YD(IP3)
         XMN = AMIN1(X1,X2,X3)
         XMX = AMAX1(X1,X2,X3)
         YMN = AMIN1(Y1,Y2,Y3)
         YMX = AMAX1(Y1,Y2,Y3)
         INSD = 0
         DO 20 IXI = 1,NXI0
            IF (XI(IXI).GE.XMN .AND. XI(IXI).LE.XMX) THEN
               IF (INSD.NE.1) THEN
                  INSD = 1
                  IXIMN = IXI
               END IF

            ELSE IF (INSD.NE.0) THEN
               GO TO 30

            END IF

   20    CONTINUE
         IF (INSD.EQ.0) THEN
            GO TO 40

         ELSE
            IXIMX = NXI0
            GO TO 50

         END IF

   30    IXIMX = IXI - 1
   50    DO 60 IYI = 1,NYI0
            YII = YI(IYI)
            IF (YII.GE.YMN .AND. YII.LE.YMX) THEN
               DO 70 IXI = IXIMN,IXIMX
                  XII = XI(IXI)
                  L = 0
                  EXPR = (X1-XII)* (Y2-YII) - (Y1-YII)* (X2-XII)
                  IF (EXPR.LT.0) GO TO 70
                  IF (EXPR.EQ.0) L = 1
                  EXPR = (X2-XII)* (Y3-YII) - (Y2-YII)* (X3-XII)
                  IF (EXPR.LT.0) GO TO 70
                  IF (EXPR.EQ.0) L = 1
                  EXPR = (X3-XII)* (Y1-YII) - (Y3-YII)* (X1-XII)
                  IF (EXPR.LT.0) GO TO 70
                  IF (EXPR.EQ.0) L = 1
                  IZI = NXI0* (IYI-1) + IXI
                  IF (L.EQ.1) THEN
                     IF (JIGP1.LE.NXINYI) THEN
                        DO 140 JIGP1I = JIGP1,NXINYI
                           IF (IZI.EQ.IGP(JIGP1I)) GO TO 70
  140                   CONTINUE
                     END IF

                     NGP1 = NGP1 + 1
                     JIGP1 = JIGP1 - 1
                     IGP(JIGP1) = IZI

                  ELSE
                     NGP0 = NGP0 + 1
                     JIGP0 = JIGP0 + 1
                     IGP(JIGP0) = IZI
                  END IF

   70          CONTINUE
            END IF

   60    CONTINUE
   40    JNGP0 = JNGP0 + 1
         NGP(JNGP0) = NGP0
         JNGP1 = JNGP1 - 1
         NGP(JNGP1) = NGP1
   10 CONTINUE
C DETERMINES GRID POINTS OUTSIDE THE DATA AREA.                         
C - IN SEMI-INFINITE RECTANGULAR AREA.                                  
      DO 150 IL0 = 1,NL0
         NGP0 = 0
         NGP1 = 0
         IL0T3 = IL0*3
         IP1 = IPL(IL0T3-2)
         IP2 = IPL(IL0T3-1)
         X1 = XD(IP1)
         Y1 = YD(IP1)
         X2 = XD(IP2)
         Y2 = YD(IP2)
         XMN = XIMN
         XMX = XIMX
         YMN = YIMN
         YMX = YIMX
         IF (Y2.GE.Y1) XMN = AMIN1(X1,X2)
         IF (Y2.LE.Y1) XMX = AMAX1(X1,X2)
         IF (X2.LE.X1) YMN = AMIN1(Y1,Y2)
         IF (X2.GE.X1) YMX = AMAX1(Y1,Y2)
         INSD = 0
         DO 160 IXI = 1,NXI0
            IF (XI(IXI).GE.XMN .AND. XI(IXI).LE.XMX) THEN
               IF (INSD.NE.1) THEN
                  INSD = 1
                  IXIMN = IXI
               END IF

            ELSE IF (INSD.NE.0) THEN
               GO TO 170

            END IF

  160    CONTINUE
         IF (INSD.EQ.0) THEN
            GO TO 180

         ELSE
            IXIMX = NXI0
            GO TO 190

         END IF

  170    IXIMX = IXI - 1
  190    DO 200 IYI = 1,NYI0
            YII = YI(IYI)
            IF (YII.GE.YMN .AND. YII.LE.YMX) THEN
               DO 210 IXI = IXIMN,IXIMX
                  XII = XI(IXI)
                  L = 0
                  EXPR = (X1-XII)* (Y2-YII) - (Y1-YII)* (X2-XII)
                  IF (EXPR.GT.0) GO TO 210
                  IF (EXPR.EQ.0) L = 1
                  EXPR = (X2-X1)* (XII-X1) + (Y2-Y1)* (YII-Y1)
                  IF (EXPR.LT.0) GO TO 210
                  IF (EXPR.EQ.0) L = 1
                  EXPR = (X1-X2)* (XII-X2) + (Y1-Y2)* (YII-Y2)
                  IF (EXPR.LT.0) GO TO 210
                  IF (EXPR.EQ.0) L = 1
                  IZI = NXI0* (IYI-1) + IXI
                  IF (L.EQ.1) THEN
                     IF (JIGP1.LE.NXINYI) THEN
                        DO 280 JIGP1I = JIGP1,NXINYI
                           IF (IZI.EQ.IGP(JIGP1I)) GO TO 210
  280                   CONTINUE
                     END IF

                     NGP1 = NGP1 + 1
                     JIGP1 = JIGP1 - 1
                     IGP(JIGP1) = IZI

                  ELSE
                     NGP0 = NGP0 + 1
                     JIGP0 = JIGP0 + 1
                     IGP(JIGP0) = IZI
                  END IF

  210          CONTINUE
            END IF

  200    CONTINUE
  180    JNGP0 = JNGP0 + 1
         NGP(JNGP0) = NGP0
         JNGP1 = JNGP1 - 1
         NGP(JNGP1) = NGP1
C - IN SEMI-INFINITE TRIANGULAR AREA.                                   
         NGP0 = 0
         NGP1 = 0
         ILP1 = MOD(IL0,NL0) + 1
         ILP1T3 = ILP1*3
         IP3 = IPL(ILP1T3-1)
         X3 = XD(IP3)
         Y3 = YD(IP3)
         XMN = XIMN
         XMX = XIMX
         YMN = YIMN
         YMX = YIMX
         IF (Y3.GE.Y2 .AND. Y2.GE.Y1) XMN = X2
         IF (Y3.LE.Y2 .AND. Y2.LE.Y1) XMX = X2
         IF (X3.LE.X2 .AND. X2.LE.X1) YMN = Y2
         IF (X3.GE.X2 .AND. X2.GE.X1) YMX = Y2
         INSD = 0
         DO 290 IXI = 1,NXI0
            IF (XI(IXI).GE.XMN .AND. XI(IXI).LE.XMX) THEN
               IF (INSD.NE.1) THEN
                  INSD = 1
                  IXIMN = IXI
               END IF

            ELSE IF (INSD.NE.0) THEN
               GO TO 300

            END IF

  290    CONTINUE
         IF (INSD.EQ.0) THEN
            GO TO 310

         ELSE
            IXIMX = NXI0
            GO TO 320

         END IF

  300    IXIMX = IXI - 1
  320    DO 330 IYI = 1,NYI0
            YII = YI(IYI)
            IF (YII.GE.YMN .AND. YII.LE.YMX) THEN
               DO 340 IXI = IXIMN,IXIMX
                  XII = XI(IXI)
                  L = 0
                  EXPR = (X1-X2)* (XII-X2) + (Y1-Y2)* (YII-Y2)
                  IF (EXPR.GT.0) GO TO 340
                  IF (EXPR.EQ.0) L = 1
                  EXPR = (X3-X2)* (XII-X2) + (Y3-Y2)* (YII-Y2)
                  IF (EXPR.GT.0) GO TO 340
                  IF (EXPR.EQ.0) L = 1
                  IZI = NXI0* (IYI-1) + IXI
                  IF (L.EQ.1) THEN
                     IF (JIGP1.LE.NXINYI) THEN
                        DO 390 JIGP1I = JIGP1,NXINYI
                           IF (IZI.EQ.IGP(JIGP1I)) GO TO 340
  390                   CONTINUE
                     END IF

                     NGP1 = NGP1 + 1
                     JIGP1 = JIGP1 - 1
                     IGP(JIGP1) = IZI

                  ELSE
                     NGP0 = NGP0 + 1
                     JIGP0 = JIGP0 + 1
                     IGP(JIGP0) = IZI
                  END IF

  340          CONTINUE
            END IF

  330    CONTINUE
  310    JNGP0 = JNGP0 + 1
         NGP(JNGP0) = NGP0
         JNGP1 = JNGP1 - 1
         NGP(JNGP1) = NGP1
  150 CONTINUE
      END


      SUBROUTINE IDPDRV(NDP,XD,YD,ZD,NCP,IPC,PD)
C THIS SUBROUTINE ESTIMATES PARTIAL DERIVATIVES OF THE FIRST AND        
C SECOND ORDER AT THE DATA POINTS.                                      
C THE INPUT PARAMETERS ARE                                              
C     NDP = NUMBER OF DATA POINTS,                                      
C     XD,YD,ZD = ARRAYS OF DIMENSION NDP CONTAINING THE X,              
C           Y, AND Z COORDINATES OF THE DATA POINTS,                    
C     NCP = NUMBER OF ADDITIONAL DATA POINTS USED FOR ESTI-             
C           MATING PARTIAL DERIVATIVES AT EACH DATA POINT,              
C     IPC = INTEGER ARRAY OF DIMENSION NCP*NDP CONTAINING               
C           THE POINT NUMBERS OF NCP DATA POINTS CLOSEST TO             
C           EACH OF THE NDP DATA POINTS.                                
C THE OUTPUT PARAMETER IS                                               
C     PD  = ARRAY OF DIMENSION 5*NDP, WHERE THE ESTIMATED               
C           ZX, ZY, ZXX, ZXY, AND ZYY VALUES AT THE DATA                
C           POINTS ARE TO BE STORED.                                    
      REAL FEPS
      PARAMETER (FEPS = 1.0E-06)
C DECLARATION STATEMENTS                                                
      DIMENSION XD(NDP),YD(NDP),ZD(NDP),IPC(*),PD(*)
      REAL NMX,NMY,NMZ,NMXX,NMXY,NMYX,NMYY
C PRELIMINARY PROCESSING                                                
      NDP0 = NDP
      NCP0 = NCP
      NCPM1 = NCP0 - 1
C ESTIMATION OF ZX AND ZY                                               
      DO 10 IP0 = 1,NDP0
         X0 = XD(IP0)
         Y0 = YD(IP0)
         Z0 = ZD(IP0)
         NMX = 0.0
         NMY = 0.0
         NMZ = 0.0
         JIPC0 = NCP0* (IP0-1)
         DO 20 IC1 = 1,NCPM1
            JIPC = JIPC0 + IC1
            IPI = IPC(JIPC)
            DX1 = XD(IPI) - X0
            DY1 = YD(IPI) - Y0
            DZ1 = ZD(IPI) - Z0
            IC2MN = IC1 + 1
            DO 30 IC2 = IC2MN,NCP0
               JIPC = JIPC0 + IC2
               IPI = IPC(JIPC)
               DX2 = XD(IPI) - X0
               DY2 = YD(IPI) - Y0
               DNMZ = DX1*DY2 - DY1*DX2
               IF (ABS(DNMZ).GT.FEPS) THEN
                  DZ2 = ZD(IPI) - Z0
                  DNMX = DY1*DZ2 - DZ1*DY2
                  DNMY = DZ1*DX2 - DX1*DZ2
                  IF (DNMZ.LT.0.0) THEN
                     DNMX = -DNMX
                     DNMY = -DNMY
                     DNMZ = -DNMZ
                  END IF

                  NMX = NMX + DNMX
                  NMY = NMY + DNMY
                  NMZ = NMZ + DNMZ
               END IF

   30       CONTINUE
   20    CONTINUE
         JPD0 = 5*IP0
         PD(JPD0-4) = -NMX/NMZ
         PD(JPD0-3) = -NMY/NMZ
   10 CONTINUE
C ESTIMATION OF ZXX, ZXY, AND ZYY                                       
      DO 40 IP0 = 1,NDP0
         JPD0 = JPD0 + 5
         X0 = XD(IP0)
         JPD0 = 5*IP0
         Y0 = YD(IP0)
         ZX0 = PD(JPD0-4)
         ZY0 = PD(JPD0-3)
         NMXX = 0.0
         NMXY = 0.0
         NMYX = 0.0
         NMYY = 0.0
         NMZ = 0.0
         JIPC0 = NCP0* (IP0-1)
         DO 50 IC1 = 1,NCPM1
            JIPC = JIPC0 + IC1
            IPI = IPC(JIPC)
            DX1 = XD(IPI) - X0
            DY1 = YD(IPI) - Y0
            JPD = 5*IPI
            DZX1 = PD(JPD-4) - ZX0
            DZY1 = PD(JPD-3) - ZY0
            IC2MN = IC1 + 1
            DO 60 IC2 = IC2MN,NCP0
               JIPC = JIPC0 + IC2
               IPI = IPC(JIPC)
               DX2 = XD(IPI) - X0
               DY2 = YD(IPI) - Y0
               DNMZ = DX1*DY2 - DY1*DX2
               IF (ABS(DNMZ).GT.FEPS) THEN
                  JPD = 5*IPI
                  DZX2 = PD(JPD-4) - ZX0
                  DZY2 = PD(JPD-3) - ZY0
                  DNMXX = DY1*DZX2 - DZX1*DY2
                  DNMXY = DZX1*DX2 - DX1*DZX2
                  DNMYX = DY1*DZY2 - DZY1*DY2
                  DNMYY = DZY1*DX2 - DX1*DZY2
                  IF (DNMZ.LT.0.0) THEN
                     DNMXX = -DNMXX
                     DNMXY = -DNMXY
                     DNMYX = -DNMYX
                     DNMYY = -DNMYY
                     DNMZ = -DNMZ
                  END IF

                  NMXX = NMXX + DNMXX
                  NMXY = NMXY + DNMXY
                  NMYX = NMYX + DNMYX
                  NMYY = NMYY + DNMYY
                  NMZ = NMZ + DNMZ
               END IF

   60       CONTINUE
   50    CONTINUE
         PD(JPD0-2) = -NMXX/NMZ
         PD(JPD0-1) = - (NMXY+NMYX)/ (2.0*NMZ)
         PD(JPD0) = -NMYY/NMZ
   40 CONTINUE
      END


      SUBROUTINE IDPTIP(XD,YD,ZD,NT,IPT,NL,IPL,PDD,ITI,XII,YII,ZII,
     +                  ITPV)
C THIS SUBROUTINE PERFORMS PUNCTUAL INTERPOLATION OR EXTRAPOLA-         
C TION, I.E., DETERMINES THE Z VALUE AT A POINT.                        
C THE INPUT PARAMETERS ARE                                              
C     XD,YD,ZD = ARRAYS OF DIMENSION NDP CONTAINING THE X,              
C           Y, AND Z COORDINATES OF THE DATA POINTS, WHERE              
C           NDP IS THE NUMBER OF THE DATA POINTS,                       
C     NT  = NUMBER OF TRIANGLES,                                        
C     IPT = INTEGER ARRAY OF DIMENSION 3*NT CONTAINING THE              
C           POINT NUMBERS OF THE VERTEXES OF THE TRIANGLES,             
C     NL  = NUMBER OF BORDER LINE SEGMENTS,                             
C     IPL = INTEGER ARRAY OF DIMENSION 3*NL CONTAINING THE              
C           POINT NUMBERS OF THE END POINTS OF THE BORDER               
C           LINE SEGMENTS AND THEIR RESPECTIVE TRIANGLE                 
C           NUMBERS,                                                    
C     PDD = ARRAY OF DIMENSION 5*NDP CONTAINING THE PARTIAL             
C           DERIVATIVES AT THE DATA POINTS,                             
C     ITI = TRIANGLE NUMBER OF THE TRIANGLE IN WHICH LIES               
C           THE POINT FOR WHICH INTERPOLATION IS TO BE                  
C           PERFORMED,                                                  
C     XII,YII = X AND Y COORDINATES OF THE POINT FOR WHICH              
C           INTERPOLATION IS TO BE PERFORMED.                           
C THE OUTPUT PARAMETER IS                                               
C     ZII = INTERPOLATED Z VALUE.                                       
C DECLARATION STATEMENTS                                                
      DIMENSION XD(*),YD(*),ZD(*),IPT(*),IPL(*),PDD(*)
      DIMENSION X(3),Y(3),Z(3),PD(15),ZU(3),ZV(3),ZUU(3),ZUV(3),ZVV(3)
      REAL LU,LV
      EQUIVALENCE (P5,P50)
C PRELIMINARY PROCESSING                                                
      IT0 = ITI
      NTL = NT + NL
      IF (IT0.LE.NTL) THEN
C CALCULATION OF ZII BY INTERPOLATION.                                  
C CHECKS IF THE NECESSARY COEFFICIENTS HAVE BEEN CALCULATED.            
         IF (IT0.NE.ITPV) THEN
C LOADS COORDINATE AND PARTIAL DERIVATIVE VALUES AT THE                 
C VERTEXES.                                                             
            JIPT = 3* (IT0-1)
            JPD = 0
            DO 10 I = 1,3
               JIPT = JIPT + 1
               IDP = IPT(JIPT)
               X(I) = XD(IDP)
               Y(I) = YD(IDP)
               Z(I) = ZD(IDP)
               JPDD = 5* (IDP-1)
               DO 20 KPD = 1,5
                  JPD = JPD + 1
                  JPDD = JPDD + 1
                  PD(JPD) = PDD(JPDD)
   20          CONTINUE
   10       CONTINUE
C DETERMINES THE COEFFICIENTS FOR THE COORDINATE SYSTEM                 
C TRANSFORMATION FROM THE X-Y SYSTEM TO THE U-V SYSTEM                  
C AND VICE VERSA.                                                       
            X0 = X(1)
            Y0 = Y(1)
            A = X(2) - X0
            B = X(3) - X0
            C = Y(2) - Y0
            D = Y(3) - Y0
            AD = A*D
            BC = B*C
            DLT = AD - BC
            AP = D/DLT
            BP = -B/DLT
            CP = -C/DLT
            DP = A/DLT
C CONVERTS THE PARTIAL DERIVATIVES AT THE VERTEXES OF THE               
C TRIANGLE FOR THE U-V COORDINATE SYSTEM.                               
            AA = A*A
            ACT2 = 2.0*A*C
            CC = C*C
            AB = A*B
            ADBC = AD + BC
            CD = C*D
            BB = B*B
            BDT2 = 2.0*B*D
            DD = D*D
            DO 30 I = 1,3
               JPD = 5*I
               ZU(I) = A*PD(JPD-4) + C*PD(JPD-3)
               ZV(I) = B*PD(JPD-4) + D*PD(JPD-3)
               ZUU(I) = AA*PD(JPD-2) + ACT2*PD(JPD-1) + CC*PD(JPD)
               ZUV(I) = AB*PD(JPD-2) + ADBC*PD(JPD-1) + CD*PD(JPD)
               ZVV(I) = BB*PD(JPD-2) + BDT2*PD(JPD-1) + DD*PD(JPD)
   30       CONTINUE
C CALCULATES THE COEFFICIENTS OF THE POLYNOMIAL.                        
            P00 = Z(1)
            P10 = ZU(1)
            P01 = ZV(1)
            P20 = 0.5*ZUU(1)
            P11 = ZUV(1)
            P02 = 0.5*ZVV(1)
            H1 = Z(2) - P00 - P10 - P20
            H2 = ZU(2) - P10 - ZUU(1)
            H3 = ZUU(2) - ZUU(1)
            P30 = 10.0*H1 - 4.0*H2 + 0.5*H3
            P40 = -15.0*H1 + 7.0*H2 - H3
            P50 = 6.0*H1 - 3.0*H2 + 0.5*H3
            H1 = Z(3) - P00 - P01 - P02
            H2 = ZV(3) - P01 - ZVV(1)
            H3 = ZVV(3) - ZVV(1)
            P03 = 10.0*H1 - 4.0*H2 + 0.5*H3
            P04 = -15.0*H1 + 7.0*H2 - H3
            P05 = 6.0*H1 - 3.0*H2 + 0.5*H3
            LU = SQRT(AA+CC)
            LV = SQRT(BB+DD)
            THXU = ATAN2(C,A)
            THUV = ATAN2(D,B) - THXU
            CSUV = COS(THUV)
            P41 = 5.0*LV*CSUV/LU*P50
            P14 = 5.0*LU*CSUV/LV*P05
            H1 = ZV(2) - P01 - P11 - P41
            H2 = ZUV(2) - P11 - 4.0*P41
            P21 = 3.0*H1 - H2
            P31 = -2.0*H1 + H2
            H1 = ZU(3) - P10 - P11 - P14
            H2 = ZUV(3) - P11 - 4.0*P14
            P12 = 3.0*H1 - H2
            P13 = -2.0*H1 + H2
            THUS = ATAN2(D-C,B-A) - THXU
            THSV = THUV - THUS
            AA = SIN(THSV)/LU
            BB = -COS(THSV)/LU
            CC = SIN(THUS)/LV
            DD = COS(THUS)/LV
            AC = AA*CC
            AD = AA*DD
            BC = BB*CC
            G1 = AA*AC* (3.0*BC+2.0*AD)
            G2 = CC*AC* (3.0*AD+2.0*BC)
            H1 = -AA*AA*AA* (5.0*AA*BB*P50+ (4.0*BC+AD)*P41) -
     +           CC*CC*CC* (5.0*CC*DD*P05+ (4.0*AD+BC)*P14)
            H2 = 0.5*ZVV(2) - P02 - P12
            H3 = 0.5*ZUU(3) - P20 - P21
            P22 = (G1*H2+G2*H3-H1)/ (G1+G2)
            P32 = H2 - P22
            P23 = H3 - P22
            ITPV = IT0
         END IF
C CONVERTS XII AND YII TO U-V SYSTEM.                                   
         DX = XII - X0
         DY = YII - Y0
         U = AP*DX + BP*DY
         V = CP*DX + DP*DY
C EVALUATES THE POLYNOMIAL.                                             
         P0 = P00 + V* (P01+V* (P02+V* (P03+V* (P04+V*P05))))
         P1 = P10 + V* (P11+V* (P12+V* (P13+V*P14)))
         P2 = P20 + V* (P21+V* (P22+V*P23))
         P3 = P30 + V* (P31+V*P32)
         P4 = P40 + V*P41
         ZII = P0 + U* (P1+U* (P2+U* (P3+U* (P4+U*P5))))

      ELSE
         IL1 = IT0/NTL
         IL2 = IT0 - IL1*NTL
         IF (IL1.EQ.IL2) THEN
C CALCULATION OF ZII BY EXTRAPOLATION IN THE RECTANGLE.                 
C CHECKS IF THE NECESSARY COEFFICIENTS HAVE BEEN CALCULATED.            
            IF (IT0.NE.ITPV) THEN
C LOADS COORDINATE AND PARTIAL DERIVATIVE VALUES AT THE END             
C POINTS OF THE BORDER LINE SEGMENT.                                    
               JIPL = 3* (IL1-1)
               JPD = 0
               DO 40 I = 1,2
                  JIPL = JIPL + 1
                  IDP = IPL(JIPL)
                  X(I) = XD(IDP)
                  Y(I) = YD(IDP)
                  Z(I) = ZD(IDP)
                  JPDD = 5* (IDP-1)
                  DO 50 KPD = 1,5
                     JPD = JPD + 1
                     JPDD = JPDD + 1
                     PD(JPD) = PDD(JPDD)
   50             CONTINUE
   40          CONTINUE
C DETERMINES THE COEFFICIENTS FOR THE COORDINATE SYSTEM                 
C TRANSFORMATION FROM THE X-Y SYSTEM TO THE U-V SYSTEM                  
C AND VICE VERSA.                                                       
               X0 = X(1)
               Y0 = Y(1)
               A = Y(2) - Y(1)
               B = X(2) - X(1)
               C = -B
               D = A
               AD = A*D
               BC = B*C
               DLT = AD - BC
               AP = D/DLT
               BP = -B/DLT
               CP = -BP
               DP = AP
C CONVERTS THE PARTIAL DERIVATIVES AT THE END POINTS OF THE             
C BORDER LINE SEGMENT FOR THE U-V COORDINATE SYSTEM.                    
               AA = A*A
               ACT2 = 2.0*A*C
               CC = C*C
               AB = A*B
               ADBC = AD + BC
               CD = C*D
               BB = B*B
               BDT2 = 2.0*B*D
               DD = D*D
               DO 60 I = 1,2
                  JPD = 5*I
                  ZU(I) = A*PD(JPD-4) + C*PD(JPD-3)
                  ZV(I) = B*PD(JPD-4) + D*PD(JPD-3)
                  ZUU(I) = AA*PD(JPD-2) + ACT2*PD(JPD-1) + CC*PD(JPD)
                  ZUV(I) = AB*PD(JPD-2) + ADBC*PD(JPD-1) + CD*PD(JPD)
                  ZVV(I) = BB*PD(JPD-2) + BDT2*PD(JPD-1) + DD*PD(JPD)
   60          CONTINUE
C CALCULATES THE COEFFICIENTS OF THE POLYNOMIAL.                        
               P00 = Z(1)
               P10 = ZU(1)
               P01 = ZV(1)
               P20 = 0.5*ZUU(1)
               P11 = ZUV(1)
               P02 = 0.5*ZVV(1)
               H1 = Z(2) - P00 - P01 - P02
               H2 = ZV(2) - P01 - ZVV(1)
               H3 = ZVV(2) - ZVV(1)
               P03 = 10.0*H1 - 4.0*H2 + 0.5*H3
               P04 = -15.0*H1 + 7.0*H2 - H3
               P05 = 6.0*H1 - 3.0*H2 + 0.5*H3
               H1 = ZU(2) - P10 - P11
               H2 = ZUV(2) - P11
               P12 = 3.0*H1 - H2
               P13 = -2.0*H1 + H2
               P21 = 0.0
               P23 = -ZUU(2) + ZUU(1)
               P22 = -1.5*P23
               ITPV = IT0
            END IF
C CONVERTS XII AND YII TO U-V SYSTEM.                                   
            DX = XII - X0
            DY = YII - Y0
            U = AP*DX + BP*DY
            V = CP*DX + DP*DY
C EVALUATES THE POLYNOMIAL.                                             
            P0 = P00 + V* (P01+V* (P02+V* (P03+V* (P04+V*P05))))
            P1 = P10 + V* (P11+V* (P12+V*P13))
            P2 = P20 + V* (P21+V* (P22+V*P23))
            ZII = P0 + U* (P1+U*P2)

         ELSE
C CALCULATION OF ZII BY EXTRAPOLATION IN THE TRIANGLE.                  
C CHECKS IF THE NECESSARY COEFFICIENTS HAVE BEEN CALCULATED.            
            IF (IT0.NE.ITPV) THEN
C LOADS COORDINATE AND PARTIAL DERIVATIVE VALUES AT THE VERTEX          
C OF THE TRIANGLE.                                                      
               JIPL = 3*IL2 - 2
               IDP = IPL(JIPL)
               X(1) = XD(IDP)
               Y(1) = YD(IDP)
               Z(1) = ZD(IDP)
               JPDD = 5* (IDP-1)
               DO 70 KPD = 1,5
                  JPDD = JPDD + 1
                  PD(KPD) = PDD(JPDD)
   70          CONTINUE
C CALCULATES THE COEFFICIENTS OF THE POLYNOMIAL.                        
               P00 = Z(1)
               P10 = PD(1)
               P01 = PD(2)
               P20 = 0.5*PD(3)
               P11 = PD(4)
               P02 = 0.5*PD(5)
               ITPV = IT0
            END IF
C CONVERTS XII AND YII TO U-V SYSTEM.                                   
            U = XII - X(1)
            V = YII - Y(1)
C EVALUATES THE POLYNOMIAL.                                             
            P0 = P00 + V* (P01+V*P02)
            P1 = P10 + V*P11
            ZII = P0 + U* (P1+U*P20)
         END IF

      END IF

      END


      SUBROUTINE IDSFFT(MD,NCP,NDP,XD,YD,ZD,NXI,NYI,XI,YI,ZI,IWK,WK)
C THIS SUBROUTINE PERFORMS SMOOTH SURFACE FITTING WHEN THE PRO-         
C JECTIONS OF THE DATA POINTS IN THE X-Y PLANE ARE IRREGULARLY          
C DISTRIBUTED IN THE PLANE.                                             
C THE INPUT PARAMETERS ARE                                              
C     MD  = MODE OF COMPUTATION (MUST BE 1, 2, OR 3),                   
C         = 1 FOR NEW NCP AND/OR NEW XD-YD,                             
C         = 2 FOR OLD NCP, OLD XD-YD, NEW XI-YI,                        
C         = 3 FOR OLD NCP, OLD XD-YD, OLD XI-YI,                        
C     NCP = NUMBER OF ADDITIONAL DATA POINTS USED FOR ESTI-             
C           MATING PARTIAL DERIVATIVES AT EACH DATA POINT               
C           (MUST BE 2 OR GREATER, BUT SMALLER THAN NDP),               
C     NDP = NUMBER OF DATA POINTS (MUST BE 4 OR GREATER),               
C     XD  = ARRAY OF DIMENSION NDP CONTAINING THE X                     
C           COORDINATES OF THE DATA POINTS,                             
C     YD  = ARRAY OF DIMENSION NDP CONTAINING THE Y                     
C           COORDINATES OF THE DATA POINTS,                             
C     ZD  = ARRAY OF DIMENSION NDP CONTAINING THE Z                     
C           COORDINATES OF THE DATA POINTS,                             
C     NXI = NUMBER OF OUTPUT GRID POINTS IN THE X COORDINATE            
C           (MUST BE 1 OR GREATER),                                     
C     NYI = NUMBER OF OUTPUT GRID POINTS IN THE Y COORDINATE            
C           (MUST BE 1 OR GREATER),                                     
C     XI  = ARRAY OF DIMENSION NXI CONTAINING THE X                     
C           COORDINATES OF THE OUTPUT GRID POINTS,                      
C     YI  = ARRAY OF DIMENSION NYI CONTAINING THE Y                     
C           COORDINATES OF THE OUTPUT GRID POINTS.                      
C THE OUTPUT PARAMETER IS                                               
C     ZI  = DOUBLY-DIMENSIONED ARRAY OF DIMENSION (NXI,NYI),            
C           WHERE THE INTERPOLATED Z VALUES AT THE OUTPUT               
C           GRID POINTS ARE TO BE STORED.                               
C THE OTHER PARAMETERS ARE                                              
C     IWK = INTEGER ARRAY OF DIMENSION                                  
C              MAX0(31,27+NCP)*NDP+NXI*NYI                              
C           USED INTERNALLY AS A WORK AREA,                             
C     WK  = ARRAY OF DIMENSION 5*NDP USED INTERNALLY AS A               
C           WORK AREA.                                                  
C THE VERY FIRST CALL TO THIS SUBROUTINE AND THE CALL WITH A NEW        
C NCP VALUE, A NEW NDP VALUE, AND/OR NEW CONTENTS OF THE XD AND         
C YD ARRAYS MUST BE MADE WITH MD=1.  THE CALL WITH MD=2 MUST BE         
C PRECEDED BY ANOTHER CALL WITH THE SAME NCP AND NDP VALUES AND         
C WITH THE SAME CONTENTS OF THE XD AND YD ARRAYS.  THE CALL WITH        
C MD=3 MUST BE PRECEDED BY ANOTHER CALL WITH THE SAME NCP, NDP,         
C NXI, AND NYI VALUES AND WITH THE SAME CONTENTS OF THE XD, YD,         
C XI, AND YI ARRAYS.  BETWEEN THE CALL WITH MD=2 OR MD=3 AND ITS        
C PRECEDING CALL, THE IWK AND WK ARRAYS MUST NOT BE DISTURBED.          
C USE OF A VALUE BETWEEN 3 AND 5 (INCLUSIVE) FOR NCP IS RECOM-          
C MENDED UNLESS THERE ARE EVIDENCES THAT DICTATE OTHERWISE.             
C THIS SUBROUTINE CALLS THE IDCLDP, IDGRID, IDPDRV, IDPTIP, AND         
C IDTANG SUBROUTINES.                                                   
C DECLARATION STATEMENTS                                                
      DIMENSION XD(NDP),YD(NDP),ZD(NDP),XI(NXI),YI(NYI),ZI(*),
     +          IWK(*),WK(*)
      LOGICAL LINEAR
C SETTING OF SOME INPUT PARAMETERS TO LOCAL VARIABLES.                  
C (FOR MD=1,2,3)                                                        
      MD0 = MD
      NCP0 = NCP
      NDP0 = NDP
      NXI0 = NXI
      NYI0 = NYI
      LINEAR = .FALSE.
      IF (NDP.GT.100) LINEAR = .TRUE.
C ERROR CHECK.  (FOR MD=1,2,3)                                          
      IF (MD0.GE.1 .AND. MD0.LE.3) THEN
         IF (NCP0.GE.2 .AND. NCP0.LT.NDP0) THEN
            IF (NDP0.GE.4) THEN
               IF (NXI0.GE.1 .AND. NYI0.GE.1) THEN
                  IF (MD0.GE.2) THEN
                     NCPPV = IWK(1)
                     NDPPV = IWK(2)
                     IF (NCP0.NE.NCPPV) THEN
                        GO TO 10

                     ELSE IF (NDP0.NE.NDPPV) THEN
                        GO TO 10

                     END IF

                  ELSE
                     IWK(1) = NCP0
                     IWK(2) = NDP0
                  END IF

                  IF (MD0.GE.3) THEN
                     NXIPV = IWK(3)
                     NYIPV = IWK(4)
                     IF (NXI0.NE.NXIPV) THEN
                        GO TO 10

                     ELSE IF (NYI0.NE.NYIPV) THEN
                        GO TO 10

                     END IF

                  ELSE
                     IWK(3) = NXI0
                     IWK(4) = NYI0
                  END IF
C ALLOCATION OF STORAGE AREAS IN THE IWK ARRAY.  (FOR MD=1,2,3)         
                  JWIPT = 16
                  JWIWL = 6*NDP0 + 1
                  JWNGP0 = JWIWL - 1
                  JWIPL = 24*NDP0 + 1
                  JWIWP = 30*NDP0 + 1
                  JWIPC = 27*NDP0 + 1
                  JWIGP0 = MAX0(31,27+NCP0)*NDP0
C TRIANGULATES THE X-Y PLANE.  (FOR MD=1)                               
                  IF (MD0.LE.1) THEN
                     CALL IDTANG(NDP0,XD,YD,NT,IWK(JWIPT),NL,IWK(JWIPL),
     +                           IWK(JWIWL),IWK(JWIWP),WK)
                     IWK(5) = NT
                     IWK(6) = NL
                     IF (NT.EQ.0) RETURN
                  END IF
C DETERMINES NCP POINTS CLOSEST TO EACH DATA POINT.  (FOR MD=1)         
                  IF (MD0.LE.1) THEN
                     CALL IDCLDP(NDP0,XD,YD,NCP0,IWK(JWIPC))
                     IF (IWK(JWIPC).EQ.0) RETURN
                  END IF
C SORTS OUTPUT GRID POINTS IN ASCENDING ORDER OF THE TRIANGLE           
C NUMBER AND THE BORDER LINE SEGMENT NUMBER.  (FOR MD=1,2)              
                  IF (MD0.NE.3) CALL IDGRID(XD,YD,NT,IWK(JWIPT),NL,
     +                                      IWK(JWIPL),NXI0,NYI0,XI,YI,
     +                                      IWK(JWNGP0+1),IWK(JWIGP0+1))
                  IF (LINEAR) THEN
C FIND THE COEFFICENTS FOR LINER INTERPOLATION OF EACH TRIANGLE
                     CALL IDLIN(XD,YD,ZD,NT,IWK(JWIPT),WK)
                  ELSE
C ESTIMATES PARTIAL DERIVATIVES AT ALL DATA POINTS.                     
C (FOR MD=1,2,3)                                                        
                     CALL IDPDRV(NDP0,XD,YD,ZD,NCP0,IWK(JWIPC),WK)
                  END IF

C INTERPOLATES THE ZI VALUES.  (FOR MD=1,2,3)                           
                  ITPV = 0
                  JIG0MX = 0
                  JIG1MN = NXI0*NYI0 + 1
                  NNGP = NT + 2*NL
                  DO 20 JNGP = 1,NNGP
                     ITI = JNGP
                     IF (JNGP.GT.NT) THEN
                        IL1 = (JNGP-NT+1)/2
                        IL2 = (JNGP-NT+2)/2
                        IF (IL2.GT.NL) IL2 = 1
                        ITI = IL1* (NT+NL) + IL2
                     END IF

                     JWNGP = JWNGP0 + JNGP
                     NGP0 = IWK(JWNGP)
                     IF (NGP0.NE.0) THEN
                        JIG0MN = JIG0MX + 1
                        JIG0MX = JIG0MX + NGP0
                        DO 30 JIGP = JIG0MN,JIG0MX
                           JWIGP = JWIGP0 + JIGP
                           IZI = IWK(JWIGP)
                           IYI = (IZI-1)/NXI0 + 1
                           IXI = IZI - NXI0* (IYI-1)
                           IF (LINEAR) THEN
                              CALL IDLCOM(XI(IXI),YI(IYI),ZI(IZI),ITI,
     +                                    XD,YD,ZD,NT,IWK(JWIPT),WK)
                           ELSE
                              CALL IDPTIP(XD,YD,ZD,NT,IWK(JWIPT),NL,
     +                                    IWK(JWIPL),WK,ITI,XI(IXI),
     +                                    YI(IYI),ZI(IZI),ITPV)
                           END IF

   30                   CONTINUE
                     END IF

                     JWNGP = JWNGP0 + 2*NNGP + 1 - JNGP
                     NGP1 = IWK(JWNGP)
                     IF (NGP1.NE.0) THEN
                        JIG1MX = JIG1MN - 1
                        JIG1MN = JIG1MN - NGP1
                        DO 40 JIGP = JIG1MN,JIG1MX
                           JWIGP = JWIGP0 + JIGP
                           IZI = IWK(JWIGP)
                           IYI = (IZI-1)/NXI0 + 1
                           IXI = IZI - NXI0* (IYI-1)
                           IF (LINEAR) THEN
                              CALL IDLCOM(XI(IXI),YI(IYI),ZI(IZI),ITI,
     +                                    XD,YD,ZD,NT,IWK(JWIPT),WK)
                           ELSE
                              CALL IDPTIP(XD,YD,ZD,NT,IWK(JWIPT),NL,
     +                                    IWK(JWIPL),WK,ITI,XI(IXI),
     +                                    YI(IYI),ZI(IZI),ITPV)
                           END IF

   40                   CONTINUE
                     END IF

   20             CONTINUE
                  RETURN

               END IF

            END IF

         END IF

      END IF
C ERROR EXIT                                                            
   10 WRITE (*,FMT=9000) MD0,NCP0,NDP0,NXI0,NYI0
C FORMAT STATEMENT FOR ERROR MESSAGE                                    
 9000 FORMAT (1X,/,' ***   IMPROPER INPUT PARAMETER VALUE(S).',/,
     +       '   MD =',I4,10X,'NCP =',I6,10X,'NDP =',I6,10X,'NXI =',I6,
     +       10X,'NYI =',I6,/,' ERROR DETECTED IN ROUTINE   IDSFFT',/)

      END


      SUBROUTINE IDTANG(NDP,XD,YD,NT,IPT,NL,IPL,IWL,IWP,WK)
C THIS SUBROUTINE PERFORMS TRIANGULATION.  IT DIVIDES THE X-Y           
C PLANE INTO A NUMBER OF TRIANGLES ACCORDING TO GIVEN DATA              
C POINTS IN THE PLANE, DETERMINES LINE SEGMENTS THAT FORM THE           
C BORDER OF DATA AREA, AND DETERMINES THE TRIANGLE NUMBERS              
C CORRESPONDING TO THE BORDER LINE SEGMENTS.                            
C AT COMPLETION, POINT NUMBERS OF THE VERTEXES OF EACH TRIANGLE         
C ARE LISTED COUNTER-CLOCKWISE.  POINT NUMBERS OF THE END POINTS        
C OF EACH BORDER LINE SEGMENT ARE LISTED COUNTER-CLOCKWISE,             
C LISTING ORDER OF THE LINE SEGMENTS BEING COUNTER-CLOCKWISE.           
C THIS SUBROUTINE CALLS THE IDXCHG FUNCTION.                            
C THE INPUT PARAMETERS ARE                                              
C     NDP = NUMBER OF DATA POINTS,                                      
C     XD  = ARRAY OF DIMENSION NDP CONTAINING THE                       
C           X COORDINATES OF THE DATA POINTS,                           
C     YD  = ARRAY OF DIMENSION NDP CONTAINING THE                       
C           Y COORDINATES OF THE DATA POINTS.                           
C THE OUTPUT PARAMETERS ARE                                             
C     NT  = NUMBER OF TRIANGLES,                                        
C     IPT = INTEGER ARRAY OF DIMENSION 6*NDP-15, WHERE THE              
C           POINT NUMBERS OF THE VERTEXES OF THE (IT)TH                 
C           TRIANGLE ARE TO BE STORED AS THE (3*IT-2)ND,                
C           (3*IT-1)ST, AND (3*IT)TH ELEMENTS,                          
C           IT=1,2,...,NT,                                              
C     NL  = NUMBER OF BORDER LINE SEGMENTS,                             
C     IPL = INTEGER ARRAY OF DIMENSION 6*NDP, WHERE THE                 
C           POINT NUMBERS OF THE END POINTS OF THE (IL)TH               
C           BORDER LINE SEGMENT AND ITS RESPECTIVE TRIANGLE             
C           NUMBER ARE TO BE STORED AS THE (3*IL-2)ND,                  
C           (3*IL-1)ST, AND (3*IL)TH ELEMENTS,                          
C           IL=1,2,..., NL.                                             
C THE OTHER PARAMETERS ARE                                              
C     IWL = INTEGER ARRAY OF DIMENSION 18*NDP USED                      
C           INTERNALLY AS A WORK AREA,                                  
C     IWP = INTEGER ARRAY OF DIMENSION NDP USED                         
C           INTERNALLY AS A WORK AREA,                                  
C     WK  = ARRAY OF DIMENSION NDP USED INTERNALLY AS A                 
C           WORK AREA.                                                  
      REAL FEPS
      PARAMETER (FEPS = 1.0E-06)
C DECLARATION STATEMENTS                                                
      DIMENSION XD(NDP),YD(NDP),IPT(*),IPL(*),IWL(*),IWP(NDP),
     +          WK(NDP)
      DIMENSION ITF(2)
      DATA RATIO/1.0E-6/,NREP/100/
C PRELIMINARY PROCESSING                                                
      NDP0 = NDP
      NDPM1 = NDP0 - 1
      IF (NDP0.LT.4) THEN
C ERROR EXIT                                                            
         WRITE (*,FMT=9000) NDP0

      ELSE
C DETERMINES THE CLOSEST PAIR OF DATA POINTS AND THEIR MIDPOINT.        
         DSQMN = (XD(2)-XD(1))**2 + (YD(2)-YD(1))**2
         IPMN1 = 1
         IPMN2 = 2
         DO 10 IP1 = 1,NDPM1
            X1 = XD(IP1)
            Y1 = YD(IP1)
            IP1P1 = IP1 + 1
            DO 20 IP2 = IP1P1,NDP0
               DSQI = (XD(IP2)-X1)**2 + (YD(IP2)-Y1)**2
               IF (ABS(DSQI).LE.FEPS) THEN
                  GO TO 30

               ELSE IF (DSQI.LT.DSQMN) THEN
                  DSQMN = DSQI
                  IPMN1 = IP1
                  IPMN2 = IP2
               END IF

   20       CONTINUE
   10    CONTINUE
         DSQ12 = DSQMN
         XDMP = (XD(IPMN1)+XD(IPMN2))/2.0
         YDMP = (YD(IPMN1)+YD(IPMN2))/2.0
C SORTS THE OTHER (NDP-2) DATA POINTS IN ASCENDING ORDER OF             
C DISTANCE FROM THE MIDPOINT AND STORES THE SORTED DATA POINT           
C NUMBERS IN THE IWP ARRAY.                                             
         JP1 = 2
         DO 40 IP1 = 1,NDP0
            IF (IP1.NE.IPMN1 .AND. IP1.NE.IPMN2) THEN
               JP1 = JP1 + 1
               IWP(JP1) = IP1
               WK(JP1) = (XD(IP1)-XDMP)**2 + (YD(IP1)-YDMP)**2
            END IF

   40    CONTINUE
         DO 50 JP1 = 3,NDPM1
            DSQMN = WK(JP1)
            JPMN = JP1
            DO 60 JP2 = JP1,NDP0
               IF (WK(JP2).LT.DSQMN) THEN
                  DSQMN = WK(JP2)
                  JPMN = JP2
               END IF

   60       CONTINUE
            ITS = IWP(JP1)
            IWP(JP1) = IWP(JPMN)
            IWP(JPMN) = ITS
            WK(JPMN) = WK(JP1)
   50    CONTINUE
C IF NECESSARY, MODIFIES THE ORDERING IN SUCH A WAY THAT THE            
C FIRST THREE DATA POINTS ARE NOT COLLINEAR.                            
         AR = DSQ12*RATIO
         X1 = XD(IPMN1)
         Y1 = YD(IPMN1)
         DX21 = XD(IPMN2) - X1
         DY21 = YD(IPMN2) - Y1
         DO 70 JP = 3,NDP0
            IP = IWP(JP)
            IF (ABS((YD(IP)-Y1)*DX21- (XD(IP)-X1)*DY21).GT.AR) GO TO 80
   70    CONTINUE
         WRITE (*,FMT=9020) NDP0
         GO TO 90

   80    IF (JP.NE.3) THEN
            JPMX = JP
            JP = JPMX + 1
            DO 100 JPC = 4,JPMX
               JP = JP - 1
               IWP(JP) = IWP(JP-1)
  100       CONTINUE
            IWP(3) = IP
         END IF
C FORMS THE FIRST TRIANGLE.  STORES POINT NUMBERS OF THE VER-           
C TEXES OF THE TRIANGLE IN THE IPT ARRAY, AND STORES POINT NUM-         
C BERS OF THE BORDER LINE SEGMENTS AND THE TRIANGLE NUMBER IN           
C THE IPL ARRAY.                                                        
         IP1 = IPMN1
         IP2 = IPMN2
         IP3 = IWP(3)
         IF (((YD(IP3)-YD(IP1))* (XD(IP2)-XD(IP1)) - 
     +        (XD(IP3)-XD(IP1))* (YD(IP2)-YD(IP1)))
     +     .LT.0.0) THEN
            IP1 = IPMN2
            IP2 = IPMN1
         END IF

         NT0 = 1
         NTT3 = 3
         IPT(1) = IP1
         IPT(2) = IP2
         IPT(3) = IP3
         NL0 = 3
         NLT3 = 9
         IPL(1) = IP1
         IPL(2) = IP2
         IPL(3) = 1
         IPL(4) = IP2
         IPL(5) = IP3
         IPL(6) = 1
         IPL(7) = IP3
         IPL(8) = IP1
         IPL(9) = 1
C ADDS THE REMAINING (NDP-3) DATA POINTS, ONE BY ONE.                   
         DO 110 JP1 = 4,NDP0
            IP1 = IWP(JP1)
            X1 = XD(IP1)
            Y1 = YD(IP1)
C - DETERMINES THE VISIBLE BORDER LINE SEGMENTS.                        
            IP2 = IPL(1)
            JPMN = 1
            DXMN = XD(IP2) - X1
            DYMN = YD(IP2) - Y1
            DSQMN = DXMN**2 + DYMN**2
            ARMN = DSQMN*RATIO
            JPMX = 1
            DXMX = DXMN
            DYMX = DYMN
            DSQMX = DSQMN
            ARMX = ARMN
            DO 120 JP2 = 2,NL0
               IP2 = IPL(3*JP2-2)
               DX = XD(IP2) - X1
               DY = YD(IP2) - Y1
               AR = DY*DXMN - DX*DYMN
               IF (AR.LE.ARMN) THEN
                  DSQI = DX**2 + DY**2
                  IF (AR.LT. (-ARMN) .OR. DSQI.LT.DSQMN) THEN
                     JPMN = JP2
                     DXMN = DX
                     DYMN = DY
                     DSQMN = DSQI
                     ARMN = DSQMN*RATIO
                  END IF

               END IF

               AR = DY*DXMX - DX*DYMX
               IF (AR.GE. (-ARMX)) THEN
                  DSQI = DX**2 + DY**2
                  IF (AR.GT.ARMX .OR. DSQI.LT.DSQMX) THEN
                     JPMX = JP2
                     DXMX = DX
                     DYMX = DY
                     DSQMX = DSQI
                     ARMX = DSQMX*RATIO
                  END IF

               END IF

  120       CONTINUE
            IF (JPMX.LT.JPMN) JPMX = JPMX + NL0
            NSH = JPMN - 1
            IF (NSH.GT.0) THEN
C - SHIFTS (ROTATES) THE IPL ARRAY TO HAVE THE INVISIBLE BORDER         
C - LINE SEGMENTS CONTAINED IN THE FIRST PART OF THE IPL ARRAY.         
               NSHT3 = NSH*3
               DO 130 JP2T3 = 3,NSHT3,3
                  JP3T3 = JP2T3 + NLT3
                  IPL(JP3T3-2) = IPL(JP2T3-2)
                  IPL(JP3T3-1) = IPL(JP2T3-1)
                  IPL(JP3T3) = IPL(JP2T3)
  130          CONTINUE
               DO 140 JP2T3 = 3,NLT3,3
                  JP3T3 = JP2T3 + NSHT3
                  IPL(JP2T3-2) = IPL(JP3T3-2)
                  IPL(JP2T3-1) = IPL(JP3T3-1)
                  IPL(JP2T3) = IPL(JP3T3)
  140          CONTINUE
               JPMX = JPMX - NSH
            END IF
C - ADDS TRIANGLES TO THE IPT ARRAY, UPDATES BORDER LINE                
C - SEGMENTS IN THE IPL ARRAY, AND SETS FLAGS FOR THE BORDER            
C - LINE SEGMENTS TO BE REEXAMINED IN THE IWL ARRAY.                    
            JWL = 0
            DO 150 JP2 = JPMX,NL0
               JP2T3 = JP2*3
               IPL1 = IPL(JP2T3-2)
               IPL2 = IPL(JP2T3-1)
               IT = IPL(JP2T3)
C - - ADDS A TRIANGLE TO THE IPT ARRAY.                                 
               NT0 = NT0 + 1
               NTT3 = NTT3 + 3
               IPT(NTT3-2) = IPL2
               IPT(NTT3-1) = IPL1
               IPT(NTT3) = IP1
C - - UPDATES BORDER LINE SEGMENTS IN THE IPL ARRAY.                    
               IF (JP2.EQ.JPMX) THEN
                  IPL(JP2T3-1) = IP1
                  IPL(JP2T3) = NT0
               END IF

               IF (JP2.EQ.NL0) THEN
                  NLN = JPMX + 1
                  NLNT3 = NLN*3
                  IPL(NLNT3-2) = IP1
                  IPL(NLNT3-1) = IPL(1)
                  IPL(NLNT3) = NT0
               END IF
C - - DETERMINES THE VERTEX THAT DOES NOT LIE ON THE BORDER             
C - - LINE SEGMENTS.                                                    
               ITT3 = IT*3
               IPTI = IPT(ITT3-2)
               IF (IPTI.EQ.IPL1 .OR. IPTI.EQ.IPL2) THEN
                  IPTI = IPT(ITT3-1)
                  IF (IPTI.EQ.IPL1 .OR. IPTI.EQ.IPL2) IPTI = IPT(ITT3)
               END IF
C - - CHECKS IF THE EXCHANGE IS NECESSARY.                              
               IF (IDXCHG(XD,YD,IP1,IPTI,IPL1,IPL2).NE.0) THEN
C - - MODIFIES THE IPT ARRAY WHEN NECESSARY.                            
                  IPT(ITT3-2) = IPTI
                  IPT(ITT3-1) = IPL1
                  IPT(ITT3) = IP1
                  IPT(NTT3-1) = IPTI
                  IF (JP2.EQ.JPMX) IPL(JP2T3) = IT
                  IF (JP2.EQ.NL0 .AND. IPL(3).EQ.IT) IPL(3) = NT0
C - - SETS FLAGS IN THE IWL ARRAY.                                      
                  JWL = JWL + 4
                  IWL(JWL-3) = IPL1
                  IWL(JWL-2) = IPTI
                  IWL(JWL-1) = IPTI
                  IWL(JWL) = IPL2
               END IF

  150       CONTINUE
            NL0 = NLN
            NLT3 = NLNT3
            NLF = JWL/2
            IF (NLF.NE.0) THEN
C - IMPROVES TRIANGULATION.                                             
               NTT3P3 = NTT3 + 3
               DO 160 IREP = 1,NREP
                  DO 170 ILF = 1,NLF
                     ILFT2 = ILF*2
                     IPL1 = IWL(ILFT2-1)
                     IPL2 = IWL(ILFT2)
C - - LOCATES IN THE IPT ARRAY TWO TRIANGLES ON BOTH SIDES OF           
C - - THE FLAGGED LINE SEGMENT.                                         
                     NTF = 0
                     DO 180 ITT3R = 3,NTT3,3
                        ITT3 = NTT3P3 - ITT3R
                        IPT1 = IPT(ITT3-2)
                        IPT2 = IPT(ITT3-1)
                        IPT3 = IPT(ITT3)
                        IF (IPL1.EQ.IPT1 .OR. IPL1.EQ.IPT2 .OR.
     +                      IPL1.EQ.IPT3) THEN
                           IF (IPL2.EQ.IPT1 .OR. IPL2.EQ.IPT2 .OR.
     +                         IPL2.EQ.IPT3) THEN
                              NTF = NTF + 1
                              ITF(NTF) = ITT3/3
                              IF (NTF.EQ.2) GO TO 190
                           END IF

                        END IF

  180                CONTINUE
                     IF (NTF.LT.2) GO TO 170
C - - DETERMINES THE VERTEXES OF THE TRIANGLES THAT DO NOT LIE          
C - - ON THE LINE SEGMENT.                                              
  190                IT1T3 = ITF(1)*3
                     IPTI1 = IPT(IT1T3-2)
                     IF (IPTI1.EQ.IPL1 .OR. IPTI1.EQ.IPL2) THEN
                        IPTI1 = IPT(IT1T3-1)
                        IF (IPTI1.EQ.IPL1 .OR.
     +                      IPTI1.EQ.IPL2) IPTI1 = IPT(IT1T3)
                     END IF

                     IT2T3 = ITF(2)*3
                     IPTI2 = IPT(IT2T3-2)
                     IF (IPTI2.EQ.IPL1 .OR. IPTI2.EQ.IPL2) THEN
                        IPTI2 = IPT(IT2T3-1)
                        IF (IPTI2.EQ.IPL1 .OR.
     +                      IPTI2.EQ.IPL2) IPTI2 = IPT(IT2T3)
                     END IF
C - - CHECKS IF THE EXCHANGE IS NECESSARY.                              
                     IF (IDXCHG(XD,YD,IPTI1,IPTI2,IPL1,IPL2).NE.0) THEN
C - - MODIFIES THE IPT ARRAY WHEN NECESSARY.                            
                        IPT(IT1T3-2) = IPTI1
                        IPT(IT1T3-1) = IPTI2
                        IPT(IT1T3) = IPL1
                        IPT(IT2T3-2) = IPTI2
                        IPT(IT2T3-1) = IPTI1
                        IPT(IT2T3) = IPL2
C - - SETS NEW FLAGS.                                                   
                        JWL = JWL + 8
                        IWL(JWL-7) = IPL1
                        IWL(JWL-6) = IPTI1
                        IWL(JWL-5) = IPTI1
                        IWL(JWL-4) = IPL2
                        IWL(JWL-3) = IPL2
                        IWL(JWL-2) = IPTI2
                        IWL(JWL-1) = IPTI2
                        IWL(JWL) = IPL1
                        DO 200 JLT3 = 3,NLT3,3
                           IPLJ1 = IPL(JLT3-2)
                           IPLJ2 = IPL(JLT3-1)
                           IF ((IPLJ1.EQ.IPL1.AND.IPLJ2.EQ.IPTI2) .OR.
     +                         (IPLJ2.EQ.IPL1.AND.IPLJ1.EQ.
     +                         IPTI2)) IPL(JLT3) = ITF(1)
                           IF ((IPLJ1.EQ.IPL2.AND.IPLJ2.EQ.IPTI1) .OR.
     +                         (IPLJ2.EQ.IPL2.AND.IPLJ1.EQ.
     +                         IPTI1)) IPL(JLT3) = ITF(2)
  200                   CONTINUE
                     END IF

  170             CONTINUE
                  NLFC = NLF
                  NLF = JWL/2
                  IF (NLF.EQ.NLFC) THEN
                     GO TO 110

                  ELSE
C - - RESETS THE IWL ARRAY FOR THE NEXT ROUND.                          
                     JWL = 0
                     JWL1MN = (NLFC+1)*2
                     NLFT2 = NLF*2
                     DO 210 JWL1 = JWL1MN,NLFT2,2
                        JWL = JWL + 2
                        IWL(JWL-1) = IWL(JWL1-1)
                        IWL(JWL) = IWL(JWL1)
  210                CONTINUE
                     NLF = JWL/2
                  END IF

  160          CONTINUE
            END IF

  110    CONTINUE
C REARRANGES THE IPT ARRAY SO THAT THE VERTEXES OF EACH TRIANGLE        
C ARE LISTED COUNTER-CLOCKWISE.                                         
         DO 220 ITT3 = 3,NTT3,3
            IP1 = IPT(ITT3-2)
            IP2 = IPT(ITT3-1)
            IP3 = IPT(ITT3)
            IF (((YD(IP3)-YD(IP1))* (XD(IP2)-XD(IP1)) - 
     +           (XD(IP3)-XD(IP1))* (YD(IP2)-YD(IP1)))
     +           .LT.0.0) THEN
               IPT(ITT3-2) = IP2
               IPT(ITT3-1) = IP1
            END IF

  220    CONTINUE
         NT = NT0
         NL = NL0
         RETURN

   30    WRITE (*,FMT=9010) NDP0,IP1,IP2,X1,Y1
      END IF

   90 WRITE (*,FMT=9030)
      NT = 0
C FORMAT STATEMENTS                                                     
 9000 FORMAT (1X,/,' ***   NDP LESS THAN 4.',/,'   NDP =',I5)
 9010 FORMAT (1X,/,' ***   IDENTICAL DATA POINTS.',/,'   NDP =',I5,5X,
     +       'IP1 =',I5,5X,'IP2 =',I5,5X,'XD =',E12.4,5X,'YD =',E12.4)
 9020 FORMAT (1X,/,' ***   ALL COLLINEAR DATA POINTS.',/,'   NDP =',I5)
 9030 FORMAT (' ERROR DETECTED IN ROUTINE   IDTANG',/)

      END


      FUNCTION IDXCHG(X,Y,I1,I2,I3,I4)
C THIS FUNCTION DETERMINES WHETHER OR NOT THE EXCHANGE OF TWO           
C TRIANGLES IS NECESSARY ON THE BASIS OF MAX-MIN-ANGLE CRITERION        
C BY C. L. LAWSON.                                                      
C THE INPUT PARAMETERS ARE                                              
C     X,Y = ARRAYS CONTAINING THE COORDINATES OF THE DATA               
C           POINTS,                                                     
C     I1,I2,I3,I4 = POINT NUMBERS OF FOUR POINTS P1, P2,                
C           P3, AND P4 THAT FORM A QUADRILATERAL WITH P3                
C           AND P4 CONNECTED DIAGONALLY.                                
C THIS FUNCTION RETURNS AN INTEGER VALUE 1 (ONE) WHEN AN EX-            
C CHANGE IS NECESSARY, AND 0 (ZERO) OTHERWISE.                          
C DECLARATION STATEMENTS                                                
      DIMENSION X(*),Y(*)
      EQUIVALENCE (C2SQ,C1SQ), (A3SQ,B2SQ), (B3SQ,A1SQ), (A4SQ,B1SQ),
     +            (B4SQ,A2SQ), (C4SQ,C3SQ)
C PRELIMINARY PROCESSING                                                
      X1 = X(I1)
      Y1 = Y(I1)
      X2 = X(I2)
      Y2 = Y(I2)
      X3 = X(I3)
      Y3 = Y(I3)
      X4 = X(I4)
      Y4 = Y(I4)
C CALCULATION                                                           
      IDX = 0
      U3 = (Y2-Y3)* (X1-X3) - (X2-X3)* (Y1-Y3)
      U4 = (Y1-Y4)* (X2-X4) - (X1-X4)* (Y2-Y4)
      IF (U3*U4.GT.0.0) THEN
         U1 = (Y3-Y1)* (X4-X1) - (X3-X1)* (Y4-Y1)
         U2 = (Y4-Y2)* (X3-X2) - (X4-X2)* (Y3-Y2)
         A1SQ = (X1-X3)**2 + (Y1-Y3)**2
         B1SQ = (X4-X1)**2 + (Y4-Y1)**2
         C1SQ = (X3-X4)**2 + (Y3-Y4)**2
         A2SQ = (X2-X4)**2 + (Y2-Y4)**2
         B2SQ = (X3-X2)**2 + (Y3-Y2)**2
         C3SQ = (X2-X1)**2 + (Y2-Y1)**2
         S1SQ = U1*U1/ (C1SQ*AMAX1(A1SQ,B1SQ))
         S2SQ = U2*U2/ (C2SQ*AMAX1(A2SQ,B2SQ))
         S3SQ = U3*U3/ (C3SQ*AMAX1(A3SQ,B3SQ))
         S4SQ = U4*U4/ (C4SQ*AMAX1(A4SQ,B4SQ))
         IF (AMIN1(S1SQ,S2SQ).LT.AMIN1(S3SQ,S4SQ)) IDX = 1
      END IF

      IDXCHG = IDX
      END


      SUBROUTINE IDLIN(XD,YD,ZD,NT,IWK,WK)
C THIS ROUTINE GENERATES THE COORDINATES USED IN A LINEAR INTERPOLATION
C OF THE TRIANGLES CREATED FROM IRREGULARLY DISTRIBUTED DATA.
      DIMENSION IWK(*),WK(*),XD(*),YD(*),ZD(*)
C   LOOP FOR ALL TRIANGLES

      DO 10 ITRI = 1,NT
C GET THE POINTS OF THE TRIANGLE
         IPOINT = (ITRI-1)*3
         IP1 = IWK(IPOINT+1)
         IP2 = IWK(IPOINT+2)
         IP3 = IWK(IPOINT+3)
C GET THE VALUES AT THE TRIANBGLE POINTS
         X1 = XD(IP1)
         Y1 = YD(IP1)
         Z1 = ZD(IP1)
         X2 = XD(IP2)
         Y2 = YD(IP2)
         Z2 = ZD(IP2)
         X3 = XD(IP3)
         Y3 = YD(IP3)
         Z3 = ZD(IP3)
C COMPUTE THE INTERPLOATING COEFICIENTS
         WK(IPOINT+1) = (Y2-Y1)*(Z3-Z1)-(Y3-Y1)*(Z2-Z1)
         WK(IPOINT+2) = (X3-X1)*(Z2-Z1)-(X2-X1)*(Z3-Z1)
         WK(IPOINT+3) = (X3-X1)*(Y2-Y1)-(X2-X1)*(Y3-Y1)

   10 CONTINUE
      END


      SUBROUTINE IDLCOM(X,Y,Z,ITRI,XD,YD,ZD,NT,IWK,WK)
C COMPUTE A Z VALUE FOR A GIVEN X,Y VALUE
      DIMENSION XD(*),YD(*),ZD(*),IWK(*),WK(*)
C IF OUTSIDE CONVEX HULL DON'T COMPUTE A VALUE
      IF (ITRI.LE.NT) THEN
         IPOINT = (ITRI-1)*3
         IV = IWK(IPOINT+1)
         X1 = X - XD(IV)
         Y1 = Y - YD(IV)
         Z1 = ZD(IV)
C COMPUTE THE Z VALUE
         Z = (WK(IPOINT+1)*X1+WK(IPOINT+2)*Y1)/WK(IPOINT+3) + Z1
      END IF

      END
