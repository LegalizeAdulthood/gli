        program mkfont

        integer irec

	open (unit=10, name='gksfont.dat', type='unknown', 
     *    form='unformatted', access='direct', recordtype='fixed', 
     *	  recordsize=64)

        call hersh (irec)
        call convf ('fillfont.dat', irec)
        call convf ('math.dat', irec)

	close (unit=10)

        end

	subroutine hersh (irec)

        integer irec

	integer nfonts, cp(95)
	integer sv(188), b(10000)

	byte left, right, size, bottom, base, cap, top, length, c(248)
	logical newlin

	integer german(6)
	data german / 65, 79, 85, 97, 111, 117 /

	integer uml, umlaut(10)
	data umlaut / 0, 1, -1, 0, 0, -1, 1, 0, 0, 1 /


	open (unit=1, name='hershey.dat', status='old', readonly)
	read (1, *) nfonts

	irec = 0

	do i = 1, nfonts

	    write (*, *) i

	    read (1, *) size, bottom, base, cap, top, cp
	    read (1, *) sv, (b(j), j=1, 12)

	    n = 12
   2	    read (1, 20) (b(n+j), j=1, 20)
  20	    format (20i4)

	    if (b(n+1) .ne. 9999) then
		n = n+20
		goto 2
	    end if

	    left = 0
	    right = size
	    length = 0
	    do k = 1, 248
		c(k) = 0
	    end do

	    irec = irec+1
	    write (10, rec=irec) left, right, size,
     *        bottom, base, cap, top, length, c

	    l = 1

	    do j = 1, 94

		left = sv(l)
		right = sv(l+1)
		length = (cp(j+1)-cp(j))/2
		if (length .gt. 124) stop 'Error 1'

		m = 0
		newlin = .true.

		do k = cp(j), cp(j+1)-2, 2
		    m = m + 1
		    ix = b(k)
		    if (b(k) .ge. 128) b(k) = b(k)-128
		    c(m) = b(k)
		    if (newlin) c(m) = -c(m)
		    newlin = ix.ge.128
		    m = m + 1
		    c(m) = b(k+1)
		end do
		do k = m+1, 248
		    c(k) = 0
		end do

		uml = 0
		do k = 1, 6
		  if (j .eq. german(k)-32) uml = k
		end do

		if (uml .ne. 0) then

		  iy = 0
		  do k = 1, length
		    m = 2*k
		    if (c(m) .gt. iy) then
		      iy = c(m)
		      icentr = 0
		      np = 0
		    end if
		    if (c(m) .eq. iy) then
		      ix = c(m-1)
		      if (ix .lt. 0) ix = -ix
		      icentr = icentr + ix
		      np = np + 1
		    end if
		  end do

		  icentr = icentr/np
		  iy  = iy+size/4
		  m = length
		  if (m .gt. 124-20) stop 'Error 2'
		  m = m*2

		  if (uml .le. 3) then
		    ix = icentr - (right-left)/3
		  else
		    ix = icentr - (right-left)/4
		  end if

		  isign = -1
		  do k = 1, 10, 2
		    m = m + 2
		    c(m-1) = isign*(ix+umlaut(k))
		    c(m)   =        iy+umlaut(k+1)
		    isign = 1
		  end do

		  if (uml .le. 3) then
		    ix = icentr + (right-left)/3
		  else
		    ix = icentr + (right-left)/4
		  end if

		  isign = -1
		  do k = 1, 10, 2
		    m = m + 2
		    c(m-1) = isign*(ix+umlaut(k))
		    c(m)   =        iy+umlaut(k+1)
		    isign = 1
		  end do

		end if

		if (left .eq. right) then
		    left = left - size/2
		    right = right + (size+1)/2
		end if

		l = l+2
		irec = irec+1
		write (10, rec=irec) left, right, size,
     *            bottom, base, cap, top, length, c

	    end do
	end do

	close (unit=1)
	end

        subroutine convf (name, irec)
c
        character*(*) name
        integer irec
c
	integer i, j, k, nchars, ncoord
	character ch(95), cbuf(256, 95)
	integer offset(95), length(95), width(95)
	integer ibuf(6000)
        integer left, right, bottom, base, cap, top, dum
c
	open (unit=1, name=name, status='old', readonly)
c
	read (1, *) nchars
	do 1 i = 1, nchars
	  read (1, '(a1, 3i6)') ch(i), offset(i), length(i), width(i)
   1	continue
	read (1, *) ncoord
	do 2 i = 1, ncoord, 10
          k = 9
          if (i+k .gt. ncoord) k = ncoord-i
          read (1, '(10i6)') (ibuf(i+j), j = 0, k)
   2    continue
c
	do 3 i = 1, nchars
	  call mkbuf (char(ichar(' ')+i-1), width(i), length(i), 
     *      ibuf(offset(i)), cbuf(1, i))
   3    continue
c
        call chext ('A', width, cbuf, dum, dum, base, cap)
        call chext ('{', width, cbuf, dum, dum, bottom, top)
c
        write (*, *) (irec-1)/95 + 2
c
	do 4 i = 1, nchars
          call chext (char(ichar(' ')+i-1), width, cbuf, left, right, 
     *      dum, dum)
          cbuf(1, i) = char(left)
          cbuf(2, i) = char(right)
          cbuf(3, i) = char(cap-base)
          cbuf(4, i) = char(bottom)
          cbuf(5, i) = char(base)
          cbuf(6, i) = char(cap)
          cbuf(7, i) = char(top)
          irec = irec+1
          write (10, rec=irec) (cbuf(j, i), j=1, 256)
   4    continue
c
	close (unit=1)
	end
c
        subroutine mkbuf (ch, width, length, ibuf, cbuf)
c
        character ch
        integer width, length, ibuf(1)
        character cbuf(1)
c
	integer i, ic, ix, iy
        logical move
c
        cbuf(3) = char(width)
        cbuf(8) = char(length)
c
        if (index('AOUaou', ch) .ne. 0) length = length+10
c
	do 1 i = 1, length
          j = 9 + 2*(i-1)
	  ic = ibuf(i)
          move = ic .lt. 16384
          ic = mod(ic, 16384)
	  ix = ic/128
	  iy = mod(ic, 128)
	  if (ix .gt. 64) ix = 64-ix
	  if (iy .gt. 64) iy = 64-iy
          if (move) then
            cbuf(j) = char(256-(48+ix))
          else
            cbuf(j) = char(48+ix)
          endif
          cbuf(j+1) = char(48+iy)
   1    continue
c
        j = 9 + 2*length
        do 2 i = j, 256
          cbuf(i) = char(0)
   2    continue
c
        end
c
        subroutine chext (ch, width, cbuf, minx, maxx, miny, maxy)
c
        character ch, cbuf(256, 95)
        integer width(95), minx, maxx, miny, maxy
c
	integer ic, i, length, ix, iy
        logical move
c
        ic = ichar(ch)-ichar(' ')+1
c
        minx = 999
        maxx = -999
        miny = 999
        maxy = -999
        length = ichar(cbuf(8, ic))
c
	do 1 i = 1, length
          j = 9 + 2*(i-1)
	  ix = ichar(cbuf(j, ic))
          iy = ichar(cbuf(j+1, ic))
          if (ix .gt. 127) ix = 256-ix
          minx = min(minx, ix)
          maxx = max(maxx, ix)
          miny = min(miny, iy)
          maxy = max(maxy, iy)
   1    continue
c
        if (length .ne. 0) then
            minx = (minx+maxx)/2-width(ic)/2
        else
            minx = 48
        endif
        maxx = minx+width(ic)
c
        end
