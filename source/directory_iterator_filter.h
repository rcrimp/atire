/*
	DIRECTORY_ITERATOR_FILTER.H
	---------------------------
*/
#ifndef DIRECTORY_ITERATOR_FILTER_H_
#define DIRECTORY_ITERATOR_FILTER_H_

#include "directory_iterator.h"

/*
	class ANT_DIRECTORY_ITERATOR_FILTER
	-----------------------------------
*/
class ANT_directory_iterator_filter : public ANT_directory_iterator
{
public:
	enum { EXCLUDE = 0, INCLUDE };

private:
	ANT_directory_iterator *source;
	static char **docids;
	long long number_docs;

public:
	ANT_directory_iterator_filter(ANT_directory_iterator *source, char *filename, long get_file = 0);
	virtual ~ANT_directory_iterator_filter();

	virtual ANT_directory_iterator_object *first(ANT_directory_iterator_object *object);
	virtual ANT_directory_iterator_object *next(ANT_directory_iterator_object *object);

private:
	long should_index(char *docid);

} ;

#endif /* DIRECTORY_ITERATOR_FILTER_H_ */