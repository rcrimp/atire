/*
	DIRECTORY_ITERATOR_FILTER.C
	---------------------------
*/
#include <stdio.h>
#include <stdlib.h>
#include "maths.h"
#include "str.h"
#include "disk.h"
#include "directory_iterator_filter.h"

char **ANT_directory_iterator_filter::docids = NULL;

/*
	ANT_DIRECTORY_ITERATOR_FILTER::ANT_DIRECTORY_ITERATOR_FILTER()
	--------------------------------------------------------------
*/
ANT_directory_iterator_filter::ANT_directory_iterator_filter(ANT_directory_iterator *source, char *filename, long get_file) : ANT_directory_iterator("", get_file)
{
this->source = source;

if (docids == NULL)
	docids = ANT_disk::buffer_to_list(ANT_disk::read_entire_file(filename), &number_docs);
}

/*
	ANT_DIRECTORY_ITERATOR_FILTER::~ANT_DIRECTORY_ITERATOR_FILTER()
	---------------------------------------------------------------
*/
ANT_directory_iterator_filter::~ANT_directory_iterator_filter()
{
if (docids != NULL)
	{
	delete [] docids;
	docids = NULL;
	}

delete source;
}

/*
	ANT_DIRECTORY_ITERATOR_FILTER::SHOULD_INDEX()
	---------------------------------------------
*/
long ANT_directory_iterator_filter::should_index(char *docid)
{
long long low = 0, mid, high = number_docs - 1;

strip_space_inplace(docid);

while (low < high)
	{
	mid = low + ((high - low) / 2);
	if (strncmp(docids[mid], docid, strlen(docids[mid])) < 0)
		low = mid + 1;
	else
		high = mid;
	}

return strncmp(docids[low], docid, strlen(docids[low])) != 0;
}

/*
	ANT_DIRECTORY_ITERATOR_FILTER::FIRST()
	--------------------------------------
*/
ANT_directory_iterator_object *ANT_directory_iterator_filter::first(ANT_directory_iterator_object *object)
{
ANT_directory_iterator_object *t = source->first(object);

if (t != NULL)
	{
	if (should_index(t->filename))
		return t;
	
	delete [] t->file;
	delete [] t->filename;
	}

return next(object);
}

/*
	ANT_DIRECTORY_ITERATOR_FILTER::NEXT()
	-------------------------------------
*/
ANT_directory_iterator_object *ANT_directory_iterator_filter::next(ANT_directory_iterator_object *object)
{
ANT_directory_iterator_object *t = source->next(object);

while (t != NULL && !should_index(t->filename))
	{
	delete [] t->file;
	delete [] t->filename;

	t = source->next(object);
	}

return t;
}