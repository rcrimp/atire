/*
	UNICODE_CASE.H
	--------------
	Routines to do case conversion in uncode
*/
#ifndef UNICODE_CASE_H_
#define UNICODE_CASE_H_

/*
	Convert to upper case or lower case.  Note that the case conversion is not one-to-one.  For example,
	character 212B -> 00E5 -> 00C5.  This is because :
		212B is "ANGSTROM SIGN"
		00E5 is "LATIN SMALL LETTER A WITH RING ABOVE"
		00C5 is "LATIN CAPITAL LETTER A WITH RING ABOVE"
	and so the conversion is no incorrect and also not symetric
*/
long ANT_UNICODE_tolower(long character);
long ANT_UNICODE_toupper(long character);

#endif /* UNICODE_CASE_H_ */
