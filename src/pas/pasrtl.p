MODULE pasrtl(output);

TYPE
	cstring = [UNSAFE] PACKED ARRAY[1..255] OF CHAR;
	string = VARYING[255] OF CHAR;

[ASYNCHRONOUS] PROCEDURE str_dec(
   %REF chars : cstring;
 %IMMED value : INTEGER); EXTERNAL;

[ASYNCHRONOUS] PROCEDURE str_flt(
   %REF chars : cstring;
 %IMMED value : REAL); EXTERNAL;

[ASYNCHRONOUS] FUNCTION str_integer(
   %REF chars : cstring;
    VAR status : INTEGER := %IMMED 0) : INTEGER; EXTERNAL;

[ASYNCHRONOUS] FUNCTION str_real(
   %REF chars : cstring;
    VAR status : INTEGER := %IMMED 0) : REAL; EXTERNAL;

[ASYNCHRONOUS] FUNCTION str_match(
   %REF candidate, pattern : cstring;
 %IMMED abbreviation : BOOLEAN) : BOOLEAN; EXTERNAL;

[ASYNCHRONOUS] PROCEDURE var_define(
   %REF name : cstring;
 %IMMED index : INTEGER := %IMMED 0;
 %IMMED length : INTEGER := %IMMED 1;
        buffer : ARRAY[_L4.._H4: INTEGER] OF REAL;
 %IMMED segment : BOOLEAN := %IMMED FALSE;
    VAR status : INTEGER := %IMMED 0); EXTERNAL;

[ASYNCHRONOUS] PROCEDURE var_delete(
   %REF name : cstring;
    VAR status : INTEGER := %IMMED 0); EXTERNAL;

[ASYNCHRONOUS] FUNCTION var_address(
   %REF name : cstring;
    VAR status : INTEGER := %IMMED 0) : POINTER; EXTERNAL;

[ASYNCHRONOUS] PROCEDURE fun_define(
   %REF name : cstring;
   %REF expression : cstring;
    VAR status : INTEGER := %IMMED 0); EXTERNAL;

[ASYNCHRONOUS] PROCEDURE fun_delete(
   %REF name : cstring;
    VAR status : INTEGER := %IMMED 0); EXTERNAL;

[ASYNCHRONOUS] FUNCTION fun_address(
   %REF name : cstring;
    VAR status : INTEGER := %IMMED 0) : POINTER; EXTERNAL;

[ASYNCHRONOUS] FUNCTION system(
   %REF command : cstring) : INTEGER; EXTERNAL;


FUNCTION strlen(s : cstring) : INTEGER;

VAR
    i : INTEGER := 1;
    length : INTEGER := 0;

BEGIN
    WHILE ((i < 256) AND (length = 0)) DO
        IF (s[i] = CHR(0)) THEN
            length := i - 1
        ELSE
            i := i + 1;

    strlen := length;
END; { strlen }
   

[GLOBAL] FUNCTION str_dec_p(value : INTEGER) : string;

VAR
    s : cstring;
    result : [volatile] string;

BEGIN
    str_dec(s, value);
    result.length := strlen(s);
    result.body := s;
    str_dec_p := result;
END; { str_dec_p }
   


[GLOBAL] FUNCTION str_flt_p(value : REAL) : string;

VAR
    s : cstring;
    result : [volatile] string;

BEGIN
    str_flt(s, value);
    result.length := strlen(s);
    result.body := s;
    str_flt_p := result;
END; { str_flt_p }


[GLOBAL] FUNCTION str_integer_p(chars : string; VAR status : INTEGER) : INTEGER;

VAR
    s : cstring;
 
BEGIN
   s := SUBSTR(chars.body, 1, chars.length) + CHR(0);
   str_integer_p := str_integer(s, status);
END;



[GLOBAL] FUNCTION str_real_p(chars : string; VAR status : INTEGER) : REAL ;

VAR
    s : cstring;
 
BEGIN
   s := SUBSTR(chars.body, 1, chars.length) + CHR(0);
   str_real_p := str_real(s, status);
END; { str_real_p }
   

[GLOBAL] FUNCTION str_cap_p(chars : string) : string;

VAR
   read_index : INTEGER;
   ch : CHAR;

BEGIN

FOR read_index := 1 TO LENGTH(chars) DO
   BEGIN
   ch := chars[read_index];
   IF ch IN ['a'..'z']
   THEN
      chars[read_index] := CHR(ORD(ch) + ORD('A') - ORD('a'));
   END;

str_cap_p := chars;

END; { str_cap_p }



[GLOBAL] FUNCTION str_match_p(
    candidate, pattern : string; abbreviation : BOOLEAN) : BOOLEAN;

VAR
   s1, s2 : cstring;
   result : [volatile] BOOLEAN;

BEGIN
   s1 := SUBSTR(candidate.body, 1, candidate.length) + CHR(0);
   s2 := SUBSTR(pattern.body, 1, pattern.length) + CHR(0);
   result := str_match(s1, s2, abbreviation);
   str_match_p := result;
END; { str_match_p }



[GLOBAL] FUNCTION str_pad_p(
    chars : string;
    fill  : CHAR;
    size  : INTEGER) : string;

VAR
   res_str : string;
   n	   : INTEGER;

BEGIN

   n := strlen(chars);   
   IF n <= size THEN
      BEGIN
      res_str := chars;

      WHILE length(res_str) < size DO
	 res_str := res_str + fill;
      END
   ELSE
      res_str := SUBSTR(chars, 1, size);

   str_pad_p := SUBSTR(res_str, 1, size);

END; { str_pad_p }


[GLOBAL] FUNCTION str_element_p(
    element : INTEGER;
    delimiter : CHAR;
    chars : string) : string;

VAR
    	start_index, end_index : INTEGER;
BEGIN

start_index := 1;
end_index := 0;

WHILE (element > 0) AND (end_index < length(chars)) DO
    BEGIN
    end_index := end_index + 1;

    IF SUBSTR(chars, end_index, 1) = delimiter
    THEN
	BEGIN
	element := element - 1;

	IF element <> 0
	THEN
	    start_index := end_index + 1;
	END;
    END;

IF (element = 1) AND (end_index = length(chars))
THEN
    end_index := end_index + 1

ELSE IF element <> 0
THEN
    start_index := length(chars);

IF start_index <= length(chars)
THEN
    str_element_p := SUBSTR(chars, start_index, end_index - start_index)
ELSE
    str_element_p := '';

END; { str_element }


[GLOBAL] FUNCTION str_remove_p(
    chars : string;
    ch : CHAR := ' ';
    all : BOOLEAN := FALSE) : string;

var
    	start_index, end_index : INTEGER;
BEGIN

start_index := 1;
end_index := length(chars);

WHILE (end_index > 0) AND (SUBSTR(chars, end_index, 1) = ch) DO
    end_index := end_index - 1;

IF all THEN
    WHILE (start_index < end_index) AND (SUBSTR(chars, start_index, 1) = ch) DO
	start_index := start_index + 1;

str_remove_p := SUBSTR(chars, start_index, end_index - start_index + 1);

END; { str_remove_p }



[GLOBAL] PROCEDURE var_define_p(
    name : string;
    index : INTEGER := 0;
    length : INTEGER := 1;
    VAR buffer : ARRAY[_L4.._U4: INTEGER] OF REAL;
    segment : BOOLEAN := FALSE;
    VAR status : INTEGER);

VAR
    s : cstring;
 
BEGIN
   s := SUBSTR(name.body, 1, name.length) + CHR(0);
   var_define(s, index, length, buffer, segment, status);
END;



[GLOBAL] PROCEDURE var_delete_p(
    name : string;
    VAR status : INTEGER);

VAR
    s : cstring;
 
BEGIN
   s := SUBSTR(name.body, 1, name.length) + CHR(0);
   var_delete(s, status);
END;



[GLOBAL] FUNCTION var_address_p(
    name : string;
    VAR status : INTEGER) : POINTER;

VAR
    s : cstring;
 
BEGIN
   s := SUBSTR(name.body, 1, name.length) + CHR(0);
   var_address_p := var_address(s, status);
END;



[GLOBAL] PROCEDURE fun_define_p(
    name : string;
    expression : string;
    VAR status : INTEGER);

VAR
    s1, s2 : cstring;
 
BEGIN
   s1 := SUBSTR(name.body, 1, name.length) + CHR(0);
   s2 := SUBSTR(expression.body, 1, expression.length) + CHR(0);
   fun_define(s1, s2, status);
END;



[GLOBAL] PROCEDURE fun_delete_p(
    name : string;
    VAR status : INTEGER);

VAR
    s : cstring;
 
BEGIN
   s := SUBSTR(name.body, 1, name.length) + CHR(0);
   fun_delete(s, status);
END;


[GLOBAL] FUNCTION fun_address_p(
    name : string;
    VAR status : INTEGER) : POINTER;

VAR
    s : cstring;
 
BEGIN
   s := SUBSTR(name.body, 1, name.length) + CHR(0);
   fun_address_p := fun_address(s, status);
END;



[GLOBAL] FUNCTION spawn(
    command : varying[u] OF CHAR) : INTEGER;

VAR
    s : cstring;
 
BEGIN
   s := SUBSTR(command.body, 1, command.length) + CHR(0);
   spawn := system(s);
END;



END. { pasrtl }
