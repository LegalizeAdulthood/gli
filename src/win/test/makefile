default: c.exe

f.obj: f.for
#        DF /c f.for
        FL32 /c f.for

c.exe: c.c f.obj
        CL c.c f.obj

clean:
        del *.exe
        del *.obj
        del *.bak
