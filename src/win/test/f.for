        subroutine fsub(i, r, s)
        integer i
        real r
        character*(*) s
        write(*,*)i, r, s
        i = 123
        r = 47.11
        s = 'Hello C'
        call csub(i, r, s)
        end
