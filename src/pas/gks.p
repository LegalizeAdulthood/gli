MODULE gks;

TYPE
	cstring = [UNSAFE] PACKED ARRAY[1..255] OF CHAR;


[ASYNCHRONOUS] FUNCTION gtxs_(
   %REF px,
	py : REAL;
   %REF nchars : INTEGER;
   %REF chars : cstring) : INTEGER; EXTERNAL;

[ASYNCHRONOUS] PROCEDURE gkopen_(
    VAR conid : INTEGER;
   %REF chars : cstring); EXTERNAL;


[GLOBAL] FUNCTION gks_text_p(
    px, py : REAL;
    chars : VARYING[u] OF CHAR) : INTEGER;

VAR
    s : cstring;
    nchars : INTEGER;
 
BEGIN
   nchars := chars.length;
   s := SUBSTR(chars.body, 1, nchars) + CHR(0);
   gks_text_p := gtxs_(px, py, nchars, s);
END;



[GLOBAL] FUNCTION gks_open_connection_p(
    name : VARYING[u] OF CHAR) : INTEGER;

VAR
    conid : INTEGER;
    s : cstring;
 
BEGIN
   s := SUBSTR(name.body, 1, name.length) + CHR(0);
   gkopen_(conid, s);
   gks_open_connection_p := conid;
END;



END. { gks }
