MODULE gus;

TYPE
	cstring = [UNSAFE] PACKED ARRAY[1..255] OF CHAR;


[ASYNCHRONOUS] FUNCTION gustx_(
   %REF px,
	py : REAL;
   %REF chars : cstring;
    VAR status : INTEGER := %IMMED 0) : INTEGER; EXTERNAL;


[GLOBAL] FUNCTION gus_text_p(
    px, py : REAL;
    chars : varying[u] OF CHAR;
    VAR status : INTEGER) : INTEGER;

VAR
    s : cstring;
    result : INTEGER;
 
BEGIN
   s := SUBSTR(chars.body, 1, chars.length) + CHR(0);
   gus_text_p := gustx_(px, py, s, result);
   if (iaddress(status) <> 0) THEN
      status := result;
END;



END. { gus }
